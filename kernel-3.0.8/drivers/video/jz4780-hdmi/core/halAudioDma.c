/*
 * @file:halAudioDma.c
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "halAudioDma.h"
#include "../bsp/access.h"
#include "../util/log.h"

/* register offsets */
static const u8 AHB_DMA_CONF0     = 0x00;
static const u8 AHB_DMA_START     = 0x01;
static const u8 AHB_DMA_STOP      = 0x02;
static const u8 AHB_DMA_THRSLD    = 0x03;
static const u8 AHB_DMA_STRADDR0  = 0x04;
static const u8 AHB_DMA_STRADDR1  = 0x05;
static const u8 AHB_DMA_STRADDR2  = 0x06;
static const u8 AHB_DMA_STRADDR3  = 0x07;
static const u8 AHB_DMA_STPADDR0  = 0x08;
static const u8 AHB_DMA_STPADDR1  = 0x09;
static const u8 AHB_DMA_STPADDR2  = 0x0A;
static const u8 AHB_DMA_STPADDR3  = 0x0B;
static const u8 AHB_DMA_BSTADDR0  = 0x0C;
static const u8 AHB_DMA_BSTADDR1  = 0x0D;
static const u8 AHB_DMA_BSTADDR2  = 0x0E;
static const u8 AHB_DMA_BSTADDR3  = 0x0F;
static const u8 AHB_DMA_MBLENGTH0 = 0x10;
static const u8 AHB_DMA_MBLENGTH1 = 0x11;
static const u8 AHB_DMA_MASK      = 0x14;
static const u8 AHB_DMA_CONF1     = 0x16;
static const u8 AHB_DMA_BUFFMASK  = 0x19;

void halAudioDma_ResetFifo(u16 baseAddress)
{
 	LOG_TRACE();
	access_CoreWrite(1, baseAddress + AHB_DMA_CONF0, 7, 1);
}

void halAudioDma_HbrEnable(u16 baseAddress, u8 enable)
{
 	LOG_TRACE();
 	/* 8 channels must be enabled */
	access_CoreWrite(enable, baseAddress + AHB_DMA_CONF0, 4, 1);
}

void halAudioDma_HlockEnable(u16 baseAddress, u8 enable)
{
 	LOG_TRACE();
	access_CoreWrite(enable, baseAddress + AHB_DMA_CONF0, 3, 1);
}

void halAudioDma_FixBurstMode(u16 baseAddress, u8 fixedBeatIncrement)
{
	LOG_TRACE();
	access_CoreWrite((fixedBeatIncrement > 3)? fixedBeatIncrement: 0, baseAddress + AHB_DMA_CONF0, 1, 2);
	access_CoreWrite((fixedBeatIncrement > 3)? 0: 1, baseAddress + AHB_DMA_CONF0, 0, 1);
}

void halAudioDma_Start(u16 baseAddress)
{
	LOG_TRACE();
	access_CoreWrite(1, baseAddress + AHB_DMA_START, 0, 1);
}

void halAudioDma_Stop(u16 baseAddress)
{
	access_CoreWrite(1, baseAddress + AHB_DMA_STOP, 0, 1);
}

void halAudioDma_Threshold(u16 baseAddress, u8 threshold)
{
	LOG_TRACE();
	access_CoreWriteByte(threshold, baseAddress + AHB_DMA_THRSLD);
}

void halAudioDma_StartAddress(u16 baseAddress, u32 startAddress)
{
	LOG_TRACE();
	access_CoreWriteByte((u8)(startAddress), baseAddress + AHB_DMA_STRADDR0);
	access_CoreWriteByte((u8)(startAddress >> 8), baseAddress + AHB_DMA_STRADDR1);
	access_CoreWriteByte((u8)(startAddress >> 16), baseAddress + AHB_DMA_STRADDR2);
	access_CoreWriteByte((u8)(startAddress >> 24), baseAddress + AHB_DMA_STRADDR3);
}

void halAudioDma_StopAddress(u16 baseAddress, u32 stopAddress)
{
	LOG_TRACE();
	access_CoreWriteByte((u8)(stopAddress), baseAddress + AHB_DMA_STPADDR0);
	access_CoreWriteByte((u8)(stopAddress >> 8), baseAddress + AHB_DMA_STPADDR1);
	access_CoreWriteByte((u8)(stopAddress >> 16), baseAddress + AHB_DMA_STPADDR2);
	access_CoreWriteByte((u8)(stopAddress >> 24), baseAddress + AHB_DMA_STPADDR3);
}

u32 halAudioDma_CurrentOperationAddress(u16 baseAddress)
{
	u32 temp = 0;
	LOG_TRACE();
	temp = access_CoreReadByte(baseAddress + AHB_DMA_BSTADDR0);
	temp |= access_CoreReadByte(baseAddress + AHB_DMA_BSTADDR0) << 8;
	temp |= access_CoreReadByte(baseAddress + AHB_DMA_BSTADDR0) << 16;
	temp |= access_CoreReadByte(baseAddress + AHB_DMA_BSTADDR0) << 24;
	return temp;
}

u16 halAudioDma_CurrentBurstLength(u16 baseAddress)
{
	u16 temp = 0;
	LOG_TRACE();
	temp = access_CoreReadByte(baseAddress + AHB_DMA_MBLENGTH0);
	temp |= access_CoreReadByte(baseAddress + AHB_DMA_MBLENGTH1) << 8;
	return temp;
}

void halAudioDma_DmaInterruptMask(u16 baseAddress, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWriteByte(mask, baseAddress + AHB_DMA_MASK);
}

void halAudioDma_BufferInterruptMask(u16 baseAddress, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWriteByte(mask, baseAddress + AHB_DMA_BUFFMASK);
}

u8 halAudioDma_DmaInterruptMaskStatus(u16 baseAddress)
{
	LOG_TRACE();
	return access_CoreReadByte(baseAddress + AHB_DMA_MASK);
}
u8 halAudioDma_BufferInterruptMaskStatus(u16 baseAddress)
{
	return access_CoreReadByte(baseAddress + AHB_DMA_BUFFMASK);
}

void halAudioDma_ChannelEnable(u16 baseAddress, u8 enable, u8 channel)
{
	LOG_TRACE1(channel);
	access_CoreWrite(enable, baseAddress + AHB_DMA_CONF1, channel, 1);
}
