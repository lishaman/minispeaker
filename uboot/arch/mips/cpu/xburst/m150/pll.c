/*
 * JZ4775 pll configuration
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_FPGA
#ifndef CONFIG_SYS_CPCCR_SEL
/**
 * default CPCCR configure.
 * It is suggested if you are NOT sure how it works.
 */
#define SEL_SCLKA		1
#define SEL_CPU			1
#define SEL_H0			1
#define SEL_H2			1
#if (CONFIG_SYS_APLL_FREQ > 1000000000)
#define DIV_PCLK		10
#define DIV_H2			5
#else
#define DIV_PCLK		8
#define DIV_H2			4
#endif
#ifdef CONFIG_SYS_MEM_DIV
#define DIV_H0			4
#else
#define DIV_H0			gd->arch.gi->ddr_div
#endif
#define DIV_L2			2
#define DIV_CPU			1
#define CPCCR_CFG		(((SEL_SCLKA & 0x3) << 30)		\
				 | ((SEL_CPU & 0x3) << 28)		\
				 | ((SEL_H0 & 0x3) << 26)			\
				 | ((SEL_H2 & 0x3) << 24)			\
				 | (((DIV_PCLK - 1) & 0xf) << 16)	\
				 | (((DIV_H2 - 1) & 0xf) << 12)		\
				 | (((DIV_H0 - 1) & 0xf) << 8)		\
				 | (((DIV_L2 - 1) & 0xf) << 4)		\
				 | (((DIV_CPU - 1) & 0xf) << 0))
#else
/**
 * Board CPCCR configure.
 * CONFIG_SYS_CPCCR_SEL should be define in [board].h
 */
#define CPCCR_CFG CONFIG_SYS_CPCCR_SEL
#endif

static unsigned int get_pllreg_value(int freq)
{
	unsigned int mnod = 0;
	int nf = 1, nr = 0, no = 1;
	int bs = 0;
	unsigned int nrok = 0;
	int fvco = 0;
	int fin = gd->arch.gi->extal/1000000;
	int fout = freq/1000000;
#ifndef CONFIG_FPGA
	if ((fin < 10) || (fin > 50) || (fout < 36))
		goto err;
#endif
	do {
		nrok++;
		nf = (fout * nrok)/fin;
		if ((nf > 128)) goto err;

		if (fin * nf != fout * nrok)
			continue;

		if (nrok <= 64) {
			no = 0;
			nr = nrok;
			fvco = fout * 1;
		} else if (nrok <= 128 && nf%2 == 0) {
			if (nf%2) goto err;
			no = 1;
			nr = nrok/2;
			fvco = fout * 2;
		} else if (nrok <= 256 && nf%4 == 0) {
			if (nf%4) goto err;
			no = 2;
			nr = nrok/4;
			fvco = fout * 4;
		} else if (nrok <= 512 && nf%8 == 0) {
			no = 3;
			nr = nrok/8;
			fvco = fout * 8;
		} else {
			goto err;
		}
#ifndef CONFIG_FPGA
		debug("fvco = %d\n", fvco);
		if (fout >= 63 && fvco >= 500) {
			bs = 1;
			break;
		} else if (fout >= 36 && fvco >= 300 && fvco <= 600) {
			bs = 0;
			break;
		}
#else
		bs = 0;
		break;
#endif
	} while (1);

	mnod = (bs << 31) | ((nf - 1) << 24) | ((nr - 1) << 18) | (no << 16);

	return mnod;
err:
	printf("no adjust parameter to the fout:%dM fin: %d\n", fout, fin);
	return -EINVAL;
}

static void pll_set(int pll,int freq)
{
        unsigned int regvalue = get_pllreg_value(freq);

        if (regvalue == -EINVAL)
                return;
        switch (pll) {
                case APLL:
                        /* Init APLL */
                        cpm_outl(regvalue | (1 << 8) | 0x20, CPM_CPAPCR);
                        while(!(cpm_inl(CPM_CPAPCR) & (1 << 10)))
                                ;
                        debug("CPM_CPAPCR %x\n", cpm_inl(CPM_CPAPCR));
                        break;
                case MPLL:
                        /* Init MPLL */
                        cpm_outl(regvalue | (1 << 7), CPM_CPMPCR);
                        while(!(cpm_inl(CPM_CPMPCR) & 1))
                                ;
                        debug("CPM_CPMPCR %x\n", cpm_inl(CPM_CPMPCR));
                        break;
        }
}


void pll_init(void)
{
	unsigned int cpccr = 0;

	debug("pll init...");
#ifdef CONFIG_BURNER
	cpccr = (0x95 << 24) | (7 << 20);
	cpm_outl(cpccr,CPM_CPCCR);
	while(cpm_inl(CPM_CPCSR) & 0x7);
#endif
	/* apll and mpll both init here */
        pll_set(APLL, CONFIG_SYS_APLL_FREQ);
        pll_set(MPLL, CONFIG_SYS_MPLL_FREQ);

	cpccr = (cpm_inl(CPM_CPCCR) & (0xff << 24))
		| (CPCCR_CFG & ~(0xff << 24))
		| (7 << 20);
	cpm_outl(cpccr,CPM_CPCCR);
	while(cpm_inl(CPM_CPCSR) & 0x7);

	cpccr = (CPCCR_CFG & (0xff << 24)) | (cpm_inl(CPM_CPCCR) & ~(0xff << 24));
	cpm_outl(cpccr,CPM_CPCCR);
	while(cpm_inl(CPM_CPCSR) & 0x7);

	debug("ok\n");
}
#else

void pll_init(void) {}
#endif
