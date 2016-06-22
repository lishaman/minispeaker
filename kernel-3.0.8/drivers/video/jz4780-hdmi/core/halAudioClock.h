/*
 * halAudioClock.h
 *
 *  Created on: Jun 28, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#ifndef HAL_AUDIO_CLOCK_H_
#define HAL_AUDIO_CLOCK_H_

#include "../util/types.h"
/*
 * @param value: new N value
 */
void halAudioClock_N(u16 baseAddress, u32 value);
/*
 * @param value: new CTS value or set to zero for automatic mode
 */
void halAudioClock_Cts(u16 baseAddress, u32 value);

void halAudioClock_F(u16 baseAddress, u8 value);

#endif /* HAL_AUDIO_CLOCK_H_ */
