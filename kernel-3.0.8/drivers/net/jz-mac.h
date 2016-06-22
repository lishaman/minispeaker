
#ifndef __JZ_MAC_H__
#define __JZ_MAC_H__

/*   The MAC's MII registers   */
#define MAC_MII_MAC1	0x0
#define MAC_MII_MAC2	0x4
#define MAC_MII_IPGT	0x8
#define MAC_MII_IPGR	0xC
#define MAC_MII_SUPP	0x18
#define MAC_MII_MCFG	0x20
#define MAC_MII_MCMD	0x24
#define MAC_MII_MADR	0x28
#define MAC_MII_MWTD	0x2C
#define MAC_MII_MRDD	0x30
#define MAC_MII_MIND	0x34
#define MAC_MII_SA0	0x40
#define MAC_MII_SA1	0x44
#define MAC_MII_SA2	0x48

#define MII_MAC1_SFT_RST	BIT(15)
#define MII_MAC1_SIM_RST	BIT(14)
#define MII_MAC1_RMII_MODE	BIT(13)
#define MII_MAC1_RST_RMCS	BIT(11)
#define MII_MAC1_RST_RFUN	BIT(10)
#define MII_MAC1_RST_TMCS	BIT(9)
#define MII_MAC1_RST_TFUN	BIT(8)
#define MII_MAC1_LOOPBACK	BIT(4)
#define MII_MAC1_TX_PAUSE	BIT(3)
#define MII_MAC1_RX_PAUSE	BIT(2)
#define MII_MAC1_PASS_ALL	BIT(1)
#define MII_MAC1_RX_EN		BIT(0)

#define MII_MAC2_LONG_PRE	BIT(9)
#define MII_MAC2_AUTO_PAD	BIT(7)
#define MII_MAC2_PAD_EN		BIT(5)
#define MII_MAC2_CRC_EN		BIT(4)
#define MII_MAC2_DLY_CRC	BIT(3)
#define MII_MAC2_HUGE_FRM	BIT(2)
#define MII_MAC2_LEN_CK		BIT(1)
#define MII_MAC2_FULL_DPX	BIT(0)

#define MII_SUPP_100M	BIT(8)

#define MII_MCMD_SCAN	BIT(1)
#define MII_MCMD_READ	BIT(0)

#define MII_MIND_BUSY	BIT(0)


/*   The MAC's FIFO registers   */
#define MAC_FIFO_CFG_R0	0x3C
#define MAC_FIFO_CFG_R1	0x4C
#define MAC_FIFO_CFG_R2	0x50
#define MAC_FIFO_CFG_R3	0x54
#define MAC_FIFO_CFG_R4	0x58
#define MAC_FIFO_CFG_R5	0x5C

/****************/
// "Receive Status Vector" in CFG_R4,CFG_R5
#define RSV_RVTD	BIT(14)	// Receive VLAN Type detected
#define RSV_RUO		BIT(13)	// Receive Unsupported Op-code
#define RSV_RPCF	BIT(12)	// Receive Pause Control Frame
#define RSV_RCF		BIT(11)	// Receive Control Frame
#define RSV_DN		BIT(10)	// Dribble Nibble
#define RSV_BP		BIT(9)	// Broadcast Packet
#define RSV_MP		BIT(8)	// Multicast Packet
#define RSV_OK		BIT(7)	// Receive OK
#define RSV_LOR		BIT(6)	// Length Out of Range
#define RSV_LCE		BIT(5)	// Length Check Error
#define RSV_CRCE	BIT(4)	// CRC Error
#define RSV_RCV		BIT(3)	// Receive Code Violation
#define RSV_CEPS	BIT(2)	// Carrier Event Previously Seen
#define RSV_REPS	BIT(1)	// RXDV Event Previously Seen
#define RSV_PPI		BIT(0)	// Packet Previously Ignored


/*   The MAC's DMA registers   */
#define MAC_DMA_TxCtrl	0x180
#define MAC_DMA_TxDesc	0x184
#define MAC_DMA_TxStat	0x188
#define MAC_DMA_RxCtrl	0x18C
#define MAC_DMA_RxDesc	0x190
#define MAC_DMA_RxStat	0x194
#define MAC_DMA_IntM	0x198
#define MAC_DMA_IntR	0x19C

#define DMA_TxCtrl_EN		BIT(0)
#define DMA_TxStat_UdRun	BIT(1)
#define DMA_TxStat_TxPkt	BIT(0)

#define DMA_RxCtrl_EN		BIT(0)
#define DMA_RxStat_BusErr	BIT(3)
#define DMA_RxStat_RxOF		BIT(2)
#define DMA_RxStat_RxPkt	BIT(0)

#define DMA_IntM_STEN	BIT(31)
#define DMA_IntM_CLRCNT	BIT(30)
#define DMA_IntM_AUTOZ	BIT(29)

#define DMA_IntR_RxErr	BIT(7)
#define DMA_IntR_RxOF	BIT(6)
#define DMA_IntR_RxPkt	BIT(4)
#define DMA_IntR_TxErr	BIT(3)
#define DMA_IntR_TxUR	BIT(1)
#define DMA_IntR_TxPkt	BIT(0)


/*   The MAC's FIFO registers   */
#define MAC_SAL_AFR	0x1A0
#define MAC_SAL_HT1	0x1A4
#define MAC_SAL_HT2	0x1A8

#define SAL_AFR_PRO	BIT(3)
#define SAL_AFR_PRM	BIT(2)
#define SAL_AFR_AMC	BIT(1)
#define SAL_AFR_ABC	BIT(0)

#endif
