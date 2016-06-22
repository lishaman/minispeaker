/* kernel/drivers/video/jz4780/jz4780_hdmi.c
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Core file for Ingenic HDMI driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/switch.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <mach/fb_hdmi_modes.h>

#include "jz4780_hdmi.h"

/*In normal mode this define should not open*/
//#define DVIMODE 1

static BLOCKING_NOTIFIER_HEAD(hdmi_notifier_chain);

#ifdef CONFIG_FORCE_RESOLUTION
static int resolution = CONFIG_FORCE_RESOLUTION;
#ifdef CONFIG_HDMI_JZ4780_MODULE
module_param(resolution, int, 0644);
MODULE_PARM_DESC(resolution, "HDMI output resolution index, \
		you can locate the valid value in \
		arch/mips/xburst/soc-4780/include/mach/fb_hdmi_modes.h");
#endif
#endif

static struct jzhdmi *global_hdmi;

void cec_switch_hdmi(void)
{
	hdmivsdb_t vsdb;
	u16 phyaddr;
	u8 value[1]={0};
	u8 value1[3]={0};
	/*send cec message <image view on>*/
	value[0]=IMAGE_VIEW_ON;
	sendmessage(value,1);
	msleep(100);
	/*send cec message <active source>*/
	value1[0]=ACTIVE_SOURCE;
	if ((api_EdidHdmivsdb(&vsdb) == TRUE)) {
		phyaddr = hdmivsdb_GetPhysicalAddress(&vsdb);
		dev_info(global_hdmi->dev, "Get device phy addr is %x\n",phyaddr);
		value1[1] = phyaddr >> 8;
		value1[2] = phyaddr & 0xf;
	}
	sendmessage(value1,3);
}

void cec_switch_standby(void)
{
	u8 value[1] = {0};
	value[0] = CEC_STANDBY;
	sendmessage(value,1);
}

void cec_switch_tv(void)
{
	hdmivsdb_t vsdb;
	u16 phyaddr;
	u8 value[3] = {0};

	cec_switch_hdmi();
	msleep(100);
	value[0] = INACTIVE_SOURCE;
	if ((api_EdidHdmivsdb(&vsdb) == TRUE)) {
		phyaddr = hdmivsdb_GetPhysicalAddress(&vsdb);
		dev_info(global_hdmi->dev, "Get device phy addr is %x\n",phyaddr);
		value[1] = phyaddr >> 8;
		value[2] = phyaddr & 0xf;
	}
	sendmessage(value,3);
}


/**
 *	hdmi_notifier_client_register - register a client notifier
 *	@nb: notifier block to callback on events
 */
int hdmi_notifier_client_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&hdmi_notifier_chain, nb);
}
EXPORT_SYMBOL(hdmi_notifier_client_register);

/**
 *	hdmi_notifier_client_unregister - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int hdmi_notifier_client_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&hdmi_notifier_chain, nb);
}
EXPORT_SYMBOL(hdmi_notifier_client_unregister);

/**
 * hdmi_notifier_call_chain - notify clients of hdmi_events
 *
 */
int hdmi_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&hdmi_notifier_chain, val, v);
}
EXPORT_SYMBOL_GPL(hdmi_notifier_call_chain);


static void edid_callback(void *param)
{
	dev_info(global_hdmi->dev, "EDID reading done\n");
	global_hdmi->edid_done = HDMI_HOTPLUG_EDID_DONE;

	wake_up(&global_hdmi->wait);

	cec_switch_hdmi();
}

static bool hdmi_init(struct jzhdmi *jzhdmi);
static int hdmi_config(struct jzhdmi *jzhdmi);
static int hdmi_read_edid(struct jzhdmi *jzhdmi);
static void jzhdmi_power_on(struct jzhdmi *jzhdmi);
static void hpd_callback(void *param)
{
	int i = 0;
	u8 hpd = *((u8*)(param));

	if (hpd != TRUE) {
		dev_info(global_hdmi->dev, "HPD DISCONNECT\n");
		switch_set_state(&global_hdmi->hdmi_switch,HDMI_HOTPLUG_DISCONNECTED);

		global_hdmi->hdmi_info.hdmi_status = HDMI_HOTPLUG_DISCONNECTED;
		global_hdmi->hdmi_is_running = 0;

		hdmi_notifier_call_chain(HDMI_EVENT_DISCONNECT, NULL);

#if defined(CONFIG_HDMI_NOT_CONTROL_IN_SURFACEFLINGER) || defined(CONFIG_HDMI_NOT_CONTROL_IN_SURFACEFLINGER_MODULE)
#ifdef CONFIG_FORCE_RESOLUTION
		api_phy_enable(PHY_ENABLE_HPD);
#endif
#endif
	} else {
		dev_info(global_hdmi->dev, "HPD CONNECT\n");

#ifdef CONFIG_HDMI_JZ4780_MODULE
		jzfb_set_videomode(resolution);
#endif
		switch_set_state(&global_hdmi->hdmi_switch,HDMI_HOTPLUG_CONNECTED);
		global_hdmi->hdmi_info.hdmi_status = HDMI_HOTPLUG_CONNECTED;

		hdmi_notifier_call_chain(HDMI_EVENT_CONNECT, NULL);

		if (global_hdmi->probe_finish == 1)
			global_hdmi->hdmi_is_running = 1;

#if defined(CONFIG_HDMI_NOT_CONTROL_IN_SURFACEFLINGER) || defined(CONFIG_HDMI_NOT_CONTROL_IN_SURFACEFLINGER_MODULE)
#ifdef CONFIG_FORCE_RESOLUTION
		if (!work_pending(&global_hdmi->detect_work))
			queue_work(global_hdmi->workqueue, &global_hdmi->detect_work);
#endif
#endif
		cec_switch_hdmi();
	}
}

#ifdef DVIMODE 
static u8 cliEdid_GetDtdCode(const dtd_t *dtd)
{
	u8 cea_mode = dtd_GetCode(dtd);
	dtd_t tmp_dtd;
	if (cea_mode == (u8)(-1))
	{
		for (cea_mode = 1; cea_mode < 60; cea_mode++)
		{
			dtd_Fill(&tmp_dtd, cea_mode, 0);
			if (dtd_IsEqual(dtd, &tmp_dtd))
			{
				break;
			}
		}
		if (cea_mode >= 60)
		{
			return -1;
		}
	}
	return cea_mode;
}
#endif

int compliance_Standby(struct jzhdmi *jzhdmi)
{
	videoParams_t *pVideo = jzhdmi->hdmi_params.pVideo;
	audioParams_t *pAudio = jzhdmi->hdmi_params.pAudio;
	hdcpParams_t *pHdcp = jzhdmi->hdmi_params.pHdcp;
	productParams_t *pProduct = jzhdmi->hdmi_params.pProduct;

	if (pAudio != 0)
	{
		kfree(pAudio);
		pAudio = 0;
	}
	if (pProduct != 0)
	{
		kfree(pProduct);
		pProduct = 0;
	}
	if (pVideo != 0)
	{
		kfree(pVideo);
		pVideo = 0;
	}
	if (pHdcp != 0)
	{
		kfree(pHdcp);
		pHdcp = 0;
	}
	return TRUE;
}
static bool hdmi_init(struct jzhdmi *jzhdmi);
static int hdmi_config(struct jzhdmi *jzhdmi);
static int hdmi_read_edid(struct jzhdmi *jzhdmi)
{
	int timeout;
	jzhdmi->edid_done = 0;
	if (!api_EdidRead(edid_callback)){
		dev_info(jzhdmi->dev, "---edid failed\n");
		return FALSE;
	}
#if 0
	while(jzhdmi->hdmi_info.hdmi_status != HDMI_HOTPLUG_CONNECTED  || jzhdmi->edid_done != HDMI_HOTPLUG_EDID_DONE){
			msleep(1);
	}
#else
	timeout = wait_event_timeout(jzhdmi->wait,
			jzhdmi->edid_done == HDMI_HOTPLUG_EDID_DONE
			&& jzhdmi->hdmi_info.hdmi_status == HDMI_HOTPLUG_CONNECTED, 2*HZ);
	if(!timeout){
		dev_err(jzhdmi->dev, "---hdmi read edid timeout\n");
		return FALSE;
	}
	return TRUE;
#endif
}

static int hdmi_config(struct jzhdmi *jzhdmi)
{
	u8 currentMode = 0;
	hdmivsdb_t vsdb;
	shortVideoDesc_t tmp_svd;
	dtd_t tmp_dtd;
	u8 ceaCode = 0;
	u8 errorCode = NO_ERROR;
	u8 svdNo=0;
	/* needed to make sure it doesn't change while in this function*/
	struct device *dev = jzhdmi->dev;

	videoParams_t *pVideo = jzhdmi->hdmi_params.pVideo;
	audioParams_t *pAudio = jzhdmi->hdmi_params.pAudio;
	hdcpParams_t *pHdcp = jzhdmi->hdmi_params.pHdcp;
	productParams_t *pProduct = jzhdmi->hdmi_params.pProduct;

	svdNo = api_EdidSvdCount();

	if ((api_EdidHdmivsdb(&vsdb) == TRUE) && (jzhdmi->edid_faild == 0)) {
		dev_info(dev, "==========>HDMI mode\n");
		videoParams_SetHdmi(pVideo, TRUE);
		/* TODO:
		   if (hdmivsdb_GetDeepColor48(&vsdb)) {
		   videoParams_SetColorResolution(pVideo, 16);
		   } else if (hdmivsdb_GetDeepColor36(&vsdb)) {
		   videoParams_SetColorResolution(pVideo, 12);
		   } else if (hdmivsdb_GetDeepColor30(&vsdb)) {
		   videoParams_SetColorResolution(pVideo, 10);
		   } else {
		   videoParams_SetColorResolution(pVideo, 0);
		   }
		   */
		videoParams_SetColorResolution(pVideo, 0);

		if (currentMode >= svdNo) {
			currentMode = 0;
		}
#ifdef HDMI_JZ4780_DEBUG
		dev_info(jzhdmi->dev, "=====> cur==%d  svno=%d\n",currentMode,svdNo);
#endif
		for (; currentMode < svdNo; currentMode++) {
			if (api_EdidSvd(currentMode, &tmp_svd)) {
				ceaCode = shortVideoDesc_GetCode(&tmp_svd);
				if (ceaCode != jzhdmi->hdmi_info.out_type) continue;
				if (board_SupportedRefreshRate(ceaCode) != -1) {
					dtd_Fill(&tmp_dtd, ceaCode, board_SupportedRefreshRate(ceaCode));
					videoParams_SetDtd(pVideo, &tmp_dtd);
				} else {
					dev_err(dev, "CEA mode not supported\n");
					continue;
				}
			}

			if (api_Configure(pVideo, pAudio, pProduct, pHdcp)) {
				dev_info(dev, "----api_Configure ok\n");
				break;
			}
			dev_err(dev, "error: VM = %d\n", ceaCode);
			errorCode = error_Get();

			if (errorCode == ERR_HDCP_FAIL) {
				system_SleepMs(1000);
				currentMode--;
			}
		}

		if (currentMode >= svdNo) {
			/* spanned all SVDs and non works, sending VGA */
			dev_err(dev, "error spanned all SVDs and non works\n");
			dtd_Fill(&tmp_dtd, 1, board_SupportedRefreshRate(1));
			videoParams_SetDtd(pVideo, &tmp_dtd);
			api_Configure(pVideo, pAudio, pProduct, pHdcp);
		}

		/* lltang add test overflow */
		if(0){
			/* ====>tmds reset  */
			access_CoreWrite(1, 0x4002, 1, 1);
			/* ====>video_Configure again */
			if (video_Configure(0, pVideo, 1, FALSE) != TRUE) {
				return FALSE;
			}
			access_CoreWriteByte(0x0, 0x10DA);
		}

	} else {
		dev_info(dev, "Force into hdmi mode!\n");
#ifndef DVIMODE 
		videoParams_SetHdmi(pVideo, TRUE);
		videoParams_SetColorResolution(pVideo, 0);

		if (currentMode >= svdNo) {
			currentMode = 0;
		}
#ifdef HDMI_JZ4780_DEBUG
		dev_info(jzhdmi->dev, "=====>out_type=%d\n",jzhdmi->hdmi_info.out_type);
#endif
		ceaCode = jzhdmi->hdmi_info.out_type;
		if (board_SupportedRefreshRate(ceaCode) != -1) {
			dtd_Fill(&tmp_dtd, ceaCode, board_SupportedRefreshRate(ceaCode));
			videoParams_SetDtd(pVideo, &tmp_dtd);
		} else {
			dev_err(dev, "CEA mode not supported\n");
		}

		if (api_Configure(pVideo, pAudio, pProduct, pHdcp)) {
			dev_info(dev, "api_Configure ok\n");
		}
	}
#else
		dev_info(dev, "====DVI mode!\n");
		videoParams_SetHdmi(pVideo, FALSE);
		videoParams_SetColorResolution(pVideo, 0);
		if (currentMode >= api_EdidDtdCount()) {
			currentMode = 0;
		}
		for (; currentMode < api_EdidDtdCount(); currentMode++) {
			continue;
			if (api_EdidDtd(currentMode, &tmp_dtd)) {
				ceaCode = cliEdid_GetDtdCode(&tmp_dtd);
				dev_info(dev, "check ceaCode = %d\n", ceaCode);
				if (ceaCode != 3) continue;
				dev_info(dev, "find ceaCode = %d\n", ceaCode);
				dtd_Fill(&tmp_dtd, ceaCode, board_SupportedRefreshRate(ceaCode));
				videoParams_SetDtd(pVideo, &tmp_dtd);
			}
			if (api_Configure(pVideo, pAudio, pProduct, pHdcp)) {
				break;
			}
			errorCode = error_Get();
			dev_err(dev, "API configure fail\n");
			if (errorCode == ERR_HDCP_FAIL) {
				system_SleepMs(1000);
				currentMode--;
			}
		}
		if (currentMode >= api_EdidDtdCount()) {
			dev_err(dev, "spanned all dtds and non works, sending VGA\n");
			dtd_Fill(&tmp_dtd, 1, board_SupportedRefreshRate(1));
			videoParams_SetDtd(pVideo, &tmp_dtd);
			api_Configure(pVideo, pAudio, pProduct, pHdcp);
		}
	}
#endif
	return TRUE;
}

static bool hdmi_init(struct jzhdmi *jzhdmi)
{
	videoParams_t *pVideo = jzhdmi->hdmi_params.pVideo;
	audioParams_t *pAudio = jzhdmi->hdmi_params.pAudio;
//	hdcpParams_t *pHdcp = jzhdmi->hdmi_params.pHdcp;
	productParams_t *pProduct = jzhdmi->hdmi_params.pProduct;

	const u8 vName[] = "Synopsys";
	const u8 pName[] = "Complnce";

	if (!pProduct) {
		dev_err(jzhdmi->dev, "pVideo is NULL\n");
		return FALSE;
	}
	productParams_Reset(pProduct);
	productParams_SetVendorName(pProduct, vName,sizeof(vName) - 1);
	productParams_SetProductName(pProduct, pName,sizeof(pName) - 1);
	productParams_SetSourceType(pProduct, 0x0A);

	if (!pAudio) {
		dev_err(jzhdmi->dev, "pAudio is NULL\n");
		return FALSE;
	}
	audioParams_Reset(pAudio);
#if 0
	/*GPA*/

	dev_info(jzhdmi->dev, "GPA interface\n");
	audioParams_SetInterfaceType(pAudio, GPA);
	audioParams_SetCodingType(pAudio, PCM);
	audioParams_SetChannelAllocation(pAudio, 0);
	audioParams_SetPacketType(pAudio, AUDIO_SAMPLE);
	audioParams_SetSampleSize(pAudio, 16);
	audioParams_SetSamplingFrequency(pAudio, 44100);
	audioParams_SetLevelShiftValue(pAudio, 0);
	audioParams_SetDownMixInhibitFlag(pAudio, 0);
	audioParams_SetClockFsFactor(pAudio, 128);
#else
	audioParams_SetInterfaceType(pAudio, I2S);
	audioParams_SetCodingType(pAudio, PCM);
	audioParams_SetChannelAllocation(pAudio, 0);
	audioParams_SetPacketType(pAudio, AUDIO_SAMPLE);
	audioParams_SetSampleSize(pAudio, 16);
	audioParams_SetSamplingFrequency(pAudio, 44100);
	//audioParams_SetSamplingFrequency(pAudio, 48000);
	audioParams_SetLevelShiftValue(pAudio, 0);
	audioParams_SetDownMixInhibitFlag(pAudio, 0);
	audioParams_SetClockFsFactor(pAudio, 64);
#endif

	if (!pVideo) {
		dev_err(jzhdmi->dev, "pVideo is NULL\n");
		return FALSE;
	}
	videoParams_Reset(pVideo);
	videoParams_SetEncodingIn(pVideo, RGB);
	videoParams_SetEncodingOut(pVideo, RGB);
	return TRUE;
}

#ifdef CONFIG_HDMI_JZ4780_DEBUG
void pri_hdmi_info(struct jzhdmi *jzhdmi)
{
	int haha = 0;
	dev_info(jzhdmi->dev, "Audio Configure:\n");
	dev_info(jzhdmi->dev, "===>REG[3100] = 0x%02x\n", api_CoreRead(0x3100));
	dev_info(jzhdmi->dev, "===>REG[3101] = 0x%02x\n", api_CoreRead(0x3101));
	dev_info(jzhdmi->dev, "===>REG[3102] = 0x%02x\n", api_CoreRead(0x3102));
	dev_info(jzhdmi->dev, "===>REG[3103] = 0x%02x\n", api_CoreRead(0x3103));
	dev_info(jzhdmi->dev, "===>REG[3200] = 0x%02x\n", api_CoreRead(0x3200));
	dev_info(jzhdmi->dev, "===>REG[3201] = 0x%02x\n", api_CoreRead(0x3201));
	dev_info(jzhdmi->dev, "===>REG[3202] = 0x%02x\n", api_CoreRead(0x3202));
	dev_info(jzhdmi->dev, "===>REG[3203] = 0x%02x\n", api_CoreRead(0x3203));
	dev_info(jzhdmi->dev, "===>REG[3204] = 0x%02x\n", api_CoreRead(0x3204));
	dev_info(jzhdmi->dev, "===>REG[3205] = 0x%02x\n", api_CoreRead(0x3205));
	dev_info(jzhdmi->dev, "===>REG[3206] = 0x%02x\n", api_CoreRead(0x3206));

	dev_info(jzhdmi->dev, "Main Clock Ctrl:\n");
	dev_info(jzhdmi->dev, "===>REG[4001] = 0x%02x\n", api_CoreRead(0x4001));
	dev_info(jzhdmi->dev, "===>REG[4002] = 0x%02x\n", api_CoreRead(0x4002));
	dev_info(jzhdmi->dev, "===>REG[4006] = 0x%02x\n", api_CoreRead(0x4006));

	dev_info(jzhdmi->dev, "FC Audio:\n");
	for (haha = 0x1025; haha <= 0x1028; haha++) {
		dev_info(jzhdmi->dev, "===>REG[%04x] = 0x%02x\n", haha, api_CoreRead(haha));
	}

	for (haha = 0x1063; haha <= 0x106f; haha++) {
		dev_info(jzhdmi->dev, "===>REG[%04x] = 0x%02x\n", haha, api_CoreRead(haha));
	}

	for (haha = 0x3000; haha <= 0x3007; haha++) {
		printk("===>REG[%04x] = 0x%02x\n",
		       haha, access_CoreReadByte(haha));
	}
}
#endif
static void jzhdmi_power_on(struct jzhdmi *jzhdmi)
{
	int i,ret = 0;
	shortVideoDesc_t tmpsvd;
	dev_info(jzhdmi->dev, "Hdmi power on\n");
	/*if(jzhdmi->hdmi_is_running != 1){*/
		api_phy_enable(PHY_ENABLE);
	/*}*/
	hdmi_init(jzhdmi);
	ret = hdmi_read_edid(jzhdmi);
	jzhdmi->hdmi_info.support_modenum = api_EdidSvdCount();

	if(ret > 0 && jzhdmi->hdmi_info.support_modenum > 0){
		jzhdmi->edid_faild = 0;
		jzhdmi->hdmi_info.support_mode = kzalloc(sizeof(int)*jzhdmi->hdmi_info.support_modenum, GFP_KERNEL);
		dev_info(jzhdmi->dev, "Hdmi get tv edid code:");
		for(i=0;i<jzhdmi->hdmi_info.support_modenum;i++){
			api_EdidSvd(i,&tmpsvd);
			printk("%d ",tmpsvd.mCode);
			jzhdmi->hdmi_info.support_mode[i] = tmpsvd.mCode;
		}
		printk("\n");
	}else{
		dev_info(jzhdmi->dev, "Read edid is failed,set mode to 720p60Hz or 1920x1080p06Hz\n");
		jzhdmi->edid_faild = 1;
		jzhdmi->hdmi_info.support_modenum = 2;
		jzhdmi->hdmi_info.support_mode = kzalloc(sizeof(int)*jzhdmi->hdmi_info.support_modenum, GFP_KERNEL);
		jzhdmi->hdmi_info.support_mode[0] = 4; /* 1280x720p @ 59.94/60Hz 16:9 */
		jzhdmi->hdmi_info.support_mode[1] = 16;/* 1920x1080p @ 59.94/60Hz 16:9 */
	}
}

static long jzhdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int index,cec_cmd = 0;
	struct jzhdmi *jzhdmi = (struct jzhdmi *)(file->private_data);

	switch (cmd) {
	case HDMI_POWER_ON:
		jzhdmi_power_on(jzhdmi);
		break;

	case HDMI_GET_TVMODENUM:
		if (copy_to_user((void __user *)arg, &jzhdmi->hdmi_info.support_modenum, sizeof(int))) {
			dev_err(jzhdmi->dev, "HDMI get support mode num failed\n");
			return -EFAULT;
		}
		break;

	case HDMI_GET_TVMODE:
		if(jzhdmi->hdmi_info.support_modenum != 0) {
			if (copy_to_user((void __user *)arg, jzhdmi->hdmi_info.support_mode,
						sizeof(int)*(jzhdmi->hdmi_info.support_modenum))) {
				dev_err(jzhdmi->dev, "HDMI get support mode failed\n");
				return -EFAULT;
			}
		} else {
			dev_err(jzhdmi->dev, "HDMI not have support mode:\n");
			return -EFAULT;
		}
		break;

	case HDMI_VIDEOMODE_CHANGE:
//		if(jzhdmi->edid_faild == 0)
//			hdmi_read_edid(jzhdmi);
		if (copy_from_user(&index, (void __user *)arg, sizeof(int)))
			return -EFAULT;
		if (index >= 1 && index <= HDMI_VIDEO_MODE_NUM) {
			if(jzhdmi->hdmi_is_running == 1 &&
					jzhdmi->hdmi_info.out_type == index){
				break;
			}
			jzhdmi->hdmi_info.out_type = index;
		} else {
			dev_err(jzhdmi->dev, "HDMI not support mode:%d\n",
				  index);
			return -EFAULT;
		}
		hdmi_config(jzhdmi);

		access_CoreWriteByte(0x0, 0x0183);   //intc fifo mute
		access_CoreWriteByte(0, 0x3302); //audio fifo status unmask

#ifdef CONFIG_HDMI_JZ4780_DEBUG
		pri_hdmi_info(jzhdmi);
#endif
		break;

	case HDMI_POWER_OFF:
		dev_info(jzhdmi->dev, "hdmi power off\n");
		if(jzhdmi->hdmi_info.support_mode != NULL)
			kzfree(jzhdmi->hdmi_info.support_mode);
		if(jzhdmi->is_suspended == 0){
			api_phy_enable(PHY_ENABLE_HPD);
		}
		break;

	case HDMI_POWER_OFF_COMPLETE:
		dev_info(jzhdmi->dev, "hdmi power off complete\n");
#if 0
		if (!api_Standby()) {
			dev_err(jzhdmi->dev, "API standby fail\n");
			return -EINVAL;
		}
#endif
		if(jzhdmi->hdmi_info.support_mode != NULL)
			kzfree(jzhdmi->hdmi_info.support_mode);
		api_phy_enable(PHY_DISABLE_ALL);
		break;
	case HDMI_CEC_CTL:
		if (copy_from_user(&cec_cmd, (void __user *)arg, sizeof(int)))
			return -EFAULT;
		printk("the cec_cmd is %d\n",cec_cmd);
		switch(cec_cmd) {
		case SWITCH_HDMI:
			cec_switch_hdmi();
			break;
		case SWITCH_TV:
			cec_switch_tv();
			break;
		case SWITCH_STANDBY:
			cec_switch_standby();
			break;
		default:
			printk("cec do not support this cmd:%d\n",cec_cmd);
			return -EINVAL;
		}
		break;
	case HDMI_GET_STATUS:
		if (copy_to_user((void __user *)arg, &jzhdmi->hdmi_info.hdmi_status, sizeof(int))) {
			dev_err(jzhdmi->dev, "HDMI get support mode num failed\n");
			return -EFAULT;
		}
		break;

	default:
		dev_info(jzhdmi->dev, "%s, cmd = %d is error\n",
			 __func__, cmd);
		return -EINVAL;
	}

	return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void hdmi_early_suspend(struct early_suspend *h)
{
	int api_mHpd = FALSE;
	struct jzhdmi *jzhdmi;

	jzhdmi = container_of(h, struct jzhdmi, early_suspend);
	mutex_lock(&jzhdmi->lock);
	jzhdmi->is_suspended = 1;
	hpd_callback(&api_mHpd);
	system_InterruptDisable(TX_INT);
	api_phy_enable(PHY_DISABLE_ALL);
//	regulator_disable(jzhdmi->hdmi_power);

	mutex_unlock(&jzhdmi->lock);
}
static void hdmi_late_resume(struct early_suspend *h)
{
	int api_mHpd = FALSE;
	struct jzhdmi *jzhdmi;
	jzhdmi = container_of(h, struct jzhdmi, early_suspend);

//	regulator_enable(jzhdmi->hdmi_power);
	api_phy_enable(PHY_ENABLE_HPD);
	jzhdmi->is_suspended = 0;
	system_InterruptEnable(TX_INT);
	api_mHpd = (phy_HotPlugDetected(0) > 0);
	hpd_callback(&api_mHpd);
}
#endif

void hdmi_detect_work_handler(struct work_struct *work)
{
	struct jzhdmi *jzhdmi =
		container_of(work, struct jzhdmi, detect_work);
	if(jzhdmi->probe_finish == 1){
		dev_info(global_hdmi->dev, "HDMI do not control in surfaceflinger\n");
		jzhdmi_power_on(global_hdmi);

		global_hdmi->hdmi_info.out_type = resolution;
		dev_info(global_hdmi->dev, "Setting video mode %d...\n", global_hdmi->hdmi_info.out_type);
		hdmi_config(global_hdmi);
		global_hdmi->hdmi_is_running = 1;
	}
}
static int jzhdmi_open(struct inode * inode, struct file * filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jzhdmi *jzhdmi = container_of(dev, struct jzhdmi, hdmi_miscdev);
#if 0
	if (!(atomic_dec_and_test(&jzhdmi->opened))) {
		dev_info(jzhdmi->dev, "HDMI already opened\n");
		atomic_inc(&jzhdmi->opened);
		return -EBUSY; /* already open */
	} /* If open success, jzhdmi->opened.counter is 0. */
#endif
	filp->private_data = jzhdmi;
	dev_info(jzhdmi->dev, "Ingenic Onchip HDMI opened\n");

	return 0;
}

static int jzhdmi_close(struct inode * inode, struct file * filp)
{
	struct jzhdmi *jzhdmi = filp->private_data;
	filp->private_data = NULL;
	jzhdmi->edid_faild = 0;
	return 0;
#if 0
	if (!(atomic_inc_and_test(&jzhdmi->opened)) &&
	    jzhdmi->opened.counter == 1) {
		dev_info(jzhdmi->dev, "Ingenic Onchip HDMI close\n");
		filp->private_data = NULL;
	} else {
		dev_info(jzhdmi->dev, "Failed to close HDMI device\n");
		atomic_dec(&jzhdmi->opened);
	}
	return 0;
#endif
}

static struct file_operations jzhdmi_fops = {
	.owner = THIS_MODULE,
	.open = jzhdmi_open,
	.release = jzhdmi_close,
	.unlocked_ioctl = jzhdmi_ioctl,
};

static int __devinit jzhdmi_probe(struct platform_device *pdev)
{
	int ret,i;
	struct jzhdmi *jzhdmi;
	struct resource *mem;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "Failed to get register memory resource\n");
		return -ENXIO;
	}

	mem = request_mem_region(mem->start, resource_size(mem), pdev->name);
	if (!mem) {
		dev_err(&pdev->dev, "Failed to request register memory\n");
		return -EBUSY;
	}

	jzhdmi = kzalloc(sizeof(struct jzhdmi), GFP_KERNEL);
	if (!jzhdmi) {
		dev_err(&pdev->dev, "Failed to allocate hdmi device\n");
		ret = -ENOMEM;
		goto err_release_mem_region;
	}

	global_hdmi = jzhdmi;

	jzhdmi->dev = &pdev->dev;
	jzhdmi->mem = mem;
	jzhdmi->hdmi_info.hdmi_status = HDMI_HOTPLUG_DISCONNECTED,
	jzhdmi->edid_faild = 0;
	jzhdmi->hdmi_is_running = 0;
	jzhdmi->hdmi_info.out_type = -1;
	jzhdmi->hdmi_info.support_modenum = 0;

	jzhdmi->probe_finish = 0;
	jzhdmi->hdmi_params.pProduct = (productParams_t*)kzalloc(
		sizeof(productParams_t),GFP_KERNEL);
	if(!jzhdmi->hdmi_params.pProduct){
		dev_err(&pdev->dev, "Failed to allocate pProduct memory\n");
		ret = -ENOMEM;
		goto err_free_jzhdmi_mem;
	}

	jzhdmi->hdmi_params.pAudio = (audioParams_t*)kzalloc(
		sizeof(audioParams_t),GFP_KERNEL);
	if(!jzhdmi->hdmi_params.pAudio){
		dev_err(&pdev->dev, "Failed to allocate pAudio memory\n");
		ret = -ENOMEM;
		goto err_free_pProduct_mem;
	}
/*this need null,wo do not need hdcp now */
#if 0
	jzhdmi->hdmi_params.pHdcp = (hdcpParams_t *)kzalloc(
		sizeof(hdcpParams_t),GFP_KERNEL);
	if(!jzhdmi->hdmi_params.pHdcp){
		dev_err(&pdev->dev, "Failed to allocate pHdcp memory\n");
		ret = -ENOMEM;
		goto err_free_pAudio_mem;
	}
#else
	jzhdmi->hdmi_params.pHdcp = NULL;
#endif
	jzhdmi->hdmi_params.pVideo = (videoParams_t*)kzalloc(
		sizeof(videoParams_t),GFP_KERNEL);
	if(!jzhdmi->hdmi_params.pVideo){
		dev_err(&pdev->dev, "Failed to allocate pVideo memory\n");
		ret = -ENOMEM;
		goto err_free_pHdcp_mem;
	}

	jzhdmi->hdmi_clk = clk_get(&pdev->dev, "hdmi");
	clk_enable(jzhdmi->hdmi_clk);

	jzhdmi->hdmi_cgu_clk = clk_get(&pdev->dev, "cgu_hdmi");
	if (IS_ERR(jzhdmi->hdmi_cgu_clk)) {
		ret = PTR_ERR(jzhdmi->hdmi_cgu_clk);
		dev_err(&pdev->dev, "Failed to get hdmi clock: %d\n", ret);
		goto err_free_pVideo_mem;
	}
	clk_disable(jzhdmi->hdmi_cgu_clk);
	clk_set_rate(jzhdmi->hdmi_cgu_clk,27000000);
	clk_enable(jzhdmi->hdmi_cgu_clk);

	jzhdmi->hdmi_power = regulator_get(&pdev->dev, "vhdmi");
	if (IS_ERR(jzhdmi->hdmi_power)) {
		dev_err(&pdev->dev, "failed to get regulator vhdmi\n");
		ret = -EINVAL;
		goto err_put_hdmi_clk;
	}
	regulator_enable(jzhdmi->hdmi_power);

	jzhdmi->base = ioremap(mem->start, resource_size(mem));
	if (!jzhdmi->base) {
		dev_err(&pdev->dev, "Failed to ioremap register memory\n");
		ret = -EBUSY;
		goto err_regulator_put;
	}

	mutex_init(&jzhdmi->lock);

	jzhdmi->hdmi_switch.name = "hdmi";
	ret = switch_dev_register(&jzhdmi->hdmi_switch);
	if (ret < 0){
		dev_err(jzhdmi->dev, "HDMI switch_dev_register fail\n");
		goto err_iounmap;
	}

	jzhdmi->workqueue = create_singlethread_workqueue("hdmiwork");
	if (!jzhdmi->workqueue){
		dev_err(&pdev->dev, "HDMI create workqueue fail\n");
		ret = -EINVAL;
		goto err_switch_dev_unregister;
	}

	/*INIT_DELAYED_WORK(&jzhdmi->detect_work,hdmi_detect_work_handler);*/
	INIT_WORK(&jzhdmi->detect_work,hdmi_detect_work_handler);

	if (!access_Initialize((u8 *)jzhdmi->base)) {
		dev_err(&pdev->dev, "HDMI initialize base address fail\n");
		ret = -EINVAL;
		goto err_destroy_workqueue;
	}

	api_EventEnable(HPD_EVENT, hpd_callback, FALSE);
	api_EventEnable(CEC_STATE, cec_callback, FALSE);

	if ((access_Read(0x3000) & 0x40)) /* check ENTMDS */
		jzhdmi->hdmi_is_running = 1;
	if (!jzhdmi->hdmi_is_running) {
		if (!api_Initialize(0, 1, 2500,0)) {
			dev_err(jzhdmi->dev, "api_Initialize fail\n");
			ret = -EINVAL;
			goto err_destroy_workqueue;
		}
	} else {
		if (!api_Reinitialize(0, 1, 2500,0)) {
			dev_err(jzhdmi->dev, "api_Reinitialize fail\n");
			ret = -EINVAL;
			goto err_destroy_workqueue;
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	jzhdmi->early_suspend.suspend = hdmi_early_suspend;
	jzhdmi->early_suspend.resume =  hdmi_late_resume;
	jzhdmi->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 15;
	register_early_suspend(&jzhdmi->early_suspend);
#endif

	platform_set_drvdata(pdev, jzhdmi);

	jzhdmi->hdmi_miscdev.minor = MISC_DYNAMIC_MINOR;
	jzhdmi->hdmi_miscdev.name = "hdmi";
	jzhdmi->hdmi_miscdev.fops = &jzhdmi_fops;

	ret = misc_register(&jzhdmi->hdmi_miscdev);
	if (ret) {
		dev_err(&pdev->dev, "hdmi misc register fail\n");
		goto err_unregister_early_suspend;
	}

	atomic_set(&jzhdmi->opened, 1);
	init_waitqueue_head(&jzhdmi->wait);

	if (jzhdmi->hdmi_is_running){
#ifdef CONFIG_FORCE_RESOLUTION
		jzhdmi->hdmi_info.out_type = resolution;
#endif
		jzhdmi->probe_finish = 1;
		return 0;
	}
#ifdef CONFIG_FORCE_RESOLUTION
	for (i = 0; i < 5; i++) {
		if (!resolution)
			break;
		if ((phy_HotPlugDetected(0) > 0)) {
			dev_info(jzhdmi->dev, "Force HDMI init VIC %d\n",
				 resolution);
			jzhdmi_power_on(jzhdmi);
			jzhdmi->hdmi_info.hdmi_status = HDMI_HOTPLUG_CONNECTED,
			jzhdmi->hdmi_info.out_type = resolution;
#ifdef CONFIG_HDMI_JZ4780_MODULE
			jzfb_set_videomode(resolution); //defined in driver/video/jz4780-fb/jz4780_fb.c
#endif
			hdmi_config(jzhdmi);
			jzhdmi->hdmi_is_running = 1;
			break;
		} else {
			if (i >= 4)
				dev_err(jzhdmi->dev, "HDMI HPD fail\n");
			mdelay(100);
		}

	}
#endif
	jzhdmi->probe_finish = 1;

	return 0;

err_unregister_early_suspend:
	unregister_early_suspend(&jzhdmi->early_suspend);
err_destroy_workqueue:
	destroy_workqueue(jzhdmi->workqueue);
err_switch_dev_unregister:
	switch_dev_unregister(&jzhdmi->hdmi_switch);
err_iounmap:
	iounmap(jzhdmi->base);
err_regulator_put:
	regulator_put(jzhdmi->hdmi_power);
err_put_hdmi_clk:
	clk_put(jzhdmi->hdmi_cgu_clk);
err_free_pVideo_mem:
	kzfree(jzhdmi->hdmi_params.pVideo);
err_free_pHdcp_mem:
	if (jzhdmi->hdmi_params.pHdcp)
		kzfree(jzhdmi->hdmi_params.pHdcp);
//err_free_pAudio_mem:
	kzfree(jzhdmi->hdmi_params.pAudio);
err_free_pProduct_mem:
	kzfree(jzhdmi->hdmi_params.pProduct);
err_free_jzhdmi_mem:
	kzfree(jzhdmi);
err_release_mem_region:
	release_mem_region(mem->start, resource_size(mem));

	jzhdmi->probe_finish = 1;
	return ret;
}

static void jzhdmi_shutdown(struct platform_device *pdev)
{
	struct jzhdmi *jzhdmi = platform_get_drvdata(pdev);

	mutex_lock(&jzhdmi->lock);
	if(!jzhdmi->is_suspended) {
		if (!api_Standby()) {
			dev_err(jzhdmi->dev, "API standby fail\n");
			return;
		}
		api_phy_enable(0);
		//regulator_disable(jzhdmi->hdmi_power);
	}
	mutex_unlock(&jzhdmi->lock);
}

static int __devexit jzhdmi_remove(struct platform_device *pdev)
{
	struct jzhdmi *jzhdmi = platform_get_drvdata(pdev);

	api_Standby();
	platform_set_drvdata(pdev, NULL);
	misc_deregister(&jzhdmi->hdmi_miscdev);
	destroy_workqueue(jzhdmi->workqueue);
	switch_dev_unregister(&jzhdmi->hdmi_switch);
	iounmap(jzhdmi->base);
	regulator_put(jzhdmi->hdmi_power);
	clk_put(jzhdmi->hdmi_clk);
	clk_put(jzhdmi->hdmi_cgu_clk);
	release_mem_region(jzhdmi->mem->start, resource_size(jzhdmi->mem));

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&jzhdmi->early_suspend);
#endif
	kzfree(jzhdmi->hdmi_params.pVideo);
	if(jzhdmi->hdmi_info.support_mode)
		kzfree(jzhdmi->hdmi_info.support_mode);
	if (jzhdmi->hdmi_params.pHdcp)
		kzfree(jzhdmi->hdmi_params.pHdcp);
	kzfree(jzhdmi->hdmi_params.pAudio);
	kzfree(jzhdmi->hdmi_params.pProduct);
	kzfree(jzhdmi);

	return 0;
}

int hdmi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct jzhdmi *jzhdmi = platform_get_drvdata(pdev);

	clk_disable(jzhdmi->hdmi_cgu_clk);
	clk_disable(jzhdmi->hdmi_clk);

	return 0;
}

int hdmi_resume(struct platform_device *pdev)
{
	struct jzhdmi *jzhdmi = platform_get_drvdata(pdev);

	clk_enable(jzhdmi->hdmi_clk);
	clk_enable(jzhdmi->hdmi_cgu_clk);

	return 0;
}

static struct platform_driver jzhdmi_driver = {
	.probe = jzhdmi_probe,
	.remove = __devexit_p(jzhdmi_remove),
	.shutdown = jzhdmi_shutdown,
	.driver = {
		.name = "jz-hdmi",
		.owner = THIS_MODULE,
	},
	.suspend = hdmi_suspend,
	.resume = hdmi_resume,
};

static int __init jzhdmi_init(void)
{
	return platform_driver_register(&jzhdmi_driver);
}

static void __exit jzhdmi_exit(void)
{
	platform_driver_unregister(&jzhdmi_driver);
}

module_init(jzhdmi_init);
module_exit(jzhdmi_exit);

MODULE_DESCRIPTION("Jz4780 HDMI driver");
MODULE_LICENSE("GPL");
