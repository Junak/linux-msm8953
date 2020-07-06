// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020 The Linux Foundation. All rights reserved.

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "common.h"
#include "clk-alpha-pll.h"
#include "clk-regmap-mux-div.h"

enum {
	APCC_CLK_C0,
	APCC_CLK_C1,
	APCC_CLK_CCI,
	APCC_CLK_PLL_C0,
	APCC_CLK_PLL_C1,
	APCC_CLK_PLL_CCI,
};

static struct clk_ops msm8953_pll_ops;

struct clk_msm8953_cpu_mux_div {
	struct clk_regmap_mux_div md;
	struct notifier_block pll_nb;
	struct notifier_block md_nb;
	struct clk_hw *variable_pll;
	bool aux_pll_used;
};

struct qcom_apcc_msm8953_data {
};

static const u8 pll_regs[PLL_OFF_MAX_REGS] = {
	[PLL_OFF_L_VAL]		= 0x08,
	[PLL_OFF_ALPHA_VAL]	= 0x10,
	[PLL_OFF_USER_CTL]	= 0x18,
	[PLL_OFF_CONFIG_CTL]	= 0x20,
	[PLL_OFF_CONFIG_CTL_U]	= 0x24,
	[PLL_OFF_STATUS]	= 0x28,
	[PLL_OFF_TEST_CTL]	= 0x30,
	[PLL_OFF_TEST_CTL_U]	= 0x34,
};

static struct pll_vco msm8953_pll_vco[] = {
	VCO(0, 652800000, 2208000000),
};

static struct pll_vco sdm632_pwr_pll_vco[] = {
	VCO(0, 614400000, 2016000000),
};

static struct pll_vco sdm632_perf_pll_vco[] = {
	VCO(0, 633600000, 2016000000),
};

static struct pll_vco sdm632_cci_pll_vco[] = {
	VCO(2, 500000000, 1000000000),
};

static struct clk_alpha_pll msm8953_pll = {
	.offset = 0x105000,
	.regs = pll_regs,
	.vco_table = msm8953_pll_vco,
	.num_vco = ARRAY_SIZE(msm8953_pll_vco),
	.clkr.hw.init = CLK_HW_INIT("apcc-pll", "xo", &msm8953_pll_ops, 0)
};

static struct clk_alpha_pll sdm632_pwr_pll = {
	.offset = 0x105000,
	.regs = pll_regs,
	.vco_table = sdm632_pwr_pll_vco,
	.num_vco = ARRAY_SIZE(sdm632_pwr_pll_vco),
	.clkr.hw.init = CLK_HW_INIT("apcc-pll-pwr", "xo", &msm8953_pll_ops, 0)
};

static struct clk_alpha_pll sdm632_perf_pll = {
	.offset = 0x005000,
	.regs = pll_regs,
	.vco_table = sdm632_perf_pll_vco,
	.num_vco = ARRAY_SIZE(sdm632_perf_pll_vco),
	.clkr.hw.init = CLK_HW_INIT("apcc-pll-perf", "xo", &msm8953_pll_ops, 0)
};

static struct clk_alpha_pll sdm632_cci_pll = {
	.offset = 0x1bf000,
	.regs = pll_regs,
	.vco_table = sdm632_cci_pll_vco,
	.num_vco = ARRAY_SIZE(sdm632_cci_pll_vco),
	.clkr.hw.init = CLK_HW_INIT("apcc-pll-cci", "xo", &clk_alpha_pll_fabia_ops, 0)
};

static const struct clk_parent_data gpll2_early_div_parent_data[] = {
	{ .fw_name = "gpll2", .name = "gpll2_early" },
};

static struct clk_fixed_factor apcc_gpll2_early_div = {
	.mult = 1,
	.div = 2,
	.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-gpll2-div2", gpll2_early_div_parent_data,
			&clk_fixed_factor_ops, 0)
};

static struct clk_fixed_factor msm8953_pll_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = CLK_HW_INIT_HW("apcc-pll-div", &msm8953_pll.clkr.hw,
			&clk_fixed_factor_ops, 0)
};

static struct clk_fixed_factor sdm632_pwr_pll_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = CLK_HW_INIT_HW("apcc-pll-pwr-div2", &sdm632_pwr_pll.clkr.hw,
			&clk_fixed_factor_ops, CLK_SET_RATE_PARENT)
};

static struct clk_fixed_factor sdm632_perf_pll_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = CLK_HW_INIT_HW("apcc-pll-perf-div2", &sdm632_perf_pll.clkr.hw,
			&clk_fixed_factor_ops, CLK_SET_RATE_PARENT)
};

static struct clk_fixed_factor sdm632_cci_pll_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = CLK_HW_INIT_HW("apcc-pll-cci-div2", &sdm632_cci_pll.clkr.hw,
			&clk_fixed_factor_ops, CLK_SET_RATE_PARENT)
};

static const u32 msm8953_mux_parent_map[] = { 2, 3, 4, 5, 6 };
static const struct clk_parent_data msm8953_mux_parent_data[] = {
	{ .name = "gpll2_early" },
	{ .hw = &msm8953_pll_div2.hw },
	{ .name = "gpll0_early" },
	{ .hw = &msm8953_pll.clkr.hw },
	{ .hw = &apcc_gpll2_early_div.hw },
};

static const u32 sdm632_mux_parent_map[] = { 4, 5 };
static const struct clk_parent_data sdm632_pwr_parent_data[] = {
	{ .name = "gpll0_early" },
	{ .hw = &sdm632_pwr_pll.clkr.hw },
};

static const struct clk_parent_data sdm632_perf_parent_data[] = {
	{ .name = "gpll0_early" },
	{ .hw = &sdm632_perf_pll.clkr.hw },
};

static const struct clk_parent_data sdm632_cci_parent_data[] = {
	{ .name = "gpll0_early" },
	{ .hw = &sdm632_cci_pll.clkr.hw },
};

static struct clk_msm8953_cpu_mux_div msm8953_apcc_c0_clk = {
	.md = {
		.reg_offset = 0x100050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = msm8953_mux_parent_map,
		.clkr = {
			.enable_reg = 0x100058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-c0-clk", msm8953_mux_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED)
		}
	}
};

static struct clk_msm8953_cpu_mux_div msm8953_apcc_c1_clk = {
	.md = {
		.reg_offset = 0x000050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = msm8953_mux_parent_map,
		.clkr = {
			.enable_reg = 0x000058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-c1-clk", msm8953_mux_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED)
		}
	}
};

static struct clk_msm8953_cpu_mux_div msm8953_apcc_cci_clk = {
	.md = {
		.reg_offset = 0x1c0050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = msm8953_mux_parent_map,
		.clkr = {
			.enable_reg = 0x1c0058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-cci-clk", msm8953_mux_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED)
		}
	}
};

static struct clk_msm8953_cpu_mux_div sdm632_apcc_pwr_clk = {
	.variable_pll = &sdm632_pwr_pll.clkr.hw,
	.md = {
		.reg_offset = 0x100050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = sdm632_mux_parent_map,
		.clkr = {
			.enable_reg = 0x100058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-pwr-clk", sdm632_pwr_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT)
		}
	}
};

static struct clk_msm8953_cpu_mux_div sdm632_apcc_perf_clk = {
	.variable_pll = &sdm632_perf_pll.clkr.hw,
	.md = {
		.reg_offset = 0x000050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = sdm632_mux_parent_map,
		.clkr = {
			.enable_reg = 0x000058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-perf-clk", sdm632_perf_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT),
		}
	}
};

static struct clk_msm8953_cpu_mux_div sdm632_apcc_cci_clk = {
	.variable_pll = &sdm632_cci_pll.clkr.hw,
	.md = {
		.reg_offset = 0x1c0050,
		.hid_width = 5,
		.src_width = 3,
		.src_shift = 8,
		.parent_map = sdm632_mux_parent_map,
		.clkr = {
			.enable_reg = 0x1c0058,
			.enable_mask = BIT(0),
			.hw.init = CLK_HW_INIT_PARENTS_DATA("apcc-cci-clk", sdm632_cci_parent_data,
				&clk_regmap_mux_div_ops, CLK_IGNORE_UNUSED | CLK_IS_CRITICAL | CLK_SET_RATE_PARENT),
		}
	}
};

static const struct alpha_pll_config msm8953_pll_config = {
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

static const struct alpha_pll_config sdm632_pll_config = {
	.config_ctl_val		= 0x200d4828,
	.config_ctl_hi_val	= 0x6,
	.test_ctl_val		= 0x1c000000,
	.test_ctl_hi_val	= 0x4000,
	.main_output_mask	= BIT(0),
	.early_output_mask	= BIT(3),
};

static const struct alpha_pll_config sdm632_cci_pll_config = {
	.config_ctl_val		= 0x4001055b,
	.early_output_mask	= BIT(3),
};

static struct regmap_config apcc_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.fast_io = true,
	.max_register = 0x1c0100,
};

static struct clk_regmap *qcom_apcc_msm8953_clocks[] = {
	[APCC_CLK_C0] = &msm8953_apcc_c0_clk.md.clkr,
	[APCC_CLK_C1] = &msm8953_apcc_c1_clk.md.clkr,
	[APCC_CLK_CCI] = &msm8953_apcc_cci_clk.md.clkr,
	[APCC_CLK_PLL_C0] = &msm8953_pll.clkr,
};

static struct clk_regmap *qcom_apcc_sdm632_clocks[] = {
	[APCC_CLK_C0] = &sdm632_apcc_pwr_clk.md.clkr,
	[APCC_CLK_C1] = &sdm632_apcc_perf_clk.md.clkr,
	[APCC_CLK_CCI] = &sdm632_apcc_cci_clk.md.clkr,
	[APCC_CLK_PLL_C0] = &sdm632_pwr_pll.clkr,
	[APCC_CLK_PLL_C1] = &sdm632_perf_pll.clkr,
	[APCC_CLK_PLL_CCI] = &sdm632_cci_pll.clkr,
};

static struct clk_hw *qcom_apcc_msm8953_hws[] = {
	&msm8953_pll_div2.hw,
	&apcc_gpll2_early_div.hw,
};

static struct clk_hw *qcom_apcc_sdm632_hws[] = {
	&sdm632_pwr_pll_div2.hw,
	&sdm632_perf_pll_div2.hw,
	&sdm632_cci_pll_div2.hw,
};

static struct qcom_cc_desc apcc_msm8953_desc = {
	.clks = qcom_apcc_msm8953_clocks,
	.num_clks = ARRAY_SIZE(qcom_apcc_msm8953_clocks),
	.clk_hws = qcom_apcc_msm8953_hws,
	.num_clk_hws = ARRAY_SIZE(qcom_apcc_msm8953_hws),
};

static struct qcom_cc_desc apcc_sdm632_desc = {
	.clks = qcom_apcc_sdm632_clocks,
	.num_clks = ARRAY_SIZE(qcom_apcc_sdm632_clocks),
	.clk_hws = qcom_apcc_sdm632_hws,
	.num_clk_hws = ARRAY_SIZE(qcom_apcc_sdm632_hws),
};

static long apcc_pll_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *prate)
{
	 return clk_alpha_pll_ops.round_rate(hw, rounddown(rate, *prate), prate);
}

static int sdm632_pll_notifier(struct notifier_block *nb,
				unsigned long event,
				void *data)
{
	struct clk_msm8953_cpu_mux_div *cmd = container_of(nb,
			struct clk_msm8953_cpu_mux_div, pll_nb);

	if (event != PRE_RATE_CHANGE)
		return NOTIFY_OK;

	if (clk_hw_get_parent(&cmd->md.clkr.hw) != cmd->variable_pll)
		return NOTIFY_OK;

	if (!cmd->aux_pll_used) {
		cmd->aux_pll_used = true;
		clk_prepare_enable(clk_hw_get_parent_by_index(&cmd->md.clkr.hw, 0)->clk);
	}

	if (cmd == &sdm632_apcc_cci_clk)
		mux_div_set_src_div(&cmd->md, 4, 5); // 320Mhz
	else
		mux_div_set_src_div(&cmd->md, 4, 3); // 533MHz

	return NOTIFY_OK;
}

static int sdm632_muxdiv_notifier(struct notifier_block *nb,
				   unsigned long event,
				   void *data)
{
	struct clk_msm8953_cpu_mux_div *cmd = container_of(nb,
			struct clk_msm8953_cpu_mux_div, md_nb);

	if (event != POST_RATE_CHANGE || !cmd->aux_pll_used)
		return NOTIFY_OK;

	cmd->aux_pll_used = false;
	clk_disable_unprepare(clk_hw_get_parent_by_index(&cmd->md.clkr.hw, 0)->clk);

	return NOTIFY_OK;
}

static int qcom_apcc_msm8953_probe(struct platform_device *pdev)
{
	struct qcom_cc_desc *data;
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct resource *res;
	void __iomem *base;
	u32 top_freq;
	int ret, i;

	data = (struct qcom_cc_desc*) of_device_get_match_data(dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(base))
		return PTR_ERR(base);

	regmap = devm_regmap_init_mmio(&pdev->dev, base, &apcc_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	msm8953_pll_ops = clk_alpha_pll_ops;
	msm8953_pll_ops.round_rate = apcc_pll_round_rate;

	if (data == &apcc_msm8953_desc) {
		struct nvmem_cell *speedbin_nvmem;
		char prop_name[sizeof("speed-bin-X-freq-khz")];
		u8 *speed_bin;
		size_t len;

		speedbin_nvmem = of_nvmem_cell_get(dev->of_node, NULL);
		if (IS_ERR(speedbin_nvmem))
			return PTR_ERR(speedbin_nvmem);

		speed_bin = nvmem_cell_read(speedbin_nvmem, &len);
		if (IS_ERR(speed_bin))
			return PTR_ERR(speed_bin);

		if (len != 1) {
			nvmem_cell_put(speedbin_nvmem);
			return -EINVAL;
		}

		snprintf(prop_name, sizeof(prop_name),
				"speed-bin-%d-freq-khz", *speed_bin & 0x7);

		ret = of_property_read_u32(dev->of_node, prop_name, &top_freq);

		nvmem_cell_put(speedbin_nvmem);

		if (ret < 0)
			return ret;

		clk_alpha_pll_configure(&msm8953_pll, regmap, &msm8953_pll_config);
	} else {
		clk_alpha_pll_configure(&sdm632_pwr_pll, regmap, &sdm632_pll_config);
		clk_alpha_pll_configure(&sdm632_perf_pll, regmap, &sdm632_pll_config);
		clk_fabia_pll_configure(&sdm632_cci_pll, regmap, &sdm632_cci_pll_config);
	}

	ret = qcom_cc_really_probe(pdev, data, regmap);
	if (ret)
		return ret;

	for (i = APCC_CLK_C0; i <= APCC_CLK_CCI; i++) {
		struct clk_msm8953_cpu_mux_div *cmd = container_of(data->clks[i],
				struct clk_msm8953_cpu_mux_div, md.clkr);

		clk_prepare_enable(cmd->md.clkr.hw.clk);

		if (data != &apcc_msm8953_desc) {
			cmd->pll_nb.notifier_call = sdm632_pll_notifier;
			cmd->md_nb.notifier_call = sdm632_muxdiv_notifier;

			clk_notifier_register(cmd->variable_pll->clk, &cmd->pll_nb);
			clk_notifier_register(cmd->md.clkr.hw.clk, &cmd->md_nb);
		}
	}

	if (data == &apcc_msm8953_desc)
		clk_set_rate(msm8953_pll.clkr.hw.clk, top_freq * 1000);

	return 0;
}

static int qcom_apcc_msm8953_remove(struct platform_device *pdev)
{
	struct qcom_cc_desc *data;
	struct device *dev = &pdev->dev;
	int i;

	data = (struct qcom_cc_desc*) of_device_get_match_data(dev);

	for (i = APCC_CLK_C0; i <= APCC_CLK_CCI; i++) {
		struct clk_msm8953_cpu_mux_div *cmd = container_of(data->clks[i],
				struct clk_msm8953_cpu_mux_div, md.clkr);

		clk_notifier_unregister(cmd->variable_pll->clk, &cmd->pll_nb);
		clk_notifier_unregister(cmd->md.clkr.hw.clk, &cmd->md_nb);
	}

	return 0;
}

static const struct of_device_id qcom_cpu_clk_msm8953_match_table[] = {
	{ .compatible = "qcom,msm8953-apcc", .data = &apcc_msm8953_desc },
	{ .compatible = "qcom,sdm632-apcc", .data = &apcc_sdm632_desc },
	{}
};
MODULE_DEVICE_TABLE(of, qcom_cpu_clk_msm8953_match_table);

static struct platform_driver qcom_cpu_clk_msm8953_driver = {
	.probe = qcom_apcc_msm8953_probe,
	.remove = qcom_apcc_msm8953_remove,
	.driver = {
		.name = "qcom-msm8953-apcc",
		.of_match_table = qcom_cpu_clk_msm8953_match_table,
	},
};

static int __init qcom_apcc_msm8953_init(void)
{
	return platform_driver_register(&qcom_cpu_clk_msm8953_driver);
}
arch_initcall(qcom_apcc_msm8953_init);

static void __init early_muxdiv_configure(unsigned int base_addr, u8 src, u8 div)
{
	int timeout = 500;
	void __iomem *base = ioremap(base_addr, SZ_8);;

	writel_relaxed(((src & 7) << 8) | (div & 0x1f), base + 4);
	mb();

	/* Set update bit */
	writel_relaxed(readl_relaxed(base) | BIT(0), base);
	mb();

	while (timeout-- && readl_relaxed(base) & BIT(0))
		udelay(1);

	/* Enable the branch */
	writel_relaxed(readl_relaxed(base + 8) | BIT(0), base + 8);
	mb();

	iounmap(base);
}

static int __init cpu_clock_pwr_init(void)
{
	struct device_node *ofnode =
		of_find_compatible_node(NULL, NULL, "qcom,msm8953-apcc") ?:
		of_find_compatible_node(NULL, NULL, "qcom,sdm632-apcc");

	if (!ofnode)
		return 0;

	/* Initialize mux/div clock to safe boot configuration. */
	early_muxdiv_configure(0xb111050, 4, 1); // 800Mhz
	early_muxdiv_configure(0xb011050, 4, 1); // 800Mhz
	early_muxdiv_configure(0xb1d1050, 4, 4); // 800/2.5Mhz
	return 0;
}

early_initcall(cpu_clock_pwr_init);
