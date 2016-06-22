/*
 * EHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for AMD Alchemy Au1xxx
 *
 * Based on "ehci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 *
 * Modified for AMD Alchemy Au1200 EHC
 *  by K.Boge <karsten.boge@amd.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <soc/cpm.h>

extern int usb_disabled(void);

struct jz_ehci {
	struct device		*dev;
	struct clk		*clk;
	struct clk		*clk_cgu;
};

#define hcd_to_jz(hcd)	((struct jz_ehci *)(hcd_to_ehci(hcd) + 1))

static void jz_start_ehc(struct jz_ehci *jz_ehci)
{
	clk_enable(jz_ehci->clk);
	clk_set_rate(jz_ehci->clk_cgu,48000000);
	clk_enable(jz_ehci->clk_cgu);

	cpm_start_ehci();
}

static void jz_stop_ehc(struct jz_ehci *jz_ehci)
{
	cpm_stop_ehci();
	clk_disable(jz_ehci->clk);
}

static const struct hc_driver ehci_jz_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "JZ EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd) + sizeof(struct jz_ehci),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static int ehci_hcd_jz_drv_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct resource	*regs;
	struct jz_ehci *jz_ehci;
#ifdef CONFIG_CPU_SUSPEND_TO_IDLE
	device_init_wakeup(&pdev->dev, 1);
#endif
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "No irq resource\n");
		return irq;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "No iomem resource\n");
		return -ENXIO;
	}

	hcd = usb_create_hcd(&ehci_jz_hc_driver, &pdev->dev, "jz");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = regs->start;
	hcd->rsrc_len = resource_size(regs);

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		ret = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err2;
	}

	jz_ehci = hcd_to_jz(hcd);

	jz_ehci->clk = clk_get(&pdev->dev, "uhc");
	if (IS_ERR(jz_ehci->clk)) {
		dev_err(&pdev->dev, "clk gate get error\n");
		ret = PTR_ERR(jz_ehci->clk);
		goto err2;
	}

	jz_ehci->clk_cgu = clk_get(&pdev->dev, "cgu_uhc");
	if (IS_ERR(jz_ehci->clk_cgu)) {
		dev_err(&pdev->dev, "clk cgu get error\n");
		ret = PTR_ERR(jz_ehci->clk_cgu);
		goto err3;
	}
	clk_enable(jz_ehci->clk);
	clk_set_rate(jz_ehci->clk_cgu,48000000);
	clk_enable(jz_ehci->clk_cgu);

	jz_ehci->dev = &pdev->dev;

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);
	jz_start_ehc(jz_ehci);
	ret = usb_add_hcd(hcd, pdev->resource[1].start,
			  IRQF_DISABLED | IRQF_SHARED);
/*
 * This should be fixed after a standard spec.
 * addr: USBOPBASE + 0x10 detail:controller width. 1:16bit  0:8 bit
 */
	*(volatile unsigned int *)(0xb34900b0) |= (1 << 6);

	if (ret == 0) {
		platform_set_drvdata(pdev, hcd);
		return ret;
	}

	jz_stop_ehc(jz_ehci);
	iounmap(hcd->regs);
	clk_put(jz_ehci->clk_cgu);
err3:
	clk_put(jz_ehci->clk);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return ret;
}

static int ehci_hcd_jz_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct jz_ehci *jz_ehci;
#ifdef CONFIG_CPU_SUSPEND_TO_IDLE
	device_init_wakeup(&pdev->dev, 0);
#endif
	jz_ehci = (struct jz_ehci *)((unsigned char *)hcd + sizeof(struct ehci_hcd));
	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	jz_stop_ehc(jz_ehci);
	clk_disable(jz_ehci->clk);
	clk_put(jz_ehci->clk);
	clk_put(jz_ehci->clk_cgu);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ehci_hcd_jz_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc;
	struct jz_ehci *jz_ehci;

	jz_ehci = (struct jz_ehci *)((unsigned char *)hcd + sizeof(struct ehci_hcd));
#ifdef CONFIG_CPU_SUSPEND_TO_IDLE
	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(hcd->irq);
#endif

	return 0;
	rc = 0;

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW) {
		ehci_halt(ehci);
		ehci_reset(ehci);
	}

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	jz_stop_ehc(jz_ehci);

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	// could save FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	return rc;
}


static int ehci_hcd_jz_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct jz_ehci *jz_ehci;

	jz_ehci = (struct jz_ehci *)((unsigned char *)hcd + sizeof(struct ehci_hcd));
#ifdef CONFIG_CPU_SUSPEND_TO_IDLE
	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(hcd->irq);
#endif

	return 0;

	jz_start_ehc(jz_ehci);

	// maybe restore FLADJ

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	/* If CF is still set, we maintained PCI Vaux power.
	 * Just undo the effect of ehci_pci_suspend().
	 */
	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}

#else
#define ehci_hcd_jz_drv_suspend NULL
#define ehci_hcd_jz_drv_resume NULL
#endif

static struct platform_driver ehci_hcd_jz_driver = {
	.probe		= ehci_hcd_jz_drv_probe,
	.remove		= ehci_hcd_jz_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_jz_drv_suspend,
	.resume		= ehci_hcd_jz_drv_resume,
	.driver = {
		.name	= "jz-ehci",
		.owner	= THIS_MODULE,
	}
};

MODULE_ALIAS("platform:jz-ehci");
