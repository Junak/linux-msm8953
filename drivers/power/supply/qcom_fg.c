/* Copyright (c) 2020, The Linux Foundation. All rights reserved. */

#define pr_fmt(fmt)	"qcom-fg: %s: " fmt, __func__

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/types.h>

/* Register offsets */

/* Interrupt offsets */
#define INT_RT_STS(base)			(base + 0x10)
#define INT_EN_CLR(base)			(base + 0x16)

/* RAM resister offsets */
#define RAM_OFFSET	0x400

enum soc_address {
	SOC_MONOTONIC_SOC	= 0x09,
	SOC_BOOT_MOD		= 0x50,
	SOC_RESTART		= 0x51,
	SOC_ALG_ST		= 0xcf,
	SOC_FG_RESET		= 0xf3,
};

enum mem_if_address {
	MEM_INTF_IMA_CFG	= 0x52,
	MEM_INTF_IMA_OPR_STS	= 0x54,
	MEM_INTF_IMA_EXP_STS	= 0x55,
	MEM_INTF_IMA_HW_STS	= 0x56,
	MEM_INTF_BEAT_COUNT	= 0x57,
	MEM_INTF_IMA_ERR_STS	= 0x5f,
	MEM_INTF_IMA_BYTE_EN	= 0x60,
};

enum register_type {
	MEM_INTF_CFG,
	MEM_INTF_CTL,
	MEM_INTF_ADDR_LSB,
	MEM_INTF_RD_DATA0,
	MEM_INTF_WR_DATA0,
	MAX_ADDRESS,
};

struct register_offset {
	u16 address[MAX_ADDRESS];
};

#define MEM_INTF_CFG(chip)	\
		((chip)->mem_base + (chip)->offset[MEM_INTF_CFG])
#define MEM_INTF_CTL(chip)	\
		((chip)->mem_base + (chip)->offset[MEM_INTF_CTL])
#define MEM_INTF_ADDR_LSB(chip) \
		((chip)->mem_base + (chip)->offset[MEM_INTF_ADDR_LSB])
#define MEM_INTF_RD_DATA0(chip) \
		((chip)->mem_base + (chip)->offset[MEM_INTF_RD_DATA0])
#define MEM_INTF_WR_DATA0(chip) \
		((chip)->mem_base + (chip)->offset[MEM_INTF_WR_DATA0])

#define BATT_INFO_THERM_C1(chip)		(chip->batt_base + 0x5c)
#define BATT_INFO_VBATT_LSB(chip)		(chip->batt_base + 0xa0)
#define BATT_INFO_VBATT_MSB(chip)		(chip->batt_base + 0xa1)
#define BATT_INFO_IBATT_LSB(chip)		(chip->batt_base + 0xa2)
#define BATT_INFO_IBATT_MSB(chip)		(chip->batt_base + 0xa3)
#define BATT_INFO_BATT_TEMP_LSB(chip)		(chip->batt_base + 0x50)
#define BATT_INFO_BATT_TEMP_MSB(chip)		(chip->batt_base + 0x51)

#define BATT_TEMP_LSB_MASK			GENMASK(7, 0)
#define BATT_TEMP_MSB_MASK			GENMASK(2, 0)

enum pmic {
	PMI8950,
	PMI8998_V1,
	PMI8998_V2
};

enum wa_flags {
	PMI8998_V1_REV_WA = BIT(0),
	PMI8998_V2_REV_WA = BIT(1),
};

enum fg_sram_param_id {
	FG_DATA_BATT_TEMP = 0,
	FG_DATA_CHARGE,
	FG_DATA_OCV,
	FG_DATA_VOLTAGE,
	FG_DATA_CURRENT,
	FG_DATA_BATT_ESR,
	FG_DATA_BATT_ESR_COUNT,
	FG_DATA_BATT_ESR_REG,
	FG_DATA_BATT_SOC,
	FG_DATA_FULL_SOC,
	FG_DATA_CYCLE_COUNT,
	FG_DATA_VINT_ERR,
	FG_DATA_CPRED_VOLTAGE,
	FG_DATA_CHARGE_COUNTER,
	FG_DATA_BATT_ID,
	FG_DATA_BATT_ID_INFO,
	FG_DATA_NOM_CAP,
	FG_DATA_ACT_CAP,

	FG_BATT_PROFILE,
	FG_BATT_PROFILE_INTEGRITY,
	FG_SETTING_ADC_CONF,
	FG_SETTING_RSLOW_CHG,
	FG_SETTING_RSLOW_DISCHG,
	FG_SETTING_ALG,
	FG_SETTING_EXTERNAL_SENSE,
	FG_SETTING_THERMAL_COEFFS,

	//Settings below this have corresponding dt entries
	FG_SETTING_TERM_CURRENT,
	FG_SETTING_SYS_TERM_CURRENT,
	FG_SETTING_CHG_TERM_CURRENT,
	FG_SETTING_CHG_TERM_BASE_CURRENT,
	FG_SETTING_CUTOFF_VOLT,
	FG_SETTING_EMPTY_VOLT,
	FG_SETTING_MAX_VOLT,
	FG_SETTING_BATT_LOW,
	FG_SETTING_CONST_CHARGE_VOLT_THR, //previsouly called FG_SETTING_BATT_FULL
	FG_SETTING_RECHARGE_THR,
	FG_SETTING_RECHARGE_VOLT_THR,
	FG_SETTING_DELTA_MSOC,
	FG_SETTING_DELTA_BSOC,
	FG_SETTING_BCL_LM_THR,
	FG_SETTING_BCL_MH_THR,
	FG_SETTING_BATT_COLD_TEMP,
	FG_SETTING_BATT_COOL_TEMP,
	FG_SETTING_BATT_WARM_TEMP,
	FG_SETTING_BATT_HOT_TEMP,
	FG_SETTING_THERM_DELAY,
	FG_PARAM_MAX
};

enum sram_param_type {
	SRAM_PARAM,
	MEM_IF_PARAM,
	BATT_BASE_PARAM,
	SOC_BASE_PARAM,
};

struct fg_sram_param {
	u16	address;
	u8	offset;
	unsigned int length;
	int value;

	u8 type: 2;
	u8 wa_flags: 6;
	/* the conversion stuff */
	int numrtr;
	int denmtr;
	int val_offset;
	void (*encode)(struct fg_sram_param sp, int val, u8 *buf);
	int (*decode)(struct fg_sram_param sp, u8* val);

	const char *name;
};

static int fg_decode_voltage_15b(struct fg_sram_param sp, u8 *val);
static int fg_decode_value_16b(struct fg_sram_param sp, u8 *val);
static int fg_decode_temperature(struct fg_sram_param sp, u8 *val);
static int fg_decode_current(struct fg_sram_param sp, u8 *val);
static int fg_decode_float(struct fg_sram_param sp, u8 *val);
static int fg_decode_default(struct fg_sram_param sp, u8 *val);
static int fg_decode_cc_soc_pmi8950(struct fg_sram_param sp, u8 *value);
static int fg_decode_cc_soc_pmi8998(struct fg_sram_param sp, u8 *value);
static int fg_decode_value_le(struct fg_sram_param sp, u8 *val);

static void fg_encode_voltage(struct fg_sram_param sp, int val_mv, u8 *buf);
static void fg_encode_current(struct fg_sram_param sp, int val_ma, u8 *buf);
static void fg_encode_float(struct fg_sram_param sp, int val_ma, u8 *buf);
static void fg_encode_roundoff(struct fg_sram_param sp, int val_ma, u8 *buf);
static void fg_encode_bcl(struct fg_sram_param sp, int val, u8 *buf);
static void fg_encode_therm_delay(struct fg_sram_param sp, int val, u8 *buf);
static void fg_encode_voltcmp8(struct fg_sram_param sp, int val, u8 *buf);
static void fg_encode_adc(struct fg_sram_param sp, int val, u8 *buf);
static void fg_encode_default(struct fg_sram_param sp, int val, u8 *buf);

#define PMI8950_LSB_16B_NUMRTR	1000
#define PMI8950_LSB_16B_DENMTR	152587
#define PMI8950_FULL_PERCENT_3B	0xffffff
static struct fg_sram_param fg_params_pmi8950[FG_PARAM_MAX] = {
	[FG_DATA_BATT_TEMP] = {
		.address	= 0x550,
		.offset		= 2,
		.length		= 2,
		.numrtr		= 1000,
		.denmtr		= 625,
		.val_offset	= -2730,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CHARGE] = {
		.address	= 0x570,
		.offset		= 0,
		.length		= 4,
		.numrtr		= PMI8950_FULL_PERCENT_3B,
		.denmtr		= 10000,
		.decode		= fg_decode_current,
	},
	[FG_DATA_OCV] = {
		.address	= 0x588,
		.offset		= 3,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_VOLTAGE] = {
		.address	= 0x5cc,
		.offset		= 1,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CPRED_VOLTAGE] = {
		.address	= 0x540,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CURRENT] = {
		.address	= 0x5cc,
		.offset		= 3,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.decode		= fg_decode_current,
	},
	[FG_DATA_BATT_ESR] = {
		.address	= 0x544,
		.offset		= 2,
		.length		= 2,
		.decode		= fg_decode_float,
	},
	[FG_DATA_BATT_ESR_COUNT] = {
		.address	= 0x558,
		.offset		= 2,
		.length		= 2,
		.decode		= fg_decode_default,
	},
	[FG_DATA_BATT_ESR_REG] = {
		.address	= 0x4f4,
		.offset		= 2,
		.length		= 2,
		.decode		= fg_decode_float,
	},
	[FG_DATA_BATT_SOC] = {
		.address	= 0x56c,
		.offset		= 1,
		.length		= 3,
		.numrtr		= PMI8950_FULL_PERCENT_3B,
		.denmtr		= 10000,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_NOM_CAP] = {
		.address	= 0x4f4,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_value_le,
	},
	[FG_DATA_ACT_CAP] = {
		.address	= 0x5e4,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_default,
	},
	[FG_DATA_CYCLE_COUNT] = {
		.address	= 0x5e8,
		.offset		= 0,
		.length		= 2,
		.decode		= fg_decode_value_le
	},
	//TODO: change to setting?
	[FG_DATA_VINT_ERR] = {
		.address	= 0x560,
		.offset		= 0,
		.length		= 4,
		.decode		= fg_decode_value_16b,
		.encode		= fg_encode_default
	},
	[FG_DATA_CHARGE_COUNTER] = {
		.address	= 0x5bc,
		.offset		= 3,
		.length		= 4,
		.decode		= fg_decode_cc_soc_pmi8950,
	},

	/*
	DATA(CC_SOC,	      0x570,   0,      4),
	DATA(CC_COUNTER,      0x5BC,   3,      4),
	DATA(BATT_ID,         0x594,   1,      1),
	DATA(BATT_ID_INFO,    0x594,   3,      1),*/

	[FG_SETTING_TERM_CURRENT] = {
		.name		= "qcom,term-current-ma",
		.address	= 0x40c,
		.offset		= 2,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.encode		= fg_encode_current,
		.value		= 250
	},
	[FG_SETTING_EXTERNAL_SENSE] = {
		.address	= 0x4ac,
		.offset		= 0,
	},
	[FG_SETTING_CHG_TERM_CURRENT] = {
		.name		= "qcom,chg-term-current-ma",
		.address	= 0x4f8,
		.offset		= 2,
		.length		= 2,
		.numrtr		= -PMI8950_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.encode		= fg_encode_current,
		.value		= 250
	},
	[FG_SETTING_CUTOFF_VOLT] = {
		.name		= "qcom,cutoff-volt-mv",
		.address	= 0x40c,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8950_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8950_LSB_16B_DENMTR,
		.encode		= fg_encode_current,
		.value		= 3200
	},
	[FG_SETTING_EMPTY_VOLT] = {
		/*.name		= "qcom,empty-volt-mv", do not parse*/
		.address	= 0x458,
		.offset		= 3,
		.length		= 1,
		.numrtr		= 1,
		.denmtr		= 9766,
		.encode		= fg_encode_voltcmp8,
		.value		= 3100
	},
	[FG_SETTING_BATT_LOW] = {
		.name		= "qcom,low-volt-thr",
		.address	= 0x458,
		.offset		= 0,
		.length		= 1,
		.numrtr		= 512,
		.denmtr		= 1000,
		.encode		= fg_encode_roundoff,
		.value		= 4200
	},
	[FG_SETTING_DELTA_MSOC] = {
		.name		= "qcom,delta-soc-thr",
		.address	= 0x450,
		.offset		= 3,
		.length		= 1,
		.numrtr		= 255,
		.denmtr		= 100,
		.encode		= fg_encode_roundoff,
		.value		= 1
	},
	[FG_SETTING_THERMAL_COEFFS] = {
		.address	= 0x444,
		.offset		= 2,
		.length		= 0,
	},
	[FG_SETTING_RECHARGE_THR] = {
		.name		= "qcom,recharge-thr",
		.address	= 0x45c,
		.offset		= 1,
		.length		= 1,
		.numrtr		= 256,
		.denmtr		= 100,
		.encode		= fg_encode_default,
	},
	[FG_SETTING_ADC_CONF] = {
		.address	= 0x4b8,
		.offset		= 3,
		.length		= 1,
		.encode		= fg_encode_default,
	},
	[FG_SETTING_BCL_LM_THR] = {
		.name		= "qcom,bcl-lm-thr-ma",
		.address	= 0x47c,
		.offset		= 2,
		.length		= 1,
		.numrtr		= 100,
		.denmtr		= 976,
		.encode		= fg_encode_bcl,
		.value		= 50
	},
	[FG_SETTING_BCL_MH_THR] = {
		.name		= "qcom,bcl-mh-thr-ma",
		.address	= 0x47c,
		.offset		= 3,
		.length		= 1,
		.numrtr		= 100,
		.denmtr		= 976,
		.encode		= fg_encode_bcl,
		.value		= 752
	},
	[FG_SETTING_BATT_COOL_TEMP] = {
		.name		= "qcom,batt-cool-temp",
		.address	= 0x454,
		.offset		= 0,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 100
	},
	[FG_SETTING_BATT_WARM_TEMP] = {
		.name		= "qcom,batt-warm-temp",
		.address	= 0x454,
		.offset		= 1,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 400
	},
	[FG_SETTING_BATT_COLD_TEMP] = {
		.name		= "qcom,batt-cold-temp",
		.address	= 0x454,
		.offset		= 2,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 50
	},
	[FG_SETTING_BATT_HOT_TEMP] = {
		.name		= "qcom,batt-hot-temp",
		.address	= 0x454,
		.offset		= 3,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 450
	},
	[FG_SETTING_THERM_DELAY] = {
		.name		= "qcom,therm-delay-us",
		.address	= 0x4ac,
		.offset		= 3,
		.length		= 0,	//to avoid using dts
		.encode		= fg_encode_therm_delay,
		.value		= 0
	},
	[FG_SETTING_CONST_CHARGE_VOLT_THR] = {
		.name		= "qcom,const-charge-volt-thr-mv",
		.address	= 0x4f8,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 32768,
		.denmtr		= 5000,
		.encode		= fg_encode_adc,
		.value		= -EINVAL,
	},
	[FG_SETTING_RSLOW_CHG] = {
		.address	= 0x514,
		.offset		= 2,
		.length		= 2,
		.encode		= fg_encode_float,
		.decode		= fg_decode_float,
	},
	[FG_SETTING_RSLOW_DISCHG] = {
		.address	= 0x514,
		.offset		= 0,
		.length		= 2,
		.encode		= fg_encode_float,
		.decode		= fg_decode_float,
	},
	[FG_BATT_PROFILE_INTEGRITY] = {
		.address	= 0x53c,
		.offset		= 0,
		.length		= 1,
	},
	[FG_SETTING_ALG] = {
		.address	= 0x4b0,
		.offset		= 3,
		.length		= 1,
	}
};

/*
 * PMI8998 is different from PMI8950 and all previous hardware
 * It likes to write and read some values from the fg_soc, instead
 * of reading from the sram, like previous hardware does.
 * For such values, a dummy name value is put here to get the value from
 * the dt and use it. However addresses are defined separately, as they
 * depend on runtime values
 */

#define PMI8998_V1_LSB_15B_NUMRTR	1000
#define PMI8998_V1_LSB_15B_DENMTR	244141
#define PMI8998_V1_LSB_16B_NUMRTR	1000
#define PMI8998_V1_LSB_16B_DENMTR	390625
static struct fg_sram_param fg_params_pmi8998_v1[FG_PARAM_MAX] = {
	[FG_DATA_BATT_TEMP] = {
		.address	= 0x50,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 4,
		.denmtr		= 10,		//Kelvin to DeciKelvin
		.val_offset	= -2730,	//DeciKelvin to DeciDegc
		.decode		= fg_decode_temperature
	},
	[FG_DATA_BATT_SOC] = {
		.address	= 91,
		.offset		= 0,
		.length		= 4,
		.decode		= fg_decode_default,
	},
	[FG_DATA_NOM_CAP] = {
		.address	= 58,
		.offset		= 0,
		.length		= 2,
		.decode		= fg_decode_value_le
	},
	[FG_DATA_FULL_SOC] = {
		.address	= 93,
		.offset		= 2,
		.length		= 2,
		.decode		= fg_decode_default,
	},
	[FG_DATA_VOLTAGE] = {
		.address	= 0xa0,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 1000,
		.denmtr		= 122070,
		.wa_flags	= PMI8998_V1_REV_WA,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CPRED_VOLTAGE] = {
		.address	= 97,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_OCV] = {
		.address	= 97,
		.offset		= 2,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_BATT_ESR] = {
		.address	= 99,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_CHARGE] = {
		.address	= 95,
		.offset		= 0,
		.length		= 4,
		.numrtr		= 1,
		.denmtr		= 1,
		.decode		= fg_decode_cc_soc_pmi8998,
	},
	[FG_DATA_CHARGE_COUNTER] = {
		.address	= 96,
		.offset		= 0,
		.length		= 4,
		.numrtr		= 1,
		.denmtr		= 1,
		.decode		= fg_decode_cc_soc_pmi8998,
	},
	[FG_DATA_NOM_CAP] = {
		.address	= 54,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_value_le,
	},
	[FG_DATA_ACT_CAP] = {
		.address	= 117,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_default,
	},
	[FG_DATA_CYCLE_COUNT] = {
		.address	= 75,
		.offset		= 0,
		.length		= 2,
		.decode		= fg_decode_value_le
	},
	[FG_DATA_CURRENT] = {
		.address	= 0xa2,
		.length		= 2,
		.type		= BATT_BASE_PARAM,
		.wa_flags	= PMI8998_V1_REV_WA,
		.numrtr		= 1000,
		.denmtr		= 488281,
		.decode 	= fg_decode_current,
	},
	[FG_SETTING_ALG] = {
		.address	= 120,
		.offset		= 1,
		.length		= 1,
		.decode		= fg_decode_default
	},
	[FG_SETTING_CUTOFF_VOLT] = {
		.name		= "qcom,cutoff-volt-mv",
		.address	= 5,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.encode		= fg_encode_voltage,
		.value		= 3200,
	},
	[FG_SETTING_EMPTY_VOLT] = {
		.name		= "qcom,empty-volt-mv",
		.address	= 15,
		.offset		= 0,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.val_offset	= -2500,
		.encode		= fg_encode_voltage,
		.value		= 2850,
	},
	[FG_SETTING_BATT_LOW] = {
		.name		= "qcom,low-volt-thr",
		.address	= 15,
		.offset		= 1,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.val_offset	= -2500,
		.encode		= fg_encode_voltage,
		.value		= -EINVAL,
	},
	[FG_SETTING_CONST_CHARGE_VOLT_THR] = {
		.name		= "qcom,const-charge-volt-thr-mv",
		.address	= 7,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.encode		= fg_encode_voltage,
		.value		= -EINVAL,
	},
	[FG_SETTING_TERM_CURRENT] = {
		.name		= "qcom,term-current-ma",
		.address	= 4,
		.offset		= 0,
		.length		= 3,
		.numrtr		= 1000000,
		.denmtr		= 122070,
		.encode		= fg_encode_current,
		.value		= 500,
	},
	[FG_SETTING_SYS_TERM_CURRENT] = {
		.name		= "qcom,sys-term-current-ma",
		.address	= 6,
		.offset		= 0,
		.length		= 3,
		.numrtr		= 100000,
		.denmtr		= 122070,
		.encode		= fg_encode_current,
		.value		= -125,
	},
	[FG_SETTING_CHG_TERM_CURRENT] = {
		.name		= "qcom,chg-term-current-ma",
		.address	= 14,
		.offset		= 1,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.encode		= fg_encode_current,
		.value		= 100,
	},
	[FG_SETTING_DELTA_BSOC] = {
		.name		= "qcom,delta-soc-thr",
		.address	= 13,
		.offset		= 2,
		.length		= 1,
		.numrtr		= 2048,
		.denmtr		= 100,
		.encode		= fg_encode_default,
		.value		= 1,
	},
	[FG_SETTING_DELTA_MSOC] = {
		.name		= "qcom,delta-soc-thr",
		.address	= 12,
		.offset		= 3,
		.length		= 1,
		.numrtr		= 2048,
		.denmtr		= 100,
		.encode		= fg_encode_default,
		.value		= 1,
	},
	[FG_SETTING_RECHARGE_THR] = {
		.name		= "qcom,recharge-thr",
		.address	= 14,
		.offset		= 0,
		.length		= 1,
		.numrtr		= 256,
		.denmtr		= 100,
		.encode		= fg_encode_default,
	},
	[FG_SETTING_RSLOW_DISCHG] = {
		.address	= 34,
		.offset		= 0,
		.length		= 1,
	},
	[FG_SETTING_RSLOW_CHG] = {
		.address	= 51,
		.offset		= 0,
		.length		= 1,
	},
	[FG_BATT_PROFILE_INTEGRITY] = {
		.address	= 79,
		.offset		= 3,
		.length		= 1,
		.value		= BIT(0),
	},
	[FG_SETTING_BATT_COOL_TEMP] = {
		.name		= "qcom,batt-cool-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x63,
		.offset		= 0,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 100
	},
	[FG_SETTING_BATT_WARM_TEMP] = {
		.name		= "qcom,batt-warm-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x64,
		.offset		= 1,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 400
	},
	[FG_SETTING_BATT_COLD_TEMP] = {
		.name		= "qcom,batt-cold-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x62,
		.offset		= 2,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 50
	},
	[FG_SETTING_BATT_HOT_TEMP] = {
		.name		= "qcom,batt-hot-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x65,
		.offset		= 3,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 450
	},
};

static struct fg_sram_param fg_params_pmi8998_v2[FG_PARAM_MAX] = {
	[FG_DATA_BATT_TEMP] = {
		.address	= 0x50,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 4,
		.denmtr		= 10,		//Kelvin to DeciKelvin
		.val_offset	= -2730,	//DeciKelvin to DeciDegc
		.decode		= fg_decode_temperature
	},
	[FG_DATA_BATT_SOC] = {
		.address	= 91,
		.offset		= 0,
		.length		= 4,
		.decode		= fg_decode_default,
	},
	[FG_DATA_FULL_SOC] = {
		.address	= 93,
		.offset		= 2,
		.length		= 2,
		.decode		= fg_decode_default,
	},
	[FG_DATA_VOLTAGE] = {
		.address	= 0xa0,
		.type		= BATT_BASE_PARAM,
		.length		= 2,
		.numrtr		= 122070,
		.denmtr		= 1000,
		.wa_flags	= PMI8998_V2_REV_WA,
		.decode		= fg_decode_value_16b,
	},
	[FG_DATA_CPRED_VOLTAGE] = {
		.address	= 97,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_OCV] = {
		.address	= 97,
		.offset		= 2,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_BATT_ESR] = {
		.address	= 99,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.decode		= fg_decode_voltage_15b,
	},
	[FG_DATA_CHARGE] = {
		.address	= 95,
		.offset		= 0,
		.length		= 4,
		.numrtr		= 1,
		.denmtr		= 1,
		.decode		= fg_decode_cc_soc_pmi8998,
	},
	[FG_DATA_CHARGE_COUNTER] = {
		.address	= 96,
		.offset		= 0,
		.length		= 4,
		.numrtr		= 1,
		.denmtr		= 1,
		.decode		= fg_decode_cc_soc_pmi8998,
	},
	[FG_DATA_NOM_CAP] = {
		.address	= 74,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_value_le
	},
	[FG_DATA_ACT_CAP] = {
		.address	= 117,
		.offset		= 0,
		.length		= 2,
		.numrtr		= 1000,
		.decode		= fg_decode_default,
	},
	[FG_DATA_CYCLE_COUNT] = {
		.address	= 75,
		.offset		= 0,
		.length		= 2,
		.decode		= fg_decode_value_le
	},
	[FG_DATA_CURRENT] = {
		.address	= 0xa2,
		.length		= 2,
		.type		= BATT_BASE_PARAM,
		.wa_flags	= PMI8998_V2_REV_WA,
		.numrtr		= 1000,
		.denmtr		= 488281,
		.decode 	= fg_decode_current,
	},
	[FG_SETTING_ALG] = {
		.address	= 120,
		.offset		= 1,
		.length		= 1,
		.decode		= fg_decode_default
	},
	[FG_SETTING_CUTOFF_VOLT] = {
		.name		= "qcom,cutoff-volt-mv",
		.address	= 5,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.encode		= fg_encode_voltage,
		.value		= 3200,
	},
	[FG_SETTING_EMPTY_VOLT] = {
		.name		= "qcom,empty-volt-mv",
		.address	= 15,
		.offset		= 3,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.val_offset	= -2500,
		.encode		= fg_encode_voltage,
		.value		= 2850,
	},
	[FG_SETTING_MAX_VOLT] = {
		.address	= 16,
		.offset		= 2,
		.length		= 1,
		.numrtr		= 1000,
		.denmtr		= 15625,
		.val_offset	= -2000,
		.encode		= fg_encode_voltage,
	},
	[FG_SETTING_BATT_LOW] = {
		.name		= "qcom,low-volt-thr",
		.address	= 16,
		.offset		= 1,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.val_offset	= -2500,
		.encode		= fg_encode_voltage,
	},
	[FG_SETTING_CONST_CHARGE_VOLT_THR] = {
		.name		= "qcom,const-charge-volt-thr-mv",
		.address	= 7,
		.offset		= 0,
		.length		= 2,
		.numrtr		= PMI8998_V1_LSB_15B_NUMRTR,
		.denmtr		= PMI8998_V1_LSB_15B_DENMTR,
		.encode		= fg_encode_voltage,
		.value		= -EINVAL,
	},
	[FG_SETTING_TERM_CURRENT] = {
		.name		= "qcom,term-current-ma",
		.address	= 4,
		.offset		= 0,
		.length		= 3,
		.numrtr		= 1000000,
		.denmtr		= 122070,
		.encode		= fg_encode_current,
		.value		= 500,
	},
	[FG_SETTING_SYS_TERM_CURRENT] = {
		.name		= "qcom,sys-term-current-ma",
		.address	= 6,
		.offset		= 0,
		.length		= 3,
		.numrtr		= 100000,
		.denmtr		= 390625,
		.encode		= fg_encode_current,
		.value		= -125,
	},
	[FG_SETTING_CHG_TERM_CURRENT] = {
		.name		= "qcom,chg-term-current-ma",
		.address	= 14,
		.offset		= 1,
		.length		= 1,
		.numrtr		= PMI8998_V1_LSB_16B_NUMRTR * 1000,
		.denmtr		= PMI8998_V1_LSB_16B_DENMTR,
		.encode		= fg_encode_current,
		.value		= 100,
	},
	[FG_SETTING_CHG_TERM_BASE_CURRENT] = {
		.name		= "qcom,chg-term-base-current-ma",
		.address	= 15,
		.offset		= 0,
		.length		= 1,
		.numrtr		= 1024,
		.denmtr		= 1000,
		.encode		= fg_encode_current,
		.value		= 75,
	},
	[FG_SETTING_DELTA_BSOC] = {
		.name		= "qcom,delta-soc-thr",
		.address	= 13,
		.offset		= 2,
		.length		= 1,
		.numrtr		= 2048,
		.denmtr		= 100,
		.encode		= fg_encode_default,
		.value		= 1,
	},
	[FG_SETTING_DELTA_MSOC] = {
		.name		= "qcom,delta-soc-thr",
		.address	= 12,
		.offset		= 3,
		.length		= 1,
		.numrtr		= 2048,
		.denmtr		= 100,
		.encode		= fg_encode_default,
		.value		= 1,
	},
	[FG_SETTING_RECHARGE_THR] = {
		.name		= "qcom,recharge-thr",
		.address	= 14,
		.offset		= 0,
		.length		= 1,
		.numrtr		= 256,
		.denmtr		= 100,
		.encode		= fg_encode_default,
	},
	[FG_SETTING_RECHARGE_VOLT_THR] = {
		.name		= "qcom,recharge-volt-thr-mv",
		.address	= 16,
		.offset		= 1,
		.length		= 1,
		.numrtr		= 1000,
		.denmtr		= 15625,
		.val_offset	= -2000,
		.encode		= fg_encode_voltage,
		.value		= 4250,
	},
	[FG_SETTING_RSLOW_DISCHG] = {
		.address	= 34,
		.offset		= 0,
		.length		= 1,
	},
	[FG_SETTING_RSLOW_CHG] = {
		.address	= 51,
		.offset		= 0,
		.length		= 1,
	},
	[FG_BATT_PROFILE_INTEGRITY] = {
		.address	= 79,
		.offset		= 3,
		.length		= 1,
		.value		= BIT(0),
	},
	[FG_SETTING_BATT_COOL_TEMP] = {
		.name		= "qcom,batt-cool-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x63,
		.offset		= 0,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 100
	},
	[FG_SETTING_BATT_WARM_TEMP] = {
		.name		= "qcom,batt-warm-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x64,
		.offset		= 1,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 400
	},
	[FG_SETTING_BATT_COLD_TEMP] = {
		.name		= "qcom,batt-cold-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x62,
		.offset		= 2,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 50
	},
	[FG_SETTING_BATT_HOT_TEMP] = {
		.name		= "qcom,batt-hot-temp",
		.type		= BATT_BASE_PARAM,
		.address	= 0x65,
		.offset		= 3,
		.length		= 1,
		.encode		= fg_encode_default,
		.value		= 450
	},
};

enum mem_if_irq {
	FG_MEM_AVAIL,
	TA_RCVRY_SUG,
	FG_MEM_IF_IRQ_COUNT,
};

struct fg_learning_data {
	int64_t		cc_uah;
	int		actual_cap_uah;
	int		init_cc_pc_val;
	int		max_start_soc;
	int		max_increment;
	int		max_decrement;
	int		vbat_est_thr_uv;
	int		max_cap_limit;
	int		min_cap_limit;
};

struct fg_rslow_data {
	u8			rslow_cfg;
	u8			rslow_thr;
	u8			rs_to_rslow[2];
	u8			rslow_comp[4];
	uint32_t		chg_rs_to_rslow;
	uint32_t		chg_rslow_comp_c1;
	uint32_t		chg_rslow_comp_c2;
	uint32_t		chg_rslow_comp_thr;
};

struct battery_info {
	const char *manufacturer;
	const char *model;
	const char *serial_num;

	bool cyc_ctr_en;
	bool nom_cap_unbound;

	u8 thermal_coeffs[6];

	u8 *batt_profile;
	unsigned batt_profile_len;

	int rconn_mohm;
	int nom_cap_uah;
	int batt_max_voltage_uv;
};

struct fg_chip {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;

	struct power_supply *bms_psy;

	u8 revision[4];
	enum pmic pmic_version;
	bool ima_supported;
	bool reset_on_lockup;

	/* base addresses of components*/
	unsigned int soc_base;
	unsigned int batt_base;
	unsigned int mem_base;
	u16 *offset;

	struct fg_sram_param *param;
	struct battery_info batt_info;

	struct fg_learning_data learning_data;
	struct fg_rslow_data rslow_comp;
	int health;
	int status;
	int vbatt_est_diff;

	//board specific init fn
	int (*init_fn)(struct fg_chip *);
};

/* All getters HERE */

#define VOLTAGE_15BIT_MASK	GENMASK(14, 0)
static int fg_decode_voltage_15b(struct fg_sram_param sp, u8 *value)
{
	int temp = *(int *) value;
	temp &= VOLTAGE_15BIT_MASK;
	return div_u64((u64)temp * sp.denmtr, sp.numrtr);
}

static int fg_decode_cc_soc_pmi8998(struct fg_sram_param sp, u8 *value)
{
	int temp = *(int *) value;
	temp = div_s64((s64)temp * sp.denmtr, sp.numrtr);
	temp = sign_extend32(temp, 31);
	return temp;
}

#define CC_SOC_MAGNITUDE_MASK	0x1FFFFFFF
#define CC_SOC_NEGATIVE_BIT	BIT(29)
#define FULL_PERCENT_28BIT	0xFFFFFFF
#define FULL_RESOLUTION		1000000
static int fg_decode_cc_soc_pmi8950(struct fg_sram_param sp, u8 *val)
{
	int64_t cc_pc_val, cc_soc_pc;
	int temp, magnitude;

	temp = val[3] << 24 | val[2] << 16 | val[1] << 8 | val[0];
	magnitude = temp & CC_SOC_MAGNITUDE_MASK;

	if (temp & CC_SOC_NEGATIVE_BIT)
		cc_pc_val = -1 * (~magnitude + 1);
	else
		cc_pc_val = magnitude;

	cc_soc_pc = div64_s64(cc_pc_val * 100 * FULL_RESOLUTION,
			      FULL_PERCENT_28BIT);
	return cc_soc_pc;
}

static int64_t twos_compliment_extend(int64_t val, int nbytes)
{
	int i;
	int64_t mask;

	mask = 0x80LL << ((nbytes - 1) * 8);
	if (val & mask) {
		for (i = 8; i > nbytes; i--) {
			mask = 0xFFLL << ((i - 1) * 8);
			val |= mask;
		}
	}

	return val;
}

static int fg_decode_current(struct fg_sram_param sp, u8 *val)
{
	int64_t temp;
	if (sp.wa_flags & PMI8998_V1_REV_WA)
		temp = val[0] << 8 | val[1];
	else
		temp = val[1] << 8 | val[0];
	temp = twos_compliment_extend(temp, 2);
	return div_s64((s64)temp * sp.denmtr,
			sp.numrtr);
}

static int fg_decode_value_16b(struct fg_sram_param sp, u8 *value)
{
	int temp = 0, i;
	if (sp.wa_flags & PMI8998_V2_REV_WA)
		for (i = 0; i < sp.length; ++i)
			temp |= value[i] << (8 * (sp.length - i));
	else
		for (i = 0; i < sp.length; ++i)
			temp |= value[i] << (8 * i);
	return div_u64((u64)(u16)temp * sp.denmtr, sp.numrtr) + sp.val_offset;
}

#define BATT_TEMP_LSB_MASK			GENMASK(7, 0)
#define BATT_TEMP_MSB_MASK			GENMASK(2, 0)
static int fg_decode_temperature(struct fg_sram_param sp, u8 *val)
{
	int temp;

	temp = ((val[1] & BATT_TEMP_MSB_MASK) << 8) |
		(val[0] & BATT_TEMP_LSB_MASK);
	temp = DIV_ROUND_CLOSEST(temp * sp.denmtr, sp.numrtr);

	return temp + sp.val_offset;
}

#define EXPONENT_MASK		0xF800
#define MANTISSA_MASK		0x3FF
#define SIGN			BIT(10)
#define EXPONENT_SHIFT		11
#define MICRO_UNIT		1000000ULL
static int fg_decode_float(struct fg_sram_param sp, u8 *raw_val)
{
	int64_t final_val, exponent_val, mantissa_val;
	int exponent, mantissa, n, i;
	bool sign;
	int value = 0;

	for (i = 0; i < sp.length; ++i)
		value |= raw_val[i] << (8 * i);

	exponent = (value & EXPONENT_MASK) >> EXPONENT_SHIFT;
	mantissa = (value & MANTISSA_MASK);
	sign = !!(value & SIGN);

	mantissa_val = mantissa * MICRO_UNIT;

	n = exponent - 15;
	if (n < 0)
		exponent_val = MICRO_UNIT >> -n;
	else
		exponent_val = MICRO_UNIT << n;

	n = n - 10;
	if (n < 0)
		mantissa_val >>= -n;
	else
		mantissa_val <<= n;

	final_val = exponent_val + mantissa_val;

	if (sign)
		final_val *= -1;

	return final_val;
}

static int fg_decode_value_le(struct fg_sram_param sp, u8 *value)
{
	int temp = value[1] << 8 | value[0];
	if (sp.numrtr)
		return temp * sp.numrtr;
	return temp;
}

static int fg_decode_default(struct fg_sram_param sp, u8 *value)
{
	int temp = (0xffffffff >> (8 * (4 - sp.length))) & *(int *) value;
	if (sp.numrtr)
		return temp * sp.numrtr;
	return temp;
}

static void fg_encode_voltage(struct fg_sram_param sp, int val_mv, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;

	val_mv += sp.val_offset;
	temp = (int64_t)div_u64((u64)val_mv * sp.numrtr, sp.denmtr);
	for (i = 0; i < sp.length; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
	}
}

static void fg_encode_current(struct fg_sram_param sp, int val_ma, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;
	s64 current_ma;

	current_ma = val_ma;
	temp = (int64_t)div_s64(current_ma * sp.numrtr, sp.denmtr);
	for (i = 0; i < sp.length; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
	}
}

static void fg_encode_roundoff(struct fg_sram_param sp, int val, u8 *buf)
{
	int i, mask = 0xff;

	val = DIV_ROUND_CLOSEST(val * sp.numrtr, sp.denmtr);
	for (i = 0; i < sp.length; ++i)
		buf[i] = val & (mask >> (8 * i));
}

static void fg_encode_voltcmp8(struct fg_sram_param sp, int val, u8 *buf)
{
	buf[0] = (u8)((val - 2500000) / 9766);
}

static void fg_encode_default(struct fg_sram_param sp, int val, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;

	temp = (int64_t)div_s64((s64)val * sp.numrtr, sp.denmtr);
	for (i = 0; i < sp.length; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
	}
}

#define MIN_HALFFLOAT_EXP_N		-15
#define MAX_HALFFLOAT_EXP_N		 16
static int log2_floor(int64_t uval)
{
	int n = 0;
	int64_t i = MICRO_UNIT;

	if (uval > i) {
		while (uval > i && n > MIN_HALFFLOAT_EXP_N) {
			i <<= 1;
			n += 1;
		}
		if (uval < i)
			n -= 1;
	} else if (uval < i) {
		while (uval < i && n < MAX_HALFFLOAT_EXP_N) {
			i >>= 1;
			n -= 1;
		}
	}

	return n;
}

static int64_t exp2_int(int64_t n)
{
	int p = n - 1;

	if (p > 0)
		return (2 * MICRO_UNIT) << p;
	else
		return (2 * MICRO_UNIT) >> abs(p);
}

static void fg_encode_float(struct fg_sram_param sp, int val, u8 *buf)
{
	int sign = 0, n, exp, mantissa;
	u16 half = 0;

	if (val < 0) {
		sign = 1;
		val = abs(val);
	}
	n = log2_floor(val);
	exp = n + 15;
	mantissa = div_s64(div_s64((val - exp2_int(n)) * exp2_int(10 - n),
				MICRO_UNIT) + MICRO_UNIT / 2, MICRO_UNIT);

	half = (mantissa & MANTISSA_MASK) | ((sign << 10) & SIGN)
		| ((exp << 11) & EXPONENT_MASK);

	buf[0] = half >> 1;
	buf[1] = half & 0xff;
}

static void fg_encode_bcl(struct fg_sram_param sp, int val, u8 *buf)
{
	buf[0] = val * sp.numrtr / sp.denmtr;
}

static void fg_encode_therm_delay(struct fg_sram_param sp, int val, u8 *buf)
{
	if (sp.value < 2560)
		buf[0] = 0;
	else if (sp.value > 163840)
		buf[0] = 7;
	else
		buf[0] = ilog2(sp.value / 10) - 7;
}

static void fg_encode_adc(struct fg_sram_param sp, int val, u8 *buf)
{
	val = DIV_ROUND_CLOSEST(val * sp.numrtr, sp.denmtr);
	buf[0] = val & 0xff;
	buf[1] = val >> 8;
}

static int fg_write(struct fg_chip *chip, u8 *val, u16 addr, int len)
{
	int rc;
	bool sec_access = (addr & 0xff) > 0xd0;
	u8 sec_addr_val = 0xa5;

	if (sec_access) {
		pr_info("sec_access addr = %d\n", addr);
		rc = regmap_bulk_write(chip->regmap,
				(addr & 0xff00) | 0xd0,
				&sec_addr_val, 1);
	}

	if ((addr & 0xff00) == 0) {
		pr_err("addr cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_write(chip->regmap, addr, val, len);
}

static int fg_read(struct fg_chip *chip, u8 *val, u16 addr, int len)
{
	if ((addr & 0xff00) == 0) {
		pr_err("base cannot be zero base=0x%02x\n", addr);
		return -EINVAL;
	}

	return regmap_bulk_read(chip->regmap, addr, val, len);
}

static int fg_masked_write(struct fg_chip *chip, u16 addr,
		u8 mask, u8 val)
{
	int error;
	u8 reg;
	error = fg_read(chip, &reg, addr, 1);
	if (error)
		return error;

	reg &= ~mask;
	reg |= val & mask;

	error = fg_write(chip, &reg, addr, 1);
	return error;
}

#define IMA_IACS_CLR			BIT(2)
#define IMA_IACS_RDY			BIT(1)
static int fg_run_iacs_clear_sequence(struct fg_chip *chip)
{
	int rc = 0;
	u8 temp;

	/* clear the error */
	rc = fg_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
				IMA_IACS_CLR, IMA_IACS_CLR);
	if (rc) {
		pr_err("Error writing to IMA_CFG, rc=%d\n", rc);
		return rc;
	}

	temp = 0x4;
	rc = fg_write(chip, &temp, MEM_INTF_ADDR_LSB(chip) + 1, 1);
	if (rc) {
		pr_err("Error writing to MEM_INTF_ADDR_MSB, rc=%d\n", rc);
		return rc;
	}

	temp = 0x0;
	rc = fg_write(chip, &temp, MEM_INTF_WR_DATA0(chip) + 3, 1);
	if (rc) {
		pr_err("Error writing to WR_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = fg_read(chip, &temp, MEM_INTF_RD_DATA0(chip) + 3, 1);
	if (rc) {
		pr_err("Error writing to RD_DATA3, rc=%d\n", rc);
		return rc;
	}

	rc = fg_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
				IMA_IACS_CLR, 0);
	if (rc) {
		pr_err("Error writing to IMA_CFG, rc=%d\n", rc);
		return rc;
	}
	return rc;
}

#define IACS_ERR_BIT		BIT(0)
#define XCT_ERR_BIT		BIT(1)
#define DATA_RD_ERR_BIT		BIT(3)
#define DATA_WR_ERR_BIT		BIT(4)
#define ADDR_BURST_WRAP_BIT	BIT(5)
#define ADDR_RNG_ERR_BIT	BIT(6)
#define ADDR_SRC_ERR_BIT	BIT(7)
static int fg_check_and_clear_ima_errors(struct fg_chip *chip,
		bool check_hw_sts)
{
	int rc = 0, ret = 0;
	u8 err_sts = 0, exp_sts = 0, hw_sts = 0;
	bool run_err_clr_seq = false;

	rc = fg_read(chip, &err_sts,
			chip->mem_base + MEM_INTF_IMA_ERR_STS, 1);
	if (rc) {
		pr_err("failed to read IMA_ERR_STS, rc=%d\n", rc);
		return rc;
	}

	rc = fg_read(chip, &exp_sts,
			chip->mem_base + MEM_INTF_IMA_EXP_STS, 1);
	if (rc) {
		pr_err("Error in reading IMA_EXP_STS, rc=%d\n", rc);
		return rc;
	}


	if (check_hw_sts) {
		rc = fg_read(chip, &hw_sts,
				chip->mem_base + MEM_INTF_IMA_HW_STS, 1);
		if (rc) {
			pr_err("Error in reading IMA_HW_STS, rc=%d\n", rc);
			return rc;
		}
		/*
		 * Lower nibble should be equal to upper nibble before SRAM
		 * transactions begins from SW side. If they are unequal, then
		 * the error clear sequence should be run irrespective of IMA
		 * exception errors.
		 */
		if ((hw_sts & 0x0f) != hw_sts >> 4) {
			pr_err("IMA HW not in correct state, hw_sts=%x\n",
					hw_sts);
			run_err_clr_seq = true;
		}
	}

	if (exp_sts & (IACS_ERR_BIT | XCT_ERR_BIT | DATA_RD_ERR_BIT |
		DATA_WR_ERR_BIT | ADDR_BURST_WRAP_BIT | ADDR_RNG_ERR_BIT |
		ADDR_SRC_ERR_BIT)) {
		pr_err("IMA exception bit set, exp_sts=%x\n", exp_sts);
		run_err_clr_seq = true;
	}

	if (run_err_clr_seq) {
		ret = fg_run_iacs_clear_sequence(chip);
		if (!ret)
			return -EAGAIN;
		else
			pr_err("Error clearing IMA exception ret=%d\n", ret);
	}

	return rc;
}

#define RESET_MASK	(BIT(7) | BIT(5))
static int fg_reset(struct fg_chip *chip, bool reset)
{
	return fg_masked_write(chip, chip->soc_base + SOC_FG_RESET,
		0xff, reset ? RESET_MASK : 0);
}

static int fg_set_sram_param(struct fg_chip *chip, enum fg_sram_param_id id,
		u8 *val);

#define EN_WR_FGXCT_PRD		BIT(6)
#define EN_RD_FGXCT_PRD		BIT(5)
#define FG_RESTART_TIMEOUT_MS	12000
static int fg_check_ima_error_handling(struct fg_chip *chip)
{
	int rc;
	u8 buf[4] = {0, 0, 0, 0};
	//TODO:
	//fg_disable_irqs(chip);

	/* Acquire IMA access forcibly from FG ALG */
	rc = fg_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
			EN_WR_FGXCT_PRD | EN_RD_FGXCT_PRD,
			EN_WR_FGXCT_PRD | EN_RD_FGXCT_PRD);
	if (rc) {
		dev_err(chip->dev, "Error in writing to IMA_CFG, rc=%d\n", rc);
		goto out;
	}

	/* Release the IMA access now so that FG reset can go through */
	rc = fg_masked_write(chip, chip->mem_base + MEM_INTF_IMA_CFG,
			EN_WR_FGXCT_PRD | EN_RD_FGXCT_PRD, 0);
	if (rc) {
		dev_err(chip->dev, "Error in writing to IMA_CFG, rc=%d\n", rc);
		goto out;
	}

	/* Assert FG reset */
	rc = fg_reset(chip, true);
	if (rc) {
		dev_err(chip->dev, "Couldn't reset FG\n");
		goto out;
	}

	/* Deassert FG reset */
	rc = fg_reset(chip, false);
	if (rc) {
		dev_err(chip->dev, "Couldn't clear FG reset\n");
		goto out;
	}

	/* Wait for at least a FG cycle before doing SRAM access */
	msleep(2000);

	chip->init_fn(chip);

out:
	rc = fg_set_sram_param(chip, FG_DATA_VINT_ERR, buf);
	if (rc < 0)
		dev_err(chip->dev, "Error in clearing VACT_INT_ERR, rc=%d\n",
				rc);
	//TODO:
	//fg_enable_irqs(chip);
	return rc;
}

#define FGXCT_PRD		BIT(7)
#define ALG_ST_CHECK_COUNT	20
static int fg_check_alg_status(struct fg_chip *chip)
{
	int rc = 0, timeout = ALG_ST_CHECK_COUNT, count = 0;
	u8 ima_opr_sts, alg_sts = 0, temp = 0;

	if (!chip->reset_on_lockup)  {
		pr_err("FG lockup detection cannot be run\n");
		return 0;
	}

	rc = fg_read(chip, &alg_sts, chip->soc_base + SOC_ALG_ST, 1);
	if (rc) {
		dev_err(chip->dev, "Error in reading SOC_ALG_ST, rc=%d\n", rc);
		return rc;
	}

	do {
		rc = fg_read(chip, &ima_opr_sts,
			chip->mem_base + MEM_INTF_IMA_OPR_STS, 1);
		if (!rc && !(ima_opr_sts & FGXCT_PRD))
			break;

		if (rc) {
			dev_err(chip->dev, "Error in reading IMA_OPR_STS, rc=%d\n",
				rc);
			break;
		}

		rc = fg_read(chip, &temp, chip->soc_base + SOC_ALG_ST,
			1);
		if (rc) {
			dev_err(chip->dev, "Error in reading SOC_ALG_ST, rc=%d\n",
				rc);
			break;
		}

		if ((ima_opr_sts & FGXCT_PRD) && (temp == alg_sts))
			count++;

		/* Wait for ~10ms while polling ALG_ST & IMA_OPR_STS */
		usleep_range(9000, 11000);
	} while (--timeout);

	if (count == ALG_ST_CHECK_COUNT) {
		/* If we are here, that means FG ALG is stuck */
		dev_err(chip->dev, "ALG is stuck\n");
		fg_check_ima_error_handling(chip);
		rc = -EBUSY;
	}
	return rc;
}

static int fg_check_iacs_ready(struct fg_chip *chip)
{
	int rc = 0, timeout = 250;
	u8 ima_opr_sts = 0;

	/*
	 * Additional delay to make sure IACS ready bit is set after
	 * Read/Write operation.
	 */

	usleep_range(30, 35);
	do {
		rc = fg_read(chip, &ima_opr_sts,
			chip->mem_base + MEM_INTF_IMA_OPR_STS, 1);
		if (!rc && (ima_opr_sts & IMA_IACS_RDY))
			break;
		/* delay for iacs_ready to be asserted */
		usleep_range(5000, 7000);
	} while (--timeout && !rc);

	if (!timeout || rc) {
		dev_err(chip->dev, "IACS_RDY not set, ima_opr_sts: %x\n",
				ima_opr_sts);
		rc = fg_check_alg_status(chip);
		if (rc && rc != -EBUSY)
			dev_err(chip->dev, "Couldn't check FG ALG status, rc=%d\n",
				rc);
		/* perform IACS_CLR sequence */
		fg_check_and_clear_ima_errors(chip, false);
		return -EBUSY;
	}

	return 0;
}

#define INTF_CTL_BURST		BIT(7)
#define INTF_CTL_WR_EN		BIT(6)
static int fg_config_access(struct fg_chip *chip, bool write,
		bool burst)
{
	int rc;
	u8 intf_ctl;

	intf_ctl = (write ? INTF_CTL_WR_EN : 0) | (burst ? INTF_CTL_BURST : 0);

	rc = fg_write(chip, &intf_ctl, MEM_INTF_CTL(chip), 1);
	if (rc) {
		pr_err("failed to set mem access bit\n");
		return -EIO;
	}

	return rc;
}

static int fg_set_ram_addr(struct fg_chip *chip, u16 *address)
{
	int rc;

	rc = fg_write(chip, (u8 *) address, MEM_INTF_ADDR_LSB(chip), 2);
	if (rc) {
		pr_err("spmi write failed: addr=%03X, rc=%d\n",
				MEM_INTF_ADDR_LSB(chip), rc);
		return rc;
	}

	return rc;
}

#define BUF_LEN		4
static int fg_sub_sram_read(struct fg_chip *chip, u8 *val, u16 address, int len,
		int offset)
{
	int rc, total_len;
	u8 *rd_data = val;

	rc = fg_config_access(chip, 0, (len > 4));
	if (rc)
		return rc;

	rc = fg_set_ram_addr(chip, &address);
	if (rc)
		return rc;

	total_len = len;
	while (len > 0) {
		if (!offset) {
			rc = fg_read(chip, rd_data, MEM_INTF_RD_DATA0(chip),
							min(len, BUF_LEN));
		} else {
			rc = fg_read(chip, rd_data,
				MEM_INTF_RD_DATA0(chip) + offset,
				min(len, BUF_LEN - offset));

			/* manually set address to allow continous reads */
			address += BUF_LEN;

			rc = fg_set_ram_addr(chip, &address);
			if (rc)
				return rc;
		}
		if (rc) {
			pr_err("spmi read failed: addr=%03x, rc=%d\n",
				MEM_INTF_RD_DATA0(chip) + offset, rc);
			return rc;
		}
		rd_data += (BUF_LEN - offset);
		len -= (BUF_LEN - offset);
		offset = 0;
	}

	return rc;
}


#define IACS_SLCT			BIT(5)
static int __fg_interleaved_sram_write(struct fg_chip *chip, u8 *val,
				u16 address, int len, int offset)
{
	int rc = 0, i;
	u8 *word = val, byte_enable = 0, num_bytes = 0;

	while (len > 0) {
			num_bytes = (offset + len) > BUF_LEN ?
				(BUF_LEN - offset) : len;
			/* write to byte_enable */
			for (i = offset; i < (offset + num_bytes); i++)
				byte_enable |= BIT(i);

			rc = fg_write(chip, &byte_enable,
				chip->mem_base + MEM_INTF_IMA_BYTE_EN, 1);
			if (rc) {
				pr_err("Unable to write to byte_en_reg rc=%d\n",
								rc);
				return rc;
			}
			/* write data */
		rc = fg_write(chip, word, MEM_INTF_WR_DATA0(chip) + offset,
				num_bytes);
		if (rc) {
			pr_err("spmi write failed: addr=%03x, rc=%d\n",
				MEM_INTF_WR_DATA0(chip) + offset, rc);
			return rc;
		}
		/*
		 * The last-byte WR_DATA3 starts the write transaction.
		 * Write a dummy value to WR_DATA3 if it does not have
		 * valid data. This dummy data is not written to the
		 * SRAM as byte_en for WR_DATA3 is not set.
		 */
		if (!(byte_enable & BIT(3))) {
			u8 dummy_byte = 0x0;
			rc = fg_write(chip, &dummy_byte,
				MEM_INTF_WR_DATA0(chip) + 3, 1);
			if (rc) {
				pr_err("Unable to write dummy-data to WR_DATA3 rc=%d\n",
									rc);
				return rc;
			}
		}

		rc = fg_check_iacs_ready(chip);
		if (rc) {
			pr_err("IACS_RDY failed post write to address %x offset %d rc=%d\n",
				address, offset, rc);
			return rc;
		}

		/* check for error condition */
		rc = fg_check_and_clear_ima_errors(chip, false);
		if (rc) {
			pr_err("IMA transaction failed rc=%d", rc);
			return rc;
		}

		word += num_bytes;
		len -= num_bytes;
		offset = byte_enable = 0;
	}

	return rc;
}

#define RIF_MEM_ACCESS_REQ	BIT(7)
static int fg_check_rif_mem_access(struct fg_chip *chip, bool *status)
{
	int rc;
	u8 mem_if_sts;

	rc = fg_read(chip, &mem_if_sts, MEM_INTF_CFG(chip), 1);
	if (rc) {
		pr_err("failed to read rif_mem status rc=%d\n", rc);
		return rc;
	}

	*status = mem_if_sts & RIF_MEM_ACCESS_REQ;
	return 0;
}

#define IMA_REQ_ACCESS		(IACS_SLCT | RIF_MEM_ACCESS_REQ)
static int fg_interleaved_sram_config(struct fg_chip *chip, u8 *val,
		u16 address, int len, int offset, int op)
{
	int rc = 0;
	bool rif_mem_sts = true;
	int time_count;

	/*
	 * Try this no more than 4 times. If RIF_MEM_ACCESS_REQ is not
	 * clear, then return an error instead of waiting for it again.
	 */
	for (time_count = 0; time_count < 4; ++time_count) {
		rc = fg_check_rif_mem_access(chip, &rif_mem_sts);
		if (rc)
			return rc;

		if (!rif_mem_sts)
			break;

		/* Wait for 4ms before reading RIF_MEM_ACCESS_REQ again */
		usleep_range(4000, 4100);
	}
	if  (time_count >= 4) {
		pr_err("Waited for ~16ms polling RIF_MEM_ACCESS_REQ\n");
		return -ETIMEDOUT;
	}


	/* configure for IMA access */
	rc = fg_masked_write(chip, MEM_INTF_CFG(chip),
				IMA_REQ_ACCESS, IMA_REQ_ACCESS);
	if (rc) {
		pr_err("failed to set mem access bit rc = %d\n", rc);
		return rc;
	}

	/* configure for the read/write single/burst mode */
	rc = fg_config_access(chip, op, (offset + len) > 4);
	if (rc) {
		pr_err("failed to set configure memory access rc = %d\n", rc);
		return rc;
	}

	rc = fg_check_iacs_ready(chip);
	if (rc) {
		pr_err("IACS_RDY failed before setting address: %x offset: %d rc=%d\n",
			address, offset, rc);
		return rc;
	}

	/* write addresses to the register */
	rc = fg_set_ram_addr(chip, &address);
	if (rc) {
		pr_err("failed to set SRAM address rc = %d\n", rc);
		return rc;
	}

	rc = fg_check_iacs_ready(chip);
	if (rc)
		pr_err("IACS_RDY failed after setting address: %x offset: %d rc=%d\n",
			address, offset, rc);

	return rc;
}

static int fg_release_access(struct fg_chip *chip)
{
	return fg_masked_write(chip, MEM_INTF_CFG(chip),
			RIF_MEM_ACCESS_REQ, 0);
}

static inline int fg_assert_sram_access(struct fg_chip *chip)
{
	int rc;
	u8 mem_if_sts;

	rc = fg_read(chip, &mem_if_sts, INT_RT_STS(chip->mem_base), 1);
	if (rc) {
		pr_err("failed to read mem status rc=%d\n", rc);
		return rc;
	}

	if ((mem_if_sts & BIT(FG_MEM_AVAIL)) == 0) {
		pr_err("mem_avail not high: %02x\n", mem_if_sts);
		return -EINVAL;
	}

	rc = fg_read(chip, &mem_if_sts, MEM_INTF_CFG(chip), 1);
	if (rc) {
		pr_err("failed to read mem status rc=%d\n", rc);
		return rc;
	}

	if ((mem_if_sts & RIF_MEM_ACCESS_REQ) == 0) {
		pr_err("rif_mem_access not high: %02x\n", mem_if_sts);
		return -EINVAL;
	}

	return 0;
}

static bool fg_check_sram_access(struct fg_chip *chip)
{
	int rc;
	u8 mem_if_sts;
	bool rif_mem_sts = false;

	rc = fg_read(chip, &mem_if_sts, INT_RT_STS(chip->mem_base), 1);
	if (rc) {
		pr_err("failed to read mem status rc=%d\n", rc);
		return false;
	}

	if ((mem_if_sts & BIT(FG_MEM_AVAIL)) == 0)
		return false;

	rc = fg_check_rif_mem_access(chip, &rif_mem_sts);
	if (rc)
		return false;

	return rif_mem_sts;
}

static int fg_req_access(struct fg_chip *chip)
{
	int rc;

	if (!fg_check_sram_access(chip)) {
		rc = fg_masked_write(chip, MEM_INTF_CFG(chip),
			RIF_MEM_ACCESS_REQ, RIF_MEM_ACCESS_REQ);
		if (rc) {
			pr_err("failed to set mem access bit\n");
			return -EIO;
		}
	}

	if (fg_check_sram_access(chip))
		return 0;
	return -EIO;
}

static int fg_conventional_sram_read(struct fg_chip *chip, u8 *val, u16 address,
		int len, int offset, bool keep_access)
{
	int rc = 0, orig_address = address;

	if (offset > 3) {
		pr_err("offset too large %d\n", offset);
		return -EINVAL;
	}

	address = ((orig_address + offset) / 4) * 4;
	offset = (orig_address + offset) % 4;

	mutex_lock(&chip->lock);
	if (!fg_check_sram_access(chip)) {
		rc = fg_req_access(chip);
		if (rc)
			goto out;
	}

	rc = fg_sub_sram_read(chip, val, address, len, offset);

out:
	rc = fg_assert_sram_access(chip);
	if (rc) {
		pr_err("memread access not correct\n");
		rc = 0;
	}

	if (!keep_access && !rc) {
		rc = fg_release_access(chip);
		if (rc) {
			pr_err("failed to set mem access bit\n");
			rc = -EIO;
		}
	}

	mutex_unlock(&chip->lock);
	return rc;
}

static int fg_conventional_sram_write(struct fg_chip *chip, u8 *val, u16 address,
		int len, int offset, bool keep_access)
{
	int rc = 0, user_cnt = 0, sublen;
	bool access_configured = false;
	u8 *wr_data = val, word[4];
	u16 orig_address = address;

	if (address < RAM_OFFSET && chip->pmic_version == PMI8950)
		return -EINVAL;

	if (offset > 3)
		return -EINVAL;

	address = ((orig_address + offset) / 4) * 4;
	offset = (orig_address + offset) % 4;

	mutex_lock(&chip->lock);
	if (!fg_check_sram_access(chip)) {
		rc = fg_req_access(chip);
		if (rc)
			goto out;
	}

	while (len > 0) {
		if (offset != 0) {
			sublen = min(4 - offset, len);
			rc = fg_sub_sram_read(chip, word, address, 4, 0);
			if (rc)
				goto out;
			memcpy(word + offset, wr_data, sublen);
			/* configure access as burst if more to write */
			rc = fg_config_access(chip, 1, (len - sublen) > 0);
			if (rc)
				goto out;
			rc = fg_set_ram_addr(chip, &address);
			if (rc)
				goto out;
			offset = 0;
			access_configured = true;
		} else if (len >= 4) {
			if (!access_configured) {
				rc = fg_config_access(chip, 1, len > 4);
				if (rc)
					goto out;
				rc = fg_set_ram_addr(chip, &address);
				if (rc)
					goto out;
				access_configured = true;
			}
			sublen = 4;
			memcpy(word, wr_data, 4);
		} else if (len > 0 && len < 4) {
			sublen = len;
			rc = fg_sub_sram_read(chip, word, address, 4, 0);
			if (rc)
				goto out;
			memcpy(word, wr_data, sublen);
			rc = fg_config_access(chip, 1, 0);
			if (rc)
				goto out;
			rc = fg_set_ram_addr(chip, &address);
			if (rc)
				goto out;
			access_configured = true;
		} else {
			pr_err("Invalid length: %d\n", len);
			break;
		}
		rc = fg_write(chip, word, MEM_INTF_WR_DATA0(chip), 4);
		if (rc) {
			pr_err("spmi write failed: addr=%03x, rc=%d\n",
					MEM_INTF_WR_DATA0(chip), rc);
			goto out;
		}
		len -= sublen;
		wr_data += sublen;
		address += 4;
	}

out:
	fg_assert_sram_access(chip);

	if (!keep_access && (user_cnt == 0) && !rc) {
		rc = fg_release_access(chip);
		if (rc) {
			pr_err("failed to set mem access bit\n");
			rc = -EIO;
		}
	}

	mutex_unlock(&chip->lock);
	return rc;
}

#define RETRY_COUNT_MAX	3
static int fg_interleaved_sram_write(struct fg_chip *chip, u8 *val, u16 address,
							int len, int offset)
{
	int rc = 0, ret, orig_address = address;
	u8 count = 0;
	bool retry = false;

	if (address < RAM_OFFSET && chip->pmic_version == PMI8950) {
		pr_err("invalid addr = %x\n", address);
		return -EINVAL;
	}

	if (offset > 3) {
		pr_err("offset too large %d\n", offset);
		return -EINVAL;
	}

	address = ((orig_address + offset) / 4) * 4;
	offset = (orig_address + offset) % 4;

	mutex_lock(&chip->lock);

retry:
	if (count >= RETRY_COUNT_MAX) {
		pr_err("Retried writing 3 times\n");
		retry = false;
		goto out;
	}

	rc = fg_interleaved_sram_config(chip, val, address, len, offset, 1);
	if (rc) {
		pr_err("failed to configure SRAM for IMA rc = %d\n", rc);
		retry = true;
		count++;
		goto out;
	}

	/* write data */
	rc = __fg_interleaved_sram_write(chip, val, address, len, offset);
	if (rc) {
		count++;
		if ((rc == -EAGAIN) && (count < RETRY_COUNT_MAX)) {
			pr_err("IMA access failed retry_count = %d\n", count);
			goto retry;
		} else {
			pr_err("failed to write SRAM address rc = %d\n", rc);
			retry = true;
			goto out;
		}
	}

out:
	/* Release IMA access */
	ret = fg_masked_write(chip, MEM_INTF_CFG(chip), IMA_REQ_ACCESS, 0);
	if (ret)
		pr_err("failed to reset IMA access bit ret = %d\n", ret);

	if (retry) {
		retry = false;
		goto retry;
	}

	mutex_unlock(&chip->lock);
	return rc;
}

static int fg_sram_write(struct fg_chip *chip, u8 *val, u16 address,
		int len, int offset, bool keep_access)
{
	if (chip->ima_supported)
		return fg_interleaved_sram_write(chip, val, address,
				len, offset);
	else
		return fg_conventional_sram_write(chip, val, address,
				len, offset, keep_access);
}

static int fg_set_sram_param(struct fg_chip *chip, enum fg_sram_param_id id,
		u8 *val)
{
	u8 buf[4];
	struct fg_sram_param param = chip->param[id];

	if (!param.address || !param.length)
		return -EINVAL;

	if (param.length > ARRAY_SIZE(buf))
		return -EINVAL;

	if (param.encode) {
		param.encode(param, param.value, buf);
		return fg_sram_write(chip, buf, param.address, param.length,
				param.offset, false);
	} else {
		return fg_sram_write(chip, val, param.address, param.length,
				param.offset, false);
	}
}

static int __fg_interleaved_sram_read(struct fg_chip *chip, u8 *val,
		u16 address, int len, int offset)
{
	int rc = 0, total_len;
	u8 *rd_data = val, num_bytes;

	total_len = len;
	while (len > 0) {
		num_bytes = (offset + len) > BUF_LEN ? (BUF_LEN - offset) : len;
		rc = fg_read(chip, rd_data,
				MEM_INTF_RD_DATA0(chip) + offset,
								num_bytes);
		if (rc) {
			pr_err("spmi read failed: addr=%03x, rc=%d\n",
				MEM_INTF_RD_DATA0(chip) + offset,
				rc);
			return rc;
		}

		rd_data += num_bytes;
		len -= num_bytes;
		offset = 0;

		rc = fg_check_iacs_ready(chip);
		if (rc) {
			pr_err("IACS_RDY failed post read for address %x offset %d rc=%d\n",
					address, offset, rc);
			return rc;
		}

		/* check for error condition */
		rc = fg_check_and_clear_ima_errors(chip, false);
		if (rc) {
			pr_err("IMA transaction failed rc=%d", rc);
			return rc;
		}

		if (len && (len + offset) < BUF_LEN) {
			/* move to single mode */
			u8 intr_ctl = 0;

			rc = fg_write(chip, &intr_ctl, MEM_INTF_CTL(chip), 1);
			if (rc) {
				pr_err("failed to move to single mode rc=%d\n",
					rc);
				return -EIO;
			}
		}
	}

	return rc;
}

static int fg_interleaved_sram_read(struct fg_chip *chip, u8 *val, u16 address,
		int len, int offset)
{
	int rc = 0, orig_address = address, ret = 0;
	u8 start_beat_count, end_beat_count, count = 0;
	const u8 BEAT_COUNT_MASK = 0x0f;
	bool retry = false;

	if (offset > 3) {
		pr_err("offset too large %d\n", offset);
		return -EINVAL;
	}

	address = ((orig_address + offset) / 4) * 4;
	offset = (orig_address + offset) % 4;

	if (address < RAM_OFFSET && chip->pmic_version == PMI8950) {
		/*
		 * OTP memory reads need a conventional memory access, do a
		 * conventional read when SRAM offset < RAM_OFFSET.
		 */
		rc = fg_conventional_sram_read(chip, val, address, len, offset,
						0);
		if (rc)
			pr_err("Failed to read OTP memory %d\n", rc);
		return rc;
	}

	mutex_lock(&chip->lock);

retry:
	if (count >= RETRY_COUNT_MAX) {
		pr_err("Retried reading 3 times\n");
		retry = false;
		goto out;
	}

	rc = fg_interleaved_sram_config(chip, val, address, len, offset, 0);
	if (rc) {
		pr_err("failed to configure SRAM for IMA rc = %d\n", rc);
		retry = true;
		count++;
		goto out;
	}

	/* read the start beat count */
	rc = fg_read(chip, &start_beat_count,
			chip->mem_base + MEM_INTF_BEAT_COUNT, 1);
	if (rc) {
		pr_err("failed to read beat count rc=%d\n", rc);
		retry = true;
		count++;
		goto out;
	}

	/* read data */
	rc = __fg_interleaved_sram_read(chip, val, address, len, offset);
	if (rc) {
		count++;
		if (rc == -EAGAIN) {
			pr_err("IMA access failed retry_count = %d\n", count);
			goto retry;
		} else {
			pr_err("failed to read SRAM address rc = %d\n", rc);
			retry = true;
			goto out;
		}
	}

	/* read the end beat count */
	rc = fg_read(chip, &end_beat_count,
			chip->mem_base + MEM_INTF_BEAT_COUNT, 1);
	if (rc) {
		pr_err("failed to read beat count rc=%d\n", rc);
		retry = true;
		count++;
		goto out;
	}

	start_beat_count &= BEAT_COUNT_MASK;
	end_beat_count &= BEAT_COUNT_MASK;
	if (start_beat_count != end_beat_count) {
		retry = true;
		count++;
	}
out:
	ret = fg_release_access(chip);
	if (ret)
		pr_err("failed to reset IMA access bit ret = %d\n", ret);

	if (retry) {
		retry = false;
		goto retry;
	}

	mutex_unlock(&chip->lock);
	return rc;
}

static int fg_sram_read(struct fg_chip *chip, u8 *val, u16 address,
		int len, int offset, bool keep_access)
{
	if (chip->ima_supported)
		return fg_interleaved_sram_read(chip, val, address,
				len, offset);
	else
		return fg_conventional_sram_read(chip, val, address,
				len, offset, keep_access);
}

static int fg_get_param(struct fg_chip *chip, enum fg_sram_param_id id,
		int *val)
{
	struct fg_sram_param param = chip->param[id];
	u8 buf[4];
	int rc;

	if (id < 0 || id >= FG_PARAM_MAX || param.length > ARRAY_SIZE(buf))
		return -EINVAL;

	if (!param.address || !param.length)
		return -EINVAL;

	switch (param.type) {
	case SRAM_PARAM:
		rc = fg_sram_read(chip, buf, param.address, param.length, param.offset,
				false);
		break;
	case BATT_BASE_PARAM:
		rc = fg_read(chip, buf, chip->batt_base + param.address, param.length);
		break;
	case SOC_BASE_PARAM:
		rc = fg_read(chip, buf, chip->soc_base + param.address, param.length);
		break;
	case MEM_IF_PARAM:
		rc = fg_read(chip, buf, chip->mem_base + param.address, param.length);
		break;
	default:
		pr_err("Invalid param type: %d\n", param.type);
		return -EINVAL;
	}
	if (rc)
		return rc;
	if (param.decode)
		*val = param.decode(param, buf);
	else
		*val = fg_decode_default(param, buf);

	return 0;
}

static int fg_sram_masked_write_param(struct fg_chip *chip,
		enum fg_sram_param_id id, u8 mask, u8 val)
{
	int rc = 0;
	u8 reg[4];
	u8 encoded_val;
	struct fg_sram_param param = chip->param[id];

	if (param.encode)
		param.encode(param, val, &encoded_val);
	else
		encoded_val = val;

	rc = fg_sram_read(chip, reg, param.address, 4, 0, true);
	if (rc)
		return rc;

	reg[param.offset] &= ~mask;
	reg[param.offset] |= encoded_val & mask;

	rc = fg_sram_write(chip, reg, param.address, 4, 0, false);

	return rc;
}

static int fg_get_capacity(struct fg_chip *chip, int *val)
{
	u8 cap[2];
	int error = fg_read(chip, cap, chip->soc_base + SOC_MONOTONIC_SOC, 2);
	if (error)
		return error;
	//choose lesser of two
	if (cap[0] != cap[1]) {
		dev_warn(chip->dev, "cap not same\n");
		cap[0] = cap[0] < cap[1] ? cap[0] : cap[1];
	}
	*val = DIV_ROUND_CLOSEST((cap[0] - 1) * 98, 0xff - 2) + 1;
	return 0;
}

#define VBATT_LOW_STS_BIT BIT(2)
static int fg_get_vbatt_status(struct fg_chip *chip, bool *vbatt_low_sts)
{
	int rc = 0;
	u8 fg_batt_sts;

	rc = fg_read(chip, &fg_batt_sts, INT_RT_STS(chip->batt_base), 1);
	if (rc)
		pr_err("vbatt spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->batt_base), rc);
	else
		*vbatt_low_sts = !!(fg_batt_sts & VBATT_LOW_STS_BIT);

	return rc;
}

static int fg_get_temperature(struct fg_chip *chip, int *val)
{
	int rc, temp;
	int cold = chip->param[FG_SETTING_BATT_COLD_TEMP].value;
	int cool = chip->param[FG_SETTING_BATT_COOL_TEMP].value;

	rc = fg_get_param(chip, FG_DATA_BATT_TEMP, &temp);
	if (rc) {
		pr_err("failed to read temperature\n");
		return rc;
	}
	if (temp < 0 && chip->pmic_version == PMI8950)
			temp += -50 * (cool - temp) / (cool - cold);
	*val = temp;
	return 0;
}

static int fg_get_health_status(struct fg_chip *chip)
{
	int temp;
	int rc;
	rc = fg_get_temperature(chip, &temp);
	if (rc) {
		return rc;
	}

	if (temp >= chip->param[FG_SETTING_BATT_HOT_TEMP].value)
		chip->health = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (temp <= chip->param[FG_SETTING_BATT_COLD_TEMP].value)
		chip->health = POWER_SUPPLY_HEALTH_COLD;
	else
		chip->health = POWER_SUPPLY_HEALTH_GOOD;
	return chip->health;
}

#define FULL_PERCENT_28BIT	0xFFFFFFF
static int fg_learn_cap(struct fg_chip *chip)
{
	//TODO: run in parallel
	//TODO: enable/disable based on temperature
	int cc_pc_val, rc = -EINVAL;
	unsigned int cc_soc_delta_pc;
	int64_t delta_cc_uah;
	int temp;
	/*if (is_battery_missing(chip)) {
		pr_err("Battery is missing!\n");
		return rc;
	}*/

	rc = fg_get_param(chip, FG_DATA_ACT_CAP,
			&chip->learning_data.actual_cap_uah);
	if (rc)
		dev_warn(chip->dev, "failed to get actual_cap\n");


	/*if (!fg_is_temperature_ok_for_learning(chip))
		return rc;*/

	if (!chip->learning_data.init_cc_pc_val) {
		rc = fg_get_param(chip, FG_DATA_CHARGE, &temp);
		if (rc) {
			dev_err(chip->dev, "failed to read CC_SOC rc=%d\n", rc);
			return rc;
		}
		chip->learning_data.init_cc_pc_val =
			div64_s64(chip->batt_info.nom_cap_uah * temp,
					100 * FULL_RESOLUTION);
		return 0;
	}

	temp = abs(cc_pc_val - chip->learning_data.init_cc_pc_val);
	cc_soc_delta_pc = DIV_ROUND_CLOSEST_ULL(temp * 100, FULL_PERCENT_28BIT);

	delta_cc_uah = div64_s64(
			chip->batt_info.nom_cap_uah * cc_soc_delta_pc,
			100);
	chip->learning_data.cc_uah = delta_cc_uah + chip->learning_data.cc_uah;

	return 0;
}

#define RCONN_CONFIG_BIT	BIT(0)
#define PROFILE_INTEGRITY_WORD	79
#define RCONN_CONF_STS_OFFSET	0
static int fg_rconn_config_pmi8998(struct fg_chip *chip)
{
	int rc, esr_uohms;
	u64 scaling_factor;
	u32 val = 0;

	if (!chip->batt_info.rconn_mohm)
		return 0;

	rc = fg_sram_read(chip, (u8 *)&val, PROFILE_INTEGRITY_WORD,
			1, RCONN_CONF_STS_OFFSET, false);
	if (rc < 0) {
		pr_err("Error in reading RCONN_CONF_STS, rc=%d\n", rc);
		return rc;
	}

	if (val & RCONN_CONFIG_BIT)
		return 0;

	rc = fg_get_param(chip, FG_DATA_BATT_ESR, &esr_uohms);
	if (rc < 0) {
		pr_err("failed to get ESR, rc=%d\n", rc);
		return rc;
	}

	scaling_factor = div64_u64((u64)esr_uohms * 1000,
				esr_uohms +
				(chip->batt_info.rconn_mohm * 1000));

	rc = fg_get_param(chip, FG_SETTING_RSLOW_CHG, &val);
	if (rc < 0) {
		pr_err("Error in reading ESR_RSLOW_CHG, rc=%d\n", rc);
		return rc;
	}

	val *= scaling_factor;
	do_div(val, 1000);
	rc = fg_set_sram_param(chip, FG_SETTING_RSLOW_CHG, (u8 *)&val);
	if (rc < 0) {
		pr_err("Error in writing ESR_RSLOW_CHG, rc=%d\n", rc);
		return rc;
	}

	rc = fg_set_sram_param(chip, FG_SETTING_RSLOW_DISCHG, (u8 *)&val);
	if (rc < 0) {
		pr_err("Error in reading ESR_RSLOW_DISCHG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	val *= scaling_factor;
	do_div(val, 1000);
	rc = fg_set_sram_param(chip, FG_SETTING_RSLOW_DISCHG, (u8 *)&val);
	if (rc < 0) {
		pr_err("Error in writing ESR_RSLOW_DISCHG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	val = RCONN_CONFIG_BIT;
	rc = fg_sram_write(chip, (u8 *)&val, PROFILE_INTEGRITY_WORD, 1,
			RCONN_CONF_STS_OFFSET, false);
	if (rc < 0) {
		pr_err("Error in writing RCONN_CONF_STS, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int fg_rconn_config_pmi8950(struct fg_chip *chip)
{
	int rs_to_rslow_chg, rs_to_rslow_dischg, batt_esr, rconn_uohm;
	int rc;

	rc = fg_get_param(chip, FG_DATA_BATT_ESR, &batt_esr);
	if (rc) {
		pr_err("unable to read battery_esr: %d\n", rc);
		return rc;
	}

	rc = fg_get_param(chip, FG_SETTING_RSLOW_DISCHG, &rs_to_rslow_dischg);
	if (rc) {
		pr_err("unable to read rs to rslow dischg: %d\n", rc);
		return rc;
	}

	rc = fg_get_param(chip, FG_SETTING_RSLOW_CHG, &rs_to_rslow_chg);
	if (rc) {
		pr_err("unable to read rs to rslow chg: %d\n", rc);
		return rc;
	}

	rconn_uohm = chip->batt_info.rconn_mohm * 1000;
	rs_to_rslow_dischg = div64_s64(rs_to_rslow_dischg * batt_esr,
					batt_esr + rconn_uohm);
	rs_to_rslow_chg = div64_s64(rs_to_rslow_chg * batt_esr,
					batt_esr + rconn_uohm);

	rc = fg_set_sram_param(chip, FG_SETTING_RSLOW_CHG,
			(u8 *)&rs_to_rslow_chg);
	if (rc) {
		pr_err("unable to write rs_to_rslow_chg: %d\n", rc);
		return rc;
	}

	rc = fg_set_sram_param(chip, FG_SETTING_RSLOW_DISCHG,
			(u8 *)&rs_to_rslow_dischg);
	if (rc) {
		pr_err("unable to write rs_to_rslow_dischg: %d\n", rc);
		return rc;
	}
	return 0;
}

#define FG_CYCLE_MS			1500
#define BCL_FORCED_HPM_IN_CHARGE	BIT(2)
#define IRQ_USE_VOLTAGE_HYST_BIT	BIT(0)
#define EMPTY_FROM_VOLTAGE_BIT		BIT(1)
#define EMPTY_FROM_SOC_BIT		BIT(2)
#define EMPTY_SOC_IRQ_MASK		(IRQ_USE_VOLTAGE_HYST_BIT | \
					EMPTY_FROM_SOC_BIT | \
					EMPTY_FROM_VOLTAGE_BIT)
static int fg_hw_init_pmi8950(struct fg_chip *chip)
{
	int rc;
	u8 buf[4];

	buf[0] = 0xff;
	rc = fg_write(chip, buf, INT_EN_CLR(chip->mem_base), 1);
	if (rc) {
		pr_err("failed to clear interrupts\n");
		return rc;
	}

	rc = fg_sram_masked_write_param(chip, FG_SETTING_EXTERNAL_SENSE,
			BIT(2), 0);

	if (chip->param[FG_SETTING_THERMAL_COEFFS].length > 0) {
		rc = fg_set_sram_param(chip, FG_SETTING_THERMAL_COEFFS,
				chip->batt_info.thermal_coeffs);
		if (rc) {
			dev_err(chip->dev, "error writing thermal coeffs to "\
					"batt soc");
		}
	}

	rc = fg_sram_masked_write_param(chip, FG_SETTING_THERM_DELAY, 0xe0,
			buf[0]);
	if (rc) {
		pr_err("failed to write therm_delay rc=%d\n", rc);
		return rc;
	}

	/*
	 * Clear bits 0-2 in 0x4B3 and set them again to make empty_soc irq
	 * trigger again.
	 */
	rc = fg_sram_masked_write_param(chip, FG_SETTING_ALG,
			EMPTY_SOC_IRQ_MASK, 0);
	if (rc) {
		pr_err("failed to write to fg_alg_sysctl rc=%d\n", rc);
		return rc;
	}

	/* Wait for a FG cycle before enabling empty soc irq configuration */
	msleep(FG_CYCLE_MS);

	rc = fg_sram_masked_write_param(chip, FG_SETTING_ALG,
			EMPTY_SOC_IRQ_MASK, EMPTY_SOC_IRQ_MASK);
	if (rc) {
		pr_err("failed to write to fg_alg_sysctl rc=%d\n", rc);
		return rc;
	}

	rc = fg_sram_masked_write_param(chip, FG_SETTING_ADC_CONF,
			BCL_FORCED_HPM_IN_CHARGE, BCL_FORCED_HPM_IN_CHARGE);
	if (rc) {
		dev_err(chip->dev, "failed to set HPM_IN_CHARGE\n");
		return rc;
	}

	rc = fg_rconn_config_pmi8950(chip);
	if (rc) {
		pr_err("failed to config rconn\n");
		return rc;
	}

	return rc;
}

static int fg_hw_init_pmi8998(struct fg_chip *chip)
{
	int rc;
	//u8 buf[4];

	//TODO: needs proper dts setup
	//TODO: write temperatures to sram
	//TODO: write ESR
	//TODO: write sleep thresh
	if (chip->param[FG_SETTING_THERMAL_COEFFS].length > 0) {
		rc = fg_write(chip, chip->batt_info.thermal_coeffs,
				BATT_INFO_THERM_C1(chip),
				chip->param[FG_SETTING_THERMAL_COEFFS].length);
		if (rc < 0) {
			pr_err("failed to write thermal coefficients, rc=%d\n",
				rc);
			return rc;
		}
	}

	if (chip->pmic_version == PMI8998_V1)
		;

	rc = fg_rconn_config_pmi8998(chip);
	if (rc) {
		pr_err("failed to config rconn\n");
		return rc;
	}
	return 0;
}

static int get_vbat_est_diff(struct fg_chip *chip, int *diff)
{
	int pred_volt, volt, rc;

	rc = fg_get_param(chip, FG_DATA_VOLTAGE, &volt);
	if (rc) {
		pr_err("failed to read voltage\n");
		return rc;
	}

	rc = fg_get_param(chip, FG_DATA_CPRED_VOLTAGE, &pred_volt);
	if (rc) {
		pr_err("failed to read pred volt\n");
		return rc;
	}

	*diff = abs(volt - pred_volt);

	return 0;
}

#define PROFILE_LOAD_BIT	BIT(0)
#define BOOTLOADER_LOAD_BIT	BIT(1)
#define BOOTLOADER_RESTART_BIT	BIT(2)
#define HLOS_RESTART_BIT	BIT(3)
#define FG_PROFILE_LEN_PMI8950		128
#define FG_PROFILE_LEN_PMI8998		224
#define PROFILE_INTEGRITY_BIT		BIT(0)
static bool is_profile_load_required(struct fg_chip *chip)
{
	u8 buf[FG_PROFILE_LEN_PMI8998];
	u32 val;
	bool profiles_same = false;
	int rc;

	rc = fg_get_param(chip, FG_BATT_PROFILE_INTEGRITY, &val);
	if (rc < 0) {
		pr_err("failed to read profile integrity rc=%d\n", rc);
		return false;
	}

	/* Check if integrity bit is set */
	if (val & PROFILE_LOAD_BIT) {
		/* Whitelist the values */
		val &= ~PROFILE_LOAD_BIT;
		if (val != HLOS_RESTART_BIT && val != BOOTLOADER_LOAD_BIT &&
			val != (BOOTLOADER_LOAD_BIT | BOOTLOADER_RESTART_BIT) &&
			chip->pmic_version != PMI8950) {
			val |= PROFILE_LOAD_BIT;
			pr_warn("Garbage value in profile integrity word: 0x%x\n",
				val);
			return true;
		}

		rc = fg_sram_read(chip, buf, chip->param[FG_BATT_PROFILE].address,
				chip->param[FG_BATT_PROFILE].length,
				chip->param[FG_BATT_PROFILE].offset, false);
		if (rc < 0) {
			pr_err("Error in reading battery profile, rc:%d\n", rc);
			return false;
		}
		profiles_same = memcmp(chip->batt_info.batt_profile, buf,
					chip->batt_info.batt_profile_len) == 0;
		if (profiles_same)
			return false;
	}
	return true;
}

#define BATT_PROFILE_OFFSET		0x4C0
#define PROFILE_COMPARE_LEN		32
#define ESR_MAX				300000
#define ESR_MIN				5000
static int fg_of_battery_profile_init(struct fg_chip *chip)
{
	struct device_node *batt_node;
	struct device_node *node = chip->dev->of_node;
	int rc = 0, len = 0;
	const char *data;

	batt_node = of_find_node_by_name(node, "qcom,battery-data");
	if (!batt_node) {
		pr_err("No available batterydata\n");
		return rc;
	}

	chip->batt_info.manufacturer = of_get_property(batt_node,
			"manufacturer", &len);
	if (!chip->batt_info.manufacturer || !len)
		pr_warn("manufacturer not dound in dts");

	chip->batt_info.model = of_get_property(batt_node,
			"model", &len);
	if (!chip->batt_info.model || !len)
		pr_warn("model not dound in dts");

	chip->batt_info.serial_num = of_get_property(batt_node,
			"serial-number", &len);
	if (!chip->batt_info.manufacturer || !len)
		pr_warn("serial number not dound in dts");

	rc = of_property_read_u32(batt_node, "qcom,chg-rs-to-rslow",
					&chip->rslow_comp.chg_rs_to_rslow);
	if (rc) {
		dev_err(chip->dev, "Could not read rs to rslow: %d\n", rc);
		return -EINVAL;
	}

	rc = of_property_read_u32(batt_node, "qcom,chg-rslow-comp-c1",
					&chip->rslow_comp.chg_rslow_comp_c1);
	if (rc) {
		dev_err(chip->dev, "Could not read rslow comp c1: %d\n", rc);
		return -EINVAL;
	}
	rc = of_property_read_u32(batt_node, "qcom,chg-rslow-comp-c2",
					&chip->rslow_comp.chg_rslow_comp_c2);
	if (rc) {
		dev_err(chip->dev, "Could not read rslow comp c2: %d\n", rc);
		return -EINVAL;
	}
	rc = of_property_read_u32(batt_node, "qcom,chg-rslow-comp-thr",
					&chip->rslow_comp.chg_rslow_comp_thr);
	if (rc) {
		dev_err(chip->dev, "Could not read rslow comp thr: %d\n", rc);
		return -EINVAL;
	}

	rc = of_property_read_u32(batt_node, "qcom,max-voltage-uv",
					&chip->batt_info.batt_max_voltage_uv);

	if (rc)
		dev_warn(chip->dev, "couldn't find battery max voltage\n");

	data = of_get_property(chip->dev->of_node,
			"qcom,thermal-coefficients", &len);
	if (data && ((len == 6 && chip->pmic_version == PMI8950) ||
			(len == 3 && chip->pmic_version != PMI8950))) {
		memcpy(chip->batt_info.thermal_coeffs, data, len);
		chip->param[FG_SETTING_THERMAL_COEFFS].length = len;
	}

	data = of_get_property(batt_node, "qcom,fg-profile-data", &len);
	if (!data) {
		pr_err("no battery profile loaded\n");
		return 0;
	}

	if ((chip->pmic_version == PMI8950 && len != FG_PROFILE_LEN_PMI8950) ||
		(chip->pmic_version == PMI8998_V1 && len != FG_PROFILE_LEN_PMI8998) ||
		(chip->pmic_version == PMI8998_V2 && len != FG_PROFILE_LEN_PMI8998)) {
		pr_err("battery profile incorrect size: %d\n", len);
		return -EINVAL;
	}

	chip->batt_info.batt_profile = devm_kzalloc(chip->dev,
			sizeof(char) * len, GFP_KERNEL);

	if (!chip->batt_info.batt_profile) {
		pr_err("coulnt't allocate mem for battery profile\n");
		rc = -ENOMEM;
		return rc;
	}

	if (!is_profile_load_required(chip))
		goto done;
	else
		pr_warn("profile load needs to be done, but not done!\n");

done:
	rc = fg_get_param(chip, FG_DATA_NOM_CAP, &chip->batt_info.nom_cap_uah);
	if (rc) {
		pr_err("Failed to read nominal capacitance: %d\n", rc);
		return -EINVAL;
	}

	//estimate_battery_age(fg, &fg->actual_cap_uah);
	return rc;
}

static int fg_parse_of_param(struct fg_chip *chip, enum fg_sram_param_id id)
{
	int rc, temp;
	struct fg_sram_param param = chip->param[id];

	if (!param.name)
		return 0;

	rc = of_property_read_u32(chip->dev->of_node, param.name, &temp);
	if (rc) {
		if (param.value == -EINVAL) {//ignore optional prop
			pr_warn("prop %s not found in dts, ignoring\n", param.name);
			return 0;
		}
		pr_warn("using default value for prop %s: %d\n", param.name, param.value);
		temp = param.value; //default value
	}
	chip->param[id].value = temp;

	if (!param.length)
		//0 length indicates its a masked write thingy
		return 0;

	return fg_set_sram_param(chip, id, (u8 *)&temp);
}

static int fg_of_init(struct fg_chip *chip)
{
	int rc, i;

	for (i = FG_SETTING_TERM_CURRENT; i < FG_PARAM_MAX; ++i) {
		rc = fg_parse_of_param(chip, i);
		if (rc) {
			pr_err("failed to write property %d\n", i);
			return rc;
		}
	}

	of_property_read_u32(chip->dev->of_node,
			"qcom,vbat-estimate-diff-mv",
			&chip->vbatt_est_diff);

	chip->reset_on_lockup = of_property_read_bool(chip->dev->of_node,
			"qcom,fg-reset-on-lockup");

	rc = of_property_read_u32(chip->dev->of_node, "qcom,fg-rconn-mohm",
					&chip->batt_info.rconn_mohm);
	if (rc)
		chip->batt_info.rconn_mohm = 1;

	return 0;
}

//TODO: clean these up
#define DMA_WRITE_ERROR_BIT			BIT(1)
#define DMA_READ_ERROR_BIT			BIT(2)
#define DMA_CLEAR_LOG_BIT			BIT(0)
int fg_check_and_clear_dma_errors(struct fg_chip *chip)
{
	int rc;
	u8 dma_sts;
	bool error_present;

	//TODO: cleanup DMA sts reads
	rc = fg_read(chip, &dma_sts, chip->mem_base + 0x70, 1);
	if (rc < 0) {
		pr_err("failed to dma_sts, rc=%d\n", rc);
		return rc;
	}

	error_present = dma_sts & (DMA_WRITE_ERROR_BIT | DMA_READ_ERROR_BIT);
	rc = fg_masked_write(chip, chip->mem_base + 0x71, DMA_CLEAR_LOG_BIT,
			error_present ? DMA_CLEAR_LOG_BIT : 0);
	if (rc < 0) {
		pr_err("failed to write dma_ctl, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

#define IACS_INTR_SRC_SLCT	BIT(3)
static int fg_init_memif(struct fg_chip *chip)
{
	static struct register_offset offset[] = {
			/* CFG   CTL   LSB   RD0   WD0 */
		[0] = {
			.address = {0x40, 0x41, 0x42, 0x4C, 0x48},
		},
		[1] = {
			.address = {0x50, 0x51, 0x61, 0x67, 0x63},
		},
	};
	enum dig_rev_offset {
		DIG_MINOR = 0x0,
		DIG_MAJOR = 0x1,
		ANA_MINOR = 0x2,
		ANA_MAJOR = 0x3,
	};
	enum dig_major {
		DIG_REV_1 = 0x1,
		DIG_REV_2 = 0x2,
		DIG_REV_3 = 0x3,
	};
	int rc;

	rc = fg_read(chip, chip->revision, chip->mem_base + DIG_MINOR, 4);
	if (rc) {
		dev_err(chip->dev, "Unable to read FG revision rc=%d\n", rc);
		return rc;
	}

	switch (chip->revision[DIG_MAJOR]) {
	case DIG_REV_1:
	case DIG_REV_2:
		chip->offset = offset[0].address;
		break;
	case DIG_REV_3:
		chip->offset = offset[1].address;
		chip->ima_supported = true;
		break;
	default:
		pr_err("Digital Major rev=%d not supported\n",
				chip->revision[DIG_MAJOR]);
		return -EINVAL;
	}

	if (chip->pmic_version == PMI8998_V1 || chip->pmic_version == PMI8998_V2) {
		chip->offset = offset[1].address;
		chip->ima_supported = true;
	}

	pr_info("FG Probe - FG Revision DIG:%d.%d ANA:%d.%d PMIC subtype=%d\n",
		chip->revision[DIG_MAJOR], chip->revision[DIG_MINOR],
		chip->revision[ANA_MAJOR], chip->revision[ANA_MINOR],
		chip->pmic_version);

	if (chip->ima_supported) {
		/*
		 * Change the FG_MEM_INT interrupt to track IACS_READY
		 * condition instead of end-of-transaction. This makes sure
		 * that the next transaction starts only after the hw is ready.
		 */
		rc = fg_masked_write(chip,
			chip->mem_base + MEM_INTF_IMA_CFG, IACS_INTR_SRC_SLCT,
			IACS_INTR_SRC_SLCT);
		if (rc) {
			dev_err(chip->dev,
				"failed to configure interrupt source %d\n",
				rc);
			return rc;
		}

		/* check for error condition */
		rc = fg_check_and_clear_ima_errors(chip, true);
		if (rc && rc != -EAGAIN) {
			pr_err("Error in clearing IMA exception rc=%d", rc);
			return rc;
		}
	}

	if (chip->pmic_version == PMI8998_V1 ||
		chip->pmic_version == PMI8998_V2)
		fg_check_and_clear_dma_errors(chip);

	return 0;
}

static enum power_supply_property fg_properties[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_STATUS,
};

static int fg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct fg_chip *chip = power_supply_get_drvdata(psy);
	int error = 0, temp;
	bool vbatt_low_sts;

	switch (psp) {
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = chip->batt_info.manufacturer;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = chip->batt_info.model;
		break;
	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
		val->strval = chip->batt_info.serial_num;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		error = fg_get_capacity(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		error = fg_get_param(chip, FG_DATA_CURRENT, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		error = fg_get_param(chip, FG_DATA_VOLTAGE, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		error = fg_get_param(chip, FG_DATA_OCV, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chip->batt_info.batt_max_voltage_uv;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		error = fg_get_temperature(chip, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		error = fg_get_param(chip, FG_DATA_CYCLE_COUNT, &val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		val->intval = 3000000; /* single-cell li-ion low end */
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		error = fg_get_param(chip, FG_DATA_CHARGE_COUNTER, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = chip->batt_info.nom_cap_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = chip->learning_data.actual_cap_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = chip->learning_data.cc_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		temp = chip->param[FG_SETTING_CHG_TERM_CURRENT].value * 1000;
		val->intval = temp;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = fg_get_health_status(chip);
		break;
	default:
		pr_err("invalid property: %d\n", psp);
		return -EINVAL;
	}
	return error;
}

static int fg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct fg_chip *chip = power_supply_get_drvdata(psy);
	//int error = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		chip->status = val->intval;
		fg_learn_cap(chip);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		chip->health = val->intval;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int fg_writable_property(struct power_supply *psy,
		enum power_supply_property psp)
{
	return 0;
}

static const struct power_supply_desc bms_psy_desc = {
	.name = "bms-bif",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = fg_properties,
	.num_properties = ARRAY_SIZE(fg_properties),
	.get_property = fg_get_property,
	.set_property = fg_set_property,
	.property_is_writeable = fg_writable_property,
};

static int fg_probe(struct platform_device *pdev)
{
	struct power_supply_config bms_cfg = {};
	struct fg_chip *chip;
	const __be32 *prop_addr;
	int rc;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	mutex_init(&chip->lock);

	chip->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!chip->regmap) {
		dev_err(&pdev->dev, "failed to locate the regmap\n");
		return -ENODEV;
	}

	prop_addr = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "invalid IO resources\n");
		return -EINVAL;
	}
	chip->soc_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 1, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "invalid IO resources\n");
		return -EINVAL;
	}
	chip->batt_base = be32_to_cpu(*prop_addr);

	prop_addr = of_get_address(pdev->dev.of_node, 2, NULL, NULL);
	if (!prop_addr) {
		dev_err(chip->dev, "invalid IO resources\n");
		return -EINVAL;
	}
	chip->mem_base = be32_to_cpu(*prop_addr);

	chip->pmic_version = (enum pmic) device_get_match_data(chip->dev);
	switch (chip->pmic_version) {
	case PMI8950:
		chip->param = fg_params_pmi8950;
		chip->init_fn = fg_hw_init_pmi8950;
		break;
	case PMI8998_V1:
		chip->param = fg_params_pmi8998_v1;
		chip->init_fn = fg_hw_init_pmi8998;
		break;
	case PMI8998_V2:
		chip->param = fg_params_pmi8998_v2;
		chip->init_fn = fg_hw_init_pmi8998;
		break;
	}

	chip->status = POWER_SUPPLY_STATUS_DISCHARGING;

	rc = fg_init_memif(chip);
	if (rc) {
		dev_err(chip->dev, "failed to init memif\n");
		return rc;
	}

	rc = fg_of_init(chip);
	if (rc) {
		dev_err(chip->dev, "failed to get config from DTS\n");
		return rc;
	}

	rc = fg_of_battery_profile_init(chip);
	if (rc) {
		pr_err("profile init failed\n");
	}

	rc = chip->init_fn(chip);
	if (rc) {
		dev_err(chip->dev, "failed to init hw\n");
		return rc;
	}

	rc = fg_learn_cap(chip);
	if (rc)
		dev_warn(chip->dev, "learning capacity failed!\n");

	bms_cfg.drv_data = chip;
	bms_cfg.of_node = pdev->dev.of_node;

	chip->bms_psy = devm_power_supply_register(chip->dev,
			&bms_psy_desc, &bms_cfg);
	if (IS_ERR(chip->bms_psy)) {
		dev_err(&pdev->dev, "failed to register battery\n");
		return PTR_ERR(chip->bms_psy);
	}

	platform_set_drvdata(pdev, chip);

	return 0;
}

static int fg_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id fg_match_id_table[] = {
	{ .compatible = "qcom,pmi8950-fg", .data = (void *) PMI8950 },
	{ .compatible = "qcom,pmi8998-v1-fg", .data = (void *) PMI8998_V1 },
	{ .compatible = "qcom,pmi8998-v2-fg", .data = (void *) PMI8998_V2 },
	{ }
};
MODULE_DEVICE_TABLE(of, fg_match_id_table);

static struct platform_driver qcom_fg_driver = {
	.probe = fg_probe,
	.remove = fg_remove,
	.driver = {
		.name = "qcom-fg",
		.of_match_table = fg_match_id_table,
	},
};

module_platform_driver(qcom_fg_driver);

MODULE_DESCRIPTION("Qualcomm Fuel Guage Driver");
MODULE_LICENSE("GPL v2");
