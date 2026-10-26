// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Degdag Mohamed
// Samsung AMB670YF08-FS DSC panel driver (OnePlus 11 5G / salami)
//
// Modes: {QHD+ 1440x3216, FHD+ 1080x2412} x {120, 90, 60} Hz.
// Refresh-rate switching is a full modeset (unprepare + prepare with the new mode);
// the per-mode command streams were checked against the vendor on-command /
// timing-switch-command sequences of the stock OnePlus 11 panel DTS.
//
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

enum panel_amb670_res {
	PANEL_AMB670_RES_QHD = 0, /* native, 1440x3216 */
	PANEL_AMB670_RES_FHD,     /* 1080x2412 */
	PANEL_AMB670_RES_COUNT,
};

struct panel_amb670_res_desc {
	void (*on)(struct mipi_dsi_multi_context *ctx, unsigned int hz);
	/* Second pass sent after on(): CASET/PASET, scaler, FFC, ADFR-off, refresh block */
	void (*post_on)(struct mipi_dsi_multi_context *ctx, unsigned int hz);
	u16 slice_width;
	u16 slice_height;
	u8  slice_count;
};

struct panel_amb670_mode_desc {
	struct drm_display_mode mode;
	enum panel_amb670_res res;
	u8 hz;
};

struct panel_samsung_amb670yf08_fs_1440_3216_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
	struct drm_connector *connector;           /* set in get_modes() */
	enum panel_amb670_res default_res;         /* boot/preferred resolution */
	const struct panel_amb670_mode_desc *cur;  /* mode of the current prepare() */
};

static const struct regulator_bulk_data panel_amb670_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vci" },
};

static inline
struct panel_samsung_amb670yf08_fs_1440_3216_dsc *to_ctx(struct drm_panel *panel)
{
	return container_of_const(panel, struct panel_samsung_amb670yf08_fs_1440_3216_dsc, panel);
}

static void panel_amb670_reset(struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}


/* ---------------------------------------------------------------------- */
/* Refresh-rate dependent blocks (shared by on() and post_on())           */
/* ---------------------------------------------------------------------- */

/* Like mipi_dsi_dcs_write_seq_multi(), but accepts runtime values */
#define amb670_write_var(ctx, cmd, ...)					\
	do {								\
		u8 _d[] = { cmd, __VA_ARGS__ };				\
		mipi_dsi_dcs_write_buffer_multi(ctx, _d, sizeof(_d));	\
	} while (0)

/* Refresh-rate dependent register block 1 (TSP_SYNC1/3): from the vendor on-command/timing-switch-command, per fps */
static void amb670_refresh_a(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	u8 r1 = 0x02;

	switch (hz) {
	case 90:
		r1 = 0x03;
		break;
	case 60:
		r1 = 0x05;
		break;
	default: /* 120 */
		break;
	}

	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0xa1, 0xb1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xb9);
	amb670_write_var(dsi_ctx, 0xb9, r1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x38, 0xb9);
	amb670_write_var(dsi_ctx, 0xb9, r1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
}

/* Refresh-rate dependent register block 2 (120/90/60Hz transition) */
static void amb670_refresh_b(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	u8 cb = 0x24, r2 = 0x00, r3 = 0x00;

	switch (hz) {
	case 90:
		cb = 0xa4; r2 = 0x09;
		break;
	case 60:
		r2 = 0x01; r3 = 0x01;
		break;
	default: /* 120 */
		break;
	}

	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	if (hz != 90) {
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21);
	}
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x08, 0xcb);
	amb670_write_var(dsi_ctx, 0xcb, cb);
	if (hz == 90) {
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21, 0x82);
	}
	amb670_write_var(dsi_ctx, 0x60, r2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	if (hz != 90) {
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xbd);
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x77);
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x14, 0xbd);
		amb670_write_var(dsi_ctx, 0xbd, r3);
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x63, 0xcb);
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x08);
	}
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	if (hz != 90) {
		mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x23, 0x02);
	}
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
}

/* ---------------------------------------------------------------------- */
/* QHD+ (1440x3216, native)                                               */
/* ---------------------------------------------------------------------- */

static void panel_amb670_on_qhd(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9e,
		0x11, 0x00, 0x00, 0xab, 0x30, 0x80, 0x0c, 0x90, 0x05, 0xa0, 0x00,
		0x18, 0x02, 0xd0, 0x02, 0xd0, 0x02, 0x00, 0x02, 0x86, 0x00, 0x20,
		0x02, 0x83, 0x00, 0x0a, 0x00, 0x0d, 0x04, 0x86, 0x03, 0x2e, 0x18,
		0x00, 0x10, 0xf0, 0x07, 0x10, 0x20, 0x00, 0x06, 0x0f, 0x0f, 0x33,
		0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79,
		0x7b, 0x7d, 0x7e, 0x02, 0x02, 0x22, 0x00, 0x2a, 0x40, 0x2a, 0xbe,
		0x3a, 0xfc, 0x3a, 0xfa, 0x3a, 0xf8, 0x3b, 0x38, 0x3b, 0x78, 0x3b,
		0xb6, 0x4b, 0xb6, 0x4b, 0xf4, 0x4b, 0xf4, 0x6c, 0x34, 0x84, 0x74,
		0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9d, 0x01);
	mipi_dsi_dcs_exit_sleep_mode_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 12000, 13000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x10, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x50, 0x60,
		0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50,
		0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 40);
	amb670_refresh_a(dsi_ctx, hz);
	mipi_dsi_msleep(dsi_ctx, 121);
	amb670_refresh_b(dsi_ctx, hz);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_set_tear_on_multi(dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_set_column_address_multi(dsi_ctx, 0x0000, 0x059f);
	mipi_dsi_dcs_set_page_address_multi(dsi_ctx, 0x0000, 0x0c8f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3, 0x00); /* (0xc3, 0x89) = FHD Mode , (0x03, 0x00) = WQHD Mode */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2b, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0x60, 0x63, 0x69);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x46, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x18, 0xb1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb1, 0x05);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0d, 0x63);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x63, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0c, 0x63);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x63, 0x20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x28);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x52, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x1f, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x54, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x1f, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_set_display_on_multi(dsi_ctx);
}

/* ---------------------------------------------------------------------- */
/* FHD+ (1080x2412)                                                       */
/* ---------------------------------------------------------------------- */

static void panel_amb670_on_fhd(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9e,
		0x11, 0x00, 0x00, 0xab, 0x30, 0x80, 0x09, 0x6c, 0x04, 0x38, 0x00,
		0x24, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x3b, 0x00, 0x20,
		0x03, 0x35, 0x00, 0x07, 0x00, 0x0e, 0x03, 0x34, 0x02, 0xd4, 0x18,
		0x00, 0x10, 0xf0, 0x07, 0x10, 0x20, 0x00, 0x06, 0x0f, 0x0f, 0x33,
		0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79,
		0x7b, 0x7d, 0x7e, 0x02, 0x02, 0x22, 0x00, 0x2a, 0x40, 0x2a, 0xbe,
		0x3a, 0xfc, 0x3a, 0xfa, 0x3a, 0xf8, 0x3b, 0x38, 0x3b, 0x78, 0x3b,
		0xb6, 0x4b, 0xb6, 0x4b, 0xf4, 0x4b, 0xf4, 0x6c, 0x34, 0x84, 0x74,
		0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9d, 0x01);
	mipi_dsi_dcs_exit_sleep_mode_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 12000, 13000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x14, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x84, 0xca,
		0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
		0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_usleep_range(dsi_ctx, 17000, 18000);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_MEMORY_START);
	mipi_dsi_dcs_nop_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 40);
	amb670_refresh_a(dsi_ctx, hz);
	mipi_dsi_msleep(dsi_ctx, 121);
	amb670_refresh_b(dsi_ctx, hz);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_set_tear_on_multi(dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_set_column_address_multi(dsi_ctx, 0x0000, 0x0437);
	mipi_dsi_dcs_set_page_address_multi(dsi_ctx, 0x0000, 0x096b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3, 0x89); //   FHD Mode 
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd,
		0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0,
		0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x50, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43,
		0x00, 0x43, 0x00, 0x43, 0x00, 0x43);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x61, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xcc, 0xcc, 0xcc, 0xcc, 0xc0, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00,
		0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2b, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0x60, 0x63, 0x69);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x46, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x18, 0xb1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb1, 0x05);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0d, 0x63);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x63, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0c, 0x63);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x63, 0x20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x28);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x52, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x1f, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x54, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x1f, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_set_display_on_multi(dsi_ctx);
}

/* ---------------------------------------------------------------------- */
/* post_on                                                                */
/* ---------------------------------------------------------------------- */

static void panel_amb670_post_on_qhd(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	/* CASET/PASET Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2a, 0x00, 0x00, 0x05, 0x9f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2b, 0x00, 0x00, 0x0c, 0x8f);

	/* Scaler Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* FFC Setting 171.3MHz WQ MIPI 1363.2Mbps */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x10, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x50, 0x60,
		0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50,
		0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* OPLUS ADFR OFF */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x02, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* Refresh-rate dependent part (TSP_SYNC1/3 + transition), see vendor timing-switch-command */
	amb670_refresh_a(dsi_ctx, hz);
	amb670_refresh_b(dsi_ctx, hz);
}

static void panel_amb670_post_on_fhd(struct mipi_dsi_multi_context *dsi_ctx, unsigned int hz)
{
	/* CASET/PASET Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2a, 0x00, 0x00, 0x04, 0x37);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2b, 0x00, 0x00, 0x09, 0x6b);

	/* Scaler Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3, 0x89);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd,
		0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0,
		0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x50, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43,
		0x00, 0x43, 0x00, 0x43, 0x00, 0x43);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x61, 0xc3);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3,
		0xcc, 0xcc, 0xcc, 0xcc, 0xc0, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00,
		0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* FFC Setting 171.3MHz FHD MIPI 825.6Mbps */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x14, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x84, 0xca,
		0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
		0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* OPLUS ADFR OFF */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x02, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* Refresh-rate dependent part (TSP_SYNC1/3 + transition), see vendor timing-switch-command */
	amb670_refresh_a(dsi_ctx, hz);
	amb670_refresh_b(dsi_ctx, hz);
}


/* ---------------------------------------------------------------------- */
/* Shared off / prepare / unprepare                                       */
/* ---------------------------------------------------------------------- */

static void panel_amb670_off(struct mipi_dsi_multi_context *dsi_ctx)
{
	mipi_dsi_dcs_set_display_off_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 21);
	mipi_dsi_dcs_enter_sleep_mode_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 101);
}

static const struct panel_amb670_res_desc panel_amb670_res[PANEL_AMB670_RES_COUNT] = {
	[PANEL_AMB670_RES_QHD] = {
		.on = panel_amb670_on_qhd,
		.post_on = panel_amb670_post_on_qhd,
		.slice_width = 720,
		.slice_height = 24,
		.slice_count = 2,
	},
	[PANEL_AMB670_RES_FHD] = {
		.on = panel_amb670_on_fhd,
		.post_on = panel_amb670_post_on_fhd,
		.slice_width = 540,
		.slice_height = 36,
		.slice_count = 2,
	},
};

/* ---------------------------------------------------------------------- */
/* Modes (timings from the vendor DTS: same DSI clock for every refresh)  */
/* ---------------------------------------------------------------------- */

#define PANEL_AMB670_MODE(_res, _hz, _hd, _hfp, _hpw, _hbp, _vd, _vfp, _vpw, _vbp) {	\
	.mode = {									\
		.clock = ((_hd) + (_hfp) + (_hpw) + (_hbp)) *				\
			 ((_vd) + (_vfp) + (_vpw) + (_vbp)) * (_hz) / 1000,		\
		.hdisplay = (_hd),							\
		.hsync_start = (_hd) + (_hfp),						\
		.hsync_end = (_hd) + (_hfp) + (_hpw),					\
		.htotal = (_hd) + (_hfp) + (_hpw) + (_hbp),				\
		.vdisplay = (_vd),							\
		.vsync_start = (_vd) + (_vfp),						\
		.vsync_end = (_vd) + (_vfp) + (_vpw),					\
		.vtotal = (_vd) + (_vfp) + (_vpw) + (_vbp),				\
		.width_mm = 70,								\
		.height_mm = 156,							\
		.type = DRM_MODE_TYPE_DRIVER,						\
	},										\
	.res = (_res),									\
	.hz = (_hz),									\
}

static const struct panel_amb670_mode_desc panel_amb670_modes[] = {
	PANEL_AMB670_MODE(PANEL_AMB670_RES_QHD, 120, 1440, 88, 8, 57, 3216, 8, 8, 8),
	/*
	 * QHD 90 Hz disabled: a line of garbage appears near the top whenever that
	 * area is redrawn (all other modes are fine). Cause not found yet.
	 * PANEL_AMB670_MODE(PANEL_AMB670_RES_QHD,  90, 1440, 88, 8, 58, 3216, 8, 8, 8),
	 */
	PANEL_AMB670_MODE(PANEL_AMB670_RES_QHD,  60, 1440, 88, 8, 56, 3216, 8, 8, 8),
	PANEL_AMB670_MODE(PANEL_AMB670_RES_FHD, 120, 1080, 64, 8, 50, 2412, 2, 2, 8),
	PANEL_AMB670_MODE(PANEL_AMB670_RES_FHD,  90, 1080, 64, 8, 49, 2412, 2, 2, 8),
	PANEL_AMB670_MODE(PANEL_AMB670_RES_FHD,  60, 1080, 64, 8, 48, 2412, 2, 2, 8),
};

static int panel_amb670_dsc_slice_for_mode(struct mipi_dsi_device *dsi,
					   const struct drm_display_mode *mode,
					   u16 *slice_width, u16 *slice_height,
					   u8 *slice_count)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(panel_amb670_modes); i++) {
		const struct panel_amb670_mode_desc *m = &panel_amb670_modes[i];
		const struct panel_amb670_res_desc *r;

		if (m->mode.hdisplay != mode->hdisplay ||
		    m->mode.vdisplay != mode->vdisplay)
			continue;

		r = &panel_amb670_res[m->res];
		*slice_width = r->slice_width;
		*slice_height = r->slice_height;
		*slice_count = r->slice_count;
		return 0;
	}

	return -EINVAL;
}

static bool panel_amb670_default_mode(const struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx,
				      const struct panel_amb670_mode_desc *m)
{
	return m->res == ctx->default_res && m->hz == 120;
}

/*
 * drm_panel has no "which mode am I preparing for" argument, so look at the
 * committed CRTC state of our connector. Falls back to the default mode.
 */
static const struct panel_amb670_mode_desc *
panel_amb670_pick_mode(struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx)
{
	struct drm_connector *conn = ctx->connector;
	const struct drm_display_mode *dm = NULL;
	const struct panel_amb670_mode_desc *def = NULL;
	int i;

	if (conn && conn->state && conn->state->crtc && conn->state->crtc->state)
		dm = &conn->state->crtc->state->adjusted_mode;

	for (i = 0; i < ARRAY_SIZE(panel_amb670_modes); i++) {
		const struct panel_amb670_mode_desc *m = &panel_amb670_modes[i];

		if (panel_amb670_default_mode(ctx, m))
			def = m;
		if (dm && m->mode.hdisplay == dm->hdisplay &&
		    m->mode.vdisplay == dm->vdisplay &&
		    m->hz == drm_mode_vrefresh(dm))
			return m;
	}

	return def;
}

static int panel_amb670_prepare(struct drm_panel *panel)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_ctx(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	const struct panel_amb670_mode_desc *m;
	const struct panel_amb670_res_desc *r;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	panel_amb670_reset(ctx);

	m = panel_amb670_pick_mode(ctx);
	r = &panel_amb670_res[m->res];
	ctx->cur = m;
	dev_dbg(dev, "preparing %ux%u@%u\n", m->mode.hdisplay, m->mode.vdisplay, m->hz);

	/*
	 * NOTE: the DSI host reads this config for its own timing/DSC setup
	 * before prepare() runs (prepare_prev_first), so changing the slice
	 * geometry here is too late for a *resolution* switch; the host asks
	 * dsc_slice_for_mode() before mode_set instead.
	 */
	ctx->dsc.slice_width = r->slice_width;
	ctx->dsc.slice_height = r->slice_height;
	ctx->dsc.slice_count = r->slice_count;

	r->on(&dsi_ctx, m->hz);
	r->post_on(&dsi_ctx, m->hz);
	ret = dsi_ctx.accum_err;
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		goto err;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(dev, "failed to transmit PPS: %d\n", ret);
		goto err;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		goto err;
	}

	msleep(28); /* TODO: Is this panel-dependent? */

	return 0;

err:
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);
	return ret;
}

static int panel_amb670_unprepare(struct drm_panel *panel)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_ctx(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	panel_amb670_off(&dsi_ctx);
	if (dsi_ctx.accum_err < 0)
		dev_err(&ctx->dsi->dev, "Failed to un-initialize panel: %d\n", dsi_ctx.accum_err);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);

	return 0;
}

static int panel_amb670_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_ctx(panel);
	int i, count = 0;

	ctx->connector = connector;

	for (i = 0; i < ARRAY_SIZE(panel_amb670_modes); i++) {
		const struct panel_amb670_mode_desc *m = &panel_amb670_modes[i];
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(connector->dev, &m->mode);
		if (!mode)
			return count ? count : -ENOMEM;

		mode->type = DRM_MODE_TYPE_DRIVER;
		if (panel_amb670_default_mode(ctx, m))
			mode->type |= DRM_MODE_TYPE_PREFERRED;
		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
		count++;
	}

	connector->display_info.width_mm = panel_amb670_modes[0].mode.width_mm;
	connector->display_info.height_mm = panel_amb670_modes[0].mode.height_mm;

	return count;
}

static const struct drm_panel_funcs panel_amb670_panel_funcs = {
	.prepare = panel_amb670_prepare,
	.unprepare = panel_amb670_unprepare,
	.get_modes = panel_amb670_get_modes,
};

/* --------------------------------------------------------------- */
/* ------------------------Backlight  ----------------------------*/
/* --------------------------------------------------------------- */

static int panel_amb670_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);

	return mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
}

static int panel_amb670_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	return brightness;
}

static const struct backlight_ops panel_amb670_bl_ops = {
	.update_status = panel_amb670_bl_update_status,
	.get_brightness = panel_amb670_bl_get_brightness,
};

static struct backlight_device *panel_amb670_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 1433,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &panel_amb670_bl_ops, &props);
}

/* ------------------------------------------------------ */
/* -------------------Probe / remove------------------*/
/* ------------------------------------------------------ */

static int panel_amb670_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx;
	const struct panel_amb670_res_desc *r;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct panel_samsung_amb670yf08_fs_1440_3216_dsc, panel,
				   &panel_amb670_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->default_res = PANEL_AMB670_RES_QHD; /* native */

	ret = devm_regulator_bulk_get_const(dev, ARRAY_SIZE(panel_amb670_supplies),
					    panel_amb670_supplies, &ctx->supplies);
	if (ret < 0)
		return ret;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB101010;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panel_amb670_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/*
	 * This panel only supports DSC; unconditionally enable it. Seed the
	 * slice geometry with the default resolution: the DSI host validates
	 * hdisplay % slice_width while probing modes, long before prepare().
	 */
	r = &panel_amb670_res[ctx->default_res];
	dsi->dsc = &ctx->dsc;
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;
	ctx->dsc.convert_rgb = true;
	ctx->dsc.slice_width = r->slice_width;
	ctx->dsc.slice_height = r->slice_height;
	ctx->dsc.slice_count = r->slice_count;

	dsi->dsc_slice_for_mode = panel_amb670_dsc_slice_for_mode;

	/* vendor DTS: qcom,mdss-dsc-slice-per-pkt = <2> */
	dsi->dsc_slice_per_pkt = 2;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void panel_amb670_remove(struct mipi_dsi_device *dsi)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id panel_amb670_of_match[] = {
	{
		.compatible = "panel,samsung-amb670yf08-fs-1440-3216-dsc",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_amb670_of_match);

static struct mipi_dsi_driver panel_amb670_driver = {
	.probe = panel_amb670_probe,
	.remove = panel_amb670_remove,
	.driver = {
		.name = "panel-samsung-amb670yf08-fs-1440-3216-dsc",
		.of_match_table = panel_amb670_of_match,
	},
};
module_mipi_dsi_driver(panel_amb670_driver);

MODULE_AUTHOR("Degdag Mohamed <degdagmohamed@gmail.com>");
MODULE_DESCRIPTION("DRM driver for samsung AMB670YF08 FS dsc cmd mode panel (QHD+/FHD+)");
MODULE_LICENSE("GPL");
