/*
 * sm5708.c - mfd core driver for the SM5708
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
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/sm5708.h>
#include <linux/regulator/machine.h>

//#include <linux/muic/muic.h>

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#define I2C_ADDR_PMIC	(0x92 >> 1)

static struct mfd_cell sm5708_devs[] = {
#if defined(CONFIG_REGULATOR_SM5708)
	{ .name = "sm5708-usbldo", },
#endif /* CONFIG_REGULATOR_SM5708 */
#if defined(CONFIG_CHARGER_SM5708)
	{ .name = "sm5708-charger", },
#endif
#if defined(CONFIG_LEDS_SM5708_RGB)
	{ .name = "leds-sm5708-rgb", },
#endif /* CONFIG_LEDS_SM5708_RGB */
#if defined(CONFIG_LEDS_SM5708)
	{ .name = "sm5708-fled", },
#endif /* CONFIG_LEDS_SM5708 */
	{ .name = "sm5708-muic", .of_compatible = "siliconmitus,sm5708-muic" },
};

int sm5708_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%x), ret(%d)\n", MFD_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(sm5708_read_reg);

int sm5708_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(sm5708_bulk_read);

int sm5708_read_word(struct i2c_client *i2c, u8 reg)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL_GPL(sm5708_read_word);

int sm5708_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%x), ret(%d)\n",
				MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5708_write_reg);

int sm5708_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(sm5708_bulk_write);

int sm5708_write_word(struct i2c_client *i2c, u8 reg, u16 value)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_write_word_data(i2c, reg, value);
	mutex_unlock(&sm5708->i2c_lock);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL_GPL(sm5708_write_word);

static struct i2c_client *i2c_glob = NULL;

int sm5708_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	if (i2c == NULL)
		i2c = i2c_glob;
	if (i2c == NULL)
		return -EINVAL;
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int ret;

	printk("SM5708 update reg %x %x %x\n", reg, val, mask);

	mutex_lock(&sm5708->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));

		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&sm5708->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sm5708_update_reg);

#if defined(CONFIG_OF)
static int of_sm5708_dt(struct device *dev, struct sm5708_platform_data *pdata)
{
	struct device_node *np_sm5708 = dev->of_node;

	if (!np_sm5708)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio(np_sm5708, "sm5708,irq-gpio", 0);
	pdata->wakeup = of_property_read_bool(np_sm5708, "sm5708,wakeup");

	pr_info("%s: irq-gpio: %u\n", __func__, pdata->irq_gpio);

	return 0;
}
#else
static int of_sm5708_dt(struct device *dev, struct sm5708_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int sm5708_i2c_probe(struct i2c_client *i2c,	const struct i2c_device_id *dev_id)
{
	struct sm5708_dev *sm5708;
	struct sm5708_platform_data *pdata = i2c->dev.platform_data;
	int ret = 0;

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	sm5708 = kzalloc(sizeof(struct sm5708_dev), GFP_KERNEL);
	if (!sm5708)
		return -ENOMEM;

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev, sizeof(struct sm5708_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto err;
		}

		ret = of_sm5708_dt(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	sm5708->dev = &i2c->dev;
	sm5708->i2c = i2c;
	sm5708->irq = i2c->irq;
	if (pdata) {
		sm5708->pdata = pdata;

		pdata->irq_base = irq_alloc_descs(-1, 0, SM5708_MAX_IRQ, -1);
		if (pdata->irq_base < 0) {
			pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",	MFD_DEV_NAME, __func__, pdata->irq_base);
			ret = -EINVAL;
			goto err;
		} else
			sm5708->irq_base = pdata->irq_base;

		sm5708->irq_gpio = pdata->irq_gpio;
		sm5708->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err;
	}
	mutex_init(&sm5708->i2c_lock);

	i2c_set_clientdata(i2c, sm5708);

	ret = sm5708_irq_init(sm5708);

	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(sm5708->dev, -1, sm5708_devs,	ARRAY_SIZE(sm5708_devs), NULL, 0, NULL);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(sm5708->dev, pdata->wakeup);

	pr_info("%s:%s DONE\n", MFD_DEV_NAME, __func__);

	i2c_glob = i2c;

	return ret;

err_mfd:
	mfd_remove_devices(sm5708->dev);
err_irq_init:
	mutex_destroy(&sm5708->i2c_lock);
err:
	kfree(sm5708);
	return ret;
}

static int sm5708_i2c_remove(struct i2c_client *i2c)
{
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);

	mfd_remove_devices(sm5708->dev);
	kfree(sm5708);

	return 0;
}

static const struct i2c_device_id sm5708_i2c_id[] = {
	{ MFD_DEV_NAME, TYPE_SM5708 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sm5708_i2c_id);

static const struct of_device_id sm5708_i2c_dt_ids[] = {
	{ .compatible = "sm,sm5708" },
	{ },
};
MODULE_DEVICE_TABLE(of, sm5708_i2c_dt_ids);

#if defined(CONFIG_PM)
static int sm5708_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(sm5708->irq);

	disable_irq(sm5708->irq);

	return 0;
}

static int sm5708_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */

	if (device_may_wakeup(dev))
		disable_irq_wake(sm5708->irq);

	enable_irq(sm5708->irq);

	return 0;
}
#else
#define sm5708_suspend	NULL
#define sm5708_resume		NULL
#endif /* CONFIG_PM */

#ifdef DISABLED

u8 sm5708_dumpaddr_led[] = {
	SM5708_RGBLED_REG_LEDEN,
	SM5708_RGBLED_REG_LED0BRT,
	SM5708_RGBLED_REG_LED1BRT,
	SM5708_RGBLED_REG_LED2BRT,
	SM5708_RGBLED_REG_LED3BRT,
	SM5708_RGBLED_REG_LEDBLNK,
	SM5708_RGBLED_REG_LEDRMP,
};

static int sm5708_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(sm5708_dumpaddr_pmic); i++)
		sm5708_read_reg(i2c, sm5708_dumpaddr_pmic[i],
				&sm5708->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(sm5708_dumpaddr_led); i++)
		sm5708_read_reg(i2c, sm5708_dumpaddr_led[i],
				&sm5708->reg_led_dump[i]);

	disable_irq(sm5708->irq);

	return 0;
}

static int sm5708_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_dev *sm5708 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(sm5708->irq);

	for (i = 0; i < ARRAY_SIZE(sm5708_dumpaddr_pmic); i++)
		sm5708_write_reg(i2c, sm5708_dumpaddr_pmic[i],
				sm5708->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(sm5708_dumpaddr_led); i++)
		sm5708_write_reg(i2c, sm5708_dumpaddr_led[i],
				sm5708->reg_led_dump[i]);

	return 0;
}
#endif

const struct dev_pm_ops sm5708_pm = {
	.suspend = sm5708_suspend,
	.resume = sm5708_resume,
#ifdef DISABLED
	.freeze =  sm5708_freeze,
	.thaw = sm5708_restore,
	.restore = sm5708_restore,
#endif
};

static struct i2c_driver sm5708_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &sm5708_pm,
#endif /* CONFIG_PM */
		.of_match_table	= sm5708_i2c_dt_ids,
	},
	.probe		= sm5708_i2c_probe,
	.remove		= sm5708_i2c_remove,
	.id_table	= sm5708_i2c_id,
};

static int __init sm5708_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&sm5708_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(sm5708_i2c_init);

static void __exit sm5708_i2c_exit(void)
{
	i2c_del_driver(&sm5708_i2c_driver);
}
module_exit(sm5708_i2c_exit);

MODULE_DESCRIPTION("SM5708 multi-function core driver");
MODULE_LICENSE("GPL");
