// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2013-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2021 Golovach Lena
 */

#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_opp.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/nvmem-consumer.h>

#define MSM8953_REG_APM_DLY_CNT				0x2ac
#define MSM8953_APM_SEL_SWITCH_DLY_SHIFT		0
#define MSM8953_APM_SEL_SWITCH_DLY_MASK			GENMASK(7, 0)
#define MSM8953_APM_RESUME_CLK_DLY_SHIFT		8
#define MSM8953_APM_RESUME_CLK_DLY_MASK			GENMASK(15, 8)
#define MSM8953_APM_HALT_CLK_DLY_SHIFT			16
#define MSM8953_APM_HALT_CLK_DLY_MASK			GENMASK(23, 16)
#define MSM8953_APM_POST_HALT_DLY_SHIFT			24
#define MSM8953_APM_POST_HALT_DLY_MASK			GENMASK(31, 24)

#define MSM8953_REG_APM_MODE				0x2a8
#define MSM8953_APM_MODE_MX_VAL				0
#define MSM8953_APM_MODE_APCC_VAL			2

#define MSM8953_REG_APM_STATUS				0x2b0
#define MSM8953_STATUS_BUSY_MASK			GENMASK(4, 0)

#define CPR_MATCH(bin, rev, corner) (((corner) << 16) | ((bin) << 8) | (rev))
#define INIT_VOLTAGE_STEP 10000
#define INIT_VOLTAGE_WIDTH 6

struct fuse_corner_data {
	int ref_uV;
	int max_uV;
	int min_uV;
	int max_volt_scale;
};

struct cpr_match_list {
	/* bitmask created by CPR_MATCH macro */
	u32 match_mask;
	int64_t value;
};

struct cpr_thread_desc {
	/* reference frequencies of fuse corners */
	struct cpr_match_list *reference_freq;
	/* open/closed-loop voltage adjustement */
	struct cpr_match_list *voltage_adjustement;

	struct fuse_corner_data *fuse_corner_data;
	unsigned int num_fuse_corners;
};

struct corner_data {
	unsigned int fuse_corner;
	unsigned long freq;
};

struct acc_desc {
	unsigned int	enable_reg;
	u32		enable_mask;

	struct reg_sequence	*config;
	struct reg_sequence	*settings;
	int			num_regs_per_fuse;
};

struct cpr_desc {
	const struct acc_desc *acc_desc;
	int apm_threshold;
	struct cpr_thread_desc *threads;
	unsigned int num_threads;
};

struct fuse_corner {
	int min_uV;
	int max_uV;
	int uV;
	unsigned long max_freq;
};

struct corner {
	int uV;
	unsigned long freq;
	struct fuse_corner *fuse_corner;
};

struct cpr_drv;
struct cpr_thread {
	int			num_corners;
	int			id;
	unsigned long 		cci_freq;
	struct clk		*cpu_clk;
	struct corner		*corner;
	struct corner		*corners;
	struct fuse_corner	*fuse_corners;
	struct cpr_drv		*drv;
	struct generic_pm_domain pd;
	struct device		*attached_cpu_dev;
	const struct cpr_thread_desc *desc;
};

struct cpr_drv {
	int			num_threads;
	struct clk		*cci_clk;
	struct device		*dev;
	struct mutex		lock;
	struct regulator	*vdd_apc;
	struct regmap		*tcsr;
	struct regmap		*apm;
	u32			speed_bin;
	u32			fusing_rev;
	u32			vdd_apc_step;
	u32			last_uV;
	int			fuse_level_set;

	struct cpr_thread	*threads;
	struct genpd_onecell_data cell_data;

	const struct cpr_desc *desc;
};

static int64_t cpr_match(struct cpr_match_list *list, u32 match_mask)
{
	for (;list && list->match_mask; list++) {
		if ((list->match_mask & match_mask) == match_mask)
			return list->value;
	}

	return 0;
};

static void msm8953_apm_switch(struct regmap* rmap, u32 mode)
{
	int timeout = 500;
	u32 regval;

	regmap_write(rmap, MSM8953_REG_APM_MODE, mode);

	for (;timeout > 0; timeout--) {
		regmap_read(rmap, MSM8953_REG_APM_STATUS, &regval);
		if (!(regval & MSM8953_STATUS_BUSY_MASK))
			break;
	}
}

static void cpr_set_acc(struct cpr_drv* drv, int f)
{
	const struct acc_desc *desc = drv->desc->acc_desc;
	struct reg_sequence *s = desc->settings;
	int n = desc->num_regs_per_fuse;

	if (!s || f == drv->fuse_level_set)
		return;

	regmap_multi_reg_write(drv->tcsr, s + (n * f), n);

	drv->fuse_level_set = f;
}

static int cpr_pre_voltage(struct cpr_drv *drv,
			   int fuse_level,
			   int voltage,
			   unsigned long cci_freq)
{
	int diff_uV = voltage - drv->last_uV;

	if (diff_uV < 0 && cci_freq)
		clk_set_rate(drv->cci_clk, cci_freq);

	if (drv->tcsr && (fuse_level < drv->fuse_level_set))
		cpr_set_acc(drv, fuse_level);


	if (drv->apm && diff_uV < 0 &&
	    drv->last_uV >= drv->desc->apm_threshold &&
	    voltage < drv->desc->apm_threshold)
		msm8953_apm_switch(drv->apm, MSM8953_APM_MODE_MX_VAL);

	return 0;
}

static int cpr_post_voltage(struct cpr_drv *drv,
			    int fuse_level,
			    int voltage,
			    unsigned long cci_freq)
{
	int diff_uV = voltage - drv->last_uV;

	if (drv->tcsr && (fuse_level > drv->fuse_level_set))
		cpr_set_acc(drv, fuse_level);

	if (drv->apm && diff_uV > 0 &&
	    drv->last_uV < drv->desc->apm_threshold &&
	    voltage >= drv->desc->apm_threshold)
		msm8953_apm_switch(drv->apm, MSM8953_APM_MODE_APCC_VAL);

	if (diff_uV > 0 && cci_freq)
		clk_set_rate(drv->cci_clk, cci_freq);

	return 0;
}

static int cpr_commit_state(struct cpr_drv *drv)
{
	int ret, i;
	int new_uV = 0, fuse_level = 0;
	unsigned long new_cci_freq = 0;

	for (i = 0; i < drv->num_threads; i++) {
		struct cpr_thread *thread = &drv->threads[i];

		if (!thread->corner)
			continue;

		fuse_level = max(fuse_level,
				 (int) (thread->corner->fuse_corner -
				 &thread->fuse_corners[0]));

		new_uV = max(new_uV, thread->corner->uV);
		new_cci_freq = max(new_cci_freq, thread->cci_freq);

	}

	dev_vdbg(drv->dev, "%s: new uV: %d, last uV: %d\n", __func__, new_uV, drv->last_uV);

	if (new_uV == drv->last_uV)
		return 0;

	ret = cpr_pre_voltage(drv, fuse_level, new_uV, new_cci_freq);
	if (ret)
		return ret;

	if (new_uV > 1065000 || new_uV < 400000) {
		panic("Limit exceeded");
		return ret;
	}

	dev_vdbg(drv->dev, "setting voltage: %d\n", new_uV);

	ret = regulator_set_voltage(drv->vdd_apc, new_uV, new_uV);
	if (ret) {
		dev_err_ratelimited(drv->dev, "failed to set apc voltage %d\n",
				    new_uV);
		return ret;
	}

	ret = cpr_post_voltage(drv, fuse_level, new_uV, new_cci_freq);
	if (ret)
		return ret;

	drv->last_uV = new_uV;

	return 0;
}

static unsigned int cpr_get_cur_perf_state(struct cpr_thread *thread)
{
	return thread->corner ? thread->corner - thread->corners + 1 : 0;
}

static int cpr_set_performance_state(struct generic_pm_domain *domain,
				     unsigned int state)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);
	struct cpr_drv *drv = thread->drv;
	struct corner *corner, *end;
	struct dev_pm_opp *opp;
	int ret = 0;

	mutex_lock(&drv->lock);

	dev_dbg(drv->dev, "%s: setting perf state: %u (prev state: %u thread: %u)\n",
		__func__, state, cpr_get_cur_perf_state(thread), thread->id);

	/*
	 * Determine new corner we're going to.
	 * Remove one since lowest performance state is 1.
	 */
	corner = thread->corners + state - 1;
	end = &thread->corners[thread->num_corners - 1];
	if (corner > end || corner < thread->corners) {
		ret = -EINVAL;
		goto unlock;
	}

	thread->corner = corner;

	opp = dev_pm_opp_find_level_exact(&domain->dev, state);
	if (!IS_ERR_OR_NULL(opp)) {
		thread->cci_freq = dev_pm_opp_get_freq(opp);
		dev_pm_opp_put(opp);
	}

	ret = cpr_commit_state(drv);
	if (ret)
		goto unlock;

unlock:
	mutex_unlock(&drv->lock);

	dev_dbg(drv->dev, "%s: set perf state: %u thread:%u\n", __func__, state, thread->id);

	return ret;
}

static int cpr_read_efusef(struct device *dev, u32 *data, const char *format, ...)
{
	va_list args;
	struct nvmem_cell *cell;
	ssize_t len;
	char *ret, cell_name[50] = { 0 };
	int i;

	*data = 0;

	va_start(args, format);
	vsnprintf(cell_name, sizeof(cell_name), format, args);
	va_end(args);

	cell = nvmem_cell_get(dev, cell_name);
	if (IS_ERR(cell)) {
		if (PTR_ERR(cell) != -EPROBE_DEFER)
			dev_err(dev, "undefined cell %s\n", cell_name);
		return PTR_ERR(cell);
	}

	ret = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR(ret)) {
		dev_err(dev, "can't read cell %s\n", cell_name);
		return PTR_ERR(ret);
	}
	
	if (len > 4) {
		dev_err(dev, "efuse read(%s) = %x, bytes %zd\n", cell_name, *data, len);
		kfree(ret);
		return -EINVAL;
	}

	for (i = 0; i < len; i++)
		*data |= ret[i] << (8 * i);

	kfree(ret);
	dev_dbg(dev, "efuse read(%s) = %x, bytes %zd\n", cell_name, *data, len);

	return 0;
}

static int cpr_fuse_corner_init(struct cpr_thread *thread)
{
	struct cpr_drv *drv = thread->drv;
	const struct cpr_thread_desc *desc = thread->desc;
	struct fuse_corner_data *fdata;
	struct fuse_corner *fuse, *end;
	unsigned int step_volt;
	int uV, ret, i, efuse, steps;

	step_volt = drv->vdd_apc_step;
	if (!step_volt) {
		return -EINVAL;
	}

	/* Populate fuse_corner members */
	fuse = thread->fuse_corners;
	end = &fuse[desc->num_fuse_corners - 1];
	fdata = desc->fuse_corner_data;

	for (i = 0; fuse <= end; fuse++, i++, fdata++) {
		/*
		 * Update SoC voltages: platforms might choose a different
		 * regulators than the one used to characterize the algorithms
		 * (ie, init_voltage_step).
		 */
		fdata->min_uV = roundup(fdata->min_uV, step_volt);
		fdata->max_uV = roundup(fdata->max_uV, step_volt);

		/* Populate uV */
		cpr_read_efusef(drv->dev, &efuse, "cpr_thread%d_init_voltage%d",
				thread->id, i + 1);

		steps = efuse & (BIT(INIT_VOLTAGE_WIDTH - 1) - 1);
		/* Not two's complement.. instead highest bit is sign bit */
		if (efuse & BIT(INIT_VOLTAGE_WIDTH - 1))
			steps = -steps;

		uV = roundup(fdata->ref_uV + steps * INIT_VOLTAGE_STEP, step_volt);

		fuse->min_uV = fdata->min_uV;
		fuse->max_uV = fdata->max_uV;

		uV += cpr_match(desc->voltage_adjustement,
				CPR_MATCH(BIT(drv->speed_bin), BIT(drv->fusing_rev), BIT(i)));

		fuse->uV = clamp(uV, fuse->min_uV, fuse->max_uV);

		/* Re-check if corner voltage range is supported by regulator */
		ret = regulator_is_supported_voltage(drv->vdd_apc,
						     fuse->min_uV,
						     fuse->min_uV);
		if (!ret) {
			dev_err(drv->dev,
				"min uV: %d (fuse corner: %d) not supported by regulator\n",
				fuse->min_uV, i);
			return -EINVAL;
		}

		ret = regulator_is_supported_voltage(drv->vdd_apc,
						     fuse->max_uV,
						     fuse->max_uV);
		if (!ret) {
			dev_err(drv->dev,
				"max uV: %d (fuse corner: %d) not supported by regulator\n",
				fuse->max_uV, i);
			return -EINVAL;
		}

		dev_dbg(drv->dev,
			"fuse corner %d: [%d %d %d]\n",
			i, fuse->min_uV, fuse->uV, fuse->max_uV);
	}

	return 0;
}

static int cpr_interpolate(const struct corner *corner, int step_volt,
			   const struct fuse_corner_data *fdata)
{
	unsigned long f_high, f_low, f_diff;
	int uV_high, uV_low, uV;
	u64 temp, temp_limit;
	const struct fuse_corner *fuse, *prev_fuse;

	fuse = corner->fuse_corner;
	prev_fuse = fuse - 1;

	f_high = fuse->max_freq;
	f_low = prev_fuse->max_freq;
	uV_high = fuse->uV;
	uV_low = prev_fuse->uV;
	f_diff = fuse->max_freq - corner->freq;

	/*
	 * Don't interpolate in the wrong direction. This could happen
	 * if the adjusted fuse voltage overlaps with the previous fuse's
	 * adjusted voltage.
	 */
	if (f_high <= f_low || uV_high <= uV_low || f_high <= corner->freq)
		return corner->uV;

	temp = f_diff * (uV_high - uV_low);
	do_div(temp, f_high - f_low);

	/*
	 * max_volt_scale has units of uV/MHz while freq values
	 * have units of Hz.  Divide by 1000000 to convert to.
	 */
	temp_limit = f_diff * fdata->max_volt_scale;
	do_div(temp_limit, 1000000);

	uV = uV_high - min(temp, temp_limit);
	return roundup(uV, step_volt);
}

static unsigned long cpr_get_opp_hz_for_req(struct dev_pm_opp *ref,
					    struct device *cpu_dev)
{
	u64 rate = 0;
	struct device_node *ref_np;
	struct device_node *desc_np;
	struct device_node *child_np = NULL;
	struct device_node *child_req_np = NULL;

	desc_np = dev_pm_opp_of_get_opp_desc_node(cpu_dev);
	if (!desc_np)
		return 0;

	ref_np = dev_pm_opp_get_of_node(ref);
	if (!ref_np)
		goto out_ref;

	do {
		of_node_put(child_req_np);
		child_np = of_get_next_available_child(desc_np, child_np);
		child_req_np = of_parse_phandle(child_np, "required-opps", 0);
	} while (child_np && child_req_np != ref_np);

	if (child_np && child_req_np == ref_np)
		of_property_read_u64(child_np, "opp-hz", &rate);

	of_node_put(child_req_np);
	of_node_put(child_np);
	of_node_put(ref_np);
out_ref:
	of_node_put(desc_np);

	return (unsigned long) rate;
}

static int cpr_corner_init(struct cpr_thread *thread)
{
	struct cpr_drv *drv = thread->drv;
	const struct cpr_thread_desc *desc = thread->desc;
	int i, level;
	unsigned int fnum;
	struct fuse_corner *fuse, *prev_fuse;
	struct corner *corner, *end;
	struct corner_data *cdata;
	const struct fuse_corner_data *fdata;
	unsigned long freq_diff, freq_diff_mhz;
	unsigned long freq;
	int step_volt = drv->vdd_apc_step;
	struct dev_pm_opp *opp;

	if (!step_volt)
		return -EINVAL;

	corner = thread->corners;
	end = &corner[thread->num_corners - 1];

	cdata = devm_kcalloc(drv->dev, thread->num_corners,
			     sizeof(struct corner_data),
			     GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;


	for (level = 0; level < desc->num_fuse_corners; level ++) {
		fuse = &thread->fuse_corners[level];

		fuse->max_freq = cpr_match(desc->reference_freq,
				     CPR_MATCH(BIT(drv->speed_bin), BIT(drv->fusing_rev), BIT(level)));

		dev_dbg(drv->dev, "max freq: %lu fuse level: %u\n",
			fuse->max_freq, level);

		if (fuse->max_freq <= 0)
			return -ENODEV;
	}

	for (level = 1, fnum = 0; level <= thread->num_corners; level++) {
		opp = dev_pm_opp_find_level_exact(&thread->pd.dev, level);
		if (IS_ERR(opp))
			return -EINVAL;

		freq = cpr_get_opp_hz_for_req(opp, thread->attached_cpu_dev);
		if (!freq) {
			thread->num_corners = max(level - 1, 0);
			end = &thread->corners[thread->num_corners - 1];
			break;
		}

		fnum = desc->num_fuse_corners - 1;
		while (fnum > 0 &&
		       freq <= thread->fuse_corners[fnum - 1].max_freq)
			fnum--;

		cdata[level - 1].fuse_corner = fnum;
		cdata[level - 1].freq = freq;

		dev_dbg(drv->dev, "freq: %lu level: %u fuse level: %u\n",
			freq, dev_pm_opp_get_level(opp) - 1, fnum);
		dev_pm_opp_put(opp);
	}

	/*
	 *    +			c = corner      	    
	 *  v |                 f = fuse corner
	 *  o |           f c
	 *  l |         c
	 *  t |       f
	 *  a |     c
	 *  g | c f
	 *  e |
	 *    +----------------
	 *      0 1 2 3 4 5 6
	 *         corner
	 *
	 */
	for (i = 0; corner <= end; corner++, i++) {
		fnum = cdata[i].fuse_corner;
		fdata = &desc->fuse_corner_data[fnum];
		fuse = &thread->fuse_corners[fnum];
		if (fnum)
			prev_fuse = &thread->fuse_corners[fnum - 1];
		else
			prev_fuse = NULL;

		corner->fuse_corner = fuse;
		corner->freq = cdata[i].freq;
		corner->uV = fuse->uV;

		if (prev_fuse) {
			freq_diff = fuse->max_freq - corner->freq;
			freq_diff_mhz = freq_diff / 1000000;
			corner->uV = cpr_interpolate(corner, step_volt, fdata);
		}

		corner->uV = clamp(corner->uV, fuse->min_uV, fuse->max_uV);

		dev_dbg(drv->dev, "corner %d: %d uV fuse corner: %d uV - %d uV]", i,
			corner->uV, fuse->min_uV, fuse->max_uV);
	}

	return 0;
}

static int cpr_find_initial_corner(struct cpr_thread *thread)
{
	struct cpr_drv *drv = thread->drv;
	struct corner *iter, *corner;
	const struct corner *end;
	unsigned long rate;
	unsigned int i = 0;

	if (!thread->cpu_clk) {
		dev_err(drv->dev, "cannot get rate from NULL clk\n");
		return -EINVAL;
	}

	end = &thread->corners[thread->num_corners - 1];
	rate = clk_get_rate(thread->cpu_clk);

	/*
	 * Some bootloaders set a CPU clock frequency that is not defined
	 * in the OPP table. When running at an unlisted frequency,
	 * cpufreq_online() will change to the OPP which has the lowest
	 * frequency, at or above the unlisted frequency.
	 * Since cpufreq_online() always "rounds up" in the case of an
	 * unlisted frequency, this function always "rounds down" in case
	 * of an unlisted frequency. That way, when cpufreq_online()
	 * triggers the first ever call to cpr_set_performance_state(),
	 * it will correctly determine the direction as UP.
	 */
	for (iter = thread->corners; iter <= end; iter++) {
		if (iter->freq > rate)
			break;
		i++;
		if (iter->freq == rate) {
			corner = iter;
			break;
		}
		if (iter->freq < rate)
			corner = iter;
	}

	if (!corner) {
		dev_err(drv->dev, "boot up corner not found\n");
		return -EINVAL;
	}

	dev_dbg(drv->dev, "boot up perf state: %u\n", i);

	thread->corner = corner;

	drv->last_uV = regulator_get_voltage(drv->vdd_apc);

	cpr_commit_state(drv);

	return 0;
}

static const struct acc_desc msm8953_acc_desc = {
	.settings = (struct reg_sequence[]){
		{ 0, 0x1 },
		{ 4, 0x1 },
		{ 0, 0x0 },
		{ 4, 0x0 },
		{ 0, 0x0 },
		{ 4, 0x0 },
		{ 0, 0x0 },
		{ 4, 0x0 },
	},
	.num_regs_per_fuse = 2,
};

static const struct cpr_desc msm8953_cpr_desc = {
	.num_threads = 1,
	.apm_threshold = 850000,
	.acc_desc = &msm8953_acc_desc,
	.threads = (struct cpr_thread_desc[]) {
		{
			.num_fuse_corners = 4,
			.reference_freq = (struct cpr_match_list[]) {
				{ CPR_MATCH(0xff, 0xff, BIT(0)), 652800000 },
				{ CPR_MATCH(0xff, 0xff, BIT(1)), 1036800000 },
				{ CPR_MATCH(0xff, 0xff, BIT(2)), 1689600000 },
				{ CPR_MATCH(BIT(2) | BIT(6), 0xff, BIT(3)), 2016000000 },
				{ CPR_MATCH(BIT(0) | BIT(7), 0xff, BIT(3)), 2208000000 },
				{ },
			},
			.voltage_adjustement = (struct cpr_match_list[]) {
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6) | BIT(7),
						GENMASK(3, 1), BIT(0)), 25000 },
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6) | BIT(7),
						GENMASK(3, 1), BIT(2)), 5000 },
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6) | BIT(7),
						GENMASK(3, 1), BIT(3)), 40000 },
				{ },
			},
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 715000,
					.min_uV = 500000,
					.max_volt_scale = 0,
				}, {	/* fuse corner 1 */
					.ref_uV = 720000,
					.max_uV = 790000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 865000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				},
			},
		},
	},
};

static const struct acc_desc sdm632_acc_desc = {
	.settings = (struct reg_sequence[]){
		{ 0x00, 0x0 },
		{ 0x04, 0x80000000 },
		{ 0x08, 0x0 },
		{ 0x0c, 0x0 },
		{ 0x10, 0x80000000 },

		{ 0x00, 0x0 },
		{ 0x04, 0x0 },
		{ 0x08, 0x0 },
		{ 0x0c, 0x0 },
		{ 0x10, 0x0 },

		{ 0x00, 0x0 },
		{ 0x04, 0x0 },
		{ 0x08, 0x0 },
		{ 0x0c, 0x0 },
		{ 0x10, 0x0 },

		{ 0x00, 0x0 },
		{ 0x04, 0x1 },
		{ 0x08, 0x0 },
		{ 0x0c, 0x10000 },
		{ 0x10, 0x0 }
	},
	.num_regs_per_fuse = 5,
};

static const struct cpr_desc sdm632_cpr_desc = {
	.num_threads = 2,
	.apm_threshold = 875000,
	.acc_desc = &sdm632_acc_desc,
	.threads = (struct cpr_thread_desc[]) {
		{
			.num_fuse_corners = 4,
			.reference_freq = (struct cpr_match_list[]) {
				{ CPR_MATCH(0xff, 0xff, BIT(0)), 633600000 },
				{ CPR_MATCH(0xff, 0xff, BIT(1)), 1094400000 },
				{ CPR_MATCH(0xff, 0xff, BIT(2)), 1401600000 },
				{ CPR_MATCH(0xff, 0xff, BIT(3)), 2016000000 },
				{ },
			},
			.voltage_adjustement = (struct cpr_match_list[]) {
				{ CPR_MATCH(0xff, 0xff, BIT(3)), 10000 },
				{ },
			},
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 720000,
					.min_uV = 500000,
					.max_volt_scale = 0,
				}, {	/* fuse corner 1 */
					.ref_uV = 790000,
					.max_uV = 865000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 865000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				},
			},
		}, {
			.num_fuse_corners = 4,
			.reference_freq = (struct cpr_match_list[]) {
				{ CPR_MATCH(0xff, 0xff, BIT(0)), 614400000 },
				{ CPR_MATCH(0xff, 0xff, BIT(1)), 1036800000 },
				{ CPR_MATCH(0xff, 0xff, BIT(2)), 1363200000 },
				{ CPR_MATCH(0xff, 0xff, BIT(3)), 1804800000 },
				{ },
			},
			.voltage_adjustement = (struct cpr_match_list[]) {
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6), BIT(0) | BIT(1), BIT(0)), 30000 },
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6), BIT(0) | BIT(1) | BIT(2), BIT(2)), 10000 },
				{ CPR_MATCH(BIT(0) | BIT(2) | BIT(6), BIT(0) | BIT(1) | BIT(2), BIT(3)), 20000 },
				{ },
			},
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 720000,
					.min_uV = 500000,
					.max_volt_scale = 0,
				}, {	/* fuse corner 1 */
					.ref_uV = 790000,
					.max_uV = 865000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 865000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				}, {	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 500000,
					.max_volt_scale = 2000,
				},
			},
		},
	},
};

static unsigned int cpr_get_performance_state(struct generic_pm_domain *genpd,
					      struct dev_pm_opp *opp)
{
	return dev_pm_opp_get_level(opp);
}

static int cpr_pd_attach_dev(struct generic_pm_domain *domain,
			     struct device *dev)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);
	struct cpr_drv *drv = thread->drv;
	const struct acc_desc *acc_desc = drv->desc->acc_desc;
	int ret = 0;

	mutex_lock(&drv->lock);

	dev_dbg(drv->dev, "attach callback for: %s\n", dev_name(dev));

	/*
	 * This driver only supports scaling voltage for a CPU cluster
	 * where all CPUs in the cluster share a single regulator.
	 * Therefore, save the struct device pointer only for the first
	 * CPU device that gets attached. There is no need to do any
	 * additional initialization when further CPUs get attached.
	 */
	if (thread->attached_cpu_dev)
		goto unlock;

	/*
	 * cpr_scale_voltage() requires the direction (if we are changing
	 * to a higher or lower OPP). The first time
	 * cpr_set_performance_state() is called, there is no previous
	 * performance state defined. Therefore, we call
	 * cpr_find_initial_corner() that gets the CPU clock frequency
	 * set by the bootloader, so that we can determine the direction
	 * the first time cpr_set_performance_state() is called.
	 */
	thread->cpu_clk = devm_clk_get(dev, NULL);
	if (IS_ERR(thread->cpu_clk)) {
		ret = PTR_ERR(thread->cpu_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(drv->dev, "could not get cpu clk: %d\n", ret);
		goto unlock;
	}
	thread->attached_cpu_dev = dev;

	dev_dbg(drv->dev, "using cpu clk from: %s\n",
		dev_name(thread->attached_cpu_dev));

	/*
	 * Everything related to (virtual) corners has to be initialized
	 * here, when attaching to the power domain, since we need to know
	 * the maximum frequency for each fuse corner, and this is only
	 * available after the cpufreq driver has attached to us.
	 * The reason for this is that we need to know the highest
	 * frequency associated with each fuse corner. Junak was here.
	 */
	ret = dev_pm_opp_get_opp_count(&thread->pd.dev);
	if (ret < 0) {
		dev_err(drv->dev, "could not get OPP count\n");
		goto unlock;
	}
	thread->num_corners = ret;

	thread->corners = devm_kcalloc(drv->dev, thread->num_corners,
				    sizeof(*thread->corners),
				    GFP_KERNEL);
	if (!thread->corners) {
		ret = -ENOMEM;
		goto unlock;
	}

	ret = cpr_corner_init(thread);
	if (ret)
		goto unlock;

	dev_dbg(drv->dev, "corners: %d thread: %d\n", ret, thread->id);

	if (thread->num_corners < 2) {
		dev_err(drv->dev, "need at least 2 OPPs to use CPR\n");
		ret = -EINVAL;
		goto unlock;
	}


	ret = cpr_find_initial_corner(thread);
	if (ret)
		goto unlock;

	if (acc_desc->config)
		regmap_multi_reg_write(drv->tcsr, acc_desc->config,
				       acc_desc->num_regs_per_fuse);

	dev_info(drv->dev, "thread initialized with %u OPPs\n",
		 thread->num_corners);

unlock:
	mutex_unlock(&drv->lock);

	return ret;
}


static int cpr_threads_init(struct cpr_drv *drv)
{
	int i, ret;

	drv->num_threads = drv->desc->num_threads;
	drv->threads = devm_kcalloc(drv->dev, drv->num_threads,
			       sizeof(*drv->threads), GFP_KERNEL);
	if (!drv->threads)
		return -ENOMEM;

	drv->cell_data.num_domains = drv->desc->num_threads,
	drv->cell_data.domains = devm_kcalloc(drv->dev, drv->cell_data.num_domains,
			       sizeof(*drv->cell_data.domains), GFP_KERNEL);
	if (!drv->cell_data.domains)
		return -ENOMEM;

	for (i = 0; i < drv->desc->num_threads; i++) {
		struct cpr_thread *thread = &drv->threads[i];
		struct cpr_thread_desc *tdesc = &drv->desc->threads[i];

		thread->id = i;
		thread->drv = drv;
		thread->desc = tdesc;
		thread->fuse_corners = devm_kcalloc(drv->dev,
					 tdesc->num_fuse_corners,
					 sizeof(*thread->fuse_corners),
					 GFP_KERNEL);
		if (!thread->fuse_corners)
			return -ENOMEM;

		ret = cpr_fuse_corner_init(thread);
		if (ret)
			return ret;

		thread->pd.name = devm_kasprintf(drv->dev, GFP_KERNEL,
						  "%s_thread%d",
						  drv->dev->of_node->full_name,
						  thread->id);
		if (!thread->pd.name)
			return -EINVAL;

		thread->pd.set_performance_state = cpr_set_performance_state;
		thread->pd.opp_to_performance_state = cpr_get_performance_state;
		thread->pd.attach_dev = cpr_pd_attach_dev;
		thread->pd.flags = GENPD_FLAG_RPM_ALWAYS_ON;

		drv->cell_data.domains[i] = &thread->pd;

		ret = pm_genpd_init(&thread->pd, NULL, false);
		if (ret)
			return ret;
	}

	return 0;
}

static int cpr_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cpr_drv *drv;
	int ret;
	struct device_node *np;

	drv = devm_kzalloc(dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;
	drv->dev = dev;
	drv->desc = of_device_get_match_data(dev);

	if (!drv->desc || !drv->desc->acc_desc)
		return -EINVAL;

	mutex_init(&drv->lock);

	drv->cci_clk = devm_clk_get_optional(dev, "cci");
	if (IS_ERR(drv->cci_clk)) {
		ret = PTR_ERR(drv->cci_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(drv->dev, "could not get cci clk: %d\n", ret);
		return ret;
	}

	np = of_parse_phandle(dev->of_node, "acc-syscon", 0);
	if (!np)
		return -ENODEV;

	drv->tcsr = syscon_node_to_regmap(np);
	of_node_put(np);
	if (IS_ERR(drv->tcsr))
		return PTR_ERR(drv->tcsr);

	np = of_parse_phandle(dev->of_node, "apm-syscon", 0);
	if (!np)
		return -ENODEV;
	drv->apm = syscon_node_to_regmap(np);
	of_node_put(np);
	if (IS_ERR(drv->apm)) {
		drv->apm = NULL;
	} else {
		regmap_update_bits(drv->apm,
			MSM8953_REG_APM_DLY_CNT,
			MSM8953_APM_POST_HALT_DLY_MASK,
			0x02 << MSM8953_APM_POST_HALT_DLY_SHIFT);
		regmap_update_bits(drv->apm,
			MSM8953_REG_APM_DLY_CNT,
			MSM8953_APM_HALT_CLK_DLY_MASK,
			0x11 << MSM8953_APM_HALT_CLK_DLY_SHIFT);
		regmap_update_bits(drv->apm,
			MSM8953_REG_APM_DLY_CNT,
			MSM8953_APM_RESUME_CLK_DLY_MASK,
			0x10 << MSM8953_APM_RESUME_CLK_DLY_SHIFT);
		regmap_update_bits(drv->apm,
			MSM8953_REG_APM_DLY_CNT,
			MSM8953_APM_SEL_SWITCH_DLY_MASK,
			0x01 << MSM8953_APM_SEL_SWITCH_DLY_SHIFT);

		msm8953_apm_switch(drv->apm, MSM8953_APM_MODE_MX_VAL);
	}

	drv->vdd_apc = devm_regulator_get(dev, "vdd-apc");
	if (IS_ERR(drv->vdd_apc))
		return PTR_ERR(drv->vdd_apc);

	if (of_property_read_u32(dev->of_node, "vdd-apc-step-uv",
				 &drv->vdd_apc_step))
		return -ENOENT;

	/*
	 * Initialize fuse corners, since it simply depends
	 * on data in efuses.
	 * Everything related to (virtual) corners has to be
	 * initialized after attaching to the power domain,
	 * since it depends on the CPU's OPP table.
	 */
	ret = cpr_read_efusef(dev, &drv->fusing_rev, "cpr_fuse_revision");
	if (ret)
		return ret;

	ret = cpr_read_efusef(dev, &drv->speed_bin, "cpr_speed_bin");
	if (ret)
		return ret;

	ret = cpr_threads_init(drv);
	if (ret)
		return ret;

	ret = of_genpd_add_provider_onecell(dev->of_node, &drv->cell_data);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, drv);

	return 0;
}

static int cpr_remove(struct platform_device *pdev)
{
	struct cpr_drv *drv = platform_get_drvdata(pdev);
	int i;

	of_genpd_del_provider(pdev->dev.of_node);

	for (i = 0; i < drv->num_threads; i++)
		pm_genpd_remove(&drv->threads[i].pd);

	return 0;
}

static const struct of_device_id cpr_match_table[] = {
	{ .compatible = "qcom,msm8953-cpr4", .data = &msm8953_cpr_desc },
	{ .compatible = "qcom,sdm632-cpr4", .data = &sdm632_cpr_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, cpr_match_table);

static struct platform_driver cpr_driver = {
	.probe		= cpr_probe,
	.remove		= cpr_remove,
	.driver		= {
		.name	= "qcom-cpr3",
		.of_match_table = cpr_match_table,
	},
};
module_platform_driver(cpr_driver);

MODULE_DESCRIPTION("Core Power (no Reduction) (CPR) v3 driver");
MODULE_LICENSE("GPL v2");
