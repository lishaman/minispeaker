/*
 * monitorRangeLimits.c
 *
 *  Created on: Jul 22, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "monitorRangeLimits.h"
#include "../util/bitOperation.h"
#include "../util/log.h"

void monitorRangeLimits_Reset(monitorRangeLimits_t * mrl)
{
	mrl->mMinVerticalRate = 0;
	mrl->mMaxVerticalRate = 0;
	mrl->mMinHorizontalRate = 0;
	mrl->mMaxHorizontalRate = 0;
	mrl->mMaxPixelClock = 0;
	mrl->mValid = FALSE;
}
int monitorRangeLimits_Parse(monitorRangeLimits_t * mrl, u8 * data)
{
	LOG_TRACE();
	monitorRangeLimits_Reset(mrl);
	if (data != 0 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00
			&& data[3] == 0xFD && data[4] == 0x00)
	{ /* block tag */
		mrl->mMinVerticalRate = data[5];
		mrl->mMaxVerticalRate = data[6];
		mrl->mMinHorizontalRate = data[7];
		mrl->mMaxHorizontalRate = data[8];
		mrl->mMaxPixelClock = data[9];
		mrl->mValid = TRUE;
		return TRUE;
	}
	return FALSE;
}

u8 monitorRangeLimits_GetMaxHorizontalRate(monitorRangeLimits_t * mrl)
{
	return mrl->mMaxHorizontalRate;
}

u8 monitorRangeLimits_GetMaxPixelClock(monitorRangeLimits_t * mrl)
{
	return mrl->mMaxPixelClock;
}

u8 monitorRangeLimits_GetMaxVerticalRate(monitorRangeLimits_t * mrl)
{
	return mrl->mMaxVerticalRate;
}

u8 monitorRangeLimits_GetMinHorizontalRate(monitorRangeLimits_t * mrl)
{
	return mrl->mMinHorizontalRate;
}

u8 monitorRangeLimits_GetMinVerticalRate(monitorRangeLimits_t * mrl)
{
	return mrl->mMinVerticalRate;
}
