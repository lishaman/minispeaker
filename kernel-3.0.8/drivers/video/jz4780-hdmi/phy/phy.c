/*
 * phy.c
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "phy.h"
#include "halSourcePhy.h"
#include "halI2cMasterPhy.h"
#include "../core/halMainController.h"
#include "../util/log.h"
#include "../util/error.h"
#include <linux/delay.h>
#include "../bsp/access.h"
#include <linux/kthread.h>
#include <linux/gpio.h>

static const u16 PHY_I2CM_BASE_ADDR = 0x3020;
static const u8 PHY_I2C_SLAVE_ADDR = 0x69;

#if 0
#define ZCALCLK	GPIO_PE(30) /* GPE(30) */
#define ZCALRST GPIO_PE(31) /* GPE(31) */
#define ZCALDONE GPIO_PE(19) /* GPE(19) */
#endif

#define PHY_GEN2_TSMC_40LP_2_5V
int phy_Initialize(u16 baseAddr, u8 dataEnablePolarity)
{
	LOG_TRACE1(dataEnablePolarity);
#ifndef PHY_THIRD_PARTY
#ifdef PHY_GEN2_TSMC_40LP_2_5V
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	LOG_NOTICE("GEN 2 TSMC 40LP 2.5V build - TQL");
#endif

#ifdef PHY_GEN2_TSMC_40G_1_8V
	LOG_NOTICE("GEN 2 TSMC 40G 1.8V build - E102");
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
#endif

#ifdef PHY_GEN2_TSMC_65LP_2_5V
	LOG_NOTICE("GEN 2 TSMC 65LP 2.5V build - E104");
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
#endif

#ifdef PHY_TNP
	LOG_NOTICE("TNP build");
#endif

#ifdef PHY_CNP
	LOG_NOTICE("CNP build");
#endif
	halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, ~0); /* mask phy interrupts */
	halSourcePhy_DataEnablePolarity(baseAddr + PHY_BASE_ADDR,
			dataEnablePolarity);
	halSourcePhy_InterfaceControl(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 0); /* disable PHY */

#if 0
	__gpio_as_output1(ZCALRST);
	//__gpio_as_output0(ZCALCLK);
	__gpio_as_input(ZCALCLK);
	__gpio_disable_pull(ZCALCLK);
	__gpio_as_input(ZCALDONE);

	gpio_request_one(ZCALRST, GPIOF_DIR_OUT, "phy-ZCALRST");
	gpio_request_one(ZCALCLK, GPIOF_DIR_IN, "phy-ZCALCLK");
	gpio_request_one(ZCALDONE, GPIOF_DIR_IN, "phy-ZCALDONE");
	gpio_direction_output(ZCALRST, 1);
	gpio_direction_input(ZCALCLK);
	gpio_direction_input(ZCALDONE);
#endif

#else
	LOG_NOTICE("Third Party PHY build");
	printk("api init faild!!!!!!!!!!!--%d\n",__LINE__);
#endif
	return TRUE;
}

int phy_Configure (u16 baseAddr, u16 pClk, u8 cRes, u8 pRep)
{
#ifndef PHY_THIRD_PARTY
#ifdef PHY_CNP
	u16 clk = 0;
	u16 rep = 0;
#endif
	u16 i = 0;
	static int wait_zdone;

	LOG_TRACE();
	/*  colour resolution 0 is 8 bit colour depth */
	if (cRes == 0)
		cRes = 8;
	if (pRep != 0)
	{
		error_Set(ERR_PIXEL_REPETITION_NOT_SUPPORTED);
		LOG_ERROR2("pixel repetition not supported", pRep);
		return FALSE;
	}

	/* The following is only for PHY_GEN1_CNP, and 1v0 NOT 1v1 */
	/* values are found in document HDMISRC1UHTCcnp_IPCS_DS_0v3.doc
	 * for the HDMISRCGPHIOcnp
	 */
	/* in the cnp PHY interface, the 3 most significant bits are ctrl (which
	 * part block to write) and the last 5 bits are data */
	/* for example 0x6A4a is writing to block  3 (ie. [14:10]) (5-bit blocks)
	 * the bits 0x0A, and  block 2 (ie. [9:5]) the bits 0x0A */
	/* configure PLL after core pixel repetition */
#ifdef PHY_GEN2_TSMC_40LP_2_5V
#if 0
	wait_zdone = 0;
	//while(1);

	if (wait_zdone) {
		//BUG_ON(__gpio_get_pin(ZCALDONE));
		//BUG_ON(gpio_get_value(ZCALDONE));
		//struct task_struct *zcalclk = kthread_run(gen_zcalclk, NULL, "zcalclk");
		//msleep(3);
		//__gpio_as_output0(ZCALRST);
		gpio_direction_output(ZCALRST, 0);
		//while(!__gpio_get_pin(ZCALDONE));
		while(!gpio_get_value(ZCALDONE));
		udelay(30);
		//__gpio_as_output1(ZCALRST);
		gpio_direction_output(ZCALRST, 1);
		//msleep(1);
		//kthread_stop(zcalclk);
		wait_zdone = 0;
	}
#endif
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1);
	//halMainController_HeacPhyReset(baseAddr + MC_BASE_ADDR, 1);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
	halI2cMasterPhy_SlaveAddress(baseAddr + PHY_I2CM_BASE_ADDR, PHY_I2C_SLAVE_ADDR);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);

#ifdef CONFIG_HDMI_JZ4780_DEBUG
	{
		int haha = 0;
		for (haha = 0x3000; haha <= 0x3007; haha++) {
			printk("===>REG[%04x] = 0x%02x\n",
			       haha, access_CoreReadByte(haha));
		}
	}
	printk("hdmi   %s: pClk = %u cRes = %d\n", __func__, pClk, cRes);
#endif
	cRes = 8;
#ifdef CONFIG_HDMI_JZ4780_DEBUG
	u16 slcaoValue;
#define READ_BACK_PRINT(addr)				\
	phy_I2cRead(baseAddr, &slcaoValue, (addr));	\
	printk("===>PHY[%02x] = 0x%04x\n", (addr), slcaoValue);
#else
#define READ_BACK_PRINT(addr) do { } while(0)
#endif

	phy_I2cWrite(baseAddr, 0x0000, 0x13); /* PLLPHBYCTRL */
	READ_BACK_PRINT(0x13);

	phy_I2cWrite(baseAddr, 0x0006, 0x17);
	READ_BACK_PRINT(0x17);

	/* RESISTANCE TERM 133Ohm Cfg  */
	phy_I2cWrite(baseAddr, 0x0005, 0x19); /* TXTERM */
	READ_BACK_PRINT(0x19);

	/* REMOVE CLK TERM */
	phy_I2cWrite(baseAddr, 0x8000, 0x05); /* CKCALCTRL */
	READ_BACK_PRINT(0x05);

	switch (pClk)
	{
		case 2520:
			switch (cRes)
			{
				case 8:
					/* PLL/MPLL Cfg */
					//phy_I2cWrite(baseAddr, 0x01e0, 0x06);
					phy_I2cWrite(baseAddr, 0x01e0, 0x06);
					READ_BACK_PRINT(0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10); /* CURRCTRL */
					READ_BACK_PRINT(0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15); /* GMPCTRL */
					READ_BACK_PRINT(0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x21e1, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x41e2, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			/* PREEMP Cgf 0.00 */
			phy_I2cWrite(baseAddr, 0x8009, 0x09); /* CKSYMTXCTRL */
			/* TX/CK LVL 10 */
			phy_I2cWrite(baseAddr, 0x0210, 0x0E); /* VLEVCTRL */
			break;
		case 2700:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x01e0, 0x06);
					READ_BACK_PRINT(0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					READ_BACK_PRINT(0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					READ_BACK_PRINT(0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x21e1, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x41e2, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0210, 0x0E);
			break;
		case 5400:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2141, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x4142, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0210, 0x0E);
			break;
		case 7200:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2141, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0210, 0x0E);
			break;
		case 7425:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x20a1, 0x06);
					phy_I2cWrite(baseAddr, 0x0b5c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0210, 0x0E);
			break;
		case 10800:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x00a0, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x20a1, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0210, 0x0E);
			break;
		case 14850:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x00a0, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					phy_I2cWrite(baseAddr, 0x8009, 0x09);
					phy_I2cWrite(baseAddr, 0x0210, 0x0E);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2001, 0x06);
					phy_I2cWrite(baseAddr, 0x0b5c, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x8009, 0x09);
					phy_I2cWrite(baseAddr, 0x0210, 0x0E);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x4002, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x800b, 0x09);
					phy_I2cWrite(baseAddr, 0x0129, 0x0E);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		default:
			error_Set(ERR_PIXEL_CLOCK_NOT_SUPPORTED);
			LOG_ERROR2("pixel clock not supported", pClk);
			return FALSE;
	}
	halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 0);
#else
#ifdef PHY_GEN2_TSMC_40G_1_8V
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1);
	halMainController_HeacPhyReset(baseAddr + MC_BASE_ADDR, 1);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
	halI2cMasterPhy_SlaveAddress(baseAddr + PHY_I2CM_BASE_ADDR, PHY_I2C_SLAVE_ADDR);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);

	phy_I2cWrite(baseAddr, 0x0000, 0x13);
	phy_I2cWrite(baseAddr, 0x0002, 0x19);
	phy_I2cWrite(baseAddr, 0x0006, 0x17);
	phy_I2cWrite(baseAddr, 0x8000, 0x05);
	switch (pClk)
	{
		case 2520:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x01e0, 0x06);
					phy_I2cWrite(baseAddr, 0x08da, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x21e1, 0x06);
					phy_I2cWrite(baseAddr, 0x08da, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x41e2, 0x06);
					phy_I2cWrite(baseAddr, 0x065a, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 2700:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x01e0, 0x06);
					phy_I2cWrite(baseAddr, 0x08da, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x21e1, 0x06);
					phy_I2cWrite(baseAddr, 0x08da, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x41e2, 0x06);
					phy_I2cWrite(baseAddr, 0x065a, 0x10);
					phy_I2cWrite(baseAddr, 0x0000, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 5400:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x09da, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2141, 0x06);
					phy_I2cWrite(baseAddr, 0x09da, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x4142, 0x06);
					phy_I2cWrite(baseAddr, 0x079a, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 7200:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x079a, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2141, 0x06);
					phy_I2cWrite(baseAddr, 0x079a, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x0a5a, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 7425:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0140, 0x06);
					phy_I2cWrite(baseAddr, 0x079a, 0x10);
					phy_I2cWrite(baseAddr, 0x0005, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x20a1, 0x06);
					phy_I2cWrite(baseAddr, 0x0bda, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x0a5a, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 10800:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x00a0, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x20a1, 0x06);
					phy_I2cWrite(baseAddr, 0x091c, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x40a2, 0x06);
					phy_I2cWrite(baseAddr, 0x06dc, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			phy_I2cWrite(baseAddr, 0x8009, 0x09);
			phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			break;
		case 14850:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x00a0, 0x06);
					phy_I2cWrite(baseAddr, 0x079a, 0x10);
					phy_I2cWrite(baseAddr, 0x000a, 0x15);
					phy_I2cWrite(baseAddr, 0x8009, 0x09);
					phy_I2cWrite(baseAddr, 0x0231, 0x0E);
					break;
				case 10:
					phy_I2cWrite(baseAddr, 0x2001, 0x06);
					phy_I2cWrite(baseAddr, 0x0bda, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x800b, 0x09);
					phy_I2cWrite(baseAddr, 0x014a, 0x0E);
					break;
				case 12:
					phy_I2cWrite(baseAddr, 0x4002, 0x06);
					phy_I2cWrite(baseAddr, 0x0a5a, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x800b, 0x09);
					phy_I2cWrite(baseAddr, 0x014a, 0x0E);
					break;
				case 16:
					phy_I2cWrite(baseAddr, 0x6003, 0x06);
					phy_I2cWrite(baseAddr, 0x07da, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x800b, 0x09);
					phy_I2cWrite(baseAddr, 0x014a, 0x0E);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		case 34000:
			switch (cRes)
			{
				case 8:
					phy_I2cWrite(baseAddr, 0x0000, 0x06);
					phy_I2cWrite(baseAddr, 0x07da, 0x10);
					phy_I2cWrite(baseAddr, 0x000f, 0x15);
					phy_I2cWrite(baseAddr, 0x800b, 0x09);
					phy_I2cWrite(baseAddr, 0x0108, 0x0E);
					break;
				default:
					error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
					LOG_ERROR2("color depth not supported", cRes);
					return FALSE;
			}
			break;
		default:
			error_Set(ERR_PIXEL_CLOCK_NOT_SUPPORTED);
			LOG_ERROR2("pixel clock not supported", pClk);
			return FALSE;
	}
	halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 0);
#else
#ifdef PHY_GEN2_TSMC_65LP_2_5V
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1);
	halMainController_HeacPhyReset(baseAddr + MC_BASE_ADDR, 1);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
	halI2cMasterPhy_SlaveAddress(baseAddr + PHY_I2CM_BASE_ADDR, PHY_I2C_SLAVE_ADDR);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);
	phy_I2cWrite(baseAddr, 0x8009, 0x09);
	phy_I2cWrite(baseAddr, 0x0004, 0x19);
	phy_I2cWrite(baseAddr, 0x0000, 0x13);
	phy_I2cWrite(baseAddr, 0x0006, 0x17);
	phy_I2cWrite(baseAddr, 0x8000, 0x05);
	switch (pClk)
		{
			case 2520:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x01E0, 0x06);
						phy_I2cWrite(baseAddr, 0x08D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x21E1, 0x06);
						phy_I2cWrite(baseAddr, 0x08D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x41E2, 0x06);
						phy_I2cWrite(baseAddr, 0x0659, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x6143, 0x06);
						phy_I2cWrite(baseAddr, 0x0999, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				phy_I2cWrite(baseAddr, 0x0231, 0x0E);
				break;
			case 2700:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x01E0, 0x06);
						phy_I2cWrite(baseAddr, 0x08D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x21E1, 0x06);
						phy_I2cWrite(baseAddr, 0x08D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x41E2, 0x06);
						phy_I2cWrite(baseAddr, 0x0659, 0x10);
						phy_I2cWrite(baseAddr, 0x0000, 0x15);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x6143, 0x06);
						phy_I2cWrite(baseAddr, 0x0999, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				phy_I2cWrite(baseAddr, 0x0231, 0x0E);
				break;
			case 5400:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x0140, 0x06);
						phy_I2cWrite(baseAddr, 0x0999, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x2141, 0x06);
						phy_I2cWrite(baseAddr, 0x0999, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x4142, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x60A3, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				phy_I2cWrite(baseAddr, 0x0231, 0x0E);
				break;
			case 7200:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x0140, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x2141, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x40A2, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x60A3, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				break;
				phy_I2cWrite(baseAddr, 0x0231, 0x0E);
			case 7425:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x0140, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x0005, 0x15);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x20A1, 0x06);
						phy_I2cWrite(baseAddr, 0x0BD9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x40A2, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x60A3, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				phy_I2cWrite(baseAddr, 0x0231, 0x0E);
				break;
			case 10800:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x00A0, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						phy_I2cWrite(baseAddr, 0x0231, 0x0E);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x20A1, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						phy_I2cWrite(baseAddr, 0x0231, 0x0E);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x40A2, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x6003, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				break;
			case 14850:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x00A0, 0x06);
						phy_I2cWrite(baseAddr, 0x06D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000A, 0x15);
						phy_I2cWrite(baseAddr, 0x0231, 0x0E);
						break;
					case 10:
						phy_I2cWrite(baseAddr, 0x2001, 0x06);
						phy_I2cWrite(baseAddr, 0x0BD9, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					case 12:
						phy_I2cWrite(baseAddr, 0x4002, 0x06);
						phy_I2cWrite(baseAddr, 0x09D9, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					case 16:
						phy_I2cWrite(baseAddr, 0x6003, 0x06);
						phy_I2cWrite(baseAddr, 0x0719, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				break;
			case 29700:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x0000, 0x06);
						phy_I2cWrite(baseAddr, 0x0719, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x014A, 0x0E);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
				break;

			case 34000:
				switch (cRes)
				{
					case 8:
						phy_I2cWrite(baseAddr, 0x0000, 0x06);
						phy_I2cWrite(baseAddr, 0x0719, 0x10);
						phy_I2cWrite(baseAddr, 0x000F, 0x15);
						phy_I2cWrite(baseAddr, 0x0129, 0x0E);
						break;
					default:
						error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
						LOG_ERROR2("color depth not supported", cRes);
						return FALSE;
				}
		}

	halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 0);
#else
	if (cRes != 8 && cRes != 12)
	{
		error_Set(ERR_COLOR_DEPTH_NOT_SUPPORTED);
		LOG_ERROR2("color depth not supported", cRes);
		return FALSE;
	}
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);
#ifndef PHY_TNP
	switch (pClk)
	{
		case 2520:
			clk = 0x93C1;
			rep = (cRes == 8) ? 0x6A4A : 0x6653;
			break;
		case 2700:
			clk = 0x96C1;
			rep = (cRes == 8) ? 0x6A4A : 0x6653;
			break;
		case 5400:
			clk = 0x8CC3;
			rep = (cRes == 8) ? 0x6A4A : 0x6653;
			break;
		case 7200:
			clk = 0x90C4;
			rep = (cRes == 8) ? 0x6A4A : 0x6654;
			break;
		case 7425:
			clk = 0x95C8;
			rep = (cRes == 8) ? 0x6A4A : 0x6654;
			break;
		case 10800:
			clk = 0x98C6;
			rep = (cRes == 8) ? 0x6A4A : 0x6653;
			break;
		case 14850:
			clk = 0x89C9;
			rep = (cRes == 8) ? 0x6A4A : 0x6654;
			break;
		default:
			error_Set(ERR_PIXEL_CLOCK_NOT_SUPPORTED);
			LOG_ERROR2("pixel clock not supported", pClk);
			return FALSE;
	}
	halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 0);
	if (phy_TestControl(baseAddr, 0x1B) != TRUE)
	{
		error_Set(ERR_PHY_TEST_CONTROL);
		return FALSE;
	}
	phy_TestData(baseAddr, (u8)(clk >> 8));
	phy_TestData(baseAddr, (u8)(clk >> 0));
	if (phy_TestControl(baseAddr, 0x1A) != TRUE)
	{
		error_Set(ERR_PHY_TEST_CONTROL);
		return FALSE;
	}
	phy_TestData(baseAddr, (u8)(rep >> 8));
	phy_TestData(baseAddr, (u8)(rep >> 0));
#endif
	if (pClk == 14850 && cRes == 12)
	{
		LOG_NOTICE("Applying Pre-Emphasis");
		if (phy_TestControl(baseAddr, 0x24) != TRUE)
		{
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0x80);
		phy_TestData(baseAddr, 0x90);
		phy_TestData(baseAddr, 0xa0);
#ifndef PHY_TNP
		phy_TestData(baseAddr, 0xb0);
		if (phy_TestControl(baseAddr, 0x20) != TRUE)
		{ /*  +11.1ma 3.3 pe */
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0x04);
		if (phy_TestControl(baseAddr, 0x21) != TRUE) /*  +11.1 +2ma 3.3 pe */
		{
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0x2a);

		if (phy_TestControl(baseAddr, 0x11) != TRUE)
		{
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0xf3);
		phy_TestData(baseAddr, 0x93);
#else
		if (phy_TestControl(baseAddr, 0x20) != TRUE)
		{
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0x00);
		if (phy_TestControl(baseAddr, 0x21) != TRUE)
		{
			error_Set(ERR_PHY_TEST_CONTROL);
			return FALSE;
		}
		phy_TestData(baseAddr, 0x00);
#endif
	}
	if (phy_TestControl(baseAddr, 0x00) != TRUE)
	{
		error_Set(ERR_PHY_TEST_CONTROL);
		return FALSE;
	}
	halSourcePhy_TestDataIn(baseAddr + PHY_BASE_ADDR, 0x00);
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1); /*  reset PHY */
	halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
	halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 1); /* enable PHY */
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0); /*  toggle TMDS */
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 1);
#endif
#endif
#endif
	//halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1); /*  reset PHY */
	//halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
	halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 1); /* enable PHY */
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0); /*  toggle TMDS */
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 1);

#ifdef CONFIG_HDMI_JZ4780_DEBUG
	{
		int haha = 0;
		for (haha = 0x3000; haha <= 0x3007; haha++) {
			printk("===>REG[%04x] = 0x%02x\n",
			       haha, access_CoreReadByte(haha));
		}

		for (haha = 0; haha < 0x27; haha++) {
			READ_BACK_PRINT(haha);
		}
	}
#endif
	/* wait PHY_TIMEOUT no of cycles at most for the pll lock signal to raise ~around 20us max */
	for (i = 0; i < PHY_TIMEOUT; i++)
	{
			if (halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) == TRUE)
			{
				break;
			}

			msleep(5);
#if 0
		if ((i % 100) == 0)
		{
			if (halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) == TRUE)
			{
				break;
			}
		}
#endif
	}
	if (halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) != TRUE)
	{
		error_Set(ERR_PHY_NOT_LOCKED);
		LOG_ERROR("PHY PLL not locked");
		return FALSE;
	}
#endif
	return TRUE;
}

int phy_Standby(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, ~0); /* mask phy interrupts */
	halSourcePhy_EnableTMDS(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_PowerDown(baseAddr + PHY_BASE_ADDR, 0); /*  disable PHY */
	halSourcePhy_Gen2TxPowerOn(baseAddr, 0);
	halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
#endif
	return TRUE;
}

int phy_EnableHpdSense(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
#endif
	return TRUE;
}

int phy_DisableHpdSense(u16 baseAddr)
{
#ifndef PHY_THIRD_PARTY
	halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 0);
#endif
	return TRUE;
}

int phy_HotPlugDetected(u16 baseAddr)
{
	/* MASK		STATUS		POLARITY	INTERRUPT		HPD
	 *   0			0			0			1			0
	 *   0			1			0			0			1
	 *   0			0			1			0			0
	 *   0			1			1			1			1
	 *   1			x			x			0			x
	 */
	int polarity = 0;
	polarity = halSourcePhy_InterruptPolarityStatus(baseAddr + PHY_BASE_ADDR, 0x02) >> 1;
	if (polarity == halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR))
	{
		halSourcePhy_InterruptPolarity(baseAddr + PHY_BASE_ADDR, 1, !polarity);
		return polarity;
	}
	return !polarity;
	/* return halSourcePhy_HotPlugState(baseAddr + PHY_BASE_ADDR); */
}

int phy_InterruptEnable(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	halSourcePhy_InterruptMask(baseAddr + PHY_BASE_ADDR, value);
	return TRUE;
}
#ifndef PHY_THIRD_PARTY
int phy_TestControl(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	halSourcePhy_TestDataIn(baseAddr + PHY_BASE_ADDR, value);
	halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 0);
	return TRUE;
}

int phy_TestData(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	halSourcePhy_TestDataIn(baseAddr + PHY_BASE_ADDR, value);
	halSourcePhy_TestEnable(baseAddr + PHY_BASE_ADDR, 0);
	halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 1);
	halSourcePhy_TestClock(baseAddr + PHY_BASE_ADDR, 0);
	return TRUE;
}

int phy_I2cWrite(u16 baseAddr, u16 data, u8 addr)
{
	LOG_TRACE2(data, addr);

	halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
	halI2cMasterPhy_WriteData(baseAddr + PHY_I2CM_BASE_ADDR, data);
	halI2cMasterPhy_WriteRequest(baseAddr + PHY_I2CM_BASE_ADDR);

	msleep(100);
	return TRUE;
}

int phy_I2cRead(u16 baseAddr, u16 *data, u8 addr)
{
	LOG_TRACE2((unsigned)data, addr);
	halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
	halI2cMasterPhy_ReadRequest(baseAddr + PHY_I2CM_BASE_ADDR);
	msleep(100);
	halI2cMasterPhy_ReadData(baseAddr + PHY_I2CM_BASE_ADDR, data);
	return TRUE;
}
#endif
