#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <mach/jztcu.h>

#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/div64.h>

#include <soc/base.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <soc/tcu.h>

struct pwm_device {
	short id;
	const char *label;
	struct tcu_device *tcu_cha;
}pwm_chs[8];

struct pwm_device *pwm_request(int id, const char *label)
{
	struct tcu_device *tcu_pwm;
	if (id < 0 || id > 7)
		return ERR_PTR(-ENODEV);
	tcu_pwm= tcu_request(id,NULL);
	pwm_chs[id].tcu_cha = tcu_pwm;
	if(IS_ERR(tcu_pwm)) {
		printk("-+-+-+-pwm_tcu_request failed!!-+-+-+\n");
		return ERR_PTR(-ENODEV);
	}
	pwm_chs[id].id = id;
	pwm_chs[id].label = label;
	tcu_pwm->pwm_flag = 1;
	tcu_pwm->irq_type = NULL_IRQ_MODE;
	tcu_pwm->init_level = 0;
	printk("request pwm channel %d successfully\n", id);
	return &pwm_chs[id];
}

void pwm_free(struct pwm_device *pwm)
{
	struct tcu_device *tcu_pwm = pwm->tcu_cha;
	tcu_free(tcu_pwm);
}

int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	unsigned long long tmp;
	unsigned long period, duty;
	int prescaler = 0; /*prescale = 0,1,2,3,4,5*/
	struct tcu_device *tcu_pwm = pwm->tcu_cha;
	if (duty_ns < 0 || duty_ns > period_ns)
		return -EINVAL;

	/* period < 200ns || period > 1s */
	if (period_ns < 200 || period_ns > 1000000000)
		return -EINVAL;
#ifndef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tmp = JZ_EXTAL;
#else
	tmp = JZ_EXTAL;//32768 RTC CLOCK failure!
#endif
	tmp = tmp * period_ns;
	do_div(tmp, 1000000000);
	period = tmp;

	while (period > 0xffff && prescaler < 6) {
		period >>= 2;
		++prescaler;
	}
	if (prescaler == 6)
		return -EINVAL;

	tmp = (unsigned long long)period * duty_ns;
	do_div(tmp, period_ns);
	duty = tmp;

	if (duty >= period)
		duty = period - 1;
	tcu_pwm->full_num = period;
	tcu_pwm->half_num = (period - duty);
	tcu_pwm->divi_ratio = prescaler;
#ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tcu_pwm->clock = RTC_EN;
#else
	tcu_pwm->clock = EXT_EN;
#endif
	tcu_pwm->count_value = 0;
	tcu_pwm->pwm_shutdown = 1;

	if(tcu_as_timer_config(tcu_pwm) != 0) return -EINVAL;

	return 0;
}

int pwm_enable(struct pwm_device *pwm)
{
	struct tcu_device *tcu_pwm = pwm->tcu_cha;
	tcu_enable(tcu_pwm);
#ifdef CONFIG_CLEAR_PWM_OUTPUT
	jzgpio_set_func(GPIO_PORT_E, GPIO_FUNC_0, 0x1 << pwm->id);
#endif
	return 0;
}

void pwm_disable(struct pwm_device *pwm)
{
	struct tcu_device *tcu_pwm = pwm->tcu_cha;
	tcu_disable(tcu_pwm);
#ifdef CONFIG_CLEAR_PWM_OUTPUT
	gpio_direction_output((GPIO_PORT_E * 32 + pwm->id), 0);
#endif
}
