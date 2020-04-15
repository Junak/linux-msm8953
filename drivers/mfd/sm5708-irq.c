/*
 * sm5708-irq.c - Interrupt controller support for sm5708
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * This driver is based on sm5708-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/sm5708.h>
/* #include <plat/gpio-cfg.h> */

#define SM5708_IRQ_OFFSET_NR 4

static const u8 sm5708_mask_reg[] = {
	SM5708_REG_INTMSK1,
	SM5708_REG_INTMSK2,
	SM5708_REG_INTMSK3,
	SM5708_REG_INTMSK4,
};

enum SM5708_IRQ_OFFSET {
	SM5708_INT1_OFFSET = 0,
	SM5708_INT2_OFFSET,
	SM5708_INT3_OFFSET,
	SM5708_INT4_OFFSET,
};

struct sm5708_irq_data {
	int mask;
	int offset;
};

#define DECLARE_IRQ(idx, _offset, _mask)		\
	[(idx)] = { .offset = (_offset), .mask = (_mask) }

static const struct sm5708_irq_data sm5708_irqs[] = {
	DECLARE_IRQ(SM5708_VBUSPOK_IRQ, SM5708_INT1_OFFSET, 1 << 0),
	DECLARE_IRQ(SM5708_VBUSUVLO_IRQ, SM5708_INT1_OFFSET, 1 << 1),
	DECLARE_IRQ(SM5708_VBUSOVP_IRQ, SM5708_INT1_OFFSET, 1 << 2),
	DECLARE_IRQ(SM5708_VBUSLIMIT_IRQ, SM5708_INT1_OFFSET, 1 << 3),

	DECLARE_IRQ(SM5708_AICL_IRQ, SM5708_INT2_OFFSET, 1 << 0),
	DECLARE_IRQ(SM5708_BATOVP_IRQ, SM5708_INT2_OFFSET, 1 << 1),
	DECLARE_IRQ(SM5708_NOBAT_IRQ, SM5708_INT2_OFFSET, 1 << 2),
	DECLARE_IRQ(SM5708_CHGON_IRQ, SM5708_INT2_OFFSET, 1 << 3),
	DECLARE_IRQ(SM5708_Q4FULLON_IRQ, SM5708_INT2_OFFSET, 1 << 4),
	DECLARE_IRQ(SM5708_TOPOFF_IRQ, SM5708_INT2_OFFSET, 1 << 5),
	DECLARE_IRQ(SM5708_DONE_IRQ, SM5708_INT2_OFFSET, 1 << 6),
	DECLARE_IRQ(SM5708_WDTMROFF_IRQ, SM5708_INT2_OFFSET, 1 << 7),

	DECLARE_IRQ(SM5708_THEMREG_IRQ, SM5708_INT3_OFFSET, 1 << 0),
	DECLARE_IRQ(SM5708_THEMSHDN_IRQ, SM5708_INT3_OFFSET, 1 << 1),
	DECLARE_IRQ(SM5708_OTGFAIL_IRQ, SM5708_INT3_OFFSET, 1 << 2),
	DECLARE_IRQ(SM5708_DISLIMIT_IRQ, SM5708_INT3_OFFSET, 1 << 3),
	DECLARE_IRQ(SM5708_PRETMROFF_IRQ, SM5708_INT3_OFFSET, 1 << 4),
	DECLARE_IRQ(SM5708_FASTTMROFF_IRQ, SM5708_INT3_OFFSET, 1 << 5),
	DECLARE_IRQ(SM5708_LOWBATT_IRQ, SM5708_INT3_OFFSET, 1 << 6),
	DECLARE_IRQ(SM5708_nENQ4_IRQ, SM5708_INT3_OFFSET, 1 << 7),

	DECLARE_IRQ(SM5708_FLED1SHORT_IRQ, SM5708_INT4_OFFSET, 1 << 0),
	DECLARE_IRQ(SM5708_FLED1OPEN_IRQ, SM5708_INT4_OFFSET, 1 << 1),
	DECLARE_IRQ(SM5708_FLED2SHORT_IRQ, SM5708_INT4_OFFSET, 1 << 2),
	DECLARE_IRQ(SM5708_FLED2OPEN_IRQ, SM5708_INT4_OFFSET, 1 << 3),
	DECLARE_IRQ(SM5708_BOOSTPOK_NG_IRQ, SM5708_INT4_OFFSET, 1 << 4),
	DECLARE_IRQ(SM5708_BOOSTPOK_IRQ, SM5708_INT4_OFFSET, 1 << 5),
	DECLARE_IRQ(SM5708_ABSTMR1OFF_IRQ, SM5708_INT4_OFFSET, 1 << 6),
	DECLARE_IRQ(SM5708_SBPS_IRQ, SM5708_INT4_OFFSET, 1 << 7),
};

static void sm5708_irq_lock(struct irq_data *data)
{
	struct sm5708_dev *sm5708 = irq_get_chip_data(data->irq);

	mutex_lock(&sm5708->irqlock);
}

static void sm5708_irq_sync_unlock(struct irq_data *data)
{
	struct sm5708_dev *sm5708 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < SM5708_IRQ_OFFSET_NR; i++) {
		u8 mask_reg = sm5708_mask_reg[i];

		if (mask_reg == SM5708_REG_INVALID || IS_ERR_OR_NULL(sm5708->i2c))
			continue;
		sm5708->irq_masks_cache[i] = sm5708->irq_masks_cur[i];

		sm5708_write_reg(sm5708->i2c, sm5708_mask_reg[i], sm5708->irq_masks_cur[i]);
	}

	mutex_unlock(&sm5708->irqlock);
}

static const inline struct sm5708_irq_data *irq_to_sm5708_irq(struct sm5708_dev *sm5708, int irq)
{
	return &sm5708_irqs[irq - sm5708->irq_base];
}

static void sm5708_irq_mask(struct irq_data *data)
{
	struct sm5708_dev *sm5708 = irq_get_chip_data(data->irq);
	const struct sm5708_irq_data *irq_data = irq_to_sm5708_irq(sm5708, data->irq);

	sm5708->irq_masks_cur[irq_data->offset] |= irq_data->mask;
}

static void sm5708_irq_unmask(struct irq_data *data)
{
	struct sm5708_dev *sm5708 = irq_get_chip_data(data->irq);
	const struct sm5708_irq_data *irq_data = irq_to_sm5708_irq(sm5708, data->irq);

	sm5708->irq_masks_cur[irq_data->offset] &= ~irq_data->mask;
}

static struct irq_chip sm5708_irq_chip = {
	.name					= MFD_DEV_NAME,
	.irq_bus_lock			= sm5708_irq_lock,
	.irq_bus_sync_unlock	= sm5708_irq_sync_unlock,
	.irq_mask				= sm5708_irq_mask,
	.irq_unmask				= sm5708_irq_unmask,
};

static int sm5708_read_irq_status(struct sm5708_dev *sm5708)
{
	int ret;
	struct i2c_client *iic = sm5708->i2c;

	ret = sm5708_read_reg(iic, SM5708_REG_INT1,	&sm5708->irq_status[0]);
	if (ret < 0) {
		pr_err("Failed on reading irq1 status\n");
		return ret;
	}

/*	printk("sm5708 irq1 = 0x%x\n", (int)sm5708->irq_status[0]); */

	ret = sm5708_read_reg(iic, SM5708_REG_INT2,	&sm5708->irq_status[1]);
	if (ret < 0) {
		pr_err("Failed on reading irq2 status\n");
		return ret;
	}
/*	printk("sm5708 irq2 = 0x%x\n", (int)sm5708->irq_status[1]); */

	ret = sm5708_read_reg(iic, SM5708_REG_INT3,	&sm5708->irq_status[2]);
	if (ret < 0) {
		pr_err("Failed on reading irq3 status\n");
		return ret;
	}
/*	printk("sm5708 irq3 = 0x%x\n", (int)sm5708->irq_status[2]); */

	ret = sm5708_read_reg(iic, SM5708_REG_INT4,	&sm5708->irq_status[3]);
	if (ret < 0) {
		pr_err("Failed on reading irq4 status\n");
		return ret;
	}
/*	printk("sm5708 irq4 = 0x%x\n", (int)sm5708->irq_status[3]); */

	return 0;
}

static irqreturn_t sm5708_irq_thread(int irq, void *data)
{
	struct sm5708_dev *sm5708 = data;
	int i, ret;

	pr_info("%s: irq gpio pre-state(0x%02x)\n", __func__, gpio_get_value(sm5708->irq_gpio));

	ret = sm5708_read_irq_status(sm5708);
	if (ret < 0) {
		pr_err("%s :Error : can't read irq status (%d)\n", __func__, ret);
		return ret;
	}

	/* Apply masking */
	for (i = 0; i < SM5708_IRQ_OFFSET_NR; i++)
		sm5708->irq_status[i] &= ~sm5708->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < SM5708_MAX_IRQ; i++) {
		if (sm5708->irq_status[sm5708_irqs[i].offset] & sm5708_irqs[i].mask)
			handle_nested_irq(sm5708->irq_base + i);
	}

	return IRQ_HANDLED;
}

static uint8_t sm5708_irqs_ctrl_mask_all_val[] = {
	0xff,
	0x9a, /* DONEM, TOPOFFM, NOBATM, AICLM */
	0xff,
	0xff,
};

static int sm5708_mask_all_irqs(struct sm5708_dev *sm5708)
{
	int rc;
	int i;

	for (i = 0; i < ARRAY_SIZE(sm5708_mask_reg); i++) {
		rc = sm5708_write_reg(sm5708->i2c, sm5708_mask_reg[i],	sm5708_irqs_ctrl_mask_all_val[i]);
		sm5708->irq_masks_cache[i] = sm5708_irqs_ctrl_mask_all_val[i];
		if (rc < 0) {
			pr_info("Error : can't write reg[0x%x] = 0x%x\n", sm5708_mask_reg[i],
				sm5708_irqs_ctrl_mask_all_val[i]);
			return rc;
		}
	}

	return 0;
}


int sm5708_irq_init(struct sm5708_dev *sm5708)
{
	int i;
	int ret;

	ret = sm5708_mask_all_irqs(sm5708);

	if (!sm5708->irq_gpio) {
		dev_warn(sm5708->dev, "No interrupt specified.\n");
		sm5708->irq_base = 0;
		return 0;
	}

	if (!sm5708->irq_base) {
		dev_err(sm5708->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&sm5708->irqlock);

	sm5708->irq = gpio_to_irq(sm5708->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__, sm5708->irq, sm5708->irq_gpio);

	ret = gpio_request(sm5708->irq_gpio, "sm5708_mfd_irq");
	if (ret) {
		dev_err(sm5708->dev, "%s: failed requesting gpio %d\n", __func__, sm5708->irq_gpio);
		return ret;
	}
	gpio_direction_input(sm5708->irq_gpio);
	gpio_free(sm5708->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < SM5708_IRQ_OFFSET_NR; i++) {
		sm5708->irq_masks_cur[i] = 0xff;
		sm5708->irq_masks_cache[i] = 0xff;

		if (IS_ERR_OR_NULL(sm5708->i2c))
			continue;
		if (sm5708_mask_reg[i] == SM5708_REG_INVALID)
			continue;

		sm5708_write_reg(sm5708->i2c, sm5708_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < SM5708_MAX_IRQ; i++) {
		int cur_irq;

		cur_irq = i + sm5708->irq_base;
		irq_set_chip_data(cur_irq, sm5708);
		irq_set_chip_and_handler(cur_irq, &sm5708_irq_chip, handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(sm5708->irq, NULL, sm5708_irq_thread,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND,
					"sm5708-irq", sm5708);
	if (ret) {
		dev_err(sm5708->dev, "Failed to request IRQ %d: %d\n",
			sm5708->irq, ret);

		return ret;
	}

	return 0;
}

void sm5708_irq_exit(struct sm5708_dev *sm5708)
{
	if (sm5708->irq)
		free_irq(sm5708->irq, sm5708);
}

