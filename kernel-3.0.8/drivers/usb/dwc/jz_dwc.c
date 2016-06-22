/*
 * linux/drivers/usb/dwc/jz_dwc.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Written by Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/jz_dwc.h>

#include <soc/base.h>
#include <soc/cpm.h>

#include "dwc_os.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_cil.h"

#define OTG_CLK_NAME		"otg1"
#define VBUS_REG_NAME		"vbus"
#define UCHARGER_REG_NAME	"ucharger"

#define USBRDT_VBFIL_LD_EN		25
#define USBPCR_TXPREEMPHTUNE		6
#define USBPCR_POR			22
#define USBPCR_USB_MODE			31
#define USBPCR_VBUSVLDEXT		24
#define USBPCR_VBUSVLDEXTSEL		23
#define USBPCR_OTG_DISABLE		20
#define USBPCR_IDPULLUP_MASK		28
#define OPCR_SPENDN0			7
#define USBPCR1_USB_SEL			28
#define USBPCR1_WORD_IF0		19
#define USBPCR1_WORD_IF1		18

struct jzdwc_pin __attribute__((weak)) dete_pin = {
	.num				= -1,
	.enable_level			= -1,
};

static inline void jz_dwc_set_device_only_mode(void)
{
	cpm_clear_bit(USBPCR_USB_MODE, CPM_USBPCR);
	cpm_clear_bit(USBPCR_OTG_DISABLE, CPM_USBPCR);
}

static inline void jz_dwc_set_dual_mode(void)
{
	unsigned int tmp;

	cpm_outl((1 << USBPCR_USB_MODE)
		 | (1 << USBPCR_VBUSVLDEXT)
		 | (1 << USBPCR_VBUSVLDEXTSEL),
		    CPM_USBPCR);
	cpm_clear_bit(USBPCR_OTG_DISABLE, CPM_USBPCR);
	tmp = cpm_inl(CPM_USBPCR);
	cpm_outl(tmp & ~(0x03 << USBPCR_IDPULLUP_MASK), CPM_USBPCR);
}

static void jz_pri_start(struct dwc_jz_pri *jz_pri)
{
#ifndef CONFIG_USB_DWC_DEV_ONLY
	jz_dwc_set_dual_mode();
#endif
	if (jz_pri->irq > 0)
		schedule_delayed_work(&jz_pri->work, 0);
}

static int get_pin_status(struct jzdwc_pin *pin)
{
	int val;

	if (pin->num < 0)
		return -1;
	val = gpio_get_value(pin->num);

	if (pin->enable_level == LOW_ENABLE)
		return !val;
	return val;
}

static inline void jz_dwc_phy_switch(struct dwc_jz_pri *jz_pri, int is_on)
{
	unsigned int value;

	if (is_on) {
		spin_lock(&jz_pri->lock);
		value = cpm_inl(CPM_OPCR);
		cpm_outl(value | (1 << OPCR_SPENDN0), CPM_OPCR);
		spin_unlock(&jz_pri->lock);

		/* Wait PHY Clock Stable. */
		msleep(1);
		pr_info("enable PHY\n");

	} else {
		spin_lock(&jz_pri->lock);
		value = cpm_inl(CPM_OPCR);
		cpm_outl(value & ~OPCR_SPENDN0, CPM_OPCR);
		spin_unlock(&jz_pri->lock);

		pr_info("disable PHY\n");
	}
}

static inline void jz_dwc_phy_reset(struct dwc_jz_pri *jz_pri)
{
	pr_debug("reset PHY\n");

	spin_lock(&jz_pri->lock);
	cpm_set_bit(USBPCR_POR, CPM_USBPCR);
	spin_unlock(&jz_pri->lock);

	msleep(1);

	spin_lock(&jz_pri->lock);
	cpm_clear_bit(USBPCR_POR, CPM_USBPCR);
	spin_unlock(&jz_pri->lock);

	msleep(1);
}

static void set_charger_current_work(struct work_struct *work)
{
	struct dwc_jz_pri *jz_pri;
	dwc_otg_core_if_t *core_if;
	int insert;
	int ret;

	jz_pri = container_of(work, struct dwc_jz_pri, charger_delay_work.work);

	if (jz_pri->core_if == NULL) {
		schedule_delayed_work(&jz_pri->charger_delay_work, msecs_to_jiffies(600));
	} else {
		core_if = (dwc_otg_core_if_t *)jz_pri->core_if;

		ret = regulator_get_current_limit(jz_pri->ucharger);
		printk("Before changed: the current is %d\n", ret);

		insert = get_pin_status(jz_pri->dete);
		if ((insert == 1) && (core_if->frame_num == 0) &&	\
			(core_if->op_state == B_PERIPHERAL) &&		\
			(jz_pri->pullup_on == 1)) {
			regulator_set_current_limit(jz_pri->ucharger, 0, 800000);
		}
		ret = regulator_get_current_limit(jz_pri->ucharger);
		printk("After changed: the current is %d\n", ret);
	}
}

static void set_charger_current(struct dwc_jz_pri *jz_pri)
{
	if ( jz_pri->irq > 0 ) {
		schedule_delayed_work(&jz_pri->charger_delay_work,
			msecs_to_jiffies(600));
	}
}

static void usb_detect_work(struct work_struct *work)
{
	struct dwc_jz_pri *jz_pri;
	int insert;

	jz_pri = container_of(work, struct dwc_jz_pri, work.work);
	insert = get_pin_status(jz_pri->dete);

	pr_info("USB %s\n", insert ? "connect" : "disconnect");
#ifdef CONFIG_DISABLE_PHY
	jz_dwc_phy_switch(jz_pri, insert);
#endif

	if (!IS_ERR(jz_pri->ucharger)) {
		if (insert) {
			schedule_delayed_work(&jz_pri->charger_delay_work,
					msecs_to_jiffies(600));
		} else {
			regulator_set_current_limit(jz_pri->ucharger, 0, 400000);
			printk("Now recovery 400mA\n");
		}
	}
	enable_irq(jz_pri->irq);
}

static irqreturn_t usb_detect_interrupt(int irq, void *dev_id)
{
	struct dwc_jz_pri *jz_pri = (struct dwc_jz_pri *)dev_id;

	disable_irq_nosync(irq);
	schedule_delayed_work(&jz_pri->work, msecs_to_jiffies(200));

	return IRQ_HANDLED;
}

void jz_dwc_set_vbus(dwc_otg_core_if_t *core_if, int is_on)
{
	int ret = 0;
	struct dwc_jz_pri *jz_pri = core_if->jz_pri;

	if (jz_pri == NULL || jz_pri->vbus == NULL) {
		return;
	}

	mutex_lock(&jz_pri->mutex);
	if (is_on) {
		if (!regulator_is_enabled(jz_pri->vbus))
			ret = regulator_enable(jz_pri->vbus);
	} else {
		if (regulator_is_enabled(jz_pri->vbus))
			ret = regulator_disable(jz_pri->vbus);
	}
	mutex_unlock(&jz_pri->mutex);
}

struct dwc_jz_pri *jz_dwc_init(void)
{
	struct dwc_jz_pri *jz_pri;
	unsigned int ref_clk_div = CONFIG_EXTAL_CLOCK / 24;
	unsigned int usbpcr1;

	jz_pri = DWC_ALLOC(sizeof(struct dwc_jz_pri));

	jz_pri->clk = clk_get(NULL, OTG_CLK_NAME);
	if (IS_ERR(jz_pri->clk)) {
		dwc_error("clk gate get error\n");
		return NULL;
	}
	clk_enable(jz_pri->clk);

#ifdef CONFIG_REGULATOR
	jz_pri->vbus = regulator_get(NULL, VBUS_REG_NAME);

	if (IS_ERR(jz_pri->vbus)) {
		dwc_error("regulator %s get error\n", VBUS_REG_NAME);
	}
	if (regulator_is_enabled(jz_pri->vbus))
		regulator_disable(jz_pri->vbus);

	jz_pri->ucharger = regulator_get(NULL, UCHARGER_REG_NAME);

	if (IS_ERR(jz_pri->ucharger)) {
		dwc_error("regulator %s get error\n", UCHARGER_REG_NAME);
	} else {
		INIT_DELAYED_WORK(&jz_pri->charger_delay_work, set_charger_current_work);
	}
#else
#error DWC OTG driver can NOT work without regulator!
#endif

	spin_lock_init(&jz_pri->lock);
	mutex_init(&jz_pri->mutex);
	jz_pri->dete = &dete_pin;
	jz_pri->start = jz_pri_start;

	jz_pri->callback = set_charger_current;
	if (gpio_request_one(jz_pri->dete->num,
			     GPIOF_DIR_IN, "usb-charger-detect")) {
		dwc_error("no detect pin available\n");
		jz_pri->dete->num = -1;
	} else {
		dwc_info("request GPIO_USB_DETE: %d\n", jz_pri->dete->num);
	}

	if (gpio_is_valid(jz_pri->dete->num)) {
		int ret;
		ret = request_irq(gpio_to_irq(jz_pri->dete->num),
					  usb_detect_interrupt,
					  IRQF_TRIGGER_RISING
					  | IRQF_TRIGGER_FALLING
					  | IRQF_DISABLED ,
					  "usb-detect", jz_pri);
		if (ret) {
			dwc_error("request usb-detect fail\n");
		} else {
			jz_pri->irq = gpio_to_irq(jz_pri->dete->num);
			disable_irq_nosync(jz_pri->irq);
			INIT_DELAYED_WORK(&jz_pri->work, usb_detect_work);
		}
	}

	/* select dwc otg */
	cpm_set_bit(USBPCR1_USB_SEL, CPM_USBPCR1);

	/* select utmi data bus width of port0 to 16bit/30M */
	cpm_set_bit(USBPCR1_WORD_IF0, CPM_USBPCR1);

	usbpcr1 = cpm_inl(CPM_USBPCR1);
	usbpcr1 &= ~(0x3 << 24);
	usbpcr1 |= (ref_clk_div << 24);
	cpm_outl(usbpcr1, CPM_USBPCR1);

	/* fil */
	cpm_outl(0, CPM_USBVBFIL);

	/* rdt */
	cpm_outl(0x96, CPM_USBRDT);

	/* rdt - filload_en */
	cpm_set_bit(USBRDT_VBFIL_LD_EN, CPM_USBRDT);

	/* TXRISETUNE & TXVREFTUNE. */
	cpm_outl(0x3f, CPM_USBPCR);
	cpm_outl(0x35, CPM_USBPCR);

	/* enable tx pre-emphasis */
	cpm_set_bit(USBPCR_TXPREEMPHTUNE, CPM_USBPCR);

	/* OTGTUNE adjust */
	cpm_outl(7 << 14, CPM_USBPCR);

        /* enalbe OTG PHY */

	jz_dwc_phy_switch(jz_pri, 1);
	jz_dwc_set_device_only_mode();
	jz_dwc_phy_reset(jz_pri);
	jz_dwc_phy_switch(jz_pri, 1);

	/*
	 * Close VBUS detect in DWC-OTG PHY.
	 */
	*(unsigned int*)0xb3500000 |= 0xc;

	return jz_pri;
}
