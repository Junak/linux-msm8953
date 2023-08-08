/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 * Copyright (C) 2020 AngeloGioacchino Del Regno <angelogioacchino.delregno@collabora.com>
 */

#ifndef NT36XXX_H
#define NT36XXX_H

/* Number of bytes for chip identification */
#define NT36XXX_ID_LEN_MAX		6

struct nt36xxx_abs_object {
	u16 x;
	u16 y;
	u16 z;
	u8 tm;
};

struct nt36xxx_fw_info {
	u8 fw_ver;
	u8 x_num;
	u8 y_num;
	u8 max_buttons;
	u16 abs_x_max;
	u16 abs_y_max;
	u16 nvt_pid;
};

struct nt36xxx_mem_map {
	u32 evtbuf_addr;
	u32 pipe0_addr;
	u32 pipe1_addr;
	u32 flash_csum_addr;
	u32 flash_data_addr;
};

enum nt36xxx_chips {
	NT36525_IC = 0,
	NT36672A_IC,
	NT36676F_IC,
	NT36772_IC,
	NT36870_IC,
	NTMAX_IC,
};

struct nt36xxx_trim_table {
	u8 id[NT36XXX_ID_LEN_MAX];
	u8 mask[NT36XXX_ID_LEN_MAX];
	enum nt36xxx_chips mapid;
};

/**
 * enum nt36xxx_fw_state - Firmware state
 * @NT36XXX_STATE_INIT: IC Reset
 * @NT36XXX_STATE_REK: ReK baseline
 * @NT36XXX_STATE_REK_FINISH: Baseline is ready
 * @NT36XXX_STATE_NORMAL_RUN: Firmware is running
 */
enum nt36xxx_cmds {
	NT36XXX_CMD_ENTER_SLEEP = 0x11,
	NT36XXX_CMD_ENTER_WKUP_GESTURE = 0x13,
	NT36XXX_CMD_UNLOCK = 0x35,
	NT36XXX_CMD_BOOTLOADER_RESET = 0x69,
	NT36XXX_CMD_SW_RESET = 0xa5,
	NT36XXX_CMD_SET_PAGE = 0xff,
};

enum nt36xxx_fw_state {
	NT36XXX_STATE_INIT = 0xa0,	/* IC reset */
	NT36XXX_STATE_REK,		/* ReK baseline */
	NT36XXX_STATE_REK_FINISH,	/* Baseline is ready */
	NT36XXX_STATE_NORMAL_RUN,	/* Normal run */
	NT36XXX_STATE_MAX = 0xaf
};

enum nt36xxx_events {
	NT36XXX_EVT_REPORT = 0x00,
	NT36XXX_EVT_CRC = 0x35,
	NT36XXX_EVT_CHIPID = 0x4e,
	NT36XXX_EVT_HOST_CMD = 0x50,
	NT36XXX_EVT_HS_OR_SUBCMD = 0x51,	/* Handshake or subcommand byte */
	NT36XXX_EVT_RESET_COMPLETE = 0x60,
	NT36XXX_EVT_FWINFO = 0x78,
	NT36XXX_EVT_PROJECTID = 0x9a,

};

#endif
