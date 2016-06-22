 /*============================================================================
 * Module Name: ioctl.c
 * Purpose:
 * Author:
 * Date:
 * Notes:
 * $Log: ioctl.c,v $
 * no message
 *
 *
 *=============================================================================
 */

/* INCLUDE FILE DECLARATIONS */
#include "ax88796c_ioctl.h"
#include "ax88796c.h"

/* NAMING CONSTANT DECLARATIONS */


/* LOCAL SUBPROGRAM DECLARATIONS */
/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_check_power_state
 * Purpose: Check AX88796C power saving status
 * ----------------------------------------------------------------------------
 */
u8 ax88796c_check_power_state (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;
	u16 power;

	/* Check media link status first */
	if (netif_carrier_ok (ndev) || (ax_local->ps_level == AX_PS_D0)
	    || (ax_local->ps_level == AX_PS_D1))
		return 0;

	power = AX_READ (ax_base + P0_MACCR) & MACCR_PMM_MASK;
	if (power != MACCR_PMM_READY) {
		unsigned long start;
		AX_WRITE (0xFF, ax_base + PG_HOST_WAKEUP);
		start = jiffies;
		while (power != MACCR_PMM_READY) {
			if (time_after(start, start + 2 * HZ / 10)) {	/* 200ms */
                        	printk (KERN_ERR
                                	"%s: timeout waiting for waking\n",
                                	ax_local->ndev->name);
                        	break;
			}
			power = AX_READ (ax_base + P0_MACCR) & MACCR_PMM_MASK;
		}
		return 1;
	}

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_power_saving
 * Purpose: Set up AX88796C power saving
 * ----------------------------------------------------------------------------
 */
void ax88796c_set_power_saving (struct net_device *ndev, u8 ps_level)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;
	u16 pmm;

	if (ps_level == AX_PS_D1) {
		pmm = PSCR_PS_D1;
	} else if (ps_level == AX_PS_D2) {
		pmm = PSCR_PS_D2;
	} else {
		pmm = PSCR_PS_D0;
	}

        AX_WRITE ((AX_READ (ax_base + P0_PSCR) & PSCR_PS_MASK) |
                        pmm, ax_base + P0_PSCR);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_mdio_read_phy
 * Purpose: Read phy register via mdio interface
 * ----------------------------------------------------------------------------
 */
int ax88796c_mdio_read_phy (struct net_device *ndev, int phy_id, int loc)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	unsigned long start_time;
	void __iomem *ax_base = ax_local->membase;
	u16 val;

	spin_lock (&ax_local->tx_busy_q.lock);
	AX_SELECT_PAGE (PAGE2, ax_base + PG_PSR);

	AX_WRITE (MDIOCR_RADDR (loc) | MDIOCR_FADDR (phy_id) | MDIOCR_READ,
				P2_MDIOCR + ax_base);

	start_time = jiffies;
	while ((AX_READ (P2_MDIOCR + ax_base) & MDIOCR_VALID) == 0) {
		if (time_after (jiffies, start_time + HZ / 100)) {
			return -EBUSY;
		}
	}

	val = AX_READ (P2_MDIODR + ax_base);

	AX_SELECT_PAGE (PAGE0, ax_base + PG_PSR);
	spin_unlock (&ax_local->tx_busy_q.lock);

	return val;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_mdio_read
 * Purpose: Exported for upper layer to read PHY register
 * ----------------------------------------------------------------------------
 */
int ax88796c_mdio_read (struct net_device *ndev, int phy_id, int loc)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	u16 val;
	unsigned long flags;
	u8 power;

	spin_lock_irqsave (&ax_local->isr_lock, flags);

	power = ax88796c_check_power_state (ndev);

	val = ax88796c_mdio_read_phy (ndev, phy_id, loc);

	if (power)
		ax88796c_set_power_saving (ndev, ax_local->ps_level);

	spin_unlock_irqrestore (&ax_local->isr_lock, flags);
	
	return val;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_mdio_write_phy
 * Purpose: Write phy register via mdio interface
 * ----------------------------------------------------------------------------
 */
void
ax88796c_mdio_write_phy (struct net_device *ndev, int phy_id, int loc, int val)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;
	unsigned long start_time;

	spin_lock (&ax_local->tx_busy_q.lock);
	
	AX_SELECT_PAGE(PAGE2, ax_base + PG_PSR);

	AX_WRITE (val, P2_MDIODR + ax_base);

	AX_WRITE (MDIOCR_RADDR (loc) | MDIOCR_FADDR (phy_id) | MDIOCR_WRITE,
						 P2_MDIOCR + ax_base);
	start_time = jiffies;
	while ((AX_READ (P2_MDIOCR + ax_base) & MDIOCR_VALID) == 0) {
		if (time_after (jiffies, start_time + HZ / 100)) {
			return;
		}
	}

	if (loc == MII_ADVERTISE) {
		AX_WRITE ((BMCR_FULLDPLX | BMCR_ANRESTART | 
			  BMCR_ANENABLE | BMCR_SPEED100), P2_MDIODR + ax_base);
		AX_WRITE ((MDIOCR_RADDR (MII_BMCR) |
			  MDIOCR_FADDR (phy_id) | MDIOCR_WRITE),
			  P2_MDIOCR + ax_base);

		start_time = jiffies;
		while ((AX_READ (P2_MDIOCR + ax_base) & MDIOCR_VALID) == 0) {
			if (time_after (jiffies, start_time + HZ / 100)) {
				return;
			}
		}
	}

	AX_SELECT_PAGE (PAGE0, ax_base + PG_PSR);

	spin_unlock(&ax_local->tx_busy_q.lock);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_mdio_write
 * Purpose: Exported for upper layer to write PHY register
 * ----------------------------------------------------------------------------
 */
void ax88796c_mdio_write (struct net_device *ndev, int phy_id, int loc, int val)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	unsigned long flags;
	u8 power;

	spin_lock_irqsave (&ax_local->isr_lock, flags);

	power = ax88796c_check_power_state (ndev);

	ax88796c_mdio_write_phy (ndev, phy_id, loc, val);

	if (power)
		ax88796c_set_power_saving (ndev, ax_local->ps_level);

	spin_unlock_irqrestore (&ax_local->isr_lock, flags);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_csums
 * Purpose: Initialize hardware checksum.
 * ----------------------------------------------------------------------------
 */
void ax88796c_set_csums (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;
	unsigned long flags;
	u8 power;

	spin_lock_irqsave (&ax_local->isr_lock, flags);

	power = ax88796c_check_power_state (ndev);

	AX_SELECT_PAGE (PAGE4, ax_base + PG_PSR);

	if (ax_local->checksum & AX_RX_CHECKSUM) {
		AX_WRITE (COERCR0_DEFAULT, ax_base + P4_COERCR0);
		AX_WRITE (COERCR1_DEFAULT, ax_base + P4_COERCR1);
	} else {
		AX_WRITE (0, ax_base + P4_COERCR0);
		AX_WRITE (0, ax_base + P4_COERCR1);
	}

	if (ax_local->checksum & AX_TX_CHECKSUM) {
		AX_WRITE (COETCR0_DEFAULT, ax_base + P4_COETCR0);
	} else {
		AX_WRITE (0, ax_base + P4_COETCR0);
	}

	AX_SELECT_PAGE (PAGE0, ax_base + PG_PSR);

	if (power)
		ax88796c_set_power_saving (ndev, ax_local->ps_level);

	spin_unlock_irqrestore (&ax_local->isr_lock, flags);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_drvinfo
 * Purpose: Exported for Ethtool to query the driver version
 * ----------------------------------------------------------------------------
 */
static void ax88796c_get_drvinfo (struct net_device *ndev,
				 struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	strncpy (info->driver, AX88796C_DRV_NAME, sizeof info->driver);
	strncpy (info->version, AX88796C_DRV_VERSION, sizeof info->version);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_link
 * Purpose: Exported for Ethtool to query the media link status
 * ----------------------------------------------------------------------------
 */
static u32 ax88796c_get_link (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	return mii_link_ok (&ax_local->mii);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_wol
 * Purpose: Exported for Ethtool to query wake on lan setting
 * ----------------------------------------------------------------------------
 */
static void
ax88796c_get_wol (struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	wolinfo->supported = WAKE_PHY | WAKE_MAGIC | WAKE_ARP;
	wolinfo->wolopts = 0;
	
	if (ax_local->wol & WFCR_LINKCH)
		wolinfo->wolopts |= WAKE_PHY;
	if (ax_local->wol & WFCR_MAGICP)
		wolinfo->wolopts |= WAKE_MAGIC;
	if (ax_local->wol & WFCR_WAKEF)
		wolinfo->wolopts |= WAKE_ARP;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_wol
 * Purpose: Exported for Ethtool to set the wake on lan setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796c_set_wol (struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	ax_local->wol = 0;

	if (wolinfo->wolopts & WAKE_PHY)
		ax_local->wol |= WFCR_LINKCH;
	if (wolinfo->wolopts & WAKE_MAGIC)
		ax_local->wol |= WFCR_MAGICP;
	if (wolinfo->wolopts & WAKE_ARP) {
		ax_local->wol |= WFCR_WAKEF;
	}

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_settings
 * Purpose: Exported for Ethtool to query PHY setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796c_get_settings (struct net_device *ndev, struct ethtool_cmd *cmd)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	return mii_ethtool_gset(&ax_local->mii, cmd);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_settings
 * Purpose: Exported for Ethtool to set PHY setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796c_set_settings (struct net_device *ndev, struct ethtool_cmd *cmd)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	int retval;

	retval = mii_ethtool_sset (&ax_local->mii, cmd);

	return retval;

}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_nway_reset
 * Purpose: Exported for Ethtool to restart PHY autonegotiation
 * ----------------------------------------------------------------------------
 */
static int ax88796c_nway_reset (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	return mii_nway_restart(&ax_local->mii);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_ethtool_getmsglevel
 * Purpose: Exported for Ethtool to query driver message level
 * ----------------------------------------------------------------------------
 */
static u32 ax88796c_ethtool_getmsglevel (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	return ax_local->msg_enable;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_ethtool_setmsglevel
 * Purpose: Exported for Ethtool to set driver message level
 * ----------------------------------------------------------------------------
 */
static void ax88796c_ethtool_setmsglevel (struct net_device *ndev, u32 level)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	ax_local->msg_enable = level;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_rx_csum
 * Purpose: Exported for Ethtool to query receive checksum setting
 * ----------------------------------------------------------------------------
 */
static u32 ax88796c_get_rx_csum (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	return (ax_local->checksum & AX_RX_CHECKSUM);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_rx_csum
 * Purpose: Exported for Ethtool to set receive checksum setting
 * ----------------------------------------------------------------------------
 */
static int ax88796c_set_rx_csum (struct net_device *ndev, u32 val)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	if (val)
		ax_local->checksum |= AX_RX_CHECKSUM;
	else
		ax_local->checksum &= ~AX_RX_CHECKSUM;

	ax88796c_set_csums (ndev);

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_get_tx_csum
 * Purpose: Exported for Ethtool to query transmit checksum setting
 * ----------------------------------------------------------------------------
 */
static u32 ax88796c_get_tx_csum (struct net_device *ndev)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	return (ax_local->checksum & AX_TX_CHECKSUM);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_set_tx_csum
 * Purpose: Exported for Ethtool to set transmit checksum setting
 * ----------------------------------------------------------------------------
 */
static int ax88796c_set_tx_csum (struct net_device *ndev, u32 val)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);

	if (val)
		ax_local->checksum |= AX_TX_CHECKSUM;
	else
		ax_local->checksum &= ~AX_TX_CHECKSUM;

	ethtool_op_set_tx_hw_csum (ndev, val);
	ax88796c_set_csums (ndev);

	return 0;
}

struct ethtool_ops ax88796c_ethtool_ops = {
	.get_drvinfo		= ax88796c_get_drvinfo,
	.get_link		= ax88796c_get_link,
	.get_wol		= ax88796c_get_wol,
	.set_wol		= ax88796c_set_wol,
	.get_settings		= ax88796c_get_settings,
	.set_settings		= ax88796c_set_settings,
	.nway_reset		= ax88796c_nway_reset,
	.get_msglevel		= ax88796c_ethtool_getmsglevel,
	.set_msglevel		= ax88796c_ethtool_setmsglevel,
	.get_tx_csum		= ax88796c_get_tx_csum,
	.set_tx_csum		= ax88796c_set_tx_csum,
	.get_rx_csum		= ax88796c_get_rx_csum,
	.set_rx_csum		= ax88796c_set_rx_csum,
};

void ioctl_signature (struct ax88796c_device *ax_local, AX_IOCTL_COMMAND *info)
{
	strncpy (info->sig, AX88796C_DRV_NAME, strlen (AX88796C_DRV_NAME));
}

void ioctl_config_test_pkt (struct ax88796c_device *ax_local, AX_IOCTL_COMMAND *info)
{
	void __iomem *ax_base = ax_local->membase;
	unsigned long flags;
	unsigned short reg_data = 0x1500;
	unsigned short phy_reg;

	strncpy (info->sig, AX88796C_DRV_NAME, strlen (AX88796C_DRV_NAME));

	spin_lock_irqsave (&ax_local->isr_lock, flags);


	/* Disable force media link up at 10M first */
	phy_reg = ax88796c_mdio_read_phy (ax_local->ndev, 
				ax_local->mii.phy_id, 0x10);
	ax88796c_mdio_write_phy (ax_local->ndev, ax_local->mii.phy_id,
				0x10, (phy_reg & 0xFFFE));
	if (info->speed == 100) {

		/* Force media link at speed 100 */
		ax88796c_mdio_write_phy (ax_local->ndev, ax_local->mii.phy_id,
				MII_BMCR, (BMCR_SPEED100 | BMCR_FULLDPLX));

	} else if (info->speed == 10) {

		/* Force media link at speed 10 */
		ax88796c_mdio_write_phy (ax_local->ndev, ax_local->mii.phy_id,
				MII_BMCR, 0);

		/* Force media link up */
		phy_reg = ax88796c_mdio_read_phy (ax_local->ndev, 
				ax_local->mii.phy_id, 0x10);
		ax88796c_mdio_write_phy (ax_local->ndev, ax_local->mii.phy_id,
				0x10, (phy_reg | 1));

	} else {
		/* Restart PHY autoneg */
		ax88796c_mdio_write_phy (ax_local->ndev, ax_local->mii.phy_id,
				MII_BMCR, (BMCR_ANENABLE | BMCR_ANRESTART));
	}

	AX_SELECT_PAGE(PAGE3, ax_base + PG_PSR);

	/* Clear current setting */
	AX_WRITE (reg_data, ax_base + P3_TPCR);

	if (info->type == AX_PACKET_TYPE_FIX) {
		reg_data |= TPCR_FIXED_PKT_EN | info->pattern;
	} else if (info->type == AX_PACKET_TYPE_RAND) {
		reg_data |= TPCR_RAND_PKT_EN;
	}

	AX_WRITE (info->length, ax_base + P3_TPLR);
	AX_WRITE (reg_data, ax_base + P3_TPCR);

	AX_SELECT_PAGE(PAGE0, ax_base + PG_PSR);
	spin_unlock_irqrestore (&ax_local->isr_lock, flags);

}

typedef void (*IOCTRL_TABLE)(struct ax88796c_device *ax_local,
						AX_IOCTL_COMMAND *info);

IOCTRL_TABLE ioctl_tbl[] = {
	ioctl_signature,	//AX_SIGNATURE
	ioctl_config_test_pkt,	//AX_CFG_TEST_PKT
};

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_ioctl()
 * Purpose:
 * Params:
 * Returns:
 * Note:
 * ----------------------------------------------------------------------------
 */
int ax88796c_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	struct ax88796c_device *ax_local = netdev_priv (ndev);
	AX_IOCTL_COMMAND info;
	AX_IOCTL_COMMAND *uptr = (AX_IOCTL_COMMAND *)ifr->ifr_data;
	int private_cmd;
	u8 power;

	switch(cmd) {

	case AX_PRIVATE:
		if ( copy_from_user(&info, uptr, sizeof(AX_IOCTL_COMMAND)) )
			return -EFAULT;
		
		private_cmd = info.ioctl_cmd;

		power = ax88796c_check_power_state (ndev);

		(*ioctl_tbl[private_cmd])(ax_local, &info);

		if (power)
			ax88796c_set_power_saving (ndev, ax_local->ps_level);

		if( copy_to_user( uptr, &info, sizeof(AX_IOCTL_COMMAND) ) )
				return -EFAULT;
		break;
	default:
		return generic_mii_ioctl(&ax_local->mii, if_mii(ifr), cmd, NULL);
	}

	return 0;
}
