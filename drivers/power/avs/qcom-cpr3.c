// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2013-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2019 Linaro Limited
 */

#define DEBUG
#include <linux/module.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_opp.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/nvmem-consumer.h>

#define CPR3_RO_COUNT				16
#define CPR3_RO_MASK				GENMASK(CPR3_RO_COUNT - 1, 0)

/* CPR3 registers */
#define CPR3_REG_CPR_VERSION			0x0

#define CPR3_REG_CPR_CTL			0x4
#define CPR3_CPR_CTL_LOOP_EN_MASK		BIT(0)
#define CPR3_CPR_CTL_IDLE_CLOCKS_MASK		GENMASK(4, 0)
#define CPR3_CPR_CTL_IDLE_CLOCKS_SHIFT		1
#define CPR3_CPR_CTL_COUNT_MODE_MASK		GENMASK(1, 0)
#define CPR3_CPR_CTL_COUNT_MODE_SHIFT		6
#define CPR3_CPR_CTL_COUNT_MODE_ALL_AT_ONCE_MIN	0
#define CPR3_CPR_CTL_COUNT_MODE_ALL_AT_ONCE_MAX	1
#define CPR3_CPR_CTL_COUNT_MODE_STAGGERED	2
#define CPR3_CPR_CTL_COUNT_MODE_ALL_AT_ONCE_AGE	3
#define CPR3_CPR_CTL_COUNT_REPEAT_MASK		GENMASK(22, 0)
#define CPR3_CPR_CTL_COUNT_REPEAT_SHIFT		9

#define CPR3_REG_CPR_STATUS			0x8
#define CPR3_CPR_STATUS_BUSY_MASK		BIT(0)
#define CPR3_CPR_STATUS_AGING_MEASUREMENT_MASK	BIT(1)

/*
 * This register is not present on controllers that support HW closed-loop
 * except CPR4 APSS controller.
 */
#define CPR3_REG_CPR_TIMER_AUTO_CONT		0xC

#define CPR3_REG_CPR_STEP_QUOT			0x14
#define CPR3_CPR_STEP_QUOT_MIN_MASK		GENMASK(5, 0)
#define CPR3_CPR_STEP_QUOT_MIN_SHIFT		0
#define CPR3_CPR_STEP_QUOT_MAX_MASK		GENMASK(5, 0)
#define CPR3_CPR_STEP_QUOT_MAX_SHIFT		6

#define CPR3_REG_GCNT(ro)			(0xA0 + 0x4 * (ro))

#define CPR3_REG_SENSOR_BYPASS_WRITE(sensor)	(0xE0 + 0x4 * ((sensor) / 32))
#define CPR3_REG_SENSOR_BYPASS_WRITE_BANK(bank)	(0xE0 + 0x4 * (bank))

#define CPR3_REG_SENSOR_MASK_WRITE(sensor)	(0x120 + 0x4 * ((sensor) / 32))
#define CPR3_REG_SENSOR_MASK_WRITE_BANK(bank)	(0x120 + 0x4 * (bank))
#define CPR3_REG_SENSOR_MASK_READ(sensor)	(0x140 + 0x4 * ((sensor) / 32))

#define CPR3_REG_SENSOR_OWNER(sensor)		(0x200 + 0x4 * (sensor))

#define CPR3_REG_CONT_CMD			0x800
#define CPR3_CONT_CMD_ACK			0x1
#define CPR3_CONT_CMD_NACK			0x0

#define CPR3_REG_THRESH(thread)			(0x808 + 0x440 * (thread))
#define CPR3_THRESH_CONS_DOWN_MASK		GENMASK(3, 0)
#define CPR3_THRESH_CONS_DOWN_SHIFT		0
#define CPR3_THRESH_CONS_UP_MASK		GENMASK(3, 0)
#define CPR3_THRESH_CONS_UP_SHIFT		4
#define CPR3_THRESH_DOWN_THRESH_MASK		GENMASK(4, 0)
#define CPR3_THRESH_DOWN_THRESH_SHIFT		8
#define CPR3_THRESH_UP_THRESH_MASK		GENMASK(4, 0)
#define CPR3_THRESH_UP_THRESH_SHIFT		13

#define CPR3_REG_RO_MASK(thread)		(0x80C + 0x440 * (thread))

#define CPR3_REG_RESULT0(thread)		(0x810 + 0x440 * (thread))
#define CPR3_RESULT0_BUSY_MASK			BIT(0)
#define CPR3_RESULT0_STEP_DN_MASK		BIT(1)
#define CPR3_RESULT0_STEP_UP_MASK		BIT(2)
#define CPR3_RESULT0_ERROR_STEPS_MASK		GENMASK(4, 0)
#define CPR3_RESULT0_ERROR_STEPS_SHIFT		3
#define CPR3_RESULT0_ERROR_MASK			GENMASK(11, 0)
#define CPR3_RESULT0_ERROR_SHIFT		8
#define CPR3_RESULT0_NEGATIVE_MASK		BIT(20)

#define CPR3_REG_RESULT1(thread)		(0x814 + 0x440 * (thread))
#define CPR3_RESULT1_QUOT_MIN_MASK		GENMASK(11, 0)
#define CPR3_RESULT1_QUOT_MIN_SHIFT		0
#define CPR3_RESULT1_QUOT_MAX_MASK		GENMASK(11, 0)
#define CPR3_RESULT1_QUOT_MAX_SHIFT		12
#define CPR3_RESULT1_RO_MIN_MASK		GENMASK(3, 0)
#define CPR3_RESULT1_RO_MIN_SHIFT		24
#define CPR3_RESULT1_RO_MAX_MASK		GENMASK(3, 0)
#define CPR3_RESULT1_RO_MAX_SHIFT		28

#define CPR3_REG_RESULT2(thread)		(0x818 + 0x440 * (thread))
#define CPR3_RESULT2_STEP_QUOT_MIN_MASK		GENMASK(5, 0)
#define CPR3_RESULT2_STEP_QUOT_MIN_SHIFT	0
#define CPR3_RESULT2_STEP_QUOT_MAX_MASK		GENMASK(5, 0)
#define CPR3_RESULT2_STEP_QUOT_MAX_SHIFT	6
#define CPR3_RESULT2_SENSOR_MIN_MASK		GENMASK(7, 0)
#define CPR3_RESULT2_SENSOR_MIN_SHIFT		16
#define CPR3_RESULT2_SENSOR_MAX_MASK		GENMASK(7, 0)
#define CPR3_RESULT2_SENSOR_MAX_SHIFT		24

#define CPR3_REG_IRQ_EN				0x81C
#define CPR3_REG_IRQ_CLEAR			0x820
#define CPR3_REG_IRQ_STATUS			0x824
#define CPR3_IRQ_UP				BIT(3)
#define CPR3_IRQ_MID				BIT(2)
#define CPR3_IRQ_DOWN				BIT(1)
#define CPR3_IRQ_DEFAULT			(CPR3_IRQ_UP | CPR3_IRQ_DOWN)
#define CPR3_IRQ_ALL				(CPR3_IRQ_UP | CPR3_IRQ_MID | CPR3_IRQ_DOWN)

#define CPR3_REG_TARGET_QUOT(thread, ro)	(0x840 + 0x440 * (thread) + 0x4 * (ro))

/* Registers found only on controllers that support HW closed-loop. */
#define CPR3_REG_PD_THROTTLE			0xE8
#define CPR3_PD_THROTTLE_DISABLE		0x0

#define CPR3_REG_HW_CLOSED_LOOP			0x3000
#define CPR3_HW_CLOSED_LOOP_ENABLE		0x0
#define CPR3_HW_CLOSED_LOOP_DISABLE		0x1

#define CPR3_REG_CPR_TIMER_MID_CONT		0x3004
#define CPR3_REG_CPR_TIMER_UP_DN_CONT		0x3008

#define CPR3_REG_LAST_MEASUREMENT		0x7F8
#define CPR3_LAST_MEASUREMENT_THREAD_DN_SHIFT	0
#define CPR3_LAST_MEASUREMENT_THREAD_UP_SHIFT	4
#define CPR3_LAST_MEASUREMENT_THREAD_DN(thread) \
		(BIT(thread) << CPR3_LAST_MEASUREMENT_THREAD_DN_SHIFT)
#define CPR3_LAST_MEASUREMENT_THREAD_UP(thread) \
		(BIT(thread) << CPR3_LAST_MEASUREMENT_THREAD_UP_SHIFT)
#define CPR3_LAST_MEASUREMENT_AGGR_DN		BIT(8)
#define CPR3_LAST_MEASUREMENT_AGGR_MID		BIT(9)
#define CPR3_LAST_MEASUREMENT_AGGR_UP		BIT(10)
#define CPR3_LAST_MEASUREMENT_VALID		BIT(11)
#define CPR3_LAST_MEASUREMENT_SAW_ERROR		BIT(12)
#define CPR3_LAST_MEASUREMENT_PD_BYPASS_MASK	GENMASK(7, 0)
#define CPR3_LAST_MEASUREMENT_PD_BYPASS_SHIFT	16

/* CPR4 controller specific registers and bit definitions */
#define CPR4_REG_CPR_TIMER_CLAMP			0x10
#define CPR4_CPR_TIMER_CLAMP_THREAD_AGGREGATION_EN	BIT(27)

#define CPR4_REG_MISC				0x700
#define CPR4_MISC_RESET_STEP_QUOT_LOOP_EN	BIT(2)
#define CPR4_MISC_THREAD_HAS_ALWAYS_VOTE_EN	BIT(3)
#define CPR4_MISC_MARGIN_TABLE_ROW_SELECT_MASK	GENMASK(3, 0)
#define CPR4_MISC_MARGIN_TABLE_ROW_SELECT_SHIFT	20
#define CPR4_MISC_TEMP_SENSOR_ID_START_MASK	GENMASK(3, 0)
#define CPR4_MISC_TEMP_SENSOR_ID_START_SHIFT	24
#define CPR4_MISC_TEMP_SENSOR_ID_END_MASK	GENMASK(3, 0)
#define CPR4_MISC_TEMP_SENSOR_ID_END_SHIFT	28

#define CPR4_REG_SAW_ERROR_STEP_LIMIT		0x7A4
#define CPR4_SAW_ERROR_STEP_LIMIT_UP_MASK	GENMASK(4, 0)
#define CPR4_SAW_ERROR_STEP_LIMIT_UP_SHIFT	0
#define CPR4_SAW_ERROR_STEP_LIMIT_DN_MASK	GENMASK(4, 0)
#define CPR4_SAW_ERROR_STEP_LIMIT_DN_SHIFT	5

#define CPR4_REG_MARGIN_TEMP_CORE_TIMERS			0x7A8
#define CPR4_MARGIN_TEMP_CORE_TIMERS_SETTLE_VOLTAGE_COUNT_MASK	GENMASK(10, 0)
#define CPR4_MARGIN_TEMP_CORE_TIMERS_SETTLE_VOLTAGE_COUNT_SHIFT	18

#define CPR4_REG_MARGIN_TEMP_CORE(core)		(0x7AC + 0x4 * (core))
#define CPR4_MARGIN_TEMP_CORE_ADJ_MASK		GENMASK(7, 0)
#define CPR4_MARGIN_TEMP_CORE_ADJ_SHIFT		8

#define CPR4_REG_MARGIN_TEMP_POINT0N1		0x7F0
#define CPR4_MARGIN_TEMP_POINT0_MASK		GENMASK(11, 0)
#define CPR4_MARGIN_TEMP_POINT0_SHIFT		0
#define CPR4_MARGIN_TEMP_POINT1_MASK		GENMASK(11, 0)
#define CPR4_MARGIN_TEMP_POINT1_SHIFT		12
#define CPR4_REG_MARGIN_TEMP_POINT2		0x7F4
#define CPR4_MARGIN_TEMP_POINT2_MASK		GENMASK(11, 0)
#define CPR4_MARGIN_TEMP_POINT2_SHIFT		0

#define CPR4_REG_MARGIN_ADJ_CTL				0x7F8
#define CPR4_MARGIN_ADJ_BOOST_EN			BIT(0)
#define CPR4_MARGIN_ADJ_CORE_ADJ_EN			BIT(1)
#define CPR4_MARGIN_ADJ_TEMP_ADJ_EN			BIT(2)
#define CPR4_MARGIN_ADJ_TIMER_SETTLE_VOLTAGE_EN		BIT(3)
#define CPR4_MARGIN_ADJ_HW_CLOSED_LOOP_EN_MASK		BIT(4)
#define CPR4_MARGIN_ADJ_HW_CLOSED_LOOP_ENABLE		BIT(4)
#define CPR4_MARGIN_ADJ_HW_CLOSED_LOOP_DISABLE		0
#define CPR4_MARGIN_ADJ_PER_RO_KV_MARGIN_EN		BIT(7)
#define CPR4_MARGIN_ADJ_KV_MARGIN_ADJ_EN		BIT(8)
#define CPR4_MARGIN_ADJ_PMIC_STEP_SIZE_MASK		GENMASK(4, 0)
#define CPR4_MARGIN_ADJ_PMIC_STEP_SIZE_SHIFT		12
#define CPR4_MARGIN_ADJ_INITIAL_TEMP_BAND_MASK		GENMASK(2, 0)
#define CPR4_MARGIN_ADJ_INITIAL_TEMP_BAND_SHIFT		19
#define CPR4_MARGIN_ADJ_MAX_NUM_CORES_MASK		GENMASK(3, 0)
#define CPR4_MARGIN_ADJ_MAX_NUM_CORES_SHIFT		22
#define CPR4_MARGIN_ADJ_KV_MARGIN_ADJ_STEP_QUOT_MASK	GENMASK(5, 0)
#define CPR4_MARGIN_ADJ_KV_MARGIN_ADJ_STEP_QUOT_SHIFT	26

#define CPR4_REG_CPR_MASK_THREAD(thread)		(0x80C + 0x440 * (thread))
#define CPR4_CPR_MASK_THREAD_DISABLE_THREAD		BIT(31)
#define CPR4_CPR_MASK_THREAD_RO_MASK4THREAD_MASK	GENMASK(15, 0)

#define CPR3_NUM_RING_OSC	16

enum voltage_change_dir {
	NO_CHANGE,
	DOWN,
	UP,
};

/* For speed-bin and revision fuse dependent adjustements */
typedef int64_t (*fuse_map_func_t)(u16 speed_bin, u16 rev, int corner);

struct cpr_fuse {
	char *ring_osc;
	char *init_voltage;
	char *quotient;
	char *quotient_offset;
};

struct fuse_corner_data {
	int ref_uV;
	int max_uV;
	int min_uV;
	int max_volt_scale;
	int max_quot_scale;
	/* fuse quot */
	int quot_offset;
	int quot_scale;
	int quot_adjust;
	/* fuse quot_offset */
	int quot_offset_scale;
	int quot_offset_adjust;
};

struct cpr_thread_desc {
	int init_voltage_step;
	int init_voltage_width;
	int sensor_range_start;
	int sensor_range_end;
	unsigned int num_fuse_corners;
	/* reference frequencies of fuse corners */
	fuse_map_func_t corner_freq_func;
	/* open/closed-loop voltage adjustement func */
	fuse_map_func_t quot_adjust_func;
	fuse_map_func_t voltage_adjust_func;
	struct fuse_corner_data *fuse_corner_data;
};

struct corner_data {
	unsigned int fuse_corner;
	unsigned long freq;
};

struct cpr_desc {
	unsigned int num_threads;
	int *ro_scaling_factor;

	unsigned int		timer_delay_us;
	unsigned int		timer_cons_up;
	unsigned int		timer_cons_down;
	unsigned int		up_threshold;
	unsigned int		down_threshold;
	unsigned int		idle_clocks;
	unsigned int		count_mode;
	unsigned int		count_repeat;
	unsigned int		step_quot_init_min;
	unsigned int		step_quot_init_max;
	unsigned int		gcnt_us;
	unsigned int		vdd_apc_step_up_limit;
	unsigned int		vdd_apc_step_down_limit;
	u32			version;

	struct cpr_thread_desc *threads;
	bool reduce_to_fuse_uV;
	bool reduce_to_corner_uV;
};

struct acc_desc {
	unsigned int	enable_reg;
	u32		enable_mask;

	struct reg_sequence	*config;
	struct reg_sequence	*settings;
	int			num_regs_per_fuse;
};

struct cpr_acc_desc {
	const struct cpr_desc *cpr_desc;
	const struct acc_desc *acc_desc;
};

struct fuse_corner {
	int min_uV;
	int max_uV;
	int uV;
	int quot;
	unsigned long max_freq;
	u8 ring_osc_idx;
};

struct corner {
	int min_uV;
	int max_uV;
	int uV;
	int last_uV;
	int quot_adjust;
	unsigned long freq;
	struct fuse_corner *fuse_corner;
};

struct cpr_drv;
struct cpr_thread {
	int			num_corners;
	int			id;
	int			ena_count;
	struct clk		*cpu_clk;
	struct corner		*corner;
	struct corner		*corners;
	struct fuse_corner	*fuse_corners;
	struct cpr_drv		*drv;
	struct generic_pm_domain pd;
	struct device		*attached_cpu_dev;
	const struct cpr_fuse	*cpr_fuses;
	const struct cpr_thread_desc *desc;
};

struct cpr_drv {
	int			num_threads;
	unsigned int		ref_clk_khz;
	struct device		*dev;
	struct mutex		lock;
	void __iomem		*base;
	struct regulator	*vdd_apc;
	struct regmap		*tcsr;
	u32			gcnt;
	u32			speed_bin;
	u32			fusing_rev;
	u32			vdd_apc_step;
	u32			last_uV;
	int			fuse_level_set;

	struct cpr_thread	*threads;
	struct genpd_onecell_data cell_data;

	const struct cpr_desc *desc;
	const struct acc_desc *acc_desc;
	struct dentry *debugfs;
};

static void cpr_write(struct cpr_drv *drv, u32 offset, u32 value)
{
	writel_relaxed(value, drv->base + offset);
}

static u32 cpr_read(struct cpr_drv *drv, u32 offset)
{
	return readl_relaxed(drv->base + offset);
}

static void
cpr_masked_write(struct cpr_drv *drv, u32 offset, u32 mask, u32 value)
{
	u32 val;

	val = readl_relaxed(drv->base + offset);
	val &= ~mask;
	val |= value & mask;
	writel_relaxed(val, drv->base + offset);
}

static void cpr_irq_clr(struct cpr_drv *drv)
{
	cpr_write(drv, CPR3_REG_IRQ_CLEAR, CPR3_IRQ_ALL);
}

static void cpr_irq_clr_nack(struct cpr_drv *drv)
{
	cpr_irq_clr(drv);
	cpr_write(drv, CPR3_REG_CONT_CMD, 0);
}

static void cpr_irq_clr_ack(struct cpr_drv *drv)
{
	cpr_irq_clr(drv);
	cpr_write(drv, CPR3_REG_CONT_CMD, 1);
}

static void cpr_irq_set(struct cpr_drv *drv, u32 int_bits)
{
	cpr_write(drv, CPR3_REG_IRQ_EN, int_bits);
}

static void cpr_ctl_enable(struct cpr_drv *drv)
{
	cpr_masked_write(drv, CPR3_REG_CPR_CTL,
			CPR3_CPR_CTL_LOOP_EN_MASK,
			CPR3_CPR_CTL_LOOP_EN_MASK);
}

static void cpr_ctl_disable(struct cpr_drv *drv)
{
	cpr_irq_set(drv, 0);
	cpr_irq_clr(drv);
	cpr_masked_write(drv, CPR3_REG_CPR_CTL,
			CPR3_CPR_CTL_LOOP_EN_MASK, 0);
}

static bool cpr_ctl_is_enabled(struct cpr_drv *drv)
{
	u32 reg_val;

	reg_val = cpr_read(drv, CPR3_REG_CPR_CTL);
	return reg_val & CPR3_CPR_CTL_LOOP_EN_MASK;
}

static bool cpr_check_threads_busy(struct cpr_drv *drv)
{
	int i;

	for (i = 0; i < drv->num_threads; i++)
		if (cpr_read(drv, CPR3_REG_RESULT0(i)) &
		    CPR3_RESULT0_BUSY_MASK)
			return true;

	return false;
}

static void cpr_corner_restore(struct cpr_thread *thread, struct corner *corner)
{
	struct cpr_drv *drv = thread->drv;
	struct fuse_corner *fuse = corner->fuse_corner;
	u32 ro_sel = fuse->ring_osc_idx;

	cpr_write(drv, CPR3_REG_GCNT(ro_sel), drv->gcnt);

	cpr_write(drv, CPR3_REG_RO_MASK(thread->id), CPR3_RO_MASK & ~BIT(ro_sel));

	cpr_write(drv, CPR3_REG_TARGET_QUOT(thread->id, ro_sel),
		  fuse->quot - corner->quot_adjust);

	thread->corner = corner;

	corner->last_uV = corner->uV;
}

static void cpr_set_acc(struct cpr_drv* drv, int f)
{
	const struct acc_desc *desc = drv->acc_desc;
	struct reg_sequence *s = desc->settings;
	int n = desc->num_regs_per_fuse;

	if (!s || f == drv->fuse_level_set)
		return;

	regmap_multi_reg_write(drv->tcsr, s + (n * f), n);

	drv->fuse_level_set = f;
}

static int cpr_pre_voltage(struct cpr_drv *drv,
			   int fuse_level)
{
	if (drv->tcsr && (fuse_level < drv->fuse_level_set))
		cpr_set_acc(drv, fuse_level);

	return 0;
}

static int cpr_post_voltage(struct cpr_drv *drv,
			   int fuse_level)
{
	if (drv->tcsr && (fuse_level > drv->fuse_level_set))
		cpr_set_acc(drv, fuse_level);

	return 0;
}

static int cpr_aggregate_voltage(struct cpr_drv *drv)
{
	int ret, i;
	//struct fuse_corner *fuse_corner = corner->fuse_corner;
	int min_uV = 0, max_uV = 0, new_uV = 0, fuse_level = 0;
	enum voltage_change_dir dir;
	u32 next_irqmask = 0;

	for (i = 0; i < drv->num_threads; i++) {
		struct cpr_thread *thread = &drv->threads[i];

		if (!thread->corner)
			continue;

		fuse_level = max(fuse_level,
				 (int) (thread->corner->fuse_corner -
				 &thread->fuse_corners[0]));

		max_uV = max(max_uV, thread->corner->max_uV);
		min_uV = max(min_uV, thread->corner->min_uV);
		new_uV = max(new_uV, thread->corner->last_uV);

	}

	dev_dbg(drv->dev, "new uV: %d, last uV: %d\n", new_uV, drv->last_uV);

	if (new_uV > drv->last_uV)
		dir = UP;
	else if (new_uV < drv->last_uV)
		dir = DOWN;
	else
		goto out;

	ret = cpr_pre_voltage(drv, fuse_level);
	if (ret)
		return ret;

	dev_dbg(drv->dev, "setting voltage: %d\n", new_uV);

	if (new_uV > 1065000 || new_uV < 400000) {
		panic("Limit exceeded");
		return ret;
	}

	ret = regulator_set_voltage(drv->vdd_apc, new_uV, new_uV);
	if (ret) {
		dev_err_ratelimited(drv->dev, "failed to set apc voltage %d\n",
				    new_uV);
		return ret;
	}

	ret = cpr_post_voltage(drv, fuse_level);
	if (ret)
		return ret;

	drv->last_uV = new_uV;
out:
	if (new_uV > min_uV)
		next_irqmask |= CPR3_IRQ_DOWN;
	if (new_uV < max_uV)
		next_irqmask |= CPR3_IRQ_UP;

	cpr_irq_set(drv, next_irqmask);

	return 0;
}

static unsigned int cpr_get_cur_perf_state(struct cpr_thread *thread)
{
	return thread->corner ? thread->corner - thread->corners + 1 : 0;
}

static int cpr_scale(struct cpr_thread *thread, enum voltage_change_dir dir)
{
	struct cpr_drv *drv = thread->drv;
	u32 val;
	u32 error_steps;
	int last_uV, new_uV, step_uV;
	struct corner *corner;

	if (dir != UP && dir != DOWN)
		return 0;

	step_uV = drv->vdd_apc_step;
	if (!step_uV)
		return -EINVAL;

	corner = thread->corner;
	val = cpr_read(drv, CPR3_REG_RESULT0(thread->id));
	error_steps = val >> CPR3_RESULT0_ERROR_STEPS_SHIFT;
	error_steps &= CPR3_RESULT0_ERROR_STEPS_MASK;

	last_uV = corner->last_uV;

	if (dir == UP) {
		if (!(val & CPR3_RESULT0_STEP_UP_MASK))
			return 0;

		/* Calculate new voltage */
		new_uV = last_uV + step_uV;
		new_uV = min(new_uV, corner->max_uV);

		dev_dbg(drv->dev,
			"UP: -> new_uV: %d last_uV: %d perf state: %u thread: %u error steps: %u\n",
			new_uV, last_uV, cpr_get_cur_perf_state(thread), thread->id, error_steps);
	} else {
		if (!(val & CPR3_RESULT0_STEP_DN_MASK))
			return 0;

		/* Calculate new voltage */
		new_uV = last_uV - step_uV;
		new_uV = max(new_uV, corner->min_uV);
		dev_dbg(drv->dev,
			"DOWN: -> new_uV: %d last_uV: %d perf state: %u thread: %u error steps: %u\n",
			new_uV, last_uV, cpr_get_cur_perf_state(thread), thread->id, error_steps);
	}

	corner->last_uV = new_uV;

	return 0;
}

static irqreturn_t cpr_irq_handler(int irq, void *dev)
{
	struct cpr_drv *drv = dev;
	struct cpr_thread *thread;
	irqreturn_t ret = IRQ_HANDLED;
	int i, rc;
	enum voltage_change_dir dir = NO_CHANGE;
	u32 val;
	bool ack = false;

	mutex_lock(&drv->lock);

	val = cpr_read(drv, CPR3_REG_IRQ_STATUS);

	dev_dbg(drv->dev, "IRQ_STATUS = %#02x\n", val);

	if (!cpr_ctl_is_enabled(drv)) {
		dev_dbg(drv->dev, "CPR is disabled\n");
		ret = IRQ_NONE;
	} else if (cpr_check_threads_busy(drv)) {
		cpr_irq_clr_nack(drv);
		dev_dbg(drv->dev, "CPR measurement is not ready\n");
	} else {
		/*
		 * Following sequence of handling is as per each IRQ's
		 * priority
		 */
		if (val & CPR3_IRQ_UP)
			dir = UP;
		else if (val & CPR3_IRQ_DOWN)
			dir = DOWN;

		if (dir != NO_CHANGE) {
			for (i = 0; i < drv->num_threads; i++) {
				thread = &drv->threads[i];
				rc = cpr_scale(thread, dir);
				if (!rc)
					ack = true;
			}

			rc = cpr_aggregate_voltage(drv);
			if (rc || !ack) {
				cpr_irq_clr_nack(drv);
			} else {
				cpr_irq_clr_ack(drv);
			}
		} else if (val & CPR3_IRQ_MID) {
			dev_dbg(drv->dev, "IRQ occurred for Mid Flag\n");
		} else {
			dev_dbg(drv->dev,
				"IRQ occurred for unknown flag (%#08x)\n", val);
		}
	}

	mutex_unlock(&drv->lock);

	return ret;
}

static int cpr_enable(struct cpr_thread *thread)
{
	int ret;
	struct cpr_drv *drv = thread->drv;

	ret = regulator_enable(drv->vdd_apc);
	if (ret)
		return ret;

	mutex_lock(&drv->lock);

	thread->ena_count = clamp(thread->ena_count + 1, 0, drv->num_threads);

	if (thread->corner) {
		cpr_irq_clr(drv);
		cpr_corner_restore(thread, thread->corner);
		cpr_ctl_enable(drv);
	}

	mutex_unlock(&drv->lock);

	return 0;
}

static int cpr_disable(struct cpr_thread *thread)
{
	int ret;
	struct cpr_drv *drv = thread->drv;

	mutex_lock(&drv->lock);

	thread->ena_count = clamp(thread->ena_count - 1, 0, drv->num_threads);

	if (thread->ena_count) {
		mutex_unlock(&drv->lock);
		return 0;
	}

	cpr_ctl_disable(drv);
	cpr_irq_clr(drv);

	mutex_unlock(&drv->lock);

	ret = regulator_disable(drv->vdd_apc);
	if (ret)
		return ret;

	return 0;
}

static int cpr_configure(struct cpr_drv *drv)
{
	int i;
	u32 val;
	const struct cpr_desc *desc = drv->desc;

	/* Disable interrupt and CPR */
	cpr_write(drv, CPR3_REG_IRQ_EN, 0);
	cpr_write(drv, CPR3_REG_CPR_CTL, 0);

	/* Init and save gcnt */
	drv->gcnt = (drv->ref_clk_khz * desc->gcnt_us) / 1000;

	/* Program the delay count for the timer */
	val = (drv->ref_clk_khz * desc->timer_delay_us) / 1000;
	cpr_write(drv, CPR3_REG_CPR_TIMER_AUTO_CONT, val);
	dev_dbg(drv->dev, "Timer count: %#0x (for %d us)\n", val,
		desc->timer_delay_us);

	/* Program the control register */
	val = desc->idle_clocks << CPR3_CPR_CTL_IDLE_CLOCKS_SHIFT
	    | desc->count_mode << CPR3_CPR_CTL_COUNT_MODE_SHIFT
	    | desc->count_repeat << CPR3_CPR_CTL_COUNT_REPEAT_SHIFT;
	cpr_write(drv, CPR3_REG_CPR_CTL, val);

	/* Configure CPR default step quotients */
	val = desc->step_quot_init_min << CPR3_CPR_STEP_QUOT_MIN_SHIFT
	    | desc->step_quot_init_max << CPR3_CPR_STEP_QUOT_MAX_SHIFT;
	cpr_write(drv, CPR3_REG_CPR_STEP_QUOT, val);

	if (desc->version != 3 && desc->version != 4)
		return -ENODEV;

	for (i = 0; i < drv->num_threads; i++) {
		const struct cpr_thread_desc *tdesc = drv->threads[i].desc;
		int s;
		/* Configure the CPR sensor ownership */
		for (s = tdesc->sensor_range_start; s < tdesc->sensor_range_end; s++)
			cpr_write(drv, CPR3_REG_SENSOR_OWNER(s), i);

		/* Program Consecutive Up & Down */
		val = desc->timer_cons_up << CPR3_THRESH_CONS_UP_SHIFT;
		val |= desc->timer_cons_down << CPR3_THRESH_CONS_DOWN_SHIFT;
		val |= desc->up_threshold << CPR3_THRESH_UP_THRESH_SHIFT;
		val |= desc->down_threshold << CPR3_THRESH_DOWN_THRESH_SHIFT;
		cpr_write(drv, CPR3_REG_THRESH(i), val);
	}

	if (desc->version == 4) {
		/* Disable closed-loop */
		cpr_masked_write(drv, CPR4_REG_MARGIN_ADJ_CTL,
			CPR4_MARGIN_ADJ_HW_CLOSED_LOOP_EN_MASK,
			CPR4_MARGIN_ADJ_HW_CLOSED_LOOP_DISABLE);

		if (drv->num_threads == 1)
			/* Disable unused thread */
			cpr_masked_write(drv, CPR4_REG_CPR_MASK_THREAD(1),
					 CPR4_CPR_MASK_THREAD_DISABLE_THREAD |
					 CPR4_CPR_MASK_THREAD_RO_MASK4THREAD_MASK,
					 CPR4_CPR_MASK_THREAD_DISABLE_THREAD |
					 CPR4_CPR_MASK_THREAD_RO_MASK4THREAD_MASK);
		else if (drv->num_threads == 2)
			cpr_masked_write(drv, CPR4_REG_MISC,
					 CPR4_MISC_RESET_STEP_QUOT_LOOP_EN |
					 CPR4_MISC_THREAD_HAS_ALWAYS_VOTE_EN,
					 CPR4_MISC_RESET_STEP_QUOT_LOOP_EN |
					 CPR4_MISC_THREAD_HAS_ALWAYS_VOTE_EN);

		cpr_masked_write(drv, CPR4_REG_MARGIN_ADJ_CTL,
				 CPR4_MARGIN_ADJ_PMIC_STEP_SIZE_MASK <<
				 CPR4_MARGIN_ADJ_PMIC_STEP_SIZE_SHIFT,
				 1 << CPR4_MARGIN_ADJ_PMIC_STEP_SIZE_SHIFT);

		cpr_masked_write(drv, CPR4_REG_SAW_ERROR_STEP_LIMIT,
				 CPR4_SAW_ERROR_STEP_LIMIT_DN_MASK <<
				 CPR4_SAW_ERROR_STEP_LIMIT_DN_SHIFT,
				 drv->desc->vdd_apc_step_down_limit <<
				 CPR4_SAW_ERROR_STEP_LIMIT_DN_SHIFT);

		cpr_masked_write(drv, CPR4_REG_SAW_ERROR_STEP_LIMIT,
				 CPR4_SAW_ERROR_STEP_LIMIT_UP_MASK <<
				 CPR4_SAW_ERROR_STEP_LIMIT_UP_SHIFT,
				 drv->desc->vdd_apc_step_up_limit <<
				 CPR4_SAW_ERROR_STEP_LIMIT_UP_SHIFT);
		/* XXX: Do we want this?
		 * Enable thread aggregation regardless of which threads are
		 * enabled or disabled.
		 */
		cpr_masked_write(drv, CPR4_REG_CPR_TIMER_CLAMP,
				  CPR4_CPR_TIMER_CLAMP_THREAD_AGGREGATION_EN,
				  CPR4_CPR_TIMER_CLAMP_THREAD_AGGREGATION_EN);
	}

	return 0;
}

static int cpr_set_performance_state(struct generic_pm_domain *domain,
				     unsigned int state)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);
	struct cpr_drv *drv = thread->drv;
	struct corner *corner, *end;
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

	cpr_ctl_disable(drv);

	cpr_irq_clr(drv);
	if (thread->corner != corner)
		cpr_corner_restore(thread, corner);

	ret = cpr_aggregate_voltage(drv);
	if (ret)
		goto unlock;

	cpr_ctl_enable(drv);
unlock:
	mutex_unlock(&drv->lock);

	dev_dbg(drv->dev, "%s: set perf state: %u thread:%u\n", __func__, state, thread->id);

	return ret;
}

static int cpr_read_efuse(struct device *dev, const char *cname, u32 *data)
{
	struct nvmem_cell *cell;
	ssize_t len;
	char *ret;
	int i;

	*data = 0;

	cell = nvmem_cell_get(dev, cname);
	if (IS_ERR(cell)) {
		if (PTR_ERR(cell) != -EPROBE_DEFER)
			dev_err(dev, "undefined cell %s\n", cname);
		return PTR_ERR(cell);
	}

	ret = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR(ret)) {
		dev_err(dev, "can't read cell %s\n", cname);
		return PTR_ERR(ret);
	}

	for (i = 0; i < len; i++)
		*data |= ret[i] << (8 * i);

	kfree(ret);
	dev_dbg(dev, "efuse read(%s) = %x, bytes %zd\n", cname, *data, len);

	return 0;
}

static int
cpr_populate_ring_osc_idx(struct cpr_thread *thread)
{
	struct fuse_corner *fuse = thread->fuse_corners;
	struct fuse_corner *end = fuse + thread->desc->num_fuse_corners;
	const struct cpr_fuse *fuses = thread->cpr_fuses;
	u32 data;
	int ret;

	for (; fuse < end; fuse++, fuses++) {
		ret = cpr_read_efuse(thread->drv->dev, fuses->ring_osc,
				     &data);
		if (ret)
			return ret;
		fuse->ring_osc_idx = data;
	}

	return 0;
}

static int cpr_read_fuse_uV(const struct cpr_thread_desc *tdata,
			    const struct fuse_corner_data *fdata,
			    const char *init_v_efuse,
			    int step_volt,
			    struct cpr_drv *drv)
{
	int step_size_uV, steps, uV;
	u32 bits = 0;
	int ret;

	ret = cpr_read_efuse(drv->dev, init_v_efuse, &bits);
	if (ret)
		return ret;

	steps = bits & (BIT(tdata->init_voltage_width - 1) - 1);
	/* Not two's complement.. instead highest bit is sign bit */
	if (bits & BIT(tdata->init_voltage_width - 1))
		steps = -steps;

	step_size_uV = tdata->init_voltage_step;

	uV = fdata->ref_uV + steps * step_size_uV;
	return DIV_ROUND_UP(uV, step_volt) * step_volt;
}

static int cpr_fuse_corner_init(struct cpr_thread *thread)
{
	struct cpr_drv *drv = thread->drv;
	const struct cpr_thread_desc *desc = thread->desc;
	const struct cpr_fuse *fuses = thread->cpr_fuses;
	int i;
	unsigned int step_volt;
	struct fuse_corner_data *fdata;
	struct fuse_corner *fuse, *end;
	int uV;
	int ret;

	step_volt = drv->vdd_apc_step;
	if (!step_volt) {
		return -EINVAL;
	}

	/* Populate fuse_corner members */
	fuse = thread->fuse_corners;
	end = &fuse[desc->num_fuse_corners - 1];
	fdata = desc->fuse_corner_data;

	for (i = 0; fuse <= end; fuse++, fuses++, i++, fdata++) {
		/*
		 * Update SoC voltages: platforms might choose a different
		 * regulators than the one used to characterize the algorithms
		 * (ie, init_voltage_step).
		 */
		fdata->min_uV = roundup(fdata->min_uV, step_volt);
		fdata->max_uV = roundup(fdata->max_uV, step_volt);

		/* Populate uV */
		uV = cpr_read_fuse_uV(desc, fdata, fuses->init_voltage,
				      step_volt, drv);
		if (uV < 0)
			return uV;

		fuse->min_uV = fdata->min_uV;
		fuse->max_uV = fdata->max_uV;

		if (desc->voltage_adjust_func)
			uV += desc->voltage_adjust_func(drv->speed_bin, drv->fusing_rev, i);

		fuse->uV = clamp(uV, fuse->min_uV, fuse->max_uV);


		if (fuse == end) {
			/*
			 * Allow the highest fuse corner's PVS voltage to
			 * define the ceiling voltage for that corner in order
			 * to support SoC's in which variable ceiling values
			 * are required.
			 */
			// XXX: this has no effect since uV is
			// already clamped to max_uV above
			end->max_uV = max(end->max_uV, end->uV);
		}

		/* Populate target quotient by scaling */
		ret = cpr_read_efuse(drv->dev, fuses->quotient, &fuse->quot);
		if (ret)
			return ret;

		fuse->quot *= fdata->quot_scale;
		fuse->quot += fdata->quot_offset;
		fuse->quot += fdata->quot_adjust;

		if (desc->quot_adjust_func) {
			int64_t ro_scale = drv->desc->ro_scaling_factor[fuse->ring_osc_idx];
			int64_t adj = desc->quot_adjust_func(drv->speed_bin, drv->fusing_rev, i);

			adj *= ro_scale;
			adj /= 1000000;

			fuse->quot += adj;
		}


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
			"fuse corner %d: [%d %d %d] RO%hhu quot %d\n",
			i, fuse->min_uV, fuse->uV, fuse->max_uV,
			fuse->ring_osc_idx, fuse->quot);
	}

	return 0;
}

static int cpr_calculate_scaling(const char *quot_offset,
				 struct cpr_drv *drv,
				 const struct fuse_corner_data *fdata,
				 const struct corner *corner)
{
	u32 quot_diff = 0;
	unsigned long freq_diff;
	int scaling;
	const struct fuse_corner *fuse, *prev_fuse;
	int ret;

	fuse = corner->fuse_corner;
	prev_fuse = fuse - 1;

	if (quot_offset) {
		ret = cpr_read_efuse(drv->dev, quot_offset, &quot_diff);
		if (ret)
			return ret;

		quot_diff *= fdata->quot_offset_scale;
		quot_diff += fdata->quot_offset_adjust;
	} else {
		quot_diff = fuse->quot - prev_fuse->quot;
	}

	freq_diff = fuse->max_freq - prev_fuse->max_freq;
	freq_diff /= 1000000; /* Convert to MHz */
	scaling = 1000 * quot_diff / freq_diff;
	return min(scaling, fdata->max_quot_scale);
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
	const struct cpr_fuse *fuses = thread->cpr_fuses;
	int i, level, scaling = 0;
	unsigned int fnum;
	const char *quot_offset;
	struct fuse_corner *fuse, *prev_fuse;
	struct corner *corner, *end;
	struct corner_data *cdata;
	const struct fuse_corner_data *fdata;
	bool apply_scaling;
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

		if (desc->corner_freq_func)
			fuse->max_freq = desc->corner_freq_func(drv->speed_bin,
								drv->fusing_rev,
								level);
		dev_dbg(drv->dev, "max freq: %lu fuse level: %u\n",
			fuse->max_freq, level);
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
	 * Get the quotient adjustment scaling factor, according to:
	 *
	 * scaling = min(1000 * (QUOT(corner_N) - QUOT(corner_N-1))
	 *		/ (freq(corner_N) - freq(corner_N-1)), max_factor)
	 *
	 * QUOT(corner_N):	quotient read from fuse for fuse corner N
	 * QUOT(corner_N-1):	quotient read from fuse for fuse corner (N - 1)
	 * freq(corner_N):	max frequency in MHz supported by fuse corner N
	 * freq(corner_N-1):	max frequency in MHz supported by fuse corner
	 *			 (N - 1)
	 *
	 * Then walk through the corners mapped to each fuse corner
	 * and calculate the quotient adjustment for each one using the
	 * following formula:
	 *
	 * quot_adjust = (freq_max - freq_corner) * scaling / 1000
	 *
	 * freq_max: max frequency in MHz supported by the fuse corner
	 * freq_corner: frequency in MHz corresponding to the corner
	 * scaling: calculated from above equation
	 *
	 *
	 *     +                           +
	 *     |                         v |
	 *   q |           f c           o |           f c
	 *   u |         c               l |         c
	 *   o |       f                 t |       f
	 *   t |     c                   a |     c
	 *     | c f                     g | c f
	 *     |                         e |
	 *     +---------------            +----------------
	 *       0 1 2 3 4 5 6               0 1 2 3 4 5 6
	 *          corner                      corner
	 *
	 *    c = corner
	 *    f = fuse corner
	 *
	 */
	for (apply_scaling = false, i = 0; corner <= end; corner++, i++) {
		fnum = cdata[i].fuse_corner;
		fdata = &desc->fuse_corner_data[fnum];
		quot_offset = fuses[fnum].quotient_offset;
		fuse = &thread->fuse_corners[fnum];
		if (fnum)
			prev_fuse = &thread->fuse_corners[fnum - 1];
		else
			prev_fuse = NULL;

		corner->fuse_corner = fuse;
		corner->freq = cdata[i].freq;
		corner->uV = fuse->uV;

		if (prev_fuse) {
			scaling = cpr_calculate_scaling(quot_offset, drv,
							fdata, corner);
			if (scaling < 0)
				return scaling;

			apply_scaling = true;
		} else if (corner->freq == fuse->max_freq) {
			/* This is a fuse corner; don't scale anything */
			apply_scaling = false;
		}

		if (apply_scaling) {
			freq_diff = fuse->max_freq - corner->freq;
			freq_diff_mhz = freq_diff / 1000000;
			corner->quot_adjust = scaling * freq_diff_mhz / 1000;

			corner->uV = cpr_interpolate(corner, step_volt, fdata);
		}

		corner->max_uV = fuse->max_uV;
		corner->min_uV = fuse->min_uV;
		corner->uV = clamp(corner->uV, corner->min_uV, corner->max_uV);
		corner->last_uV = corner->uV;

		/* Reduce the ceiling voltage if needed */
		if (drv->desc->reduce_to_corner_uV && corner->uV < corner->max_uV)
			corner->max_uV = corner->uV;
		else if (drv->desc->reduce_to_fuse_uV && fuse->uV < corner->max_uV)
			corner->max_uV = max(corner->min_uV, fuse->uV);

		corner->min_uV = corner->max_uV - 50000;

		dev_dbg(drv->dev, "corner %d: [%d %d %d] scaling %d quot %d\n", i,
			corner->min_uV, corner->uV, corner->max_uV, scaling,
			fuse->quot - corner->quot_adjust);
	}

	return 0;
}

static const struct cpr_fuse *cpr_get_fuses(struct cpr_thread *thread)
{
	const struct cpr_thread_desc *desc = thread->desc;
	struct device *dev = thread->drv->dev;
	struct cpr_fuse *fuses;
	int i, id = thread->id;

	fuses = devm_kcalloc(dev, desc->num_fuse_corners,
			     sizeof(struct cpr_fuse),
			     GFP_KERNEL);
	if (!fuses)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < desc->num_fuse_corners; i++) {
		char tbuf[50];

		snprintf(tbuf, sizeof(tbuf), "cpr_thread%d_ring_osc%d", id, i + 1);
		fuses[i].ring_osc = devm_kstrdup(dev, tbuf, GFP_KERNEL);
		if (!fuses[i].ring_osc)
			return ERR_PTR(-ENOMEM);

		snprintf(tbuf, sizeof(tbuf), "cpr_thread%d_init_voltage%d", id, i + 1);
		fuses[i].init_voltage = devm_kstrdup(dev, tbuf,
						     GFP_KERNEL);
		if (!fuses[i].init_voltage)
			return ERR_PTR(-ENOMEM);

		snprintf(tbuf, sizeof(tbuf), "cpr_thread%d_quotient%d", id, i + 1);
		fuses[i].quotient = devm_kstrdup(dev, tbuf, GFP_KERNEL);
		if (!fuses[i].quotient)
			return ERR_PTR(-ENOMEM);

		snprintf(tbuf, sizeof(tbuf), "cpr_thread%d_quotient_offset%d", id, i + 1);
		fuses[i].quotient_offset = devm_kstrdup(dev, tbuf,
							GFP_KERNEL);
		if (!fuses[i].quotient_offset)
			return ERR_PTR(-ENOMEM);
	}

	return fuses;
}

static int cpr_init_parameters(struct cpr_drv *drv)
{
	const struct cpr_desc *desc = drv->desc;
	struct clk *clk;

	clk = clk_get(drv->dev, "ref");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	drv->ref_clk_khz = clk_get_rate(clk) / 1000;
	clk_put(clk);

	if (desc->timer_cons_up > CPR3_THRESH_CONS_UP_MASK ||
	    desc->timer_cons_down > CPR3_THRESH_CONS_DOWN_MASK ||
	    desc->up_threshold > CPR3_THRESH_UP_THRESH_MASK ||
	    desc->down_threshold > CPR3_THRESH_DOWN_THRESH_MASK ||
	    desc->idle_clocks > CPR3_CPR_CTL_IDLE_CLOCKS_MASK ||
	    desc->count_mode > CPR3_CPR_CTL_COUNT_MODE_MASK ||
	    desc->count_repeat > CPR3_CPR_CTL_COUNT_REPEAT_MASK ||
	    desc->step_quot_init_min > CPR3_CPR_STEP_QUOT_MIN_MASK ||
	    desc->step_quot_init_max > CPR3_CPR_STEP_QUOT_MAX_MASK)
		return -EINVAL;

	dev_dbg(drv->dev, "up threshold = %u, down threshold = %u\n",
		desc->up_threshold, desc->down_threshold);

	return 0;
}

static int cpr_find_initial_corner(struct cpr_thread *thread)
{
	struct cpr_drv *drv = thread->drv;
	unsigned long rate;
	int uV;
	const struct corner *end;
	struct corner *iter, *corner;
	unsigned int i = 0;

	if (!thread->cpu_clk) {
		dev_err(drv->dev, "cannot get rate from NULL clk\n");
		return -EINVAL;
	}

	// TODO
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

	cpr_corner_restore(thread, corner);

	uV = regulator_get_voltage(drv->vdd_apc);
	uV = clamp(uV, corner->min_uV, corner->max_uV);

	corner->last_uV = uV;
	if (!drv->last_uV)
		drv->last_uV = uV;

	cpr_aggregate_voltage(drv);
	cpr_ctl_enable(drv);

	return 0;
}

int64_t cpr_msm8953_quot_adjust(u16 speed_bin, u16 rev, int corner)
{
	switch (speed_bin) {
		case 0: case 2: case 6: case 7: break;
		default: return 0;
	}

	switch (rev) {
		case 1 ... 2:
			switch (corner) {
				case 0: return 10000;
				case 1: return -15000;
				case 3: return 25000;
				default: return 0;
			}
		case 3:
			switch (corner) {
				case 0: return -5000;
				case 1: return -30000;
				case 2: return -15000;
				case 3: return 10000;
				default: return 0;
			}
	}
	return 0;
}

int64_t cpr_msm8953_voltage_adjust(u16 speed_bin, u16 rev, int corner)
{
	switch (speed_bin) {
		case 0: case 2: case 6: case 7: break;
		default: return 0;
	}

	switch (rev) {
		case 1 ... 3: break;
		default: return 0;
	}

	switch (corner) {
		case 0: return 25000;
		case 2: return 5000;
		case 3: return 40000;
	}
	return 0;
}

int64_t cpr_msm8953_corner_freq(u16 speed_bin, u16 rev, int corner)
{
	switch (corner) {
		case 0: return 652800000;
		case 1: return 1036800000;
		case 2: return 1689600000;
		case 3: break;
		default: return 0;
	}

	switch (speed_bin) {
		case 2: case 6: return 2016000000;
		case 0: case 7: return 2208000000;
	}
	return 0;
}

static const struct cpr_desc msm8953_cpr_desc = {
	.num_threads = 1,
	.ro_scaling_factor = (int []){
		3610, 3790,    0, 2200,
		2450, 2310, 2170, 2210,
		2330, 2210, 2470, 2340,
		 780, 2700, 2450, 2090,
	},
	.timer_delay_us = 5000,
	.timer_cons_up = 0,
	.timer_cons_down = 2,
	.version = 4,
	.up_threshold = 2,
	.down_threshold = 1,
	.idle_clocks = 15,
	.count_mode = 0,
	.count_repeat = 14,
	.step_quot_init_min = 12,
	.step_quot_init_max = 14,
	.gcnt_us = 1,
	.vdd_apc_step_up_limit = 1,
	.vdd_apc_step_down_limit = 1,
	.reduce_to_corner_uV = true,
	.threads = (struct cpr_thread_desc[]) {
		{
			.sensor_range_start = 0,
			.sensor_range_end = 13,
			.num_fuse_corners = 4,
			.corner_freq_func = cpr_msm8953_corner_freq,
			.quot_adjust_func = cpr_msm8953_quot_adjust,
			.voltage_adjust_func = cpr_msm8953_voltage_adjust,
			.init_voltage_step = 10000,
			.init_voltage_width = 6,
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 645000 + 31 * 10000,
					.min_uV = 400000,
					.max_volt_scale = 0,
					.max_quot_scale = 0,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 1 */
					.ref_uV = 720000,
					.max_uV = 720000 + 31 * 10000,
					.min_uV = 720000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 1065000,
					.min_uV = 865000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 1065000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
			},
		},
	},
};

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

static const struct cpr_acc_desc msm8953_cpr_acc_desc = {
	.cpr_desc = &msm8953_cpr_desc,
	.acc_desc = &msm8953_acc_desc,
};


int64_t cpr_sdm632_pwr_quot_adjust(u16 speed_bin, u16 rev, int corner)
{
	switch (corner) {
		case 0: return -10000;
		case 3: return 10000;
	}
	return 0;
}

int64_t cpr_sdm632_perf_quot_adjust(u16 speed_bin, u16 rev, int corner)
{
	switch (speed_bin) {
		case 0: case 2: case 6: break;
		default: return 0;
	}

	if (corner != 0)
		return 0;

	switch (rev) {
		case 0 ... 1: return 30000;
		case 2: return -10000;
	}
	return 0;
}

int64_t cpr_sdm632_pwr_voltage_adjust(u16 speed_bin, u16 rev, int corner)
{
	if (corner == 3)
		return 10000;

	return 0;
}

int64_t cpr_sdm632_perf_voltage_adjust(u16 speed_bin, u16 rev, int corner)
{
	switch (speed_bin) {
		case 0: case 2: case 6: break;
		default: return 0;
	}

	if (rev > 2)
		return 0;

	switch (corner) {
		case 0: return rev < 2 ? 30000 : 0;
		case 2: return 10000;
		case 3: return 20000;
	}
	return 0;
}

int64_t cpr_sdm632_pwr_corner_freq(u16 speed_bin, u16 rev, int corner)
{
	switch (corner) {
		case 0: return 614400000;
		case 1: return 1036800000;
		case 2: return 1363200000;
		case 4: return 1804800000;
	}
	return 0;
}

int64_t cpr_sdm632_perf_corner_freq(u16 speed_bin, u16 rev, int corner)
{
	switch (corner) {
		case 0: return 633600000;
		case 1: return 1094400000;
		case 2: return 1401600000;
		case 4: return 2016000000;
	}
	return 0;
}

static const struct cpr_desc sdm632_cpr_desc = {
	.num_threads = 2,
	.ro_scaling_factor = (int []){
		3600, 3600, 3830, 2430,
		2520, 2700, 1790, 1760,
		1970, 1880, 2110, 2010,
		2510, 4900, 4370, 4780,
	},
	.timer_delay_us = 5000,
	.timer_cons_up = 0,
	.timer_cons_down = 2,
	.version = 4,
	.up_threshold = 2,
	.down_threshold = 1,
	.idle_clocks = 15,
	.count_mode = 0,
	.count_repeat = 14,
	.step_quot_init_min = 12,
	.step_quot_init_max = 14,
	.gcnt_us = 1,
	.vdd_apc_step_up_limit = 1,
	.vdd_apc_step_down_limit = 1,
	.reduce_to_corner_uV = true,
	.threads = (struct cpr_thread_desc[]) {
		{
			.sensor_range_start = 0,
			.sensor_range_end = 7,
			.num_fuse_corners = 4,
			.corner_freq_func = cpr_sdm632_pwr_corner_freq,
			.quot_adjust_func = cpr_sdm632_pwr_quot_adjust,
			.voltage_adjust_func = cpr_sdm632_pwr_voltage_adjust,
			.init_voltage_step = 10000,
			.init_voltage_width = 6,
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 645000 + 31 * 10000,
					.min_uV = 400000,
					.max_volt_scale = 0,
					.max_quot_scale = 0,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 1 */
					.ref_uV = 790000,
					.max_uV = 790000 + 31 * 10000,
					.min_uV = 790000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = -20,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 1065000,
					.min_uV = 865000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 1065000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
			},
		},
		{
			.sensor_range_start = 7,
			.sensor_range_end = 16,
			.num_fuse_corners = 4,
			.corner_freq_func = cpr_sdm632_perf_corner_freq,
			.quot_adjust_func = cpr_sdm632_perf_quot_adjust,
			.voltage_adjust_func = cpr_sdm632_perf_voltage_adjust,
			.init_voltage_step = 10000,
			.init_voltage_width = 6,
			.fuse_corner_data = (struct fuse_corner_data[]) {
				{	/* fuse corner 0 */
					.ref_uV = 645000,
					.max_uV = 645000 + 31 * 10000,
					.min_uV = 400000,
					.max_volt_scale = 0,
					.max_quot_scale = 0,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 1 */
					.ref_uV = 790000,
					.max_uV = 790000 + 31 * 10000,
					.min_uV = 790000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 2 */
					.ref_uV = 865000,
					.max_uV = 1065000,
					.min_uV = 865000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
				},
				{	/* fuse corner 3 */
					.ref_uV = 1065000,
					.max_uV = 1065000,
					.min_uV = 1065000 - 31 * 10000,
					.max_volt_scale = 2000,
					.max_quot_scale = 1400,
					.quot_offset = 0,
					.quot_scale = 1,
					.quot_adjust = 0,
					.quot_offset_scale = 5,
					.quot_offset_adjust = 0,
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

static const struct cpr_acc_desc sdm632_cpr_acc_desc = {
	.cpr_desc = &sdm632_cpr_desc,
	.acc_desc = &sdm632_acc_desc,
};

static unsigned int cpr_get_performance_state(struct generic_pm_domain *genpd,
					      struct dev_pm_opp *opp)
{
	return dev_pm_opp_get_level(opp);
}

static int cpr_power_off(struct generic_pm_domain *domain)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);

	return cpr_disable(thread);
}

static int cpr_power_on(struct generic_pm_domain *domain)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);

	return cpr_enable(thread);
}

static int cpr_pd_attach_dev(struct generic_pm_domain *domain,
			     struct device *dev)
{
	struct cpr_thread *thread = container_of(domain, struct cpr_thread, pd);
	struct cpr_drv *drv = thread->drv;
	const struct acc_desc *acc_desc = drv->acc_desc;
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

	dev_dbg(drv->dev, "corners: %d\n", ret);

	if (thread->num_corners < 2) {
		dev_err(drv->dev, "need at least 2 OPPs to use CPR\n");
		ret = -EINVAL;
		goto unlock;
	}

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

	ret = cpr_find_initial_corner(thread);
	if (ret)
		goto unlock;

	if (acc_desc->config)
		regmap_multi_reg_write(drv->tcsr, acc_desc->config,
				       acc_desc->num_regs_per_fuse);

	/* Enable ACC if required */
	if (acc_desc->enable_mask)
		regmap_update_bits(drv->tcsr, acc_desc->enable_reg,
				   acc_desc->enable_mask,
				   acc_desc->enable_mask);

	dev_info(drv->dev, "thread initialized with %u OPPs\n",
		 thread->num_corners);

unlock:
	mutex_unlock(&drv->lock);

	return ret;
}

static int cpr_debug_thread_show(struct seq_file *s, void *unused)
{
	struct cpr_thread *thread = s->private;
	struct fuse_corner *fuse = NULL;
	struct corner *corner, *end;

	seq_printf(s, "ena_count = %d\n", thread->ena_count);
	seq_printf(s, "corners = %d\n", thread->num_corners);

	end = &thread->corners[thread->num_corners - 1];

	for (corner = thread->corners; corner <= end; corner++) {
		if (fuse != corner->fuse_corner) {
			fuse = corner->fuse_corner;
			seq_printf(s, "fuse corner min=%d max=%d uv=%d quot=%d freq=%lu ro=%d\n",
					fuse->min_uV,
					fuse->max_uV,
					fuse->uV,
					fuse->quot,
					fuse->max_freq,
					fuse->ring_osc_idx);
		}

		if (corner == thread->corner)
			seq_printf(s, "current ");

		seq_printf(s, "corner min=%d max=%d uv=%d last=%d quot_adjust=%d freq=%lu\n",
					corner->min_uV,
					corner->max_uV,
					corner->uV,
					corner->last_uV,
					corner->quot_adjust,
					corner->freq);
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cpr_debug_thread);

static int cpr_debug_info_show(struct seq_file *s, void *unused)
{
	u32 ro_sel, ctl, irq_status, reg, quot;
	struct cpr_drv *drv = s->private;
	struct fuse_corner *fuse_corner;
	struct corner *corner;
	int i;

	const struct {
		const char *name;
		uint32_t mask;
		uint8_t shift;
	} result0_fields[] = {
		{ "busy", 1, 0 },
		{ "step_dn", 1, 1 },
		{ "step_up", 1, 2 },
		{ "error_steps", CPR3_RESULT0_ERROR_STEPS_MASK, CPR3_RESULT0_ERROR_STEPS_SHIFT },
		{ "error", CPR3_RESULT0_ERROR_MASK	, CPR3_RESULT0_ERROR_SHIFT },
		{ "negative", 1, 20 },
	}, result1_fields[] = {
		{ "quot_min", CPR3_RESULT1_QUOT_MIN_MASK, CPR3_RESULT1_QUOT_MIN_SHIFT },
		{ "quot_max", CPR3_RESULT1_QUOT_MAX_MASK, CPR3_RESULT1_QUOT_MAX_SHIFT },
		{ "ro_min", CPR3_RESULT1_RO_MIN_MASK, CPR3_RESULT1_RO_MIN_SHIFT },
		{ "ro_max", CPR3_RESULT1_RO_MAX_MASK, CPR3_RESULT1_RO_MAX_SHIFT },
	}, result2_fields[] = {
		{ "qout_step_min", CPR3_RESULT2_STEP_QUOT_MIN_MASK, CPR3_RESULT2_STEP_QUOT_MIN_SHIFT },
		{ "qout_step_max", CPR3_RESULT2_STEP_QUOT_MAX_MASK, CPR3_RESULT2_STEP_QUOT_MAX_SHIFT },
		{ "sensor_min", CPR3_RESULT2_SENSOR_MIN_MASK, CPR3_RESULT2_SENSOR_MIN_SHIFT },
		{ "sensor_max", CPR3_RESULT2_SENSOR_MAX_MASK, CPR3_RESULT2_SENSOR_MAX_SHIFT },
	};

	seq_printf(s, "current_volt = %d uV\n", drv->last_uV);

	irq_status = cpr_read(drv, CPR3_REG_IRQ_STATUS);
	seq_printf(s, "irq_status = %#02X\n", irq_status);

	ctl = cpr_read(drv, CPR3_REG_CPR_CTL);
	seq_printf(s, "cpr_ctl = %#02X\n", ctl);


	for (i = 0; i < drv->num_threads; i ++) {
		struct cpr_thread *thread = &drv->threads[i];
		int field = 0;

		corner = thread->corner;
		if (!corner)
			continue;

		fuse_corner = corner->fuse_corner;

		seq_printf(s, "thread %d:\n", i);
		seq_printf(s, "requested voltage: %d uV\n", corner->last_uV);

		ro_sel = fuse_corner->ring_osc_idx;
		quot = cpr_read(drv, CPR3_REG_TARGET_QUOT(i, ro_sel));
		seq_printf(s, "quot_target (%u) = %#02X\n", ro_sel, quot);

		reg = cpr_read(drv, CPR3_REG_RESULT0(i));
		seq_printf(s, "cpr_result_0 = %#02X\n  [", reg);
		for (field = 0; field < ARRAY_SIZE(result0_fields); field++)
			seq_printf(s, "%s%s = %u",
				   field ? ", ": "",
				   result0_fields[field].name,
				   (reg >> result0_fields[field].shift) &
					result0_fields[field].mask);
		seq_printf(s, "]\n");
		reg = cpr_read(drv, CPR3_REG_RESULT1(i));
		seq_printf(s, "cpr_result_1 = %#02X\n  [", reg);
		for (field = 0; field < ARRAY_SIZE(result1_fields); field++)
			seq_printf(s, "%s%s = %u",
				   field ? ", ": "",
				   result1_fields[field].name,
				   (reg >> result1_fields[field].shift) &
					result1_fields[field].mask);
		seq_printf(s, "]\n");
		reg = cpr_read(drv, CPR3_REG_RESULT2(i));
		seq_printf(s, "cpr_result_2 = %#02X\n  [", reg);
		for (field = 0; field < ARRAY_SIZE(result2_fields); field++)
			seq_printf(s, "%s%s = %u",
				   field ? ", ": "",
				   result2_fields[field].name,
				   (reg >> result2_fields[field].shift) &
					result2_fields[field].mask);
		seq_printf(s, "]\n");
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cpr_debug_info);

static void cpr_debugfs_init(struct cpr_drv *drv)
{
	int i;

	drv->debugfs = debugfs_create_dir("qcom_cpr", NULL);

	debugfs_create_file("debug_info", 0444, drv->debugfs,
			    drv, &cpr_debug_info_fops);

	for (i = 0; i < drv->num_threads; i++) {
		char buf[50];

		snprintf(buf, sizeof(buf), "thread%d", i);

		debugfs_create_file(buf, 0444, drv->debugfs,
			    &drv->threads[i], &cpr_debug_thread_fops);
	}
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

		thread->cpr_fuses = cpr_get_fuses(thread);
		if (IS_ERR(thread->cpr_fuses))
			return PTR_ERR(thread->cpr_fuses);

		ret = cpr_populate_ring_osc_idx(thread);
		if (ret)
			return ret;

		ret = cpr_fuse_corner_init(thread);
		if (ret)
			return ret;

		thread->pd.name = devm_kasprintf(drv->dev, GFP_KERNEL,
						  "%s_%d",
						  drv->dev->of_node->full_name,
						  thread->id);
		if (!thread->pd.name)
			return -EINVAL;

		thread->pd.power_off = cpr_power_off;
		thread->pd.power_on = cpr_power_on;
		thread->pd.set_performance_state = cpr_set_performance_state;
		thread->pd.opp_to_performance_state = cpr_get_performance_state;
		thread->pd.attach_dev = cpr_pd_attach_dev;

		drv->cell_data.domains[i] = &thread->pd;

		ret = pm_genpd_init(&thread->pd, NULL, true);
		if (ret)
			return ret;
	}

	return 0;
}

static int cpr_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct cpr_drv *drv;
	int irq, ret;
	const struct cpr_acc_desc *data;
	struct device_node *np;

	data = of_device_get_match_data(dev);
	if (!data || !data->cpr_desc || !data->acc_desc)
		return -EINVAL;

	drv = devm_kzalloc(dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;
	drv->dev = dev;
	drv->desc = data->cpr_desc;
	drv->acc_desc = data->acc_desc;

	mutex_init(&drv->lock);

	np = of_parse_phandle(dev->of_node, "acc-syscon", 0);
	if (!np)
		return -ENODEV;

	drv->tcsr = syscon_node_to_regmap(np);
	of_node_put(np);
	if (IS_ERR(drv->tcsr))
		return PTR_ERR(drv->tcsr);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	drv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(drv->base))
		return PTR_ERR(drv->base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -EINVAL;

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
	ret = cpr_read_efuse(dev, "cpr_fuse_revision", &drv->fusing_rev);
	if (ret)
		return ret;

	ret = cpr_read_efuse(dev, "cpr_speed_bin", &drv->speed_bin);
	if (ret)
		return ret;

	ret = cpr_threads_init(drv);
	if (ret)
		return ret;

	ret = cpr_init_parameters(drv);
	if (ret)
		return ret;

	/* Configure CPR HW but keep it disabled */
	ret = cpr_configure(drv);
	if (ret)
		return ret;

	ret = devm_request_threaded_irq(dev, irq, NULL,
					cpr_irq_handler,
					IRQF_ONESHOT | IRQF_TRIGGER_RISING,
					"cpr", drv);
	if (ret)
		return ret;

	ret = of_genpd_add_provider_onecell(dev->of_node, &drv->cell_data);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, drv);
	cpr_debugfs_init(drv);

	return 0;
}

static int cpr_remove(struct platform_device *pdev)
{
	struct cpr_drv *drv = platform_get_drvdata(pdev);
	int i;

	cpr_ctl_disable(drv);
	cpr_irq_set(drv, 0);

	of_genpd_del_provider(pdev->dev.of_node);
	for (i = 0; i < drv->num_threads; i++)
		pm_genpd_remove(&drv->threads[i].pd);

	debugfs_remove_recursive(drv->debugfs);

	return 0;
}

static const struct of_device_id cpr_match_table[] = {
	{ .compatible = "qcom,msm8953-cpr4", .data = &msm8953_cpr_acc_desc },
	{ .compatible = "qcom,sdm632-cpr4", .data = &sdm632_cpr_acc_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, cpr_match_table);

static struct platform_driver cpr_driver = {
	.probe		= cpr_probe,
	.remove		= cpr_remove,
	.driver		= {
		.name	= "qcom-cpr",
		.of_match_table = cpr_match_table,
	},
};
module_platform_driver(cpr_driver);

MODULE_DESCRIPTION("Core Power Reduction (CPR) v3 driver");
MODULE_LICENSE("GPL v2");
