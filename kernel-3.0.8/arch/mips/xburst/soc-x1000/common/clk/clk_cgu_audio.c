#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/ddr.h>
#include "clk.h"

#define EXT_CLK 	24000000
#define LEN 		13

static unsigned int support_sample_rate[LEN] = {8000, 11025, 12000, 16000, 22050, 24000, 32000,
					44100, 48000,88200, 96000, 176400, 192000};
static unsigned int audio_div_apll[LEN * 3] = {0};
static unsigned int audio_div_mpll[LEN * 3] = {0};
bool first_time_calculate_mpll = true;
bool first_time_calculate_apll = true;
static unsigned int div_m = 0;
static unsigned int div_n = 0;

static DEFINE_SPINLOCK(cpm_cgu_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_AUDIO = 0,
};
const struct clk_selectors audio_selector[] = {
#define CLK(X)  CLK_ID_##X
/*
 *         bit31,bit30
 *          0   , 0       EXT1
 *          0   , 1       APLL
 *          1   , 0       EXT1
 *          1   , 1       MPLL
 */
	[SELECTOR_AUDIO].route = {CLK(EXT1),CLK(APLL),CLK(EXT1),CLK(MPLL)},
#undef CLK
};
struct cgu_clk {
	int off,en,maskm,bitm,maskn,bitn,maskd,bitd,sel,cache;
};
static struct cgu_clk cgu_clks[] = {
	[CGU_AUDIO_I2S] = 	{ CPM_I2SCDR, 1<<29, 0x1ff << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_I2S1] = 	{ CPM_I2SCDR1, -1, -1, -1, -1, -1, -1},
	[CGU_AUDIO_PCM] = 	{ CPM_PCMCDR, 1<<29, 0x1ff << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_PCM1] = 	{ CPM_PCMCDR1, -1, -1, -1, -1, -1, -1},
};

/***************************************************************\
 *  Use codec slave mode clock rate list
 *  We do not hope change APLL,so we use 1008M hz (fix) apllclk
 *  for minimum error
 *  1008M ---	PLLM:41  PLLN:0  PLOD:0
 *	 rate		I2SCDR_M	I2SCDR_N	rate_err(%)
 *	|192000		| 128		| 2625		| 0
 *	|176400		| 28		| 625		| 0
 *	|96000		| 64		| 2625		| 0
 *	|88200		| 14		| 625		| 0
 *	|48000		| 32		| 2625		| 0
 *	|44100		| 7		| 625		| 0
 *	|32000		| 64		| 7875		| 0
 *	|24000		| 16		| 2625		| 0
 *	|22050		| 7		| 1250		| 0
 *	|16000		| 32		| 7875		| 0
 *	|12000		| 8		| 2625		| 0
 *	|11025		| 7		| 2500		| 0
 *	|8000		| 16		| 7875		| 0
\***************************************************************/

/*this func is to verify the M and N whether meet the requirement:
 *	De_p =(ceil(N/M) - N/M)/(N/M) < 5%;
 *	De_n = (floor(N/M) - N/M)/(N/M) > -5%
 *NOTE:For rate=176400 and 192000, if we use MPLL = 600Mhz, can not meet
 	the limitation of De_p and De_n;
	so to support all kind rates,so we must use APLL = 1008Mhz
 */
#define MUL	100000
static int cal_err(int m ,int n, unsigned long rate)
{
	unsigned int tmp;
	int err_min, err_max, div, div_min, div_max;
	int DE_P, DE_N;

	div = n / m;
	div_min = div;
	div_max = div + 1;

	DE_P = 50;
	DE_N = 50;

	tmp = n * MUL / m;
	err_max = (div_max * MUL - tmp) * 1000 / tmp;
	err_min = (tmp - div_min * MUL) * 1000 / tmp;

	if((err_max < DE_P) && (err_min < DE_N))
		return 0;
	else
		return -1;
}

static unsigned long calculate_cpm_pll(int val)
{
	unsigned int PLLM, PLLN, PLOD;
	unsigned int NF, NR, NO;
	unsigned long pll;
	PLLM = val & (0x7f << 24);
	PLLN = val & (0x1f << 18);
	PLOD = val & (0x3 << 16);

	NF = (PLLM >> 24) + 1;
	NR = (PLLN >> 18) + 1;
	NO = 1 << (PLOD >> 16);
	pll = EXT_CLK * NF / NR / NO;
	return pll;
}

#define DIV	500
static int cal_div_for_sysclk(unsigned int *rate, unsigned long pll, bool flag)
{
	int i, j, tmp_err, max_err, m_max, n_max;
	unsigned long M, N;
	int ret = 0;

	m_max = 0x1ff;
	n_max = 0x1fff;
	for(i = 0; i < LEN; i++) {
		/*the maximum error between theoretical and practical rate*/
		max_err = rate[i] / 2000;
		for(j = 1; j < m_max; j++) {
			M = j;
			N = (pll / DIV * M) / ((rate[i] * 256) / DIV);
			if(N == 0)
				return -1;
			if(N > n_max)
				continue;
			tmp_err = abs((pll / DIV * M) / ((N * 256) / DIV)  - rate[i]);
			if(tmp_err < max_err) {
				max_err = tmp_err;
				ret = cal_err(M, N, rate[i]);
				if(!ret) {
					if(flag) {
						audio_div_apll[3 * i] = rate[i];
						audio_div_apll[3 * i + 1] = M;
						audio_div_apll[3 * i + 2] = N;
					} else {
						audio_div_mpll[3 * i] = rate[i];
						audio_div_mpll[3 * i + 1] = M;
						audio_div_mpll[3 * i + 2] = N;
					}
				}
			}
		}
	}
	return 0;
}
static int cal_div_for_mclk(unsigned long rate, unsigned long pll)
{
	int i, tmp_err, max_err, m_max, n_max;
	unsigned long M, N;
	int ret = 0;
	m_max = 0x1ff;
	n_max = 0x1fff;

	/*the maximum error between theoretical and practical rate*/
	max_err = rate / 2000;
	for(i = 1; i < m_max; i++) {
		M = i;
		N = (pll / DIV * M) / (rate / DIV);
		if(N ==0)
			return -1;
		if(N > n_max)
			continue;
		tmp_err = abs((pll / DIV * M)/ N * DIV - rate);
		if(tmp_err < max_err) {
			max_err = tmp_err;
			ret = cal_err(M, N, rate);
			if(!ret) {
				div_m = M;
				div_n = N;
			}
		}
	}
	return 0;
}
static int get_pcm_div_val(int max1,int max2,unsigned long rate, int* res1, int* res2)
{
	int tmp1 = 0,tmp2 = 0;
	unsigned long tmp_val;
	for (tmp1 = 1; tmp1 < max1; tmp1++) {
		tmp_val = rate * 256 / (tmp1 +1);
		if(tmp_val == 256 * 1000 || tmp_val == 128 *100) {
			for(tmp2 = 1; tmp2 < max2; tmp2++) {
				if(tmp_val / (8 * (tmp2 +1)) == rate) {
					*res1 = tmp1;
					*res2 = tmp2;
					return 0;
				}
			}
		}
	}
	if(tmp1 >= max1){
		printk("can't find match val\n");
		return -1;
	}
	return 0;
}

static unsigned long cgu_audio_get_rate(struct clk *clk)
{
	if(!clk)
		return -EINVAL;
	if(clk->parent == get_clk_from_id(CLK_ID_EXT1))
		return clk->parent->rate;

	return clk ? clk->rate : 0;
}
static int cgu_audio_enable(struct clk *clk,int on)
{
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	int reg_val;
	unsigned long flags;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	if(on){
		reg_val = cpm_inl(cgu_clks[no].off);
		if(reg_val & (cgu_clks[no].en))
			goto cgu_enable_finish;
		if(!cgu_clks[no].cache)
			printk("must set rate before enable\n");
		cpm_outl(cgu_clks[no].cache, cgu_clks[no].off);
		cpm_outl(cgu_clks[no].cache | cgu_clks[no].en, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
	}else{
		reg_val = cpm_inl(cgu_clks[no].off);
		reg_val &= ~cgu_clks[no].en;
		cpm_outl(reg_val,cgu_clks[no].off);
	}
cgu_enable_finish:
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static int cgu_audio_calculate_set_rate(struct clk* clk, unsigned long rate, unsigned int pid)
{
	int i, m, n, d, sync,tmp_val, d_max, sync_max;
	unsigned long flags, pll_val;
	unsigned int *audio_div;
	bool rate_flag = true;
	int ret = 0;
	int no = CLK_CGU_AUDIO_NO(clk->flags);

	for(i = 0; i < LEN; i++) {
		if(rate / 256 == support_sample_rate[i])
			break;
	}
	if(i < LEN)
		rate_flag = true;	/*the rate is sysclk:256*rate */
	else
		rate_flag = false;

	if(rate_flag) {
		if(pid == CLK_ID_MPLL) {
			if(first_time_calculate_mpll) {
				tmp_val = readl((volatile unsigned int*)CPM_MPLL_CTRL);
				pll_val = calculate_cpm_pll(tmp_val);
				ret = cal_div_for_sysclk(support_sample_rate, pll_val, 0);
				if(ret < 0) {
					printk("calculat div for sysclk failed\n");
					return -1;
				}
			}
			first_time_calculate_mpll = false;
			audio_div = (unsigned int*)audio_div_mpll;
		}else if(pid == CLK_ID_APLL) {
			if(first_time_calculate_apll) {
				tmp_val = readl((volatile unsigned int*)CPM_APLL_CTRL);
				pll_val = calculate_cpm_pll(tmp_val);
				ret = cal_div_for_sysclk(support_sample_rate, pll_val, 1);
				if(ret < 0) {
					printk("calculat div for sysclk failed\n");
					return -1;
				}
			}
			first_time_calculate_apll = false;
			audio_div = (unsigned int*)audio_div_apll;
		}else
			return 0;

		m = audio_div[3 * i + 1];
		n = audio_div[3 * i + 2];
		/*if use 384K, add the judgement: m != -1 || n != -1*/
	}else {
		if(pid == CLK_ID_MPLL)
			tmp_val = readl((volatile unsigned int*)CPM_MPLL_CTRL);
		else if(pid == CLK_ID_APLL)
			tmp_val = readl((volatile unsigned int*)CPM_APLL_CTRL);
		else
			return 0;
		pll_val = calculate_cpm_pll(tmp_val);
		ret = cal_div_for_mclk(rate, pll_val);
		if(ret < 0) {
			printk("calculate div for mclk failed\n");
			return -EINVAL;
		}
		m = div_m;
		n = div_n;
	}

	if((m != 0) && (n != 0)) {
		if(no == CGU_AUDIO_I2S) {
			spin_lock_irqsave(&cpm_cgu_lock,flags);
			tmp_val = cpm_inl(cgu_clks[no].off) & (~(cgu_clks[no].maskm | cgu_clks[no].maskn));
			tmp_val |= (m << cgu_clks[no].bitm) | (n << cgu_clks[no].bitn);
			if(tmp_val & cgu_clks[no].en) {
				cpm_outl(tmp_val, cgu_clks[no].off);
			} else {
				cgu_clks[no].cache = tmp_val;
			}
			spin_unlock_irqrestore(&cpm_cgu_lock, flags);
		} else if (no == CGU_AUDIO_PCM) {
			spin_lock_irqsave(&cpm_cgu_lock,flags);
			tmp_val = cpm_inl(cgu_clks[no].off)&(~(cgu_clks[no].maskm|cgu_clks[no].maskn));
			tmp_val |= (m<<cgu_clks[no].bitm)|(n<<cgu_clks[no].bitn);
			if(tmp_val&cgu_clks[no].en){
				cpm_outl(tmp_val,cgu_clks[no].off);
			}else{
				cgu_clks[no].cache = tmp_val;
			}

			d_max = 0x3f, sync_max = 0x1f;
			if(get_pcm_div_val(d_max, sync_max, rate, &d, &sync)) {
				printk("audio_pcm div calculate err!\n");
				return -EINVAL;
			}
			tmp_val = readl((volatile unsigned int*)PCM_PRI_DIV);
			tmp_val &= (~(0x3f | (0x1f << 6) | (0x3f << 11)));
			tmp_val = d | (sync << 6);
			writel(tmp_val, (volatile unsigned int*)PCM_PRI_DIV);
			spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		}
	} else {
		printk("Note: M and N can not meet the limition\n");
		return -EINVAL;
	}
	clk->rate = rate;
	return 0;
}

static struct clk* cgu_audio_get_parent(struct clk *clk)
{
	unsigned int no,cgu,idx,pidx;
	unsigned long flags;
	struct clk* pclk;

	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu = cpm_inl(cgu_clks[no].off);
	idx = cgu >> 30;
	pidx = audio_selector[cgu_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID){
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return NULL;
	}
	pclk = get_clk_from_id(pidx);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return pclk;
}

static int cgu_audio_set_parent(struct clk *clk, struct clk *parent)
{
	int tmp_val,i;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	unsigned long flags;
	for(i = 0;i < 4;i++) {
		if(audio_selector[cgu_clks[no].sel].route[i] == get_clk_id(parent))
			break;
	}
	if(i >= 4)
		return -EINVAL;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	if(get_clk_id(parent) != CLK_ID_EXT1){
		tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30));
		tmp_val |= i<<30;
		cpm_outl(tmp_val,cgu_clks[no].off);
	}else{
		tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30|0x3fffff));
		tmp_val |= i<<30|1<<13|1;
		cpm_outl(tmp_val,cgu_clks[no].off);
	}
	clk->parent = parent;
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return 0;
}

static int cgu_audio_set_rate(struct clk *clk, unsigned long rate)
{
	int tmp_val;
	unsigned long flags;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	int ret = 0;
	if(rate == EXT_CLK){
		cgu_audio_set_parent(clk,get_clk_from_id(CLK_ID_EXT1));
		clk->rate = rate;
		spin_lock_irqsave(&cpm_cgu_lock,flags);
		tmp_val = cpm_inl(cgu_clks[no].off);
		tmp_val &= ~0x3fffff;
		tmp_val |= 1<<13|1;
		if(tmp_val&cgu_clks[no].en)
			cpm_outl(tmp_val,cgu_clks[no].off);
		else
			cgu_clks[no].cache = tmp_val;
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return 0;
	}else{
		/*
		 * In X1000, we can just use APLL(1008MHZ) to generate all the I2S clk exactly.
		 * Because the I2SCDR add the decimal divider.
		 */
		if(get_clk_id(clk->parent) != CLK_ID_APLL) {
			ret = cgu_audio_set_parent(clk, get_clk_from_id(CLK_ID_APLL));
			if (ret) {
				printk("audio_set_parent error\n");
				return -EINVAL;
			}
		}
		ret = cgu_audio_calculate_set_rate(clk,rate,CLK_ID_APLL);
		if (ret < 0) {
			printk("audio_calculate_set_rate error\n");
			return -EINVAL;
		}
	}
	return 0;
}


static int cgu_audio_is_enabled(struct clk *clk) {
	int no,state;
	unsigned long flags;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	state = (cpm_inl(cgu_clks[no].off) & cgu_clks[no].en);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return state;
}

static struct clk_ops clk_cgu_audio_ops = {
	.enable	= cgu_audio_enable,
	.get_rate = cgu_audio_get_rate,
	.set_rate = cgu_audio_set_rate,
	.get_parent = cgu_audio_get_parent,
	.set_parent = cgu_audio_set_parent,
};
void __init init_cgu_audio_clk(struct clk *clk)
{
	int no,id;
	unsigned long flags, tmp_val;
	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = cgu_audio_get_parent(clk);
	}

	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu_clks[no].cache = 0;
	if(cgu_audio_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}
	clk->rate = cgu_audio_get_rate(clk);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	tmp_val = cpm_inl(cgu_clks[no].off);
	tmp_val &= ~0x3fffff;
	tmp_val |= 1<<13|1;
	if((tmp_val&cgu_clks[no].en)&&(clk->rate == EXT_CLK))
		cpm_outl(tmp_val,cgu_clks[no].off);
	else
		cgu_clks[no].cache = tmp_val;
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	clk->ops = &clk_cgu_audio_ops;
}
