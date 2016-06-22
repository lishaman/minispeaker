/*
 * halAudioGpa.c
 *
 * Created on Oct. 30th 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "halAudioGpa.h"
#include "../bsp/access.h"
#include "../util/log.h"

static const u16 GP_CONF0 = 0x00;
static const u16 GP_CONF1 = 0x01;
static const u16 GP_CONF2 = 0x02;
static const u16 GP_STAT  = 0x03;

static const u16 GP_MASK  = 0x06;
static const u16 GP_POL   = 0x05;

void halAudioGpa_ResetFifo(u16 baseAddress)
{
 	LOG_TRACE();
	access_CoreWrite(1, baseAddress + GP_CONF0, 0, 1);
}

void halAudioGpa_ChannelEnable(u16 baseAddress, u8 enable, u8 channel)
{
	LOG_TRACE();
	access_CoreWrite(enable, baseAddress + GP_CONF1, channel, 1);
}

void halAudioGpa_HbrEnable(u16 baseAddress, u8 enable)
{
 	LOG_TRACE();
 	/* 8 channels must be enabled */
	access_CoreWrite(enable, baseAddress + GP_CONF2, 0, 1);
}

void halAudioGpa_InsertPucv(u16 baseAddress, u8 enable)
{
 	LOG_TRACE();
	access_CoreWrite(enable, baseAddress + GP_CONF2, 1, 1);
}

u8 halAudioGpa_InterruptStatus(u16 baseAddress)
{
	LOG_TRACE();
	return access_CoreRead(baseAddress + GP_STAT, 0, 2);
}

void halAudioGpa_InterruptMask(u16 baseAddress, u8 value)
{
	LOG_TRACE();
	access_CoreWrite(value, baseAddress + GP_MASK, 0, 2);
}

void halAudioGpa_InterruptPolarity(u16 baseAddress, u8 value)
{
	LOG_TRACE();
	access_CoreWrite(value, baseAddress + GP_POL, 0, 2);
}
