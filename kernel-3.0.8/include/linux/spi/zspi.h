#ifndef __ZSPI_H__
#define __ZSPI_H__

struct spi_pin {
	int sck;
	int mosi;
	int miso;
	int cs;
};

struct zspi {
	struct spi_pin gpio;
	int nsecs;
	spinlock_t lock;
};

extern unsigned char zspi_read(struct zspi *zspi);
extern void zspi_write(struct zspi *zspi, unsigned char data);
extern struct zspi *zspi_driver_register(void);
extern void zspi_driver_unregister(struct zspi *zspi);
#endif
