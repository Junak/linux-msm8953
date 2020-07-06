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

#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/io.h>
#include <linux/clk.h>

#include "clk-alpha-pll.h"
#include "clk-regmap-mux-div.h"

static struct clk_ops hfpll_ops;
static struct clk_ops clk_apcs_mux_div_ops;

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

static struct clk_alpha_pll apcs_c0_hfpll = {
	.offset = 0x105000,
	.regs = apcs_pll_regs,
	.clkr.hw.init = CLK_HW_INIT("apcs-hfpll-c0", "xo", &hfpll_ops, 0)
};

static struct clk_alpha_pll apcs_c1_hfpll = {
	.offset = 0x005000,
	.regs = apcs_pll_regs,
	.clkr.hw.init = CLK_HW_INIT("apcs-hfpll-c1", "xo", &hfpll_ops, 0)
};

static struct clk_alpha_pll apcs_cci_hfpll = {
	.offset = 0x1bf000,
	.regs = apcs_pll_regs,
	.clkr.hw.init = CLK_HW_INIT("apcs-hfpll-cci", "xo", &hfpll_ops, 0)
};

static const struct clk_parent_data c1_parent_data[] = {
	{ .fw_name = "gpll", .name = "gpll0_early" },
	{ .fw_name = "pll1", .name = "apcs-hfpll-c1" },
};

static const struct clk_parent_data cci_parent_data[] = {
	{ .fw_name = "gpll", .name = "gpll0_early" },
	// { .fw_name = "ccipll", .name = "apcs-hfpll-cci" },
	// FIXME Dyna PLL is not supported
};

static const struct clk_parent_data c0_c1_cci_parent_data[] = {
	{ .fw_name = "gpll", .name = "gpll0_early" },
	{ .fw_name = "pll0", .name = "apcs-hfpll-c0" },
};

static const u32 apcs_mux_parent_map[] = { 4, 5 };
static const u32 apcs_mux_keep_rate_map[] = { 1, 0 };
static struct clk_regmap_mux_div apcs_c0_clk = {
	.reg_offset = 0x100050,
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.keep_rate_map = apcs_mux_keep_rate_map,
	.parent_map = apcs_mux_parent_map,
	.clkr = {
		.enable_reg = 0x100058,
		.enable_mask = BIT(0),
		.hw.init = CLK_HW_INIT_PARENTS_DATA("apcs-c0-clk", c0_c1_cci_parent_data,
			&clk_apcs_mux_div_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT)
	},
};

static struct clk_regmap_mux_div apcs_c1_clk = {
	.reg_offset = 0x000050,
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.keep_rate_map = apcs_mux_keep_rate_map,
	.parent_map = apcs_mux_parent_map,
	.clkr = {
		.enable_reg = 0x000058,
		.enable_mask = BIT(0),
		.hw.init = CLK_HW_INIT_PARENTS_DATA("apcs-c1-clk", c0_c1_cci_parent_data,
			&clk_apcs_mux_div_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT)
	},
};

static struct clk_regmap_mux_div apcs_cci_clk = {
	.reg_offset = 0x1c0050,
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.parent_map = apcs_mux_parent_map,
	.clkr = {
		.enable_reg = 0x1c0058,
		.enable_mask = BIT(0),
		.hw.init = CLK_HW_INIT_PARENTS_DATA("apcs-cci-clk", c0_c1_cci_parent_data,
			&clk_apcs_mux_div_ops, CLK_IGNORE_UNUSED | CLK_IS_CRITICAL)
	},
};

static struct alpha_pll_config msm8953_pll_config = {
	.config_ctl_val		= 0x200d4828,
	.config_ctl_hi_val	= 0x6,
	.test_ctl_val		= 0x1c000000,
	.test_ctl_hi_val	= 0x4000,
	.main_output_mask	= BIT(0),
	.early_output_mask	= BIT(3),
	.pre_div_val		= 0,
	.pre_div_mask		= BIT(12),
	.post_div_val		= BIT(8),
	.post_div_mask		= GENMASK(9, 8),
};

static struct alpha_pll_config sdm632_pll_config = {
	.config_ctl_val		= 0x200d4828,
	.config_ctl_hi_val	= 0x6,
	.test_ctl_val		= 0x1c000000,
	.test_ctl_hi_val	= 0x4000,
	.main_output_mask	= BIT(0),
	.early_output_mask	= BIT(3),
};

static struct alpha_pll_config sdm632_cci_pll_config = {
	.config_ctl_val		= 0x4001055b,
	.early_output_mask	= BIT(3),
};

static int cci_mux_notifier(struct notifier_block *nb,
				unsigned long event,
				void *data)
{
	long long c0 = clk_hw_get_rate(&apcs_c0_clk.clkr.hw);
	long long c1 = clk_hw_get_rate(&apcs_c1_clk.clkr.hw);
	long long cci_rate;

	if (event == PRE_RATE_CHANGE) {
		cci_rate = min(c0, c1);
	} else if (event == POST_RATE_CHANGE) {
		cci_rate = max(c0, c1);
	} else return 0;

	cci_rate = cci_rate * 2 / 5;

	if (cci_rate < 320000000)
		cci_rate = 320000000;

	clk_set_rate(apcs_cci_clk.clkr.hw.clk, cci_rate);

	return 0;
}

static long hfpll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *prate)
{
	return rounddown(rate, *prate);
}

static int msm8953_apcs_mux_determine_rate(struct clk_hw *hw,
				  struct clk_rate_request *req)
{
	struct clk_hw *other = (hw == &apcs_c0_clk.clkr.hw) ?
		               &apcs_c1_clk.clkr.hw : &apcs_c0_clk.clkr.hw;

	req->best_parent_hw = NULL;
	req->best_parent_rate = max(clk_hw_get_rate(other), req->rate);
	return clk_regmap_mux_div_ops.determine_rate(hw, req);
}

static int sdm632_apcs_mux_determine_rate(struct clk_hw *hw,
				  struct clk_rate_request *req)
{
	req->best_parent_hw = NULL;
	req->best_parent_rate = req->rate;
	return clk_regmap_mux_div_ops.determine_rate(hw, req);
}

static int apcs_msm8953_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct clk_hw_onecell_data *clk_data;
	struct clk_regmap **rclk = (struct clk_regmap *[]) {
		&apcs_c0_hfpll.clkr,
		&apcs_c0_clk.clkr,
		&apcs_c1_clk.clkr,
		&apcs_cci_clk.clkr,
		NULL,
	};
	int ret;

	clk_data = (struct clk_hw_onecell_data*) devm_kzalloc(dev,
			struct_size (clk_data, hws, 2), GFP_KERNEL);

	if (IS_ERR(clk_data))
		return -ENOMEM;

	clk_data->num = 2;
	clk_data->hws[0] = &apcs_c0_clk.clkr.hw;
	clk_data->hws[1] = &apcs_c1_clk.clkr.hw;

	regmap = dev_get_regmap(dev->parent, NULL);
	if (IS_ERR(regmap)) {
		dev_err(dev, "failed to get regmap: %ld\n", PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	hfpll_ops = clk_alpha_pll_huayra_ops;
	hfpll_ops.round_rate = hfpll_round_rate;
	clk_apcs_mux_div_ops = clk_regmap_mux_div_ops;
	clk_apcs_mux_div_ops.determine_rate = msm8953_apcs_mux_determine_rate;
	clk_alpha_pll_configure(&apcs_c0_hfpll, regmap, &msm8953_pll_config);

	for (ret = 0; *rclk && !ret; rclk++)
		ret = devm_clk_register_regmap(dev, *rclk);

	if (ret) {
		dev_err(dev, "failed to register regmap clock: %d\n", ret);
		return ret;
	}

	apcs_cci_clk.clk_nb.notifier_call = cci_mux_notifier;
	clk_notifier_register(apcs_c1_clk.clkr.hw.clk, &apcs_cci_clk.clk_nb);
	clk_notifier_register(apcs_c0_clk.clkr.hw.clk, &apcs_cci_clk.clk_nb);

	clk_set_rate(apcs_c0_hfpll.clkr.hw.clk, 883200000);
	clk_prepare_enable(apcs_c0_hfpll.clkr.hw.clk);

	ret = devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get, clk_data);
	if (ret)
		dev_err(dev, "failed to add clock provider: %d\n", ret);

	return ret;
}

static int apcs_sdm632_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct clk_hw_onecell_data *clk_data;
	struct clk_regmap **rclk = (struct clk_regmap *[]) {
		&apcs_c0_hfpll.clkr,
		&apcs_c1_hfpll.clkr,
		// &apcs_cci_hfpll.clkr,
		&apcs_c0_clk.clkr,
		&apcs_c1_clk.clkr,
		&apcs_cci_clk.clkr,
		NULL,
	};
	int ret;

	clk_data = (struct clk_hw_onecell_data*) devm_kzalloc(dev,
			struct_size (clk_data, hws, 2), GFP_KERNEL);

	if (IS_ERR(clk_data))
		return -ENOMEM;

	clk_data->num = 2;
	clk_data->hws[0] = &apcs_c0_clk.clkr.hw;
	clk_data->hws[1] = &apcs_c1_clk.clkr.hw;

	apcs_c1_clk.clkr.hw.init = CLK_HW_INIT_PARENTS_DATA("apcs-c1-clk", c1_parent_data,
			&clk_apcs_mux_div_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT);
	apcs_cci_clk.clkr.hw.init = CLK_HW_INIT_PARENTS_DATA("apcs-cci-clk", cci_parent_data,
			&clk_apcs_mux_div_ops, CLK_IGNORE_UNUSED | CLK_IS_CRITICAL);

	regmap = dev_get_regmap(dev->parent, NULL);
	if (IS_ERR(regmap)) {
		dev_err(dev, "failed to get regmap: %ld\n", PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	hfpll_ops = clk_alpha_pll_huayra_ops;
	hfpll_ops.round_rate = hfpll_round_rate;
	clk_apcs_mux_div_ops = clk_regmap_mux_div_ops;
	clk_apcs_mux_div_ops.determine_rate = sdm632_apcs_mux_determine_rate;
	clk_alpha_pll_configure(&apcs_c0_hfpll, regmap, &sdm632_pll_config);
	clk_alpha_pll_configure(&apcs_c1_hfpll, regmap, &sdm632_pll_config);
	//clk_alpha_pll_configure(&apcs_cci_hfpll, regmap, &sdm632_cci_pll_config); FIXME

	for (ret = 0; *rclk && !ret; rclk++)
		ret = devm_clk_register_regmap(dev, *rclk);

	if (ret) {
		dev_err(dev, "failed to register regmap clock: %d\n", ret);
		return ret;
	}

	apcs_cci_clk.clk_nb.notifier_call = cci_mux_notifier;
	clk_notifier_register(apcs_c1_clk.clkr.hw.clk, &apcs_cci_clk.clk_nb);
	clk_notifier_register(apcs_c0_clk.clkr.hw.clk, &apcs_cci_clk.clk_nb);

	clk_set_rate(apcs_c0_hfpll.clkr.hw.clk, 883200000);
	clk_prepare_enable(apcs_c0_hfpll.clkr.hw.clk);

	clk_set_rate(apcs_c1_hfpll.clkr.hw.clk, 883200000);
	clk_prepare_enable(apcs_c1_hfpll.clkr.hw.clk);

	ret = devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get, clk_data);
	if (ret)
		dev_err(dev, "failed to add clock provider: %d\n", ret);

	return ret;
}

static struct platform_driver qcom_apcs_msm8953_clk_driver = {
	.probe = apcs_msm8953_probe,
	.driver = {
		.name = "qcom-apcs-msm8953-clk",
		.owner = THIS_MODULE,
	},
};

static struct platform_driver qcom_apcs_sdm632_clk_driver = {
	.probe = apcs_sdm632_probe,
	.driver = {
		.name = "qcom-apcs-sdm632-clk",
		.owner = THIS_MODULE,
	},
};

static int __init qcom_apcs_init(void)
{
	int ret;

	ret = platform_driver_register(&qcom_apcs_msm8953_clk_driver);
	if (ret)
		return ret;

	ret = platform_driver_register(&qcom_apcs_sdm632_clk_driver);

	return ret;
}
arch_initcall(qcom_apcs_init);

static void __init early_muxdiv_configure(unsigned int base_addr, u8 src, u8 div)
{
	int timeout;
	void __iomem *base = ioremap(base_addr, SZ_8);;

	writel_relaxed(((src & 7) << 8) | (div & 0x1f), base + 4);
	mb();

	/* Set update bit */
	writel_relaxed(readl_relaxed(base) | BIT(0), base);
	mb();

	for (timeout = 500;
	     timeout && readl_relaxed(base) & BIT(0);
	     timeout--)
		udelay(1);

	/* Enable the branch */
	writel_relaxed(readl_relaxed(base + 8) | BIT(0), base + 8);
	mb();

	iounmap(base);
}

static int __init cpu_clock_pwr_init(void)
{
	struct device_node *ofnode =
		of_find_compatible_node(NULL, NULL, "qcom,msm8953-apcs-kpss-global") ?:
		of_find_compatible_node(NULL, NULL, "qcom,sdm632-apcs-kpss-global");

	if (!ofnode)
		return 0;

	/* Initialize the PLLs */
	early_muxdiv_configure(0xb111050, 4, 1); // 800Mhz
	early_muxdiv_configure(0xb011050, 4, 1); // 800Mhz
	early_muxdiv_configure(0xb1d1050, 4, 4); // 800/2.5Mhz
	return 0;
}

early_initcall(cpu_clock_pwr_init);
