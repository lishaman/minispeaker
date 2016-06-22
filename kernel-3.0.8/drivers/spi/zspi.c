/*
 * zspi.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Written by Zoro <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spi/zspi.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <gpio.h>

#define ZSPI_SCK		GPIO_PB(28)
#define ZSPI_MOSI		GPIO_PB(29)
#define ZSPI_MISO		GPIO_PB(20)
#define ZSPI_CS			GPIO_PB(31)
#define ZSPI_HZ			75000000

#define SPI_CPOL		0
#define tISCO			2500
#define MAX_USE_COUNT		1

#define zspi_print(fmt, ...) printk("zspi l%d: "fmt, __LINE__, ##__VA_ARGS__)
#define spidelay(nsecs)	do {} while (0)

int use_count = 0;

static inline void setsck(struct zspi *spi, int is_on)
{
	gpio_set_value(spi->gpio.sck, is_on);
}

static inline void setmosi(struct zspi *spi, int is_on)
{
	gpio_set_value(spi->gpio.mosi, is_on);
}

static inline int getmiso(struct zspi *spi)
{
	return !!gpio_get_value(spi->gpio.miso);
}

void chipselect(struct zspi *spi, int is_active)
{
	if (is_active)
		setsck(spi, SPI_CPOL);

	gpio_set_value(spi->gpio.cs, !is_active);
}

static u32 zspi_txrx(struct zspi *spi, u32 word, u32 nsecs)
{
	u8 bits = 8;
	unsigned int cpol = SPI_CPOL;

	/* clock starts at inactive polarity */
	for (word <<= (32 - bits); likely(bits); bits--) {

		/* setup MSB (to slave) on trailing edge */
		setmosi(spi, word & (1 << 31));
		spidelay(nsecs);	/* T(setup) */

		setsck(spi, !cpol);
		spidelay(nsecs);

		/* sample MSB (from slave) on leading edge */
		word <<= 1;
		word |= getmiso(spi);

		setsck(spi, cpol);
	}
	return word;
}

unsigned char zspi_read(struct zspi *zspi)
{
	unsigned char value = 0;
	unsigned int nsecs = 100;
	unsigned long flag;

	spin_lock_irqsave(&zspi->lock, flag);

	chipselect(zspi, 1);
	ndelay(nsecs);

	value = zspi_txrx(zspi, 0, zspi->nsecs);

	ndelay(nsecs);
	chipselect(zspi, 0);
	/*
	 * Chip select off time output mode 2500ns.
	 */
	ndelay(tISCO);

	spin_unlock_irqrestore(&zspi->lock, flag);

	return value;
}
EXPORT_SYMBOL(zspi_read);

void zspi_write(struct zspi *zspi, unsigned char data)
{
	unsigned char value = 0;
	unsigned int nsecs = 100;
	unsigned long flag;

	spin_lock_irqsave(&zspi->lock, flag);

	chipselect(zspi, 1);
	ndelay(nsecs);

	value = zspi_txrx(zspi, data, zspi->nsecs);

	ndelay(nsecs);
	chipselect(zspi, 0);
	/*
	 * Chip select off time input mode 2500ns.
	 */
	ndelay(tISCO);

	spin_unlock_irqrestore(&zspi->lock, flag);
}
EXPORT_SYMBOL(zspi_write);

struct zspi *zspi_driver_register(void)
{
	struct zspi *zspi;

	if (use_count >= MAX_USE_COUNT) {
		zspi_print("Can't register more than %d times\n", MAX_USE_COUNT);
		return NULL;
	}
	use_count++;

	zspi_print("register\n");
#define ZSPI_PIN_ALLOC(NUM, NAME, DIR, IV)					\
	do {									\
		if (gpio_request(NUM, NAME)) {					\
			zspi_print("no %s(%d) pin available\n", NAME, NUM);	\
			return NULL;						\
		} else {							\
			if (DIR)						\
				gpio_direction_output(NUM, IV);			\
			else							\
				gpio_direction_input(NUM);			\
		}								\
	} while (0)

	ZSPI_PIN_ALLOC(ZSPI_SCK, "zspi-sck", 1, 1);
	ZSPI_PIN_ALLOC(ZSPI_MOSI, "zspi-mosi", 1, 1);
	ZSPI_PIN_ALLOC(ZSPI_MISO, "zspi-miso", 0, 0);
	ZSPI_PIN_ALLOC(ZSPI_CS, "zspi-cs", 1, 1);

	zspi = kmalloc(sizeof(struct zspi), GFP_KERNEL);
	if (!zspi) {
		use_count--;
		zspi_print("zspi alloc error\n");
		return NULL;
	}

	zspi->gpio.sck	= ZSPI_SCK;
	zspi->gpio.mosi	= ZSPI_MOSI;
	zspi->gpio.miso	= ZSPI_MISO;
	zspi->gpio.cs	= ZSPI_CS;

	zspi->nsecs = (1000000000/2) / ZSPI_HZ;
	spin_lock_init(&zspi->lock);

	return zspi;
}
EXPORT_SYMBOL(zspi_driver_register);

void zspi_driver_unregister(struct zspi *zspi)
{
	if (zspi == NULL) {
		zspi_print("unregister error\n");
		return;
	}

	gpio_free(zspi->gpio.sck);
	gpio_free(zspi->gpio.mosi);
	gpio_free(zspi->gpio.miso);
	gpio_free(zspi->gpio.cs);

	kfree(zspi);

	if (use_count)
		use_count--;
	zspi_print("unregister\n");
}
EXPORT_SYMBOL(zspi_driver_unregister);

struct zspi *zspi_proc;

static int zspi_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	struct zspi *zspi;

	zspi_print("%s: %s\n", __func__, buffer);
	if (strncmp(buffer, "register", 8) == 0) {
		zspi = zspi_driver_register();
		if (zspi == NULL)
			zspi_print("proc register error\n");
		else
			zspi_proc = zspi;
	} else if (strncmp(buffer, "unregister", 10) == 0) {
		zspi_driver_unregister(zspi_proc);
		zspi_proc = NULL;
	}

	return count;
}

static int __init zspi_debug_proc(void)
{
	struct proc_dir_entry *res;

	res = create_proc_entry("zspi", 0666, NULL);
	if (res) {
		res->read_proc = NULL;
		res->write_proc = zspi_write_proc;
		res->data = NULL;
	}
	return 0;
}

module_init(zspi_debug_proc);
