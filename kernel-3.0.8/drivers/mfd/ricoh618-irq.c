/*
 * driver/mfd/ricoh618-irq.c
 *
 * Interrupt driver for RICOH RN5T618 power management chip.
 *
 * Copyright (C) 2012-2013 RICOH COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mfd/ricoh618.h>


enum int_type {
	SYS_INT  = 0x1,
	DCDC_INT = 0x2,
	ADC_INT  = 0x8,
	GPIO_INT = 0x10,
	CHG_INT	= 0x40,
};


static int irq_en_add[] = {
	RICOH618_INT_EN_SYS,
	RICOH618_INT_EN_DCDC,
	RICOH618_INT_EN_ADC1,
	RICOH618_INT_EN_ADC2,
	RICOH618_INT_EN_ADC3,
	RICOH618_INT_EN_GPIO,
	RICOH618_INT_EN_GPIO2,
	RICOH618_INT_MSK_CHGCTR,
	RICOH618_INT_MSK_CHGSTS1,
	RICOH618_INT_MSK_CHGSTS2,
	RICOH618_INT_MSK_CHGERR
};

static int irq_mon_add[] = {
	RICOH618_INT_MON_SYS,
	RICOH618_INT_MON_DCDC,
	RICOH618_INT_IR_ADCL,
	RICOH618_INT_IR_ADCH,
	RICOH618_INT_IR_ADCEND,
	RICOH618_INT_IR_GPIOR,
	RICOH618_INT_IR_GPIOF,
	RICOH618_INT_MON_CHGCTR,
	RICOH618_INT_MON_CHGSTS1,
	RICOH618_INT_MON_CHGSTS2,
	RICOH618_INT_MON_CHGERR
};

static int irq_clr_add[] = {
	RICOH618_INT_IR_SYS,
	RICOH618_INT_IR_DCDC,
	RICOH618_INT_IR_ADCL,
	RICOH618_INT_IR_ADCH,
	RICOH618_INT_IR_ADCEND,
	RICOH618_INT_IR_GPIOR,
	RICOH618_INT_IR_GPIOF,
	RICOH618_INT_IR_CHGCTR,
	RICOH618_INT_IR_CHGSTS1,
	RICOH618_INT_IR_CHGSTS2,
	RICOH618_INT_IR_CHGERR
};

static int main_int_type[] = {
	SYS_INT,
	DCDC_INT,
	ADC_INT,
	ADC_INT,
	ADC_INT,
	GPIO_INT,
	GPIO_INT,
	CHG_INT,
	CHG_INT,
	CHG_INT,
	CHG_INT,
};

struct ricoh618_irq_data {
	u8	int_type;
	u8	master_bit;
	u8	int_en_bit;
	u8	mask_reg_index;
	int	grp_index;
};

#define RICOH618_IRQ(_int_type, _master_bit, _grp_index, _int_bit, _mask_ind) \
	{						\
		.int_type	= _int_type,		\
		.master_bit	= _master_bit,		\
		.grp_index	= _grp_index,		\
		.int_en_bit	= _int_bit,		\
		.mask_reg_index	= _mask_ind,		\
	}

static const struct ricoh618_irq_data ricoh618_irqs[RICOH618_NR_IRQS] = {
	[RICOH618_IRQ_POWER_ON]		= RICOH618_IRQ(SYS_INT,  0, 0, 0, 0),
	[RICOH618_IRQ_EXTIN]		= RICOH618_IRQ(SYS_INT,  0, 1, 1, 0),
	[RICOH618_IRQ_PRE_VINDT]	= RICOH618_IRQ(SYS_INT,  0, 2, 2, 0),
	[RICOH618_IRQ_PREOT]		= RICOH618_IRQ(SYS_INT,  0, 3, 3, 0),
	[RICOH618_IRQ_POWER_OFF]	= RICOH618_IRQ(SYS_INT,  0, 4, 4, 0),
	[RICOH618_IRQ_NOE_OFF]		= RICOH618_IRQ(SYS_INT,  0, 5, 5, 0),
	[RICOH618_IRQ_WD]		= RICOH618_IRQ(SYS_INT,  0, 6, 6, 0),

	[RICOH618_IRQ_DC1LIM]		= RICOH618_IRQ(DCDC_INT, 1, 0, 0, 1),
	[RICOH618_IRQ_DC2LIM]		= RICOH618_IRQ(DCDC_INT, 1, 1, 1, 1),
	[RICOH618_IRQ_DC3LIM]		= RICOH618_IRQ(DCDC_INT, 1, 2, 2, 1),

	[RICOH618_IRQ_ILIMLIR]		= RICOH618_IRQ(ADC_INT,  3, 0, 0, 2),
	[RICOH618_IRQ_VBATLIR]		= RICOH618_IRQ(ADC_INT,  3, 1, 1, 2),
	[RICOH618_IRQ_VADPLIR]		= RICOH618_IRQ(ADC_INT,  3, 2, 2, 2),
	[RICOH618_IRQ_VUSBLIR]		= RICOH618_IRQ(ADC_INT,  3, 3, 3, 2),
	[RICOH618_IRQ_VSYSLIR]		= RICOH618_IRQ(ADC_INT,  3, 4, 4, 2),
	[RICOH618_IRQ_VTHMLIR]		= RICOH618_IRQ(ADC_INT,  3, 5, 5, 2),
	[RICOH618_IRQ_AIN1LIR]		= RICOH618_IRQ(ADC_INT,  3, 6, 6, 2),
	[RICOH618_IRQ_AIN0LIR]		= RICOH618_IRQ(ADC_INT,  3, 7, 7, 2),

	[RICOH618_IRQ_ILIMHIR]		= RICOH618_IRQ(ADC_INT,  3, 8, 0, 3),
	[RICOH618_IRQ_VBATHIR]		= RICOH618_IRQ(ADC_INT,  3, 9, 1, 3),
	[RICOH618_IRQ_VADPHIR]		= RICOH618_IRQ(ADC_INT,  3, 10, 2, 3),
	[RICOH618_IRQ_VUSBHIR]		= RICOH618_IRQ(ADC_INT,  3, 11, 3, 3),
	[RICOH618_IRQ_VSYSHIR]		= RICOH618_IRQ(ADC_INT,  3, 12, 4, 3),
	[RICOH618_IRQ_VTHMHIR]		= RICOH618_IRQ(ADC_INT,  3, 13, 5, 3),
	[RICOH618_IRQ_AIN1HIR]		= RICOH618_IRQ(ADC_INT,  3, 14, 6, 3),
	[RICOH618_IRQ_AIN0HIR]		= RICOH618_IRQ(ADC_INT,  3, 15, 7, 3),

	[RICOH618_IRQ_ADC_ENDIR]	= RICOH618_IRQ(ADC_INT,  3, 16, 0, 4),

	[RICOH618_IRQ_GPIO0]		= RICOH618_IRQ(GPIO_INT, 4, 0, 0, 5),
	[RICOH618_IRQ_GPIO1]		= RICOH618_IRQ(GPIO_INT, 4, 1, 1, 5),
	[RICOH618_IRQ_GPIO2]		= RICOH618_IRQ(GPIO_INT, 4, 2, 2, 5),
	[RICOH618_IRQ_GPIO3]		= RICOH618_IRQ(GPIO_INT, 4, 3, 3, 5),

	[RICOH618_IRQ_FVADPDETSINT]	= RICOH618_IRQ(CHG_INT, 6, 0, 0, 7),
	[RICOH618_IRQ_FVUSBDETSINT]	= RICOH618_IRQ(CHG_INT, 6, 1, 1, 7),
	[RICOH618_IRQ_FVADPLVSINT]	= RICOH618_IRQ(CHG_INT, 6, 2, 2, 7),
	[RICOH618_IRQ_FVUSBLVSINT]	= RICOH618_IRQ(CHG_INT, 6, 3, 3, 7),
	[RICOH618_IRQ_FWVADPSINT]	= RICOH618_IRQ(CHG_INT, 6, 4, 4, 7),
	[RICOH618_IRQ_FWVUSBSINT]	= RICOH618_IRQ(CHG_INT, 6, 5, 5, 7),

	[RICOH618_IRQ_FONCHGINT]	= RICOH618_IRQ(CHG_INT, 6, 6, 0, 8),
	[RICOH618_IRQ_FCHGCMPINT]	= RICOH618_IRQ(CHG_INT, 6, 7, 1, 8),
	[RICOH618_IRQ_FBATOPENINT]	= RICOH618_IRQ(CHG_INT, 6, 8, 2, 8),
	[RICOH618_IRQ_FSLPMODEINT]	= RICOH618_IRQ(CHG_INT, 6, 9, 3, 8),
	[RICOH618_IRQ_FBTEMPJTA1INT]	= RICOH618_IRQ(CHG_INT, 6, 10, 4, 8),
	[RICOH618_IRQ_FBTEMPJTA2INT]	= RICOH618_IRQ(CHG_INT, 6, 11, 5, 8),
	[RICOH618_IRQ_FBTEMPJTA3INT]	= RICOH618_IRQ(CHG_INT, 6, 12, 6, 8),
	[RICOH618_IRQ_FBTEMPJTA4INT]	= RICOH618_IRQ(ADC_INT, 6, 13, 7, 8),

	[RICOH618_IRQ_FCURTERMINT]	= RICOH618_IRQ(CHG_INT, 6, 14, 0, 9),
	[RICOH618_IRQ_FVOLTERMINT]	= RICOH618_IRQ(CHG_INT, 6, 15, 1, 9),
	[RICOH618_IRQ_FICRVSINT]	= RICOH618_IRQ(CHG_INT, 6, 16, 2, 9),
	[RICOH618_IRQ_FPOOR_CHGCURINT]	= RICOH618_IRQ(CHG_INT, 6, 17, 3, 9),
	[RICOH618_IRQ_FOSCFDETINT1]	= RICOH618_IRQ(CHG_INT, 6, 18, 4, 9),
	[RICOH618_IRQ_FOSCFDETINT2]	= RICOH618_IRQ(CHG_INT, 6, 19, 5, 9),
	[RICOH618_IRQ_FOSCFDETINT3]	= RICOH618_IRQ(CHG_INT, 6, 20, 6, 9),
	[RICOH618_IRQ_FOSCMDETINT]	= RICOH618_IRQ(CHG_INT, 6, 21, 7, 9),

	[RICOH618_IRQ_FDIEOFFINT]	= RICOH618_IRQ(CHG_INT, 6, 22, 0, 10),
	[RICOH618_IRQ_FDIEERRINT]	= RICOH618_IRQ(CHG_INT, 6, 23, 1, 10),
	[RICOH618_IRQ_FBTEMPERRINT]	= RICOH618_IRQ(CHG_INT, 6, 24, 2, 10),
	[RICOH618_IRQ_FVBATOVINT]	= RICOH618_IRQ(CHG_INT, 6, 25, 3, 10),
	[RICOH618_IRQ_FTTIMOVINT]	= RICOH618_IRQ(CHG_INT, 6, 26, 4, 10),
	[RICOH618_IRQ_FRTIMOVINT]	= RICOH618_IRQ(CHG_INT, 6, 27, 5, 10),
	[RICOH618_IRQ_FVADPOVSINT]	= RICOH618_IRQ(CHG_INT, 6, 28, 6, 10),
	[RICOH618_IRQ_FVUSBOVSINT]	= RICOH618_IRQ(CHG_INT, 6, 29, 7, 10),

};

static void ricoh618_irq_lock(struct irq_data *irq_data)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);

	mutex_lock(&ricoh618->irq_lock);
}

static void ricoh618_irq_unmask(struct irq_data *irq_data)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - ricoh618->irq_base;
	const struct ricoh618_irq_data *data = &ricoh618_irqs[__irq];

	ricoh618->group_irq_en[data->master_bit] |= (1 << data->grp_index);
	if (ricoh618->group_irq_en[data->master_bit]) {
		ricoh618->intc_inten_reg |= 1 << data->master_bit;
		/*printk("======> %s    %d   %d  0x%x \n\n", __func__, __LINE__, data->master_bit, ricoh618->group_irq_en[data->master_bit]);*/
	}

	if(data->master_bit == 6)	/* if Charger */
		ricoh618->irq_en_reg[data->mask_reg_index] &= ~(1 << data->int_en_bit);
	else
		ricoh618->irq_en_reg[data->mask_reg_index] |= 1 << data->int_en_bit;
}

static void ricoh618_irq_mask(struct irq_data *irq_data)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - ricoh618->irq_base;
	const struct ricoh618_irq_data *data = &ricoh618_irqs[__irq];

	ricoh618->group_irq_en[data->master_bit] &= ~(1 << data->grp_index);
	if (!ricoh618->group_irq_en[data->master_bit])
		ricoh618->intc_inten_reg &= ~(1 << data->master_bit);

	if(data->master_bit == 6)	/* if Charger */
		ricoh618->irq_en_reg[data->mask_reg_index] |= 1 << data->int_en_bit;
	else
		ricoh618->irq_en_reg[data->mask_reg_index] &= ~(1 << data->int_en_bit);
}

static void ricoh618_irq_sync_unlock(struct irq_data *irq_data)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);
	int i;

	for (i = 0; i < ARRAY_SIZE(ricoh618->gpedge_reg); i++) {
		if (ricoh618->gpedge_reg[i] != ricoh618->gpedge_cache[i]) {
			if (!WARN_ON(ricoh618_write(ricoh618->dev,
						    ricoh618->gpedge_add[i],
						    ricoh618->gpedge_reg[i])))
				ricoh618->gpedge_cache[i] =
						ricoh618->gpedge_reg[i];
		}
	}

	for (i = 0; i < ARRAY_SIZE(ricoh618->irq_en_reg); i++) {
		if (ricoh618->irq_en_reg[i] != ricoh618->irq_en_cache[i]) {
			if (!WARN_ON(ricoh618_write(ricoh618->dev,
					    ricoh618->irq_en_add[i],
						    ricoh618->irq_en_reg[i])))
				ricoh618->irq_en_cache[i] =
						ricoh618->irq_en_reg[i];
			}
	}

	if (ricoh618->intc_inten_reg != ricoh618->intc_inten_cache) {
		if (!WARN_ON(ricoh618_write(ricoh618->dev,
				RICOH618_INTC_INTEN, ricoh618->intc_inten_reg)))
			ricoh618->intc_inten_cache = ricoh618->intc_inten_reg;
	}

	mutex_unlock(&ricoh618->irq_lock);
}

static int ricoh618_irq_set_type(struct irq_data *irq_data, unsigned int type)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - ricoh618->irq_base;
	const struct ricoh618_irq_data *data = &ricoh618_irqs[__irq];
	int val = 0;
	int gpedge_index;
	int gpedge_bit_pos;

	if (data->int_type & GPIO_INT) {
		gpedge_index = data->int_en_bit / 4;
		gpedge_bit_pos = data->int_en_bit % 4;

		if (type & IRQ_TYPE_EDGE_FALLING)
			val |= 0x2;

		if (type & IRQ_TYPE_EDGE_RISING)
			val |= 0x1;

		ricoh618->gpedge_reg[gpedge_index] &= ~(3 << gpedge_bit_pos);
		ricoh618->gpedge_reg[gpedge_index] |= (val << gpedge_bit_pos);
		ricoh618_irq_unmask(irq_data);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ricoh618_irq_set_wake(struct irq_data *irq_data, unsigned int on)
{
	struct ricoh618 *ricoh618 = irq_data_get_irq_chip_data(irq_data);
	return irq_set_irq_wake(ricoh618->chip_irq, on);	//i2c->irq
}
#else
#define ricoh618_irq_set_wake NULL
#endif

static irqreturn_t ricoh618_irq(int irq, void *data)
{
	struct ricoh618 *ricoh618 = data;
	u8 int_sts[MAX_INTERRUPT_MASKS];
	u8 master_int;
	u8 master_test;
	int i;
	int ret;

	/* Clear the status */
	for (i = 0; i < MAX_INTERRUPT_MASKS; i++)
		int_sts[i] = 0;

	ret  = ricoh618_read(ricoh618->dev, RICOH618_INTC_INTMON,
						&master_int);
	printk("----> the int monitor register 0x%x \n", master_int);
	if (ret < 0) {
		dev_err(ricoh618->dev, "Error in reading reg 0x%02x "
			"error: %d\n", RICOH618_INTC_INTMON, ret);
		return IRQ_HANDLED;
	}
	/*ret  = ricoh618_read(ricoh618->dev, RICOH618_INTC_INTEN,
						&master_test);
	printk("----> the int enable register 0x%x \n", master_test);
	if (ret < 0) {
		dev_err(ricoh618->dev, "Error in reading reg 0x%02x "
			"error: %d\n", RICOH618_INTC_INTMON, ret);
		return IRQ_HANDLED;
	}
*/
	for (i = 0; i <= MAX_INTERRUPT_MASKS; ++i) {
		if (!(master_int & ricoh618->main_int_type[i]))
			continue;
		ret = ricoh618_read(ricoh618->dev,
				ricoh618->irq_mon_add[i], &int_sts[i]);
		if (ret < 0) {
			dev_err(ricoh618->dev, "Error in reading reg 0x%02x "
				"error: %d\n", ricoh618->irq_mon_add[i], ret);
			int_sts[i] = 0;
			continue;
		}
		/*printk("----> read the interrupt register 0x%x  0x%x  %d \n",
				ricoh618->irq_mon_add[i], int_sts[i], i);*/

		ret = ricoh618_write(ricoh618->dev,
				ricoh618->irq_clr_add[i], ~int_sts[i]);
		if (ret < 0) {
			dev_err(ricoh618->dev, "Error in reading reg 0x%02x "
				"error: %d\n", ricoh618->irq_clr_add[i], ret);
		}
	}

	/* Merge gpio interrupts  for rising and falling case*/
	int_sts[5] |= int_sts[6];

	/* Call interrupt handler if enabled */
	for (i = 0; i < RICOH618_NR_IRQS; ++i) {
		const struct ricoh618_irq_data *data = &ricoh618_irqs[i];
		if ((int_sts[data->mask_reg_index] & (1 << data->int_en_bit)) &&
			(ricoh618->group_irq_en[data->master_bit] &
					(1 << data->grp_index))) {
			handle_nested_irq(ricoh618->irq_base + i);
		}
		/*if (i == 31)
			printk("~~~~~> dispatch 0x%x  0x%x %d  0x%x  0x%x  %d \n",int_sts[data->mask_reg_index], (1 << data->int_en_bit), data->int_en_bit,ricoh618->group_irq_en[data->master_bit], (1 << data->grp_index), data->grp_index);*/
	}
	printk("----> ricoh618_irq running!!!\n");
	return IRQ_HANDLED;
}

static struct irq_chip ricoh618_irq_chip = {
	.name = "ricoh618",
	.irq_mask = ricoh618_irq_mask,
	.irq_unmask = ricoh618_irq_unmask,
	.irq_bus_lock = ricoh618_irq_lock,
	.irq_bus_sync_unlock = ricoh618_irq_sync_unlock,
	.irq_set_type = ricoh618_irq_set_type,
	.irq_set_wake = ricoh618_irq_set_wake,
};

int ricoh618_irq_init(struct ricoh618 *ricoh618, int irq,
				int irq_base)
{
	int i, ret;
	u8 master_test;

	if (!irq_base) {
		dev_warn(ricoh618->dev, "No interrupt support on IRQ base\n");
		return -EINVAL;
	}

	mutex_init(&ricoh618->irq_lock);

	ricoh618->irq_en_add = irq_en_add;
	ricoh618->irq_mon_add = irq_mon_add;
	ricoh618->irq_clr_add = irq_clr_add;
	ricoh618->main_int_type = main_int_type;
	for (i = 0; i < MAX_INTERRUPT_MASKS; ++i) {
		printk("   0x%x \n",ricoh618->main_int_type[i]);
	}

	/* Initialize all locals to 0 */
	for (i = 0; i < MAX_INTERRUPT_MASKS; i++) {
		ricoh618->irq_en_cache[i] = 0;
		ricoh618->irq_en_reg[i] = 0;
	}
	ricoh618->intc_inten_cache = 0;
	ricoh618->intc_inten_reg = 0;
	for (i = 0; i < MAX_GPEDGE_REG; i++) {
		ricoh618->gpedge_cache[i] = 0;
		ricoh618->gpedge_reg[i] = 0;
	}

	/* Initailize all int register to 0 */
	for (i = 0; i < MAX_INTERRUPT_MASKS; i++)  {
		ret = ricoh618_write(ricoh618->dev,
				ricoh618->irq_en_add[i],
				ricoh618->irq_en_reg[i]);
		printk("     %x    %x    \n", ricoh618->irq_en_add[i], ricoh618->irq_en_reg[i]);
		if (ret < 0)
			dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", ricoh618->irq_en_add[i], ret);
	}

	for (i = 0; i < MAX_GPEDGE_REG; i++)  {
		ret = ricoh618_write(ricoh618->dev,
				ricoh618->gpedge_add[i],
				ricoh618->gpedge_reg[i]);
		if (ret < 0)
			dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", ricoh618->gpedge_add[i], ret);
	}

	/*ret = ricoh618_write(ricoh618->dev, RICOH618_INTC_INTEN, 0xdb);*/
	ret = ricoh618_write(ricoh618->dev, RICOH618_INTC_INTEN, 0x0);
	if (ret < 0)
		dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", RICOH618_INTC_INTEN, ret);
	ret  = ricoh618_read(ricoh618->dev, RICOH618_INTC_INTEN,
						&master_test);
	if (ret < 0)
		dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", RICOH618_INTC_INTEN, ret);
	printk("----> the int enable register 0x%x \n", master_test);

	/*ret = ricoh618_write(ricoh618->dev, RICOH618_INTC_INTPOL, 0x1);
	ret  = ricoh618_read(ricoh618->dev, RICOH618_INTC_INTPOL,
						&master_test);
	if (ret < 0)
		dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", RICOH618_INTC_INTPOL, ret);
	printk("----> the int polarity register 0x%x \n", master_test);
*/

	/* Clear all interrupts in case they woke up active. */
	for (i = 0; i < MAX_INTERRUPT_MASKS; i++)  {
		ret = ricoh618_write(ricoh618->dev,
					ricoh618->irq_clr_add[i], 0);
		if (ret < 0)
			dev_err(ricoh618->dev, "Error in writing reg 0x%02x "
				"error: %d\n", ricoh618->irq_clr_add[i], ret);
	}

	ricoh618->irq_base = irq_base;
	ricoh618->chip_irq = irq;

	for (i = 0; i < RICOH618_NR_IRQS; i++) {
		int __irq = i + ricoh618->irq_base;
		irq_set_chip_data(__irq, ricoh618);
		irq_set_chip_and_handler(__irq, &ricoh618_irq_chip,
					 handle_simple_irq);
		irq_set_nested_thread(__irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(__irq, IRQF_VALID);
#endif
	}

	ret = request_threaded_irq(irq, NULL, ricoh618_irq, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "ricoh618", ricoh618);
	if (ret < 0)
		dev_err(ricoh618->dev, "Error in registering interrupt "
				"error: %d\n", ret);
	if (!ret) {
		device_init_wakeup(ricoh618->dev, 1);
		enable_irq_wake(irq);
	}
	return ret;
}

int ricoh618_irq_exit(struct ricoh618 *ricoh618)
{
	if (ricoh618->chip_irq)
		free_irq(ricoh618->chip_irq, ricoh618);
	return 0;
}

