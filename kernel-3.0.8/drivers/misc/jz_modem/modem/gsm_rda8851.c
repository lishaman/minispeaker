#include <linux/gpio.h>
#include <linux/modem_pm.h>
#include <linux/delay.h>

/*
 * modem ops
 */

static void rda8851cl_init(struct modem_data *bp)
{
	if (bp->bp_status.gpio)
		gpio_request(bp->bp_status.gpio, "bp status");
	modem_gpio_request(&bp->bp_pwr, "rda8851 power");
	modem_gpio_request(&bp->ap_status, "ap status");
	modem_gpio_request(&bp->bp_onoff, "bp on");
	modem_gpio_request(&bp->ap_wake_bp, "ap wake");
}

static void rda8851cl_suspend(struct modem_data *bp)
{
	modem_gpio_out(&bp->ap_status, BP_DEACTIVE);
}

static void rda8851cl_resume(struct modem_data *bp)
{
	modem_gpio_out(&bp->ap_status, BP_ACTIVE);
}

static void rda8851cl_poweron(struct modem_data *bp)
{
	modem_gpio_out(&bp->bp_pwr, BP_ACTIVE);
	msleep(10);
	modem_gpio_out(&bp->bp_onoff, BP_ACTIVE);
}

static void rda8851cl_poweroff(struct modem_data *bp)
{
	modem_gpio_out(&bp->bp_onoff, BP_DEACTIVE);
	modem_gpio_out(&bp->bp_pwr, BP_DEACTIVE);
}

static void rda8851cl_wakeup(struct modem_data *bp)
{
//	if (__gpio_get_value(bp->bp_status.gpio) != !!bp->bp_status.active_level) {
		modem_gpio_out(&bp->ap_wake_bp, BP_ACTIVE);
		msleep(10);
		modem_gpio_out(&bp->ap_wake_bp, BP_DEACTIVE);
//	}
}

static struct modem_ops bp_ops = {
	.name = "RDA8851CL",
	.init = rda8851cl_init,
	.poweron = rda8851cl_poweron,
	.poweroff = rda8851cl_poweroff,
	.wakeup = rda8851cl_wakeup,
	.suspend = rda8851cl_suspend,
	.resume = rda8851cl_resume,
	.reset = NULL,
};

static int __init rda8851cl_register(void)
{
	return modem_register_ops(&bp_ops);
}

module_init(rda8851cl_register);
