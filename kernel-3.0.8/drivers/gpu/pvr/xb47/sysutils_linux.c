/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
#include <linux/opp.h>
#endif

#if defined(SUPPORT_DRI_DRM_PLUGIN)
#include <drm/drmP.h>
#include <drm/drm.h>

#include <linux/xb_gpu.h>

#include "pvr_drm.h"
#endif

#include <soc/cpm.h>
#include <soc/base.h>
#include <mach/jzcpm_pwc.h>
#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#define SGX_PARENT_CLOCK "core_ck"

// The proc entry for our /proc/pvr directory
extern struct proc_dir_entry * dir;

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
extern struct platform_device *gpsPVRLDMDev;
#endif

static IMG_BOOL PowerSupplyIsOFF(IMG_VOID *handle)
{
	return !cpm_pwc_is_enabled(handle);
}

static IMG_VOID TurnOffPowerSupply(IMG_UINT32 uData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *)uData;
	if(!PowerSupplyIsOFF(psSysSpecData->pPowerHandle)) {
		cpm_pwc_disable(psSysSpecData->pPowerHandle);
	}
}

static IMG_VOID TurnOnPowerSupply(IMG_UINT32 uData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *)uData;
	if(PowerSupplyIsOFF(psSysSpecData->pPowerHandle))
		cpm_pwc_enable(psSysSpecData->pPowerHandle);
}

static PVRSRV_ERROR SetClockRate(SYS_SPECIFIC_DATA *psSysSpecificData, IMG_UINT32 ui32RequiredRate)
{
    struct clk *parent;
    IMG_UINT32 ui32ParentRate, ui32RealRate;
    
    if(psSysSpecificData->psTimer_Divider) {
        parent = clk_get_parent(psSysSpecificData->psTimer_Divider);
        ui32ParentRate = clk_get_rate(parent);

        if(ui32ParentRate % ui32RequiredRate)
            ui32RealRate = ((ui32ParentRate / (ui32ParentRate / ui32RequiredRate)) > SYS_SGX_CLOCK_MAX_SPEED) ?
                    ui32RequiredRate : (ui32ParentRate / (ui32ParentRate / ui32RequiredRate));
        else
            ui32RealRate = ui32RequiredRate;

        clk_set_rate(psSysSpecificData->psTimer_Divider, ui32RealRate);
        psSysSpecificData->ui32CurrentSpeed = clk_get_rate(psSysSpecificData->psTimer_Divider);
        
        // Rarely happen, but SHOULD double check
        if(psSysSpecificData->ui32CurrentSpeed > ui32RealRate) {
            PVR_DPF((PVR_DBG_ERROR, "Ops, Should NOT happen, please check CPM\n"));
            return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
        }
    }
    return PVRSRV_OK;
}

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData, IMG_BOOL bTryLock)
{
	if (!in_interrupt())
	{
		if (bTryLock)
		{
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
			{
				return PVRSRV_ERROR_RETRY;
			}
		}
		else
		{
			mutex_lock(&psSysSpecData->sPowerLock);
		}
	}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		mutex_unlock(&psSysSpecData->sPowerLock);
	}
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
}

IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#endif
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	psTimingInfo->ui32CoreClockSpeed =
		gpsSysSpecificData->pui32SGXFreqList[gpsSysSpecificData->ui32SGXFreqListIndex];
#else
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
#endif
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
    //    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;


	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		IMG_UINT32 max_freq_index;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;
		max_freq_index = psSysSpecData->ui32SGXFreqListSize - 2;


		if (psSysSpecData->ui32SGXFreqListIndex != max_freq_index)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
									  &gpsPVRLDMDev->dev,
									  psSysSpecData->pui32SGXFreqList[max_freq_index]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = max_freq_index;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif
        {
            del_timer_sync(&psSysSpecData->psPowerDown_Timer);
            clk_enable(psSysSpecData->psTimer_Divider);
            clk_enable(psSysSpecData->psTimer_Gate);
            TurnOnPowerSupply((IMG_UINT32)psSysSpecData);
        }

	{
		int res = pm_runtime_get_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: pm_runtime_get_sync failed (%d)", -res));
			return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
		}
	}
#endif

	SysEnableSGXInterrupts(psSysData);


	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif
	return PVRSRV_OK;
}


IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
    //    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;


	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
		}
	}
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;


		if (psSysSpecData->ui32SGXFreqListIndex != 0)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
									  &gpsPVRLDMDev->dev,
									  psSysSpecData->pui32SGXFreqList[0]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = 0;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_WARNING, "DisableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif
        {

        }
#endif

        {
		int count = 10000;
		//wait gpu idle
		while(!cpm_test_bit(20,CPM_LCR) && count--);
		if(count <= 0) {
			printk("wait idle timeout\n");
		}
		if(count < 9999)
			printk("gpu count = %d\n",count);
		if(count > 0) {
			clk_disable(psSysSpecData->psTimer_Gate);
			clk_disable(psSysSpecData->psTimer_Divider);
			mod_timer(&psSysSpecData->psPowerDown_Timer,
				  jiffies + msecs_to_jiffies(SYS_SGX_ACTIVE_POWER_POWER_DOWN_LATENCY_MS));

		}else {
			del_timer_sync(&psSysSpecData->psPowerDown_Timer);
		}

        }

	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif
}

static int turbo_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len = 0;
        SYS_SPECIFIC_DATA *psSysSpecData = data;

        len += sprintf(page+len,"%s\n", (psSysSpecData->bTurboState == IMG_TRUE) ? "on" : "off");

	return len;
}

static int turbo_write_proc(struct file *file, const char __user *buffer,
                            unsigned long count, void *data)
{
    SYS_SPECIFIC_DATA *psSysSpecData = data;
    const IMG_CHAR TurboSwitchOn[] = "on";

    if(!strncmp(buffer, TurboSwitchOn, strlen(TurboSwitchOn))) {
        if(psSysSpecData->psTimer_Divider) {
            if(SetClockRate(psSysSpecData, SYS_SGX_CLOCK_MAX_SPEED) != PVRSRV_OK) {
                PVR_DPF((PVR_DBG_ERROR, "SwitchToTurbo: Couldn't set clock to MAX"));
                return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
            }
            psSysSpecData->bTurboState = IMG_TRUE;
        }
    } else {
        if(psSysSpecData->psTimer_Divider) {
            if(SetClockRate(psSysSpecData, SYS_SGX_CLOCK_SPEED) != PVRSRV_OK) {
                PVR_DPF((PVR_DBG_ERROR, "SwitchToTurbo: Couldn't set clock to Normal"));
                return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
            }

            psSysSpecData->bTurboState = IMG_FALSE;
        }
    }

    return count;
}

static IMG_INT CreateProcEntries(SYS_SPECIFIC_DATA *psSysSpecData)
{
    struct proc_dir_entry *res;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: cannot make /proc/pvr/turbo"));

        return -ENOMEM;
    }

    res = create_proc_entry("turbo", 0644, dir);
    if (res) {
        res->read_proc = turbo_read_proc;
        res->write_proc = turbo_write_proc;
        res->data = psSysSpecData;
    }

    return PVRSRV_OK;
}

IMG_VOID RemoveXBProcEntries(IMG_VOID)
{
    if (dir)
    {
        remove_proc_entry("turbo", dir);
    }
}

static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
#if defined(PVR_XB47_TIMING_CPM)
        PVRSRV_ERROR eError;
        struct clk *sys_ck;
        struct clk *psCLK;

	sys_ck = clk_get(NULL, "gpu");
	if (IS_ERR(sys_ck))
	{
            PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
            eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
            goto Exit;
	}
	psSysSpecData->psTimer_Gate = sys_ck;

        psCLK = clk_get(NULL, "cgu_gpu");
	if (IS_ERR(psCLK))
	{
            PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
            goto ExitPutGate;
	}
	psSysSpecData->psTimer_Divider = psCLK;

        if(psSysSpecData->ui32CurrentSpeed != 0) {
            if(SetClockRate(psSysSpecData, psSysSpecData->ui32CurrentSpeed) != PVRSRV_OK)
                goto ExitDisableTimerGate;
        } else {
            if(SetClockRate(psSysSpecData, SYS_SGX_CLOCK_SPEED) != PVRSRV_OK)
                goto ExitDisableTimerGate;
        }

        // Init the timer for turning off the power supply
        psSysSpecData->psPowerDown_Timer.expires = jiffies + msecs_to_jiffies(SYS_SGX_ACTIVE_POWER_POWER_DOWN_LATENCY_MS);
        psSysSpecData->psPowerDown_Timer.function = TurnOffPowerSupply;
        psSysSpecData->psPowerDown_Timer.data = (IMG_UINT32)psSysSpecData;

        psSysSpecData->pPowerHandle = cpm_pwc_get(PWC_GPU);

        init_timer(&psSysSpecData->psPowerDown_Timer);

        CreateProcEntries(psSysSpecData);

	eError = PVRSRV_OK;

	goto Exit;

ExitDisableTimerGate:
	eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
        clk_put(psSysSpecData->psTimer_Divider);
ExitPutGate:
        clk_put(psSysSpecData->psTimer_Gate);
        eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
Exit:
	return eError;

#else //#if defined(PVR_XB47_TIMING_CPM)

	PVR_UNREFERENCED_PARAMETER(psSysSpecData);

	return PVRSRV_OK;
#endif //#if defined(PVR_XB47_TIMING_CPM)
}

static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
#if defined(PVR_XB47_TIMING_CPM)

    //    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	clk_put(psSysSpecData->psTimer_Gate);
	clk_put(psSysSpecData->psTimer_Divider);

	cpm_pwc_put(psSysSpecData->pPowerHandle);
        RemoveXBProcEntries();

#else //#if defined(PVR_XB47_TIMING_CPM)
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
#endif
}

PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	return AcquireGPTimer(psSysSpecData);
}

IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));


	DisableSGXClocks(psSysData);

	ReleaseGPTimer(psSysSpecData);
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_enable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_disable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
    //	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

        psSysSpecificData->bTurboState = IMG_FALSE;
        psSysSpecificData->ui32CurrentSpeed = 0;
#else
	IMG_UINT32 i, *freq_list;
	IMG_INT32 opp_count;
	unsigned long freq;
	struct opp *opp;


	rcu_read_lock();
	opp_count = opp_get_opp_count(&gpsPVRLDMDev->dev);
	if (opp_count < 1)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp count"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}


	freq_list = kmalloc((opp_count + 1) * sizeof(IMG_UINT32), GFP_ATOMIC);
	if (!freq_list)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not allocate frequency list"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}


	freq = 0;
	for (i = 0; i < opp_count; i++)
	{
		opp = opp_find_freq_ceil(&gpsPVRLDMDev->dev, &freq);
		if (IS_ERR_OR_NULL(opp))
		{
			rcu_read_unlock();
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp level %d", i));
			kfree(freq_list);
			return PVRSRV_ERROR_NOT_SUPPORTED;
		}
		freq_list[i] = (IMG_UINT32)freq;
		freq++;
	}
	rcu_read_unlock();
	freq_list[opp_count] = freq_list[opp_count - 1];

	psSysSpecificData->ui32SGXFreqListSize = opp_count + 1;
	psSysSpecificData->pui32SGXFreqList = freq_list;


	psSysSpecificData->ui32SGXFreqListIndex = opp_count;
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
#else

	if (psSysSpecificData->ui32SGXFreqListIndex != 0)
	{
		struct gpu_platform_data *pdata;
		IMG_INT32 res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;

		PVR_ASSERT(pdata->device_scale != IMG_NULL);
		res = pdata->device_scale(&gpsPVRLDMDev->dev,
								  &gpsPVRLDMDev->dev,
								  psSysSpecificData->pui32SGXFreqList[0]);
		if (res == -EBUSY)
		{
			PVR_DPF((PVR_DBG_WARNING, "SysDvfsDeinitialize: Unable to scale SGX frequency (EBUSY)"));
		}
		else if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsDeinitialize: Unable to scale SGX frequency (%d)", res));
		}

		psSysSpecificData->ui32SGXFreqListIndex = 0;
	}

	kfree(psSysSpecificData->pui32SGXFreqList);
	psSysSpecificData->pui32SGXFreqList = 0;
	psSysSpecificData->ui32SGXFreqListSize = 0;
#endif

	return PVRSRV_OK;
}
