/*
 *  drivers/input/misc/sc8800g.c
 *  SC8800S/G TD module driver, spi version
 *
 *  Copyright (C) 2010 Ingenic Semiconductor, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/sc8800s.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/sched.h>

//#define SPI_DEBUG_DATA

/* buffer infomation. */
#define SC8800S_BUFFER_SIZE	(16 * 1024)
#define SPI_BUFFER_SIZE		(8 * 1024)
#define	PACKET_HEADER_SIZE	(16)
#define	PACKET_ALIGNMENT	(64)
#define	PACKET_ALIGNMENT_WRITE	(32)	/* FIXME */
#define	MAX_PACKET_LENGTH	(SPI_BUFFER_SIZE - 2 * PACKET_ALIGNMENT)

/* header magic */
#define	HEADER_TAG		(0x7e7f)
#define	HEADER_TYPE		(0xaa55)

#define	HEADER_VALID	1
#define	HEADER_INVALID	0

/* fsm: spi bus status */
#define	SPI_BUS_IDLE		0
#define	SPI_BUS_SENDING		1
#define	SPI_BUS_RECEIVING	2

/* bp_rts is pending or not. */
#define	BP_RTS_PENDING		(1)
#define	BP_RTS_NO_PENDING	(0)

#define	AP_RTS_PENDING		(1)
#define	AP_RTS_NO_PENDING	(0)

#define SPI_BUS_STATUS_STRING(state) (\
    (state == SPI_BUS_IDLE)     ? "SPI_BUS_IDLE" :\
    (state == SPI_BUS_SENDING)  ? "SPI_BUS_SENDING" :\
    (state == SPI_BUS_RECEIVING)? "SPI_BUS_RECEIVING" : \
						  "UNKNOWN")

#define MIN(x, y)  ((x) < (y) ? (x) : (y))

static struct workqueue_struct *tu830_wqueue;
static int enable = 0;
static struct sc8800s_platform_data *pdata;
static bool sc8800s_resumed = true;

/* save information of packet to be sent. */
struct tx_packet_queue {
	u32 length;
	u8 *data;
};

/* packet header. */
struct modem_header {
	u16		tag;
	u16		type;
	u32		length;
	u32		index;
	u32		header_valid;
};

struct sc8800s_buf {
	/* spi read && write buffer */
	unsigned char *wdata;
	unsigned char *rdata;

	/* buffer for uplayer, cache for spi read data */
	unsigned char *buf;
	unsigned char *readp;
	unsigned char *writep;
	unsigned char *tail;
	unsigned int count;
};

struct sc8800s_data {
	struct	spi_device	*spi;
	struct	work_struct	read_work;	/* read packet */
	struct	work_struct	ready_work;	/* write packet */
	/* struct	mutex		rlock;	*/
	struct	mutex		lock;

	/* wait queue for sleep */
	wait_queue_head_t	queue_wr;
	wait_queue_head_t	queue_rd;
	wait_queue_head_t	queue_buf;
	wait_queue_head_t	queue_bus_idle;

	int can_read;
	int can_write;
	int buf_waiting;

	/* spi bus status. */
	int bus_status;

	/* bp_rst is pending.
	 * bp_rst comes while spi bus is busy 
	 * for transmitting data, in this case, 
	 * set bp_rts_pending.
	 */
	int bp_rts_pending;

	int ap_rts_pending;

	int td_rts_irq;
	int td_rdy_irq;

	/* spin lock to protect bus status. */
	spinlock_t spi_spin_lock;
	spinlock_t spin_rlock;

	/* packet to be transmitted. */
	struct tx_packet_queue packet_tx;

	struct modem_header *header;

	struct mutex buf_lock;

	struct wake_lock wakelock;
	struct wake_lock resume_lock;
};

static struct sc8800s_buf	spi_buf;
static struct sc8800s_data	*spi_info = NULL;

static void sc8800s_ap_ready(u8 val)
{
	gpio_direction_output(pdata->ap_bb_spi_rdy, (val > 0) ? 1 : 0);
}

/* send request to modem */
static void sc8800s_ap_rts(u8 val)
{
	gpio_direction_output(pdata->ap_bb_spi_rts, (val > 0) ? 1 : 0);
}

/* check packet header. */
static int verify_header(struct modem_header *hdr)
{
	if ((hdr->tag != HEADER_TAG) ||
	    (hdr->type != HEADER_TYPE) ||
	    (hdr->length > MAX_PACKET_LENGTH)) {
		return -1;
	} else {		/* ok */
		return 0;
	}
}

/* circular buffer routines */
static int sc8800s_buf_init(void)
{
	spi_info->header = kzalloc(sizeof(struct modem_header), GFP_KERNEL);
	if (spi_info->header == NULL) {
		spi_error("malloc header failed! \n");
		goto error_alloc_modem_header;
	}

	spi_buf.wdata = kzalloc(SPI_BUFFER_SIZE, GFP_KERNEL);	/* the first */
	if (spi_buf.wdata == NULL) {
		spi_error("malloc tx buffer failed! \n");
		goto error_alloc_write_buffer;
	}

	spi_buf.rdata = kzalloc(SPI_BUFFER_SIZE, GFP_KERNEL);
	if (spi_buf.rdata == NULL) {
		spi_error("malloc rx buffer failed! \n");
		goto error_alloc_read_buffer;
	}

	spi_buf.buf = kzalloc(SC8800S_BUFFER_SIZE, GFP_KERNEL);
	if (spi_buf.buf == NULL) {
		spi_error("malloc sc8800s buffer failed! \n");
		goto error_alloc_sc8800s_buffer;
	}

	spi_buf.readp = spi_buf.writep = spi_buf.buf;
	spi_buf.tail = spi_buf.buf + SC8800S_BUFFER_SIZE;

	return 0;

error_alloc_sc8800s_buffer:
	kfree (spi_buf.rdata);
error_alloc_read_buffer:
	kfree (spi_buf.wdata);
error_alloc_write_buffer:
	kfree (spi_info->header);
error_alloc_modem_header:
	return -ENOMEM;
}

static int get_buffer_free_room(void)
{
	return SC8800S_BUFFER_SIZE - spi_buf.count;
}

static int put_data_to_buffer(void *data, int count)
{
	int len = MIN(count, spi_buf.tail - spi_buf.writep);
	if (len < count) {
		memcpy(spi_buf.writep, data, len);
		memcpy(spi_buf.buf, (unsigned char *)data + len, count - len);
		spi_buf.writep = count - len + spi_buf.buf;
	} else {
		memcpy(spi_buf.writep, data, len);
		spi_buf.writep += len;
		if (spi_buf.writep == spi_buf.tail) {
			spi_buf.writep = spi_buf.buf;
		}
	}

	return count;
}

static int copy_to_user_buffer(char __user *dst, char *src, int count)
{
	int copies = 0;
	int left = count;
	int ret;

	do {
		ret = copy_to_user(dst + copies, src + copies, left);
		copies = left - ret;
		left = ret;
	} while (ret != 0);

	return ret;
}

static int get_userdata_from_buffer(char __user *buf, int count)
{
	int len;

	count = MIN(count, spi_buf.count);
	len  = MIN(count, spi_buf.tail - spi_buf.readp);
	if (len < count) {
		copy_to_user_buffer(buf, spi_buf.readp, len);
		copy_to_user_buffer(buf + len, spi_buf.buf, count - len);

		spi_buf.readp = count - len + spi_buf.buf;
	} else {
		copy_to_user_buffer(buf, spi_buf.readp, len);

		spi_buf.readp += len;
		if (spi_buf.readp == spi_buf.tail) {
			spi_buf.readp = spi_buf.buf;
		}
	}

	mutex_lock(&spi_info->buf_lock);
	spi_buf.count -= count;
	if (spi_info->buf_waiting) {
		spi_info->buf_waiting = 0;
		mutex_unlock(&spi_info->buf_lock);
		wake_up(&spi_info->queue_buf);
		spi_demand("Wake up waiting buffer room! \n");
	} else {
		mutex_unlock(&spi_info->buf_lock);
	}

	return count;
}

static void sc8800s_buf_free(void)
{
	kfree (spi_buf.rdata);
	kfree (spi_buf.wdata);
	kfree (spi_info->header);
	kfree (spi_buf.buf);
}

/* get packet header, first 16-byte. */
static int sc8800s_get_packet_header(struct sc8800s_data *sc8800s)
{
	struct spi_device *spi  = sc8800s->spi;
	struct modem_header *hdr = sc8800s->header;
	u16 *pbuf = (u16 *)hdr;
	int ret = 0;
	int err = 0;

	ret = spi_read(spi, (u8 *)hdr, PACKET_HEADER_SIZE);
#ifdef SPI_DEBUG_DATA
	print_hex_dump(KERN_DEFAULT, "SPI Header:", DUMP_PREFIX_NONE,
		16, 1, (u8 *)hdr, PACKET_HEADER_SIZE, 1);
#endif

	hdr->header_valid = HEADER_VALID;

	ret = verify_header(hdr);
	if (unlikely(ret)) {
		spi_error("SPI next check!\n");
		ret = verify_header((struct modem_header*)(++pbuf));
		if (ret == 0) {
			memmove(hdr, pbuf, PACKET_HEADER_SIZE - sizeof (u16));
			err = 1;
		}
	}

	if (unlikely(ret)) {
		spi_error("++++++:  packet info: tag = %04x, type = %04x, "
			  "len = %#x, index = %#x\n",
			  hdr->tag, hdr->type,
			  hdr->length, hdr->index);

		/*
		 * in case we receive a bad packet header, 
		 * we set packet length at its the maxim value to
		 * clear modem tx fifo which will make modem sofware
		 * more easy.
		 */
		hdr->length = 0;
		//hdr->length = MAX_PACKET_LENGTH;
		hdr->header_valid = HEADER_INVALID;

		goto out;
	}

	ret = err;

	spi_debug("#### hdr info: tag = %04x, type = %04x, "
		  "len = %08x, index = %#x\n",
		  hdr->tag, hdr->type, hdr->length, hdr->index);
out:
	return ret;
}

static ssize_t sc8800s_show_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", enable);
}

static ssize_t sc8800s_store_enable(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;
	int ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (enable == val)
		return count;

	if (0 == val)
		enable = val;

	mutex_lock(&spi_info->lock);

	if (val == 1) {
		gpio_direction_output(pdata->power_on, 1);
		msleep(10000);
		enable_irq(spi_info->td_rts_irq);
		enable_irq(spi_info->td_rdy_irq);
		enable = val;
	} else {
		gpio_direction_output(pdata->power_on, 0);
		disable_irq(spi_info->td_rts_irq);
		disable_irq(spi_info->td_rdy_irq);
	}

	mutex_unlock(&spi_info->lock);

	return ret ? ret : count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO,
		   sc8800s_show_enable,
		   sc8800s_store_enable);

static ssize_t sc8800s_show_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", spi_info->bus_status);
}

static ssize_t sc8800s_store_status(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;
	int ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	spi_info->bus_status = val;

	return ret ? ret : count;
}

static DEVICE_ATTR(status, S_IWUSR | S_IRUGO,
		   sc8800s_show_status,
		   sc8800s_store_status);

static struct attribute *sc8800s_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_status.attr,
	NULL
};

static const struct attribute_group sc8800s_attr_group = {
	.attrs	= sc8800s_attributes,
};

/* read data from modem */
static void sc8800s_rts_work(struct work_struct *work)
{
	struct sc8800s_data *sc8800s = container_of(work,
					struct sc8800s_data, read_work);
	struct modem_header *hdr = sc8800s->header;
	u32 len = 0;
	int extend_len_needed;

	extend_len_needed = sc8800s_get_packet_header(sc8800s);

	if (hdr->length > SPI_BUFFER_SIZE) {
		spi_error("data length: %d too big\n", hdr->length);
		hdr->header_valid = HEADER_INVALID;
	}

	/*
	 * clear modem tx fifo when get a invalid packet header,
	 * do not just return
	 */
	if (hdr->header_valid == HEADER_VALID) {
		/* read length should be 64-byte aligned */
		len = ((hdr->length + PACKET_HEADER_SIZE + PACKET_ALIGNMENT - 1) /
			PACKET_ALIGNMENT) * PACKET_ALIGNMENT - PACKET_HEADER_SIZE;

		if (extend_len_needed) {
			len += 2;
		}
	
		/* read packet data */
		if (len > 0)
			spi_read(sc8800s->spi, (u8 *)spi_buf.rdata, len);
#ifdef SPI_DEBUG_DATA
		print_hex_dump(KERN_DEFAULT, "SPI Data:", DUMP_PREFIX_NONE,
			16, 1, (u8 *)spi_buf.rdata, hdr->length, 1);
#endif

		mutex_lock(&sc8800s->buf_lock);
		if (get_buffer_free_room() < hdr->length) {
			sc8800s->buf_waiting = 1;
			mutex_unlock(&sc8800s->buf_lock);
			wait_event(sc8800s->queue_buf, sc8800s->buf_waiting == 0);
			spi_demand("Waiting buffer room! \n");
			mutex_lock(&sc8800s->buf_lock);
		}
		len = put_data_to_buffer(spi_buf.rdata, hdr->length);
		spi_buf.count += len;
		mutex_unlock(&sc8800s->buf_lock);

		/* wake up read thread */
		wake_up_interruptible(&sc8800s->queue_rd);
	}

	sc8800s->bus_status = SPI_BUS_IDLE;
	sc8800s_ap_ready(1);
	if (sc8800s->ap_rts_pending == AP_RTS_PENDING) {
		wake_up_interruptible(&sc8800s->queue_bus_idle);
	}
	//__gpio_unmask_irq(BB_AP_SPI_RTS);
	wake_unlock(&sc8800s->wakelock);
}

static irqreturn_t sc8800s_rts_handler(int irq, void *dev_id)
{
	struct sc8800s_data *sc8800s = dev_id;

	if (1 == gpio_get_value(pdata->td_rts_pin)) {
		//__gpio_ack_irq(BB_AP_SPI_RTS);
		//__gpio_unmask_irq(BB_AP_SPI_RTS);
		return IRQ_HANDLED;
	}

	if (sc8800s_resumed == false) {
		sc8800s_resumed = true;
		spi_debug("wake_lock_timeout = 1s\n");
		wake_lock_timeout(&sc8800s->resume_lock, 2000);

	}
	/* check bus status. */
	if (SPI_BUS_IDLE != sc8800s->bus_status) {
		if (printk_ratelimit())
			spi_error("+++: %s: bus busy: %s\n", __func__,
				SPI_BUS_STATUS_STRING(sc8800s->bus_status));
		//__gpio_ack_irq(BB_AP_SPI_RTS);
		//__gpio_unmask_irq(BB_AP_SPI_RTS);
		return IRQ_HANDLED;
	} else {
		sc8800s_ap_ready(0);
		sc8800s->bus_status = SPI_BUS_RECEIVING;
		wake_lock(&sc8800s->wakelock);
		spi_debug("###: spi bus idle, start to receive packet!\n");
		spi_debug("bp_rts_handler(): %s\n",
				SPI_BUS_STATUS_STRING(sc8800s->bus_status));
		if (!work_pending(&sc8800s->read_work))
			queue_work(tu830_wqueue, &sc8800s->read_work);
		else {
			pr_debug("pending rts work\n");
		}

		//__gpio_ack_irq(BB_AP_SPI_RTS);
		return IRQ_HANDLED;
	}
}

/* send data to modem */
static void sc8800s_ready_work(struct work_struct *work)
{
	struct sc8800s_data *sc8800s = container_of(work,
					struct sc8800s_data, ready_work);
	struct modem_header *hdr;

	u32 *data;
	u32 len;
	int ret;

	spi_debug("####: Modem is ready for receiving a packet......\n");

	/* send data to modem */
	hdr = (struct modem_header*)spi_buf.wdata;

	hdr->tag = HEADER_TAG;
	hdr->type = HEADER_TYPE;
	hdr->length = sc8800s->packet_tx.length;
	hdr->header_valid = 0xabcdefef;	/* for test only */
	spi_debug("write length: %#x\n", hdr->length);

	len = sc8800s->packet_tx.length + PACKET_HEADER_SIZE;
	data = (u32 *) hdr;

	/* total length should be 64-byte aligned. */
	len = ((len + PACKET_ALIGNMENT_WRITE - 1) /
		     PACKET_ALIGNMENT_WRITE) * PACKET_ALIGNMENT_WRITE;

	spi_debug("total_len in bytes(64-byte aligned): %#x\n", len);

	ret = spi_write(sc8800s->spi, (const u8 *)data, len);
	sc8800s->bus_status = SPI_BUS_IDLE;
	sc8800s_ap_rts(1);

	//spin_lock_irqsave(&sc8800s->spi_spin_lock, flags);
	sc8800s->can_write = 1;
	wake_up_interruptible(&sc8800s->queue_wr);

	//spin_unlock_irqrestore(&sc8800s->spi_spin_lock, flags);
}

/* modem is ready for receiving a packet from host */
static irqreturn_t sc8800s_rdy_handler(int irq, void *dev_id)
{
	struct sc8800s_data *sc8800s = dev_id;
	unsigned long flags;

	if (gpio_get_value(pdata->ap_bb_spi_rts)) {
		//__gpio_ack_irq(BB_AP_SPI_RDY);
		return IRQ_HANDLED;
	}

	/* check bus status. */
	spin_lock_irqsave(&sc8800s->spi_spin_lock, flags);

	if (SPI_BUS_IDLE != sc8800s->bus_status) {
		spi_error("+++++ In modem_rdy_handler(): wrong status %s\n",
			  SPI_BUS_STATUS_STRING(sc8800s->bus_status));
		spin_unlock_irqrestore(&sc8800s->spi_spin_lock, flags);
		//__gpio_ack_irq(BB_AP_SPI_RDY);
		return IRQ_HANDLED;
	} else {
		sc8800s->bus_status = SPI_BUS_SENDING;
		spi_debug("bp_rdy_handler(): %s\n",
				SPI_BUS_STATUS_STRING(sc8800s->bus_status));

		if (!work_pending(&sc8800s->ready_work))
			queue_work(tu830_wqueue, &sc8800s->ready_work);
		else
			pr_debug("pending write work\n");
		spin_unlock_irqrestore(&sc8800s->spi_spin_lock, flags);
		//__gpio_ack_irq(BB_AP_SPI_RDY);
		return IRQ_HANDLED;
	}
}

static int sc8800s_open(struct inode *inode, struct file *filp)
{
	filp->private_data = spi_info;

	return nonseekable_open(inode, filp);
}

static int sc8800s_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t sc8800s_read(struct file *filp, char __user *buf,
		size_t count, loff_t *pos)
{
	ssize_t ret = 0;
	struct sc8800s_data *sc8800s = filp->private_data;

	if (filp->f_flags & O_NONBLOCK) {
		ret = wait_event_interruptible_timeout(sc8800s->queue_rd, (spi_buf.count > 0),
					msecs_to_jiffies(20000));
		if (ret <= 0) {
			return 0;
		}
	} else {
		ret = wait_event_interruptible(sc8800s->queue_rd, (spi_buf.count > 0));
		if (ret != 0) {
			return -EFAULT;
		}
	}

	ret = get_userdata_from_buffer(buf, count);

	return ret;
}

static ssize_t sc8800s_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *pos)
{
	struct sc8800s_data *sc8800s = filp->private_data;
	unsigned long flags;
	int ret;

	mutex_lock(&sc8800s->lock);
	if (!enable) {
		mutex_unlock(&sc8800s->lock);
		return -EBUSY;
	}
	count  = MIN(count, MAX_PACKET_LENGTH);
	ret = copy_from_user(spi_buf.wdata + PACKET_HEADER_SIZE, buf, count);
	count -= ret;

	sc8800s->packet_tx.length = count;

rewrite:
	local_irq_save(flags);
	if (SPI_BUS_IDLE != sc8800s->bus_status) {
		sc8800s->can_write = 0;
		sc8800s->ap_rts_pending = AP_RTS_PENDING;
		local_irq_restore(flags);
		ret  = wait_event_interruptible_timeout(sc8800s->queue_bus_idle,
				SPI_BUS_IDLE == sc8800s->bus_status, msecs_to_jiffies(3000));
		if (ret <= 0) {
			spi_error("%s: bus busy %s\n", __func__,
					SPI_BUS_STATUS_STRING(sc8800s->bus_status));
			goto rewrite;
		};
		local_irq_save(flags);
		sc8800s->ap_rts_pending = AP_RTS_NO_PENDING;
	}

	sc8800s_ap_rts(0);
	local_irq_restore(flags);

	ret = wait_event_interruptible_timeout(sc8800s->queue_wr, 
			(sc8800s->can_write == 1), msecs_to_jiffies(3000));
	if (0 == ret) {
		spi_error("wait_event timeout, bp_rdy not ready: %s\n",
			  SPI_BUS_STATUS_STRING(sc8800s->bus_status));
		sc8800s_ap_rts(1);
		if (!enable) {
			sc8800s->can_write = 0;
			mutex_unlock(&sc8800s->lock);
			return -EBUSY;
		}
		goto rewrite;
	} 

	sc8800s->can_write = 0;
	while (!gpio_get_value_cansleep(pdata->td_rdy_pin));
	mutex_unlock(&sc8800s->lock);

	return count;
}

static const struct file_operations sc8800s_fops = {
	.owner		= THIS_MODULE,
	.open		= sc8800s_open,
	.read		= sc8800s_read,
	.write		= sc8800s_write,
	.release	= sc8800s_close,
};

static struct miscdevice sc8800s_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "sc8800s",
	.fops	= &sc8800s_fops,
};

static int sc8800s_suspend(struct spi_device *spi, pm_message_t state)
{
	sc8800s_resumed = false;

	return 0;
}

static int sc8800s_resume(struct spi_device *spi)
{
	return 0;
}

static int sc8800s_probe(struct spi_device *spi)
{
	struct sc8800s_data *sc8800s = NULL;
	int err = 0;

	sc8800s = kzalloc(sizeof(*sc8800s), GFP_KERNEL);
	if (!sc8800s) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	sc8800s->spi = spi;
	spi_set_drvdata(spi, sc8800s);
	spi_info = sc8800s;

	pdata = spi->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&spi->dev, "%s: platform data is NULL\n", __func__);
		goto exit_platform_data_null;
	}

	gpio_request(pdata->td_rts_pin, "td_rts");
	gpio_request(pdata->td_rdy_pin, "td_rdy");
	gpio_request(pdata->ap_bb_spi_rts, "ap_bb_spi_rts");
	gpio_request(pdata->ap_bb_spi_rdy, "ap_bb_spi_rdy");
	gpio_request(pdata->power_on, "power_on");

	sc8800s->td_rts_irq = gpio_to_irq(pdata->td_rts_pin);
	sc8800s->td_rdy_irq = gpio_to_irq(pdata->td_rdy_pin);

	/* initialise AP to BB pin */
	gpio_direction_output(pdata->ap_bb_spi_rts, 1);
	gpio_direction_output(pdata->ap_bb_spi_rdy, 1);


#if 0
	/* power on TD */
	mdelay(3000);
	gpio_direction_output(pdata->power_on, 0);
	gpio_direction_output(pdata->power_on, 1);

#else
	gpio_direction_output(pdata->power_on, 0);
#endif

	mutex_init(&sc8800s->lock);
	mutex_init(&sc8800s->buf_lock);

	spi->bits_per_word = 16;
	err = spi_setup(spi);
	if (err < 0) {
		dev_err(&spi->dev, "spi setup failed\n");
		goto exit_spi_setup_failed;
	}

	/* initialize sc8800s */
	sc8800s->bus_status = SPI_BUS_IDLE;

	init_waitqueue_head(&sc8800s->queue_wr);
	init_waitqueue_head(&sc8800s->queue_rd);
	init_waitqueue_head(&sc8800s->queue_buf);
	init_waitqueue_head(&sc8800s->queue_bus_idle);

	spin_lock_init(&sc8800s->spi_spin_lock);
	spin_lock_init(&sc8800s->spin_rlock);

	err = sc8800s_buf_init();
	if (err < 0) {
		err = -ENOMEM;
		goto exit_sc8800s_init_failed;
	}

	INIT_WORK(&sc8800s->read_work, sc8800s_rts_work);
	INIT_WORK(&sc8800s->ready_work, sc8800s_ready_work);

	tu830_wqueue = create_singlethread_workqueue("sc8800s");
	//tu830_wqueue = create_rt_workqueue("sc8800s");
	if (!tu830_wqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	wake_lock_init(&sc8800s->wakelock, WAKE_LOCK_SUSPEND, "sc8800s");
	wake_lock_init(&sc8800s->resume_lock, WAKE_LOCK_SUSPEND, "sc8800s");

	err = request_irq(sc8800s->td_rts_irq, sc8800s_rts_handler, 
			IRQF_ONESHOT | IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING,
			"sc8800s_rts", sc8800s);
	if (err < 0) {
		dev_err(&spi->dev, "%s: request rts irq(%d) failed\n",
				__func__, sc8800s->td_rts_irq);
		goto exit_rts_irq_request_failed;
	}
	disable_irq(sc8800s->td_rts_irq);

	err = request_irq(sc8800s->td_rdy_irq, sc8800s_rdy_handler, 
			IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			"sc8800s_ready", sc8800s);
	if (err < 0) {
		dev_err(&spi->dev, "%s: request ready irq(%d) failed\n",
				__func__, sc8800s->td_rdy_irq);
		goto exit_rdy_irq_request_failed;
	}
	disable_irq(sc8800s->td_rdy_irq);

	err = misc_register(&sc8800s_device);
	if (err) {
		dev_err(&spi->dev, "%s: device register failed\n", __func__);
		goto exit_misc_device_register_failed;
	}

	/* register sysfs hooks */
	err = sysfs_create_group(&sc8800s_device.this_device->kobj, &sc8800s_attr_group);
	if (err)
		goto exit_create_sysfs_group_failed;

	return 0;

exit_create_sysfs_group_failed:
	misc_deregister(&sc8800s_device);
exit_misc_device_register_failed:
	free_irq (sc8800s->td_rdy_irq, sc8800s);
exit_rdy_irq_request_failed:
	free_irq (sc8800s->td_rts_irq, sc8800s);
exit_rts_irq_request_failed:
	destroy_workqueue(tu830_wqueue);
exit_create_singlethread:
exit_sc8800s_init_failed:
exit_spi_setup_failed:
exit_platform_data_null:
	spi_set_drvdata(spi, NULL);
exit_alloc_data_failed:
	kfree(sc8800s);
	return err;
}

static int __devexit sc8800s_remove(struct spi_device *spi)
{
	struct sc8800s_data *sc8800s = spi_get_drvdata(spi);

	sc8800s_buf_free();

	free_irq(sc8800s->td_rts_irq, sc8800s);
	free_irq(sc8800s->td_rdy_irq, sc8800s);
	destroy_workqueue(tu830_wqueue);
	sysfs_remove_group(&spi->dev.kobj, &sc8800s_attr_group);
	misc_deregister(&sc8800s_device);

	spi_set_drvdata(spi, NULL);
	kfree(sc8800s);
	return 0;
}

static struct spi_driver sc8800s_driver = {
	.driver	= {
		.name	= "sc8800s",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= sc8800s_probe,
	.remove		= __devexit_p(sc8800s_remove),
	.suspend	= sc8800s_suspend,
	.resume		= sc8800s_resume,
};

static int __init sc8800s_init(void)
{
	return spi_register_driver(&sc8800s_driver);
}

static void __exit sc8800s_exit(void)
{
	spi_unregister_driver(&sc8800s_driver);
}

module_init(sc8800s_init);
module_exit(sc8800s_exit);

MODULE_AUTHOR("<fdbai@ingenic.cn");
MODULE_DESCRIPTION("TD SC8800S/G module driver");
MODULE_LICENSE("GPL");
