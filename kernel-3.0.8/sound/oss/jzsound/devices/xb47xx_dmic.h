#ifndef _XB_SND_DMIC_H__
#define _XB_SND_DMIC_H__

#include <asm/io.h>
#include <linux/delay.h>
#include "../interface/xb_snd_dsp.h"
#include "../xb_snd_detect.h"

extern unsigned int DEFAULT_RECORD_ROUTE;
extern unsigned int DEFAULT_REPLAY_ROUTE;

extern void volatile __iomem *volatile dmic_iomem;

#define NEED_RECONF_DMA         0x00000001
#define NEED_RECONF_TRIGGER     0x00000002
#define NEED_RECONF_FILTER      0x00000004

static unsigned long read_val;
static unsigned long tmp_val;

#define DMICCR0		0x0
#define DMICGCR		0x4
#define DMICIMR		0x8 //
#define DMICINTCR	0xc
#define DMICTRICR	0x10
#define DMICTHRH	0x14
#define DMICTHRL	0x18
#define DMICTRIMMAX	0x1c //
#define DMICTRINMAX 0x20 //
#define DMICDR		0x30
#define DMICFTHR	0x34
#define DMICFSR		0x38
#define DMICCGDIS	0x50 //
/**
 * DMIC register control
 **/
#define dmic_write_reg(addr,val)        \
	writel(val,dmic_iomem+addr)

#define dmic_read_reg(addr)             \
	readl(dmic_iomem+addr)

#define dmic_set_dreg(addr,val,mask,offset)\
	do {										\
		tmp_val = val;							\
		tmp_val = ((tmp_val << offset) & mask); \
		dmic_write_reg(addr,tmp_val);           \
	} while(0)

#define dmic_set_reg(addr,val,mask,offset)\
	do {										\
		tmp_val = val;							\
		read_val = dmic_read_reg(addr);         \
		read_val &= (~mask);                    \
		tmp_val = ((tmp_val << offset) & mask); \
		tmp_val |= read_val;                    \
		dmic_write_reg(addr,tmp_val);           \
	} while(0)

#define dmic_get_reg(addr,mask,offset)  \
	((dmic_read_reg(addr) & mask) >> offset)

#define dmic_clear_reg(addr,mask)       \
	dmic_write_reg(addr,~mask)
/*    DMICCR0  */
#define DMIC_RESET		31
#define DMIC_RESET_MASK		(0x1 << DMIC_RESET)
#define DMIC_RESET_TRI	30
#define DMIC_RESET_TRI_MASK	(0x1 << DMIC_RESET_TRI)
#define DMIC_UNPACK_MSB 13
#define	DMIC_UNPACK_MSB_MASK	(0x1 << DMIC_UNPACK_MSB)
#define DMIC_UNPACK_DIS	12
#define DMIC_UNPACK_DIS_MASK	(0x1 << DMIC_UNPACK_DIS)
#define DMIC_SW_LR		11
#define DMIC_SW_LR_MASK		(0x1 << DMIC_SW_LR)
#define DMIC_SPLIT_DI	10
#define DMIC_SPLIT_DI_MASK	(0x1 << DMIC_SPLIT_DI)
#define DMIC_PACK_EN	8
#define DMIC_PACK_EN_MASK	(0x1 << DMIC_PACK_EN)
#define DMIC_SR			6
#define DMIC_SR_MASK		(0x3 << DMIC_SR)
#define DMIC_STEREO		5
#define DMIC_STEREO_MASK	(0x1 << DMIC_STEREO)
#define DMIC_CHNUM			16
#define DMIC_CHNUM_MASK			(0x7 << DMIC_CHNUM)
#define DMIC_LP_MODE	3
#define DMIC_LP_MODE_MASK	(0x1 << DMIC_LP_MODE)
#define DMIC_HPF1_MODE	2
#define DMIC_HPF1_MODE_MASK	(0x1 << DMIC_HPF1_MODE)
#define DMIC_TRI_EN		1
#define DMIC_TRI_EN_MASK	(0x1 << DMIC_TRI_EN)
#define DMIC_EN			0
#define DMIC_EN_MASK		(0x1 << DMIC_EN)

#define __dmic_enable()	\
	dmic_set_reg(DMICCR0, 1, DMIC_EN_MASK, DMIC_EN)
#define __dmic_disable()	\
	dmic_set_reg(DMICCR0, 0, DMIC_EN_MASK, DMIC_EN)
#define __dmic_enable_tri()	\
	dmic_set_reg(DMICCR0, 1, DMIC_TRI_EN_MASK, DMIC_TRI_EN)
#define __dmic_disable_tri()	\
	dmic_set_reg(DMICCR0, 0, DMIC_TRI_EN_MASK, DMIC_TRI_EN)
#define __dmic_enable_hpf()	\
	dmic_set_reg(DMICCR0, 1, DMIC_HPF1_MODE_MASK, DMIC_HPF1_MODE)
#define __dmic_disable_hpf()	\
	dmic_set_reg(DMICCR0, 0, DMIC_HPF1_MODE_MASK, DMIC_HPF1_MODE)
#define __dmic_select_chnum(num)	\
	dmic_set_reg(DMICCR0, num, DMIC_CHNUM_MASK, DMIC_CHNUM)
#define __dmic_enable_lp_mode()	\
	dmic_set_reg(DMICCR0, 1, DMIC_LP_MODE_MASK, DMIC_LP_MODE)
#define __dmic_disable_lp_mode()	\
	dmic_set_reg(DMICCR0, 0, DMIC_LP_MODE_MASK, DMIC_LP_MODE)
#define __dmic_enble_switch_rl()	\
	dmic_set_reg(DMICCR0, 1, DMIC_SW_LR_MASK, DMIC_SW_LR)
#define __dmic_disable_switch_rl()	\
	dmic_set_reg(DMICCR0, 0, DMIC_SW_LR_MASK, DMIC_SW_LR)
#define __dmic_select_stereo()	\
	dmic_set_reg(DMICCR0, 1, DMIC_STEREO_MASK, DMIC_STEREO)
#define __dmic_select_mono()	\
	dmic_set_reg(DMICCR0, 0, DMIC_STEREO_MASK, DMIC_STEREO)
#define __dmic_select_8k_mode()	\
	dmic_set_reg(DMICCR0, 0, DMIC_SR_MASK, DMIC_SR)
#define __dmic_select_16k_mode()	\
	dmic_set_reg(DMICCR0, 1, DMIC_SR_MASK, DMIC_SR)
#define __dmic_select_48k_mode()	\
	dmic_set_reg(DMICCR0, 2, DMIC_SR_MASK, DMIC_SR)
#define __dmic_disable_interface()	\
	dmic_set_reg(DMICCR0, 3, DMIC_SR_MASK, DMIC_SR)
#define __dmic_enable_pack()	\
	dmic_set_reg(DMICCR0, 1, DMIC_PACK_EN_MASK, DMIC_PACK_EN)
#define __dmic_disable_pack()	\
	dmic_set_reg(DMICCR0, 0, DMIC_PACK_EN_MASK, DMIC_PACK_EN)
#define __dmic_enable_split_lr()	\
	dmic_set_reg(DMICCR0, 1, DMIC_SPLIT_DI_MASK, DMIC_SPLIT_DI)
#define __dmic_disable_split_lr()	\
	dmic_set_reg(DMICCR0, 0, DMIC_SPLIT_DI_MASK, DMIC_SPLIT_DI)
#define __dmic_en_sw_lr()		\
	dmic_set_reg(DMICCR0, 1, DMIC_SW_LR_MASK, DMIC_SW_LR)
#define __dmic_dis_sw_lr()		\
	dmic_set_reg(DMICCR0, 0, DMIC_SW_LR_MASK, DMIC_SW_LR)
#define __dmic_disable_unpack_dis()	\
	dmic_set_reg(DMICCR0, 0, DMIC_UNPACK_DIS_MASK,DMIC_UNPACK_DIS)
#define __dmic_enable_unpack_dis()	\
	dmic_set_reg(DMICCR0, 1, DMIC_UNPACK_DIS_MASK,DMIC_UNPACK_DIS)
#define __dmic_enable_unpack_msb()	\
	dmic_set_reg(DMICCR0, 1 ,DMIC_UNPACK_MSB_MASK,DMIC_UNPACK_MSB)
#define __dmic_disable_unpack_msb()	\
	dmic_set_reg(DMICCR0, 0 ,DMIC_UNPACK_MSB_MASK,DMIC_UNPACK_MSB)
#define __dmic_reset_tri()	\
	dmic_set_reg(DMICCR0, 1, DMIC_RESET_TRI_MASK, DMIC_RESET_TRI)
#define __dmic_reset()	\
	dmic_set_reg(DMICCR0, 1, DMIC_RESET_MASK, DMIC_RESET)
#define __dmic_get_reset()	\
	dmic_get_reg(DMICCR0, DMIC_RESET_MASK, DMIC_RESET)

/*DMICGCR*/
#define DMIC_GCR	0
#define DMIC_GCR_MASK	(0X1f << DMIC_GCR)
#define __dmic_set_gm(value)							\
	  do {									\
		  dmic_write_reg(DMICGCR, (value & DMIC_GCR_MASK));	\
										\
	  } while (0)
#define __dmic_get_gm()	 ( dmic_read_reg(DMICGCR) &  DMIC_GCR_MASK )

/* DMICIMR */
#define DMIC_FIFO_TRIG_MASK	5
#define DMIC_FIFO_TRIG_MSK		(1 << DMIC_FIFO_TRIG_MASK)
#define DMIC_WAKE_MASK	4
#define DMIC_WAKE_MSK		(1 << DMIC_WAKE_MASK)
#define DMIC_EMPTY_MASK 3
#define DMIC_EMPTY_MSK		(1 << DMIC_EMPTY_MASK)
#define DMIC_FULL_MASK	2
#define DMIC_FULL_MSK		(1 << DMIC_FULL_MASK)
#define DMIC_PRERD_MASK	1
#define DMIC_PRERD_MSK		(1 << DMIC_PRERD_MASK)
#define DMIC_TRI_MASK	0
#define DMIC_TRI_MSK		(1 << DMIC_TRI_MASK)

#define __dmic_enable_fifo_trig_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_FIFO_TRIG_MSK, DMIC_FIFO_TRIG_MASK)
#define __dmic_disable_fifo_trig_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_FIFO_TRIG_MSK, DMIC_FIFO_TRIG_MASK)
#define __dmic_get_fifo_trig_int()	\
	dmic_get_reg(DMICINTCR, DMIC_FIFO_TRIG_MSK, DMIC_FIFO_TRIG_MASK)

#define __dmic_enable_wake_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_WAKE_MSK, DMIC_WAKE_MASK)
#define __dmic_disable_wake_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_WAKE_MSK, DMIC_WAKE_MASK)
#define __dmic_get_wake_int()	\
	dmic_get_reg(DMICINTCR, DMIC_WAKE_MSK, DMIC_WAKE_MASK)

#define __dmic_enable_empty_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_EMPTY_MSK, DMIC_EMPTY_MASK)
#define __dmic_disable_empty_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_EMPTY_MSK, DMIC_EMPTY_MASK)
#define __dmic_get_empty_int()	\
	dmic_get_reg(DMICINTCR, DMIC_EMPTY_MSK, DMIC_EMPTY_MASK)

#define __dmic_enable_full_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_FULL_MSK, DMIC_FULL_MASK)
#define __dmic_disable_full_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_FULL_MSK, DMIC_FULL_MASK)
#define __dmic_get_full_int()	\
	dmic_get_reg(DMICINTCR, DMIC_FULL_MSK, DMIC_FULL_MASK)

#define __dmic_enable_prerd_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_PRERD_MSK, DMIC_PRERD_MASK)
#define __dmic_disable_prerd_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_PRERD_MSK, DMIC_PRERD_MASK)
#define __dmic_get_prerd_int()	\
	dmic_get_reg(DMICINTCR, DMIC_PRERD_MSK, DMIC_PRERD_MASK)

#define __dmic_enable_tri_int()	\
	dmic_set_reg(DMICIMR, 0, DMIC_TRI_MSK, DMIC_TRI_MASK)
#define __dmic_disable_tri_int()	\
	dmic_set_reg(DMICIMR, 1, DMIC_TRI_MSK, DMIC_TRI_MASK)
#define __dmic_get_tri_int()	\
	dmic_get_reg(DMICINTCR, DMIC_TRI_MSK, DMIC_TRI_MASK)

#define __dmic_disable_all_int()	\
	dmic_set_reg(DMICIMR, 0x3f, 0x3f, 0)

/*#define DMIC_INT	16
#define DMIC_INT_MASK (0x1 << 16)
#define DMIC_INT_CLEAR 0
#define DMIC_INT_CLEAR_MASK	(0x1 << DMIC_INT_CLEAR)

#define __dmic_set_int_mask()	\
	dmic_set_reg(DMICINTCR, 1, DMIC_INT_MASK, DMIC_INT)
#define __dmic_clear_int_mask()	\
	dmic_set_reg(DMICINTCR, 1, DMIC_INT_MASK, DMIC_INT)
#define __dmic_clear_int()	\
	dmic_clear_reg(DMICINTCR, DMIC_INT_CLEAR_MASK, DMIC_INT_CLEAR)
*/

/*DMICINTCR*/
#define DMIC_FIFO_TRIG_FLAG	4
#define DMIC_FIFO_TRIG_FLAG_MASK		(1 << DMIC_WAKE_FLAG)
#define DMIC_WAKE_FLAG	4
#define DMIC_WAKE_FLAG_MASK		(1 << DMIC_WAKE_FLAG)
#define DMIC_EMPTY_FLAG 3
#define DMIC_EMPTY_FLAG_MASK	(1 << DMIC_EMPTY_FLAG)
#define DMIC_FULL_FLAG	2
#define DMIC_FULL_FLAG_MASK		(1 << DMIC_FULL_FLAG)
#define DMIC_PRERD_FLAG	1
#define DMIC_PRERD_FLAG_MASK	(1 << DMIC_PRERD_FLAG)
#define DMIC_TRI_FLAG	0
#define DMIC_TRI_FLAG_MASK		(1 << DMIC_TRI_FLAG)

#define __dmic_test_fifo_trig_int()	\
	dmic_get_reg(DMICINTCR, DMIC_FIFO_TRIG_FLAG_MASK, DMIC_FIFO_TRIG_FLAG)
#define __dmic_clear_fifo_trig_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_FIFO_TRIG_FLAG_MASK, DMIC_FIFO_TRIG_FLAG)
#define __dmic_test_wake_int()	\
	dmic_get_reg(DMICINTCR, DMIC_WAKE_FLAG_MASK, DMIC_WAKE_FLAG)
#define __dmic_clear_wake_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_WAKE_FLAG_MASK, DMIC_WAKE_FLAG)
#define __dmic_test_empty_int()	\
	dmic_get_reg(DMICINTCR, DMIC_EMPTY_FLAG_MASK, DMIC_EMPTY_FLAG)
#define __dmic_clear_empty_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_EMPTY_FLAG_MASK, DMIC_EMPTY_FLAG)
#define __dmic_test_full_int()	\
	dmic_get_reg(DMICINTCR, DMIC_FULL_FLAG_MASK, DMIC_FULL_FLAG)
#define __dmic_clear_full_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_FULL_FLAG_MASK, DMIC_FULL_FLAG)
#define __dmic_test_prerd_int()	\
	dmic_get_reg(DMICINTCR, DMIC_PRERD_FLAG_MASK, DMIC_PRERD_FLAG)
#define __dmic_clear_prerd_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_PRERD_FLAG_MASK, DMIC_PRERD_FLAG)
#define __dmic_test_tri_int()	\
	dmic_get_reg(DMICINTCR, DMIC_TRI_FLAG_MASK, DMIC_TRI_FLAG)
#define __dmic_clear_tri_flag()	\
	dmic_set_dreg(DMICINTCR, 1, DMIC_TRI_FLAG_MASK, DMIC_TRI_FLAG)
#define __dmic_clear_tur()	\
	dmic_set_reg(DMICINTCR, 0x3f, 0x3f, 0)

/*DMICTRICR*/
#define DMIC_TRI_MODE	16
#define DMIC_TRI_MODE_MASK	(0xf << DMIC_TRI_MODE)
#define DMIC_TRI_DEBUG	4
#define DMIC_TRI_DEBUG_MASK	(0x1 << DMIC_TRI_DEBUG)
#define DMIC_HPF2_EN	3
#define DMIC_HPF2_EN_MASK (0x1 << DMIC_HPF2_EN)
#define DMIC_PREFETCH	1
#define DMIC_PREFETCH_MASK	(0x3 << DMIC_PREFETCH)
#define DMIC_TRI_CLR	0
#define DMIC_TRI_CLR_MASK	(0x1 << DMIC_TRI_CLR)

#define __dmic_clear_trigger()	\
	dmic_set_reg(DMICTRICR, 1, DMIC_TRI_CLR_MASK, DMIC_TRI_CLR)
#define __dmic_enable_hpf2()	\
	dmic_set_reg(DMICTRICR, 1, DMIC_HPF2_EN_MASK, DMIC_HPF2_EN)
#define __dmic_disable_hpf2()	\
	dmic_set_reg(DMICTRICR, 0, DMIC_HPF2_EN_MASK, DMIC_HPF2_EN)
#define __dmic_disable_prefetch()	\
	dmic_set_reg(DMICTRICR, 0, DMIC_PREFETCH_MASK, DMIC_PREFETCH)
#define __dmic_select_prefetch_8k()	\
	dmic_set_reg(DMICTRICR, 1, DMIC_PREFETCH_MASK, DMIC_PREFETCH)
#define __dmic_select_prefetch_16k()	\
	dmic_set_reg(DMICTRICR, 2, DMIC_PREFETCH_MASK, DMIC_PREFETCH)
#define __dmic_enable_debug()	\
	dmic_set_reg(DMICTRICR, 1, DMIC_TRI_DEBUG_MASK, DMIC_TRI_DEBUG)
#define __dmic_disable_debug()	\
	dmic_set_reg(DMICTRICR, 0, DMIC_TRI_DEBUG_MASK, DMIC_TRI_DEBUG)
#define __dmic_set_tri_mode(n)	\
	dmic_set_reg(DMICTRICR, n, DMIC_TRI_MODE_MASK, DMIC_TRI_MODE)

/*DMICTHRH*/
#define DMIC_THR_H 0
#define DMIC_THR_H_MASK (0xfffff << DMIC_THR_H)

#define __dmic_set_thr_high(n)	\
	dmic_set_reg(DMICTHRH, n, DMIC_THR_H_MASK, DMIC_THR_H)

/*DMICTHRL*/
#define DMIC_THR_L 0
#define DMIC_THR_L_MASK (0xfffff << DMIC_THR_L)

#define __dmic_set_thr_low(n)	\
	dmic_set_reg(DMICTHRL, n, DMIC_THR_L_MASK, DMIC_THR_L)

/* DMICTRIMMAX */
#define DMIC_M_MAX	0
#define DMIC_M_MAX_MASK	(0xffffff << DMIC_M_MAX)

#define	__dmic_set_m_max(n)	\
	dmic_set_reg(DMICTRIMMAX, n, DMIC_M_MAX_MASK, DMIC_M_MAX)

/* DMICTRINMAX */
#define DMIC_N_MAX	0
#define DMIC_N_MAX_MASK	(0xffff << DMIC_N_MAX)

#define	__dmic_set_n_max(n)	\
	dmic_set_reg(DMICTRINMAX, n, DMIC_N_MAX_MASK, DMIC_N_MAX)

/*DMICDR*/
#define __dmic_get_fifo()	\
	dmic_get_reg(DMICDR, 0xffffffff, 0)

/* DMICFTHR */
#define DMIC_RDMS	31
#define DMIC_RDMS_MASK	(0x1 << DMIC_RDMS)
#define DMIC_FIFO_THR	0
#define DMIC_FIFO_THR_MASK	(0x3f << DMIC_FIFO_THR)

#define __dmic_enable_rdms()	\
	dmic_set_reg(DMICFTHR, 1, DMIC_RDMS_MASK, DMIC_RDMS)
#define __dmic_disable_rdms()	\
	dmic_set_reg(DMICFTHR, 0, DMIC_RDMS_MASK, DMIC_RDMS)
#define __dmic_set_request(n)	\
	dmic_set_reg(DMICFTHR, n, DMIC_FIFO_THR_MASK, DMIC_FIFO_THR)

/*DMICFSR*/
#define DMIC_FULLS	19
#define DMIC_FULLS_MASK	(0x1 << DMIC_FULLS)
#define DMIC_TRIGS	18
#define DMIC_TRIGS_MASK	(0x1 << DMIC_TRIGS)
#define DMIC_PRERDS	17
#define DMIC_PRERDS_MASK	(0x1 << DMIC_PRERDS)
#define DMIC_EMPTYS	16
#define DMIC_EMPTYS_MASK	(0x1 << DMIC_EMPTYS)
#define DMIC_FIFO_LVL 0
#define DMIC_FIFO_LVL_MASK	(0x3f << DMIC_FIFO_LVL)

#define __dmic_test_trig()	\
	dmic_get_reg(DMICFSR, DMIC_TRIGS_MASK, DMIC_TRIGS)
#define __dmic_test_full()	\
	dmic_get_reg(DMICFSR, DMIC_FULLS_MASK, DMIC_FULLS)
#define __dmic_test_prerd()	\
	dmic_get_reg(DMICFSR, DMIC_PRERDS_MASK, DMIC_PRERDS)
#define __dmic_test_empty()	\
	dmic_get_reg(DMICFSR, DMIC_EMPTY_MASK, DMIC_EMPTYS)
#define __dmic_get_lol()	\
	dmic_get_reg(DMICFSR, DMIC_FIFO_LVL_MASK, DMIC_FIFO_LVL)
#define __dmic_get_fsr()	\
	dmic_get_reg(DMICFSR, 0xffffffff, 0)

/* DMICCGDIS */
#define DMIC_HPF_CGDIS	17
#define DMIC_HPF_CGDIS_MASK	(0x1 << 17)
#define DMIC_TRI_CGDIS	16
#define DMIC_TRI_CGDIS_MASK	(0x1 << 16)
#define DMIC_HPF1R_CGDIS	13
#define DMIC_HPF1R_CGDIS_MASK	(0x1 << 13)
#define DMIC_SRC3R_CGDIS	12
#define DMIC_SRC3R_CGDIS_MASK	(0x1 << 12)
#define DMIC_SRC2R_CGDIS	11
#define DMIC_SRC2R_CGDIS_MASK	(0x1 << 11)
#define DMIC_SRC1R_CGDIS	10
#define DMIC_SRC1R_CGDIS_MASK	(0x1 << 10)
#define DMIC_LPF_240KR_CGDIS	9
#define DMIC_LPF_240KR_CGDIS_MASK	(0x1 << 9)
#define DMIC_LPF_DMCKR_CGDIS	8
#define DMIC_LPF_DMCKR_CGDIS_MASK	(0x1 << 8)
#define DMIC_HPF1L_CGDIS	5
#define DMIC_HPF1L_CGDIS_MASK	(0x1 << 5)
#define DMIC_SRC3L_CGDIS	4
#define DMIC_SRC3L_CGDIS_MASK	(0x1 << 4)
#define DMIC_SRC2L_CGDIS	3
#define DMIC_SRC2L_CGDIS_MASK	(0x1 << 3)
#define DMIC_SRC1L_CGDIS	2
#define DMIC_SRC1L_CGDIS_MASK	(0x1 << 2)
#define DMIC_LPF_240KL_CGDIS	1
#define DMIC_LPF_240KL_CGDIS_MASK	(0x1 << 1)
#define DMIC_LPF_DMCKL_CGDIS	0
#define DMIC_LPF_DMCKL_CGDIS_MASK	(0x1 << 0)


#define __dmic_enable_hpf_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_HPF_CGDIS_MASK, DMIC_HPF_CGDIS)
#define __dmic_disable_hpf_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_HPF_CGDIS_MASK, DMIC_HPF_CGDIS)
#define __dmic_enable_tri_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_TRI_CGDIS_MASK, DMIC_TRI_CGDIS)
#define __dmic_disable_tri_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_TRI_CGDIS_MASK, DMIC_TRI_CGDIS)
#define __dmic_enable_hpf1r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_HPF1R_CGDIS_MASK, DMIC_HPF1R_CGDIS)
#define __dmic_disable_hpf1r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_HPF1R_CGDIS_MASK, DMIC_HPF1R_CGDIS)
#define __dmic_enable_src3r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_SRC3R_CGDIS_MASK, DMIC_SRC3R_CGDIS)
#define __dmic_disable_src3r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_SRC3R_CGDIS_MASK, DMIC_SRC3R_CGDIS)
#define __dmic_enable_src2r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_SRC2R_CGDIS_MASK, DMIC_SRC2R_CGDIS)
#define __dmic_disable_src2r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_SRC2R_CGDIS_MASK, DMIC_SRC2R_CGDIS)
#define __dmic_enable_src1r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 0, DMIC_SRC1R_CGDIS_MASK, DMIC_SRC1R_CGDIS)
#define __dmic_disable_src1r_cgdis()	\
	dmic_set_reg(DMICCGDIS, 1, DMIC_SRC1R_CGDIS_MASK, DMIC_SRC1R_CGDIS)
#define __dmic_set_gate()	\
	dmic_set_reg(DMICCGDIS, 0xffffffff, 0xffffffff, 0)
#define __dmic_clear_gate()	\
	dmic_set_reg(DMICCGDIS, 0x0, 0xffffffff, 0)

#endif
