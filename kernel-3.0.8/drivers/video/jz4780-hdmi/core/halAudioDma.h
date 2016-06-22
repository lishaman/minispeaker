/*
 * @file:halAudioDma.h
 *
 *  Created on: May 3, 2011
 *
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef HALAUDIODMA_H_
#define HALAUDIODMA_H_

#include "../util/types.h"

void halAudioDma_ResetFifo(u16 baseAddress);
void halAudioDma_HbrEnable(u16 baseAddress, u8 enable);
void halAudioDma_HlockEnable(u16 baseAddress, u8 enable);
void halAudioDma_FixBurstMode(u16 baseAddress, u8 fixedBeatIncrement);
void halAudioDma_Start(u16 baseAddress);
void halAudioDma_Stop(u16 baseAddress);
void halAudioDma_Threshold(u16 baseAddress, u8 threshold);
void halAudioDma_StartAddress(u16 baseAddress, u32 startAddress);
void halAudioDma_StopAddress(u16 baseAddress, u32 stopAddress);
u32 halAudioDma_CurrentOperationAddress(u16 baseAddress);
u16 halAudioDma_CurrentBurstLength(u16 baseAddress);
void halAudioDma_DmaInterruptMask(u16 baseAddress, u8 mask);
void halAudioDma_BufferInterruptMask(u16 baseAddress, u8 mask);
u8 halAudioDma_DmaInterruptMaskStatus(u16 baseAddress);
u8 halAudioDma_BufferInterruptMaskStatus(u16 baseAddress);
void halAudioDma_ChannelEnable(u16 baseAddress, u8 enable, u8 channel);


#endif /* HALAUDIODMA_H_ */
