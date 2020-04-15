 /*
  * sm5708.h - Driver for the SM5708
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
  * SM5708 has Flash, RGB, Charger, Regulator devices.
  * The devices share the same I2C bus and included in
  * this mfd driver.
  */

#ifndef __SM5708_H__
#define __SM5708_H__
#include <linux/platform_device.h>
#include <linux/regmap.h>
//#include "../../../../drivers/battery_v2/include/sec_charging_common.h"

//#include <linux/leds-sm5708.h>
/* #include <linux/battery/charger/sm5708_charger.h> */

#define MFD_DEV_NAME "sm5708-mfd"


#define SM5708_I2C_ADDR			(0x92)
#define SM5708_REG_INVALID		(0xff)

enum sm5708_reg {
	SM5708_REG_INT1				= 0x00,
	SM5708_REG_INT2				= 0x01,
	SM5708_REG_INT3				= 0x02,
	SM5708_REG_INT4				= 0x03,
	SM5708_REG_INTMSK1			= 0x04,
	SM5708_REG_INTMSK2			= 0x05,
	SM5708_REG_INTMSK3			= 0x06,
	SM5708_REG_INTMSK4			= 0x07,
	SM5708_REG_STATUS1			= 0x08,
	SM5708_REG_STATUS2			= 0x09,
	SM5708_REG_STATUS3			= 0x0A,
	SM5708_REG_STATUS4			= 0x0B,
	SM5708_REG_CNTL				= 0x0C,
	SM5708_REG_VBUSCNTL			= 0x0D,

	SM5708_REG_CHGCNTL1			= 0x0F,
	SM5708_REG_CHGCNTL2			= 0x10,

	SM5708_REG_CHGCNTL3			= 0x12,
	SM5708_REG_CHGCNTL4			= 0x13,
	SM5708_REG_CHGCNTL5			= 0x14,
	SM5708_REG_CHGCNTL6			= 0x15,
	SM5708_REG_CHGCNTL7			= 0x16,
	SM5708_REG_FLED1CNTL1		= 0x17,
	SM5708_REG_FLED1CNTL2		= 0x18,
	SM5708_REG_FLED1CNTL3		= 0x19,
	SM5708_REG_FLED1CNTL4		= 0x1A,
	SM5708_REG_FLED2CNTL1		= 0x1B,
	SM5708_REG_FLED2CNTL2		= 0x1C,
	SM5708_REG_FLED2CNTL3		= 0x1D,
	SM5708_REG_FLED2CNTL4		= 0x1E,
	SM5708_REG_FLEDCNTL5		= 0x1F,
	SM5708_REG_FLEDCNTL6		= 0x20,
	SM5708_REG_SBPSCNTL			= 0x21,
	SM5708_REG_CNTLMODEONOFF	= 0x22,
	SM5708_REG_CNTLPWM			= 0x23,
	SM5708_REG_RLEDCURRENT		= 0x24,
	SM5708_REG_GLEDCURRENT		= 0x25,
	SM5708_REG_BLEDCURRENT		= 0x26,
	SM5708_REG_DIMSLPRLEDCNTL	= 0x27,
	SM5708_REG_DIMSLPGLEDCNTL	= 0x28,
	SM5708_REG_DIMSLPBLEDCNTL	= 0x29,
	SM5708_REG_RLEDCNTL1		= 0x2A,
	SM5708_REG_RLEDCNTL2		= 0x2B,
	SM5708_REG_RLEDCNTL3		= 0x2C,
	SM5708_REG_RLEDCNTL4		= 0x2D,
	SM5708_REG_GLEDCNTL1		= 0x2E,
	SM5708_REG_GLEDCNTL2		= 0x2F,
	SM5708_REG_GLEDCNTL3		= 0x30,
	SM5708_REG_GLEDCNTL4		= 0x31,
	SM5708_REG_BLEDCNTL1		= 0x32,
	SM5708_REG_BLEDCNTL2		= 0x33,
	SM5708_REG_BLEDCNTL3		= 0x34,
	SM5708_REG_BLEDCNTL4		= 0x35,
	SM5708_REG_HAPTICCNTL		= 0x36,
	SM5708_REG_DEVICEID			= 0x37,
	SM5708_REG_FACTORY			= 0x3E,

	SM5708_REG_MAX,
};

enum sm5708_irq {
	SM5708_VBUSPOK_IRQ,
	SM5708_VBUSUVLO_IRQ,
	SM5708_VBUSOVP_IRQ,
	SM5708_VBUSLIMIT_IRQ,

	SM5708_AICL_IRQ,
	SM5708_BATOVP_IRQ,
	SM5708_NOBAT_IRQ,
	SM5708_CHGON_IRQ,
	SM5708_Q4FULLON_IRQ,
	SM5708_TOPOFF_IRQ,
	SM5708_DONE_IRQ,
	SM5708_WDTMROFF_IRQ,

	SM5708_THEMREG_IRQ,
	SM5708_THEMSHDN_IRQ,
	SM5708_OTGFAIL_IRQ,
	SM5708_DISLIMIT_IRQ,
	SM5708_PRETMROFF_IRQ,
	SM5708_FASTTMROFF_IRQ,
	SM5708_LOWBATT_IRQ,
	SM5708_nENQ4_IRQ,

	SM5708_FLED1SHORT_IRQ,
	SM5708_FLED1OPEN_IRQ,
	SM5708_FLED2SHORT_IRQ,
	SM5708_FLED2OPEN_IRQ,
	SM5708_BOOSTPOK_NG_IRQ,
	SM5708_BOOSTPOK_IRQ,
	SM5708_ABSTMR1OFF_IRQ,
	SM5708_SBPS_IRQ,

	SM5708_MAX_IRQ,
};

struct sm5708_dev {
	struct device *dev;
	struct i2c_client *i2c; /* PMIC, CHARGER, FLASH, RGB */
	struct mutex i2c_lock;

	int type;

	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[SM5708_MAX_IRQ];
	int irq_masks_cache[SM5708_MAX_IRQ];
	uint8_t irq_status[4];

#ifdef CONFIG_HIBERNATION
	/* For hibernation */
	u8 reg_dump[SM5708_REG_MAX];
#endif

	struct sm5708_platform_data *pdata;
};

enum sm5708_types {
	TYPE_SM5708,
};

extern int sm5708_irq_init(struct sm5708_dev *sm5708);
extern void sm5708_irq_exit(struct sm5708_dev *sm5708);

/* SM5708 shared i2c API function */
extern int sm5708_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int sm5708_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int sm5708_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int sm5708_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int sm5708_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int sm5708_read_word(struct i2c_client *i2c, u8 reg);

extern int sm5708_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

/* for charger api */
extern void sm5708_hv_muic_charger_init(void); /* dong : delet */

/* SM5708 check muic path function */
extern bool is_muic_usb_path_ap_usb(void);
extern bool is_muic_usb_path_cp_usb(void);

struct sm5708_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct sm5708_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;
	int chg_irq;

#if defined(CONFIG_CHARGER_SM5708)
	sec_charger_platform_data_t *charger_data;
#endif
#ifdef CONFIG_LEDS_SM5708
	struct sm5708_fled_platform_data *fled_platform_data;
#endif
#if defined(CONFIG_REGULATOR_SM5708)
	int num_regulators;
	struct sm5708_regulator_data *regulators;
#endif
};

struct sm5708 {
	struct regmap *regmap;
};

#endif /* __SM5708_H__ */
