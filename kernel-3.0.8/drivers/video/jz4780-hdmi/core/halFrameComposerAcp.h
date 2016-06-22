/*
 * halFrameComposerAcp.h
 *
 *  Created on: Jun 30, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef HALFRAMECOMPOSERACP_H_
#define HALFRAMECOMPOSERACP_H_

#include "../util/types.h"

void halFrameComposerAcp_Type(u16 baseAddr, u8 type);

void halFrameComposerAcp_TypeDependentFields(u16 baseAddr, u8 * fields,
		u8 fieldsLength);

#endif /* HALFRAMECOMPOSERACP_H_ */
