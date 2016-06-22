/*
 * system.c
 *
 *  Created on: Jun 25, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 *
 *
 * 	@note: this file should be re-written to match the environment the
 * 	API is running on
 */

#include <linux/delay.h>
#include "../util/types.h"
#include "system.h"
#include <linux/interrupt.h>
#include <soc/irq.h>

struct hdmi_irq_handler {
	handler_t handler;
	void *param;
	int irq;
	struct work_struct  work;
	struct workqueue_struct *workqueue;
};

static struct hdmi_irq_handler m_irq_handler[3];

void system_SleepMs(unsigned ms)
{
	msleep(ms);
}

int system_ThreadCreate(void* handle, thread_t pFunc, void* param)
{
	/* TODO */
	return FALSE;
}

int system_ThreadDestruct(void* handle)
{
	/* TODO */
	return FALSE;
}

int system_Start(thread_t thread)
{
	thread(NULL);
	return FALSE;	      /* note: false is 0, but here we means success */
}

static unsigned system_InterruptMap(interrupt_id_t id)
{
	/* TODO */
	switch (id) {
	case TX_WAKEUP_INT:
		//return IRQ_HDMI_WAKEUP;
		return ~0;

	case TX_INT:
		return IRQ_HDMI;
	};
	return ~0;
}

int system_InterruptDisable(interrupt_id_t id)
{
	unsigned int irq = system_InterruptMap(id);
	disable_irq(irq);
	return TRUE;
}

int system_InterruptEnable(interrupt_id_t id)
{
	unsigned int irq = system_InterruptMap(id);
	enable_irq(irq);
	return TRUE;
}

int system_InterruptAcknowledge(interrupt_id_t id)
{
	/* do nothing */
	return TRUE;
}

static void interrupt_work_handler(struct work_struct *work)
{
	struct hdmi_irq_handler *handler = container_of(work, struct hdmi_irq_handler,work);
	handler->handler(handler->param);
	enable_irq(handler->irq);
}

static irqreturn_t jzhdmi_irq_handler(int irq, void *devid)
{
	struct hdmi_irq_handler *handler = (struct hdmi_irq_handler *)devid;
	disable_irq_nosync(handler->irq);
	if (!work_pending(&handler->work)) {
		queue_work(handler->workqueue, &handler->work);
	}

	return IRQ_HANDLED;
}

static char *m_irq_name[3] = {
	"HDMI RX_INT",
	"HDMI TX_WAKEUP_INT",
	"HDMI TX_INT"
};

// typedef void (*handler_t)(void *)
int system_InterruptHandlerRegister(interrupt_id_t id, handler_t handler,
				    void * param) {
	int ret;
	unsigned int irq = system_InterruptMap(id);
	struct hdmi_irq_handler *irq_handler = &m_irq_handler[id - 1];
	irq_handler->handler = handler;
	irq_handler->param = param;
#ifdef CONFIG_HDMI_JZ4780_DEBUG
	printk("hdmid %s  id=%d irq=%d---\n",__func__,id,irq);
#endif

	INIT_WORK(&irq_handler->work, interrupt_work_handler);
	irq_handler->workqueue = create_singlethread_workqueue("hdmi_workqueue");
	irq_handler->irq = irq;
	ret = request_irq(irq,
			  jzhdmi_irq_handler,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			  | IRQF_DISABLED,
			  m_irq_name[id - 1],
			  (void *)irq_handler);
	return (ret ? FALSE : TRUE);
}

int system_InterruptHandlerUnregister(interrupt_id_t id)
{
	unsigned int irq = system_InterruptMap(id);
	struct hdmi_irq_handler *irq_handler = &m_irq_handler[id - 1];

	free_irq(irq, irq_handler);
	return TRUE;
}
