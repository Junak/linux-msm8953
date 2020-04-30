/*
 * Copyright (c) 2015-2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "apcs-cpu-msm8953: " fmt

#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/io.h>

#include "clk-alpha-pll.h"
#include "clk-rcg.h"
#include "clk-regmap-mux-div.h"
#include "common.h"
#include <linux/clk.h>

#define APCS_CMD_RCGR		0xb011050
#define APCS_CFG_OFF		0x4
#define APCS_CORE_CBCR_OFF	0x8

enum {
	CLK_C0,
	CLK_C1,
	//CLK_CCI,
	CLK_HFPLL,
	CLK_MAX,

	CLK_GPLL0,
	CLK_XO,
};

static const u8 apcs_pll_regs[PLL_OFF_MAX_REGS] = {
	[PLL_OFF_L_VAL]		= 0x08,
	[PLL_OFF_ALPHA_VAL]	= 0x10,
	[PLL_OFF_USER_CTL]	= 0x18,
	[PLL_OFF_CONFIG_CTL]	= 0x20,
	[PLL_OFF_CONFIG_CTL_U]	= 0x24,
	[PLL_OFF_STATUS]	= 0x28,
	[PLL_OFF_TEST_CTL]	= 0x30,
	[PLL_OFF_TEST_CTL_U]	= 0x34,
};

static const u32 apcs_mux_parent_map[] = { 4, 5 };

static const char * const c0_c1_cci_parent_names[] = {
	"gpll0_early", "apcs-hfpll",
};


static struct clk_alpha_pll apcs_hfpll = {
	.offset = 0x105000,
	.regs = apcs_pll_regs,
	.clkr.hw.init = CLK_HW_INIT("apcs-hfpll", "xo",
			&clk_alpha_pll_ops, 0)
};


static struct clk_hw *gpll0_hw;
struct clk_ops clk_cust_ops;

static struct apcs_cluster_mux {
	struct clk_regmap_mux_div muxdiv;
	unsigned long freq;
} apcs_c0_clk = {
	.muxdiv = {
		.reg_offset = 0x100050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.src = 4,
		.div = 1,
		.parent_map = apcs_mux_parent_map,
		.clkr = {
			.enable_reg = 0x000058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS("apcs-c0-clk", c0_c1_cci_parent_names,
				&clk_cust_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT)
		},
	}
}, apcs_c1_clk = {
	.muxdiv = {
		.reg_offset = 0x000050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.src = 4,
		.div = 1,
		.parent_map = apcs_mux_parent_map,
		.clkr = {
			.enable_reg = 0x000058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS("apcs-c1-clk", c0_c1_cci_parent_names,
				&clk_cust_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT)
		},
	},
};

#if 0
static const struct freq_tbl cci_freq_tbl [] = {
	F(19200000, CLK_XO, 1, 0, 0),
	{}
};

static struct clk_regmap_mux_div apcs_cci_clk = {
	.cmd_rcgr = 0x1c0050,
	.hid_width = 5,
	.parent_map = c0_c1_cci_parent_map,
	.clkr = {
		.enable_reg = 0x1c0058,
		.enable_mask = BIT(0),
		.hw.init = CLK_HW_INIT_PARENTS("apcs-cci-clk", c0_c1_cci_parent_names,
			&clk_rcg2_ops, CLK_IS_CRITICAL )
	}
};
#endif


static bool frac2_div_strict(u32 value, u32 divisor, u32 tr, u32 *result) {
	u64 value2 = 2 * value;
	if (value2 % divisor > tr)
		return false;
	if (value2 / divisor < 2)
		return false;
	*result = value2 / divisor;
	return true;
}

static int mux_div_determine_rate(struct clk_hw *hw,
				  struct clk_rate_request *req)
{
	struct clk_regmap_mux_div *md = container_of(to_clk_regmap(hw),
					struct clk_regmap_mux_div, clkr);
	struct apcs_cluster_mux* mux = container_of(md, struct apcs_cluster_mux, muxdiv);
	struct apcs_cluster_mux* other = mux == &apcs_c0_clk ? &apcs_c1_clk : &apcs_c0_clk;
	unsigned long rate = clk_hw_get_rate(gpll0_hw);
	unsigned int div;

	if (frac2_div_strict(rate, req->rate, 5000, &div)) {
		req->best_parent_hw = gpll0_hw;
		req->best_parent_rate = req->rate / 2 * div;
		req->rate = 2 * ((u64) rate) / div;
		return 0;
	}

	rate = max(other->freq, req->rate);
	if (!frac2_div_strict(rate, req->rate, 0, &div))
		return -EINVAL;


	req->best_parent_hw = &apcs_hfpll.clkr.hw;
	req->best_parent_rate = rate;
	req->rate = ((u64) rate) * 2 / div;

	return 0;
}

static int mux_div_set_src_div_cache(struct clk_regmap_mux_div *md,
					u32 src, u32 div)
{
	int ret = mux_div_set_src_div(md, src, div);

	if (ret == 0) {
		md->src = src;
		md->div = div;
	}

	return ret;
}

static void mux_div_get_src_div_fixed(struct clk_regmap_mux_div *md,
					u32 *src, u32 *div)
{
	mux_div_get_src_div(md, src, div);
	if (*div == 0)
		*div = 1;
}

static unsigned long mux_div_recalc_rate(struct clk_hw *hw, unsigned long prate)
{
	struct clk_regmap_mux_div *md = container_of(to_clk_regmap(hw),
					struct clk_regmap_mux_div, clkr);

	return mult_frac(prate, 2, md->div + 1);
}

static int mux_div_set_rate(struct clk_hw *hw,
			    unsigned long rate, unsigned long prate)
{
	u32 src, div;
	struct clk_regmap_mux_div *md = container_of(to_clk_regmap(hw),
					struct clk_regmap_mux_div, clkr);
	struct apcs_cluster_mux *mux = container_of(md, struct apcs_cluster_mux, muxdiv);

	mux_div_get_src_div_fixed(md, &src, &div);

	if (!frac2_div_strict(prate, rate, 5000 ? src == 4 : 0, &div) ||
				div < 2 || div > 32)
			return -EINVAL;

	mux->freq = rate;
	return mux_div_set_src_div_cache(md, src, div - 1);
}

static int mux_div_set_rate_and_parent(struct clk_hw *hw,
				       unsigned long rate, unsigned long prate, u8 index)
{
	struct clk_regmap_mux_div *md = container_of(to_clk_regmap(hw),
					struct clk_regmap_mux_div, clkr);
	struct apcs_cluster_mux* mux = container_of(md, struct apcs_cluster_mux, muxdiv);
	u32 src = md->parent_map[index], div;

	if (!frac2_div_strict(prate, rate, 5000 ? src == 4 : 0, &div) ||
				div < 2 || div > 32)
			return -EINVAL;

	mux->freq = rate;
	return mux_div_set_src_div_cache(md, src, div - 1);
}

static int cluster_mux_notifier(struct notifier_block *nb,
				unsigned long event,
				void *data)
{
	struct clk_regmap_mux_div *md = container_of(nb, struct clk_regmap_mux_div, clk_nb);
	//struct apcs_cluster_mux* mux = container_of(md, struct apcs_cluster_mux, muxdiv);
	u32 src, div;

	mux_div_get_src_div_fixed(md, &src, &div);

	if (event == PRE_RATE_CHANGE) {
		if (src == 5)
			mux_div_set_src_div(md, 4, 1);
	} else if (event == POST_RATE_CHANGE) {
		mux_div_get_src_div_fixed(md, &src, &div);
		if (src != md->src || div != md->div)
			mux_div_set_src_div(md, md->src, md->div);
	}
	mux_div_get_src_div_fixed(md, &src, &div);
	return 0;
}

static int apcs_msm8953_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk *gpll0;
	struct regmap *regmap;
	struct clk_hw_onecell_data *clk_data;
	struct clk_regmap *regmap_clks [] = {
		&apcs_hfpll.clkr, &apcs_c0_clk.muxdiv.clkr,
		&apcs_c1_clk.muxdiv.clkr, //&apcs_cci_clk.clkr,
		(struct clk_regmap*) NULL,
	};
	struct clk_regmap **rclk = &regmap_clks[0];
	struct regmap_config config = {
		.reg_bits	= 32,
		.reg_stride	= 4,
		.val_bits	= 32,
		.fast_io	= true,
	};
	struct alpha_pll_config pll_config = {
		.config_ctl_val		= 0x200d4828,
		.config_ctl_hi_val	= 0x6,
		.test_ctl_val		= 0x1c000000,
		.test_ctl_hi_val	= 0x4000,
		.main_output_mask	= BIT(0),
		.early_output_mask	= BIT(3),
		.pre_div_mask		= BIT(12),
		.post_div_val		= BIT(8),
		.post_div_mask		= GENMASK(9, 8),
	};
	void __iomem *base;
	int ret;
	struct resource *res;

	gpll0 = devm_clk_get(dev, "gpll0");
	if (IS_ERR_OR_NULL(gpll0))
		return -EPROBE_DEFER;

	gpll0_hw = __clk_get_hw(gpll0);
	if (IS_ERR_OR_NULL(gpll0_hw))
		return -EINVAL;

	clk_cust_ops.get_parent = clk_regmap_mux_div_ops.get_parent;
	clk_cust_ops.set_parent = clk_regmap_mux_div_ops.set_parent;
	clk_cust_ops.set_rate = mux_div_set_rate;
	clk_cust_ops.set_rate_and_parent = mux_div_set_rate_and_parent;
	clk_cust_ops.determine_rate = mux_div_determine_rate;
	clk_cust_ops.recalc_rate = mux_div_recalc_rate;
	clk_data = (struct clk_hw_onecell_data*) devm_kzalloc(dev,
			struct_size (clk_data, hws, CLK_MAX), GFP_KERNEL);
	if (IS_ERR(clk_data))
		return -ENOMEM;

	clk_data->num = CLK_MAX;
	clk_data->hws[CLK_C0]	= &apcs_c0_clk.muxdiv.clkr.hw;
	clk_data->hws[CLK_C1]	= &apcs_c1_clk.muxdiv.clkr.hw;
	//clk_data->hws[CLK_CCI]	= &apcs_cci_clk.clkr.hw;
	clk_data->hws[CLK_HFPLL]= &apcs_hfpll.clkr.hw;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;

	base = devm_ioremap(dev, res->start, resource_size(res));
	if (!base)
		return -ENOMEM;

	config.max_register = resource_size(res) - 4;

	regmap = devm_regmap_init_mmio(dev, base, &config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	if (dev_get_regmap(dev, NULL) != regmap) {
		dev_err(dev, "failed to get regmap: %d\n", ret);
		return ret;
	}


	for (ret = 0; *rclk && !ret; rclk++) {
		ret = devm_clk_register_regmap(dev, *rclk);
	}

	apcs_c0_clk.muxdiv.clk_nb.notifier_call = cluster_mux_notifier;
	clk_notifier_register(apcs_hfpll.clkr.hw.clk, &apcs_c0_clk.muxdiv.clk_nb);

	apcs_c1_clk.muxdiv.clk_nb.notifier_call = cluster_mux_notifier;
	clk_notifier_register(apcs_hfpll.clkr.hw.clk, &apcs_c1_clk.muxdiv.clk_nb);

	if (ret) {
		dev_err(dev, "failed to register regmap clock: %d\n", ret);
		return ret;
	}

	clk_alpha_pll_configure(&apcs_hfpll, regmap, &pll_config);

	ret = clk_set_rate(apcs_hfpll.clkr.hw.clk, 614400000);
	if (ret) {
		dev_err(dev, "failed to set pll rate: %d\n", ret);
		return ret;
	}
	ret = clk_prepare_enable(apcs_hfpll.clkr.hw.clk);
	if (ret) {
		dev_err(dev, "failed to enable pll: %d\n", ret);
		return ret;
	}

	ret = devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get, clk_data);
	if (ret) {
		dev_err(dev, "failed to add clock provider: %d\n", ret);
	}

	return ret;
}

static const struct of_device_id apcs_msm8953_match_table[] = {
	{ .compatible = "qcom,apcs-msm8953" },
	{},
};

static struct platform_driver apcs_msm8953_driver = {
	.probe = apcs_msm8953_probe,
	.driver = {
		.name = "qcom-apcs-msm8953-clk",
		.of_match_table = apcs_msm8953_match_table,
		.owner = THIS_MODULE,
	},
};

static int __init msm_apcs_init(void)
{
	return platform_driver_register(&apcs_msm8953_driver);
}

static int __init cpu_clock_pwr_init(void)
{
	void __iomem  *base;
	int regval = 0;
	struct device_node *ofnode = of_find_compatible_node(NULL, NULL,
						"qcom,apcs-msm8953");
	if (!ofnode)
		return 0;

	/* Initialize the PLLs */
	base = ioremap(APCS_CMD_RCGR, SZ_8);
	regval = readl_relaxed(base);

	/* Source GPLL0 and at the rate of GPLL0 */
	regval = 0x401; /* source - 4, div - 1  */
	writel_relaxed(regval, base + APCS_CFG_OFF);
	/* Make sure src sel and src div is set before update bit */
	mb();

	/* update bit */
	regval = readl_relaxed(base);
	regval |= BIT(0);
	writel_relaxed(regval, base);
	/* Make sure src sel and src div is set before update bit */
	mb();

	/* Enable the branch */
	regval =  readl_relaxed(base + APCS_CORE_CBCR_OFF);
	regval |= BIT(0);
	writel_relaxed(regval, base + APCS_CORE_CBCR_OFF);
	/* Branch enable should be complete */
	mb();
	iounmap(base);

	pr_debug("2nd cluster clocks configured\n");

	return 0;
}

arch_initcall(msm_apcs_init);
early_initcall(cpu_clock_pwr_init);
