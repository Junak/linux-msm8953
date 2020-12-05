/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2020, The Linux Foundation. All rights reserved. */

#include <linux/backlight.h>
#include <linux/module.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>

#define MIPI_DSI_PANEL_DRIVER(_prefix, _name, _compatible)		\
	static const struct of_device_id _prefix##_of_match[] = {	\
		{ .compatible = _compatible, &_prefix##_info },		\
		{ }							\
	};								\
	MODULE_DEVICE_TABLE(of, tianma_622_v0_of_match);		\
	static struct mipi_dsi_driver _prefix##_driver = {		\
		.probe = panel_mipi_dsi_common_probe,			\
		.remove = panel_mipi_dsi_common_remove,			\
		.driver = {						\
			.name = "panel-" _name,				\
			.of_match_table = _prefix##_of_match,		\
		},							\
	};								\
	module_mipi_dsi_driver(_prefix##_driver)

#define __dsi_write_seq(dsi, suffix, seq...) do {			\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_##suffix(dsi, d, ARRAY_SIZE(d));		\
		if (ret < 0)						\
			return ret;					\
	} while (0)

#define dsi_generic_write_seq(dsi, seq...) __dsi_write_seq(dsi, generic_write, seq)
#define dsi_dcs_write_seq(dsi, seq...) __dsi_write_seq(dsi, dcs_write_buffer, seq)

typedef int (*panel_power_func_t)(struct mipi_dsi_device *dsi);
typedef void (*panel_reset_func_t)(struct gpio_desc *reset_gpio);

extern int panel_mipi_dsi_common_probe(struct mipi_dsi_device *dsi);
extern int panel_mipi_dsi_common_remove(struct mipi_dsi_device *dsi);
extern void* panel_mipi_dsi_common_get_data(struct mipi_dsi_device *dsi);
extern bool panel_mipi_dsi_common_is_prepared(struct mipi_dsi_device *dsi);
extern struct backlight_device* panel_mipi_dsi_common_get_backlight(struct mipi_dsi_device *dsi);

struct panel_mipi_dsi_info {
	const struct drm_display_mode mode;

	panel_reset_func_t reset;
	panel_power_func_t power_on;
	panel_power_func_t power_off;

	/* DSI configuration */
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;

	const struct backlight_ops *bl_ops;
	const struct backlight_properties *bl_props;

	unsigned int data_len;
};
