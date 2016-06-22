/*
 * access.c
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "access.h"
#include "../util/log.h"

#include "../util/error.h"
#include "mutex.h"

static mutex_t access_mMutex;
//DECLARE_MUTEX(access_mMutex);
static int access_mutex_inited = 0;
static volatile u32 * access_mBaseAddr = 0;

int access_Initialize(u8 * baseAddr)
{
	access_mBaseAddr = (u32 *)baseAddr;
	if (!access_mutex_inited)
	{
		mutex_Initialize(&access_mMutex);
		access_mutex_inited = 1;
	}
	return TRUE;
}

int access_Standby()
{
	if (access_mutex_inited)
	{
		mutex_Destruct(&access_mMutex);
		access_mutex_inited = 0;
	}
	return TRUE;
}

u8 access_CoreReadByte(u16 addr)
{
	u8 data = 0;
	mutex_Lock(&access_mMutex);
	data = access_Read(addr);
	mutex_Unlock(&access_mMutex);
	return data;
}

u8 access_CoreRead(u16 addr, u8 shift, u8 width)
{
	if (width <= 0)
	{
		error_Set(ERR_DATA_WIDTH_INVALID);
		LOG_ERROR("Invalid parameter: width == 0");
		return 0;
	}
	return (access_CoreReadByte(addr) >> shift) & (BIT(width) - 1);
}

void access_CoreWriteByte(u8 data, u16 addr)
{
	mutex_Lock(&access_mMutex);
	access_Write(data, addr);
	mutex_Unlock(&access_mMutex);
}

void access_CoreWrite(u8 data, u16 addr, u8 shift, u8 width)
{
	u8 temp = 0;
	u8 mask = 0;
	if (width <= 0)
	{
		error_Set(ERR_DATA_WIDTH_INVALID);
		LOG_ERROR("Invalid parameter: width == 0");
		return;
	}
	mask = BIT(width) - 1;
	if (data > mask)
	{
		error_Set(ERR_DATA_GREATER_THAN_WIDTH);
		LOG_ERROR("Invalid parameter: data > width");
		return;
	}
	mutex_Lock(&access_mMutex);
	temp = access_Read(addr);
	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	access_Write(temp, addr);
	mutex_Unlock(&access_mMutex);
}

u8 access_Read(u16 addr)
{
	/* mutex is locked */
	return access_mBaseAddr[addr];
}

void access_Write(u8 data, u16 addr)
{
	/* mutex is locked */
	LOG_WRITE(addr, data);
	access_mBaseAddr[addr] = data;
}
