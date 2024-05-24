// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2013, The Linux Foundation. All rights reserved.

static const struct drm_display_mode hx8279_tv101wum_wuxga_mode = {
	.clock = (1200 + 20 + 24 + 46) * (1920 + 30 + 1 + 13) * 60 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 20,
	.hsync_end = 1200 + 20 + 24,
	.htotal = 1200 + 20 + 24 + 46,
	.vdisplay = 1920,
	.vsync_start = 1920 + 30,
	.vsync_end = 1920 + 30 + 1,
	.vtotal = 1920 + 30 + 1 + 13,
	.width_mm = 141,
	.height_mm = 226,
};

static const struct panel_desc_dsi hx8279_tv101wum_wuxga = {
	.desc = {
		.modes = &hx8279_tv101wum_wuxga_mode,
		.num_modes = 1,
		.bpc = 8,
		.size = {
			.width = 141,
			.height = 226,
		},
		.connector_type = DRM_MODE_CONNECTOR_DSI,
	},
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		 MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM,
	.format = MIPI_DSI_FMT_RGB888,
	.lanes = 4,
};
