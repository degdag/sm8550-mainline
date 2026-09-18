// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Degdag Mohamed
// Samsung AMB670YF08-FS DSC panel driver (OnePlus 11 5G / salami)
//
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

static bool fhd_mode;
module_param(fhd_mode, bool, 0444);
MODULE_PARM_DESC(fhd_mode,
	"Boot the panel in FHD+ (1080x2412) instead of native QHD+ (1440x3216). Reboot to change.");

enum panel_amb670_mode_id {
	PANEL_AMB670_MODE_QHD = 0, /* native, 1440x3216 */
	PANEL_AMB670_MODE_FHD,     /* 1080x2412 */
	PANEL_AMB670_MODE_COUNT,
};

struct panel_amb670_mode_desc {
	struct drm_display_mode mode;
	void (*on)(struct mipi_dsi_multi_context *ctx);
	/*
	 * A second pass of CASET/PASET, scaler, FFC, ADFR-off, TSP_SYNC1/3
	 * and 120Hz-transition commands, sent AFTER on() returns and BEFORE
	 * PPS/compression-mode - present in the working 6.14 driver's
	 * prepare() function, largely repeating command categories already
	 * sent inside on() but with re-asserted values. Kept as a faithful
	 * duplicate rather than assumed-redundant and dropped.
	 */
	void (*post_on)(struct mipi_dsi_multi_context *ctx);
	u16 slice_width;
	u16 slice_height;
	u8  slice_count;
};

struct panel_amb670_desc {
	const struct panel_amb670_mode_desc *modes; /* PANEL_AMB670_MODE_COUNT entries */
};

struct panel_samsung_amb670yf08_fs_1440_3216_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
	const struct panel_amb670_desc *desc;
	enum panel_amb670_mode_id mode_id; /* fixed at probe() from fhd_mode param */
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

/* ----------------------------------*/
/* QHD+ (1440x3216, native)      */
/* ----------------------------------*/

static void panel_amb670_on_qhd(struct mipi_dsi_multi_context *dsi_ctx)
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
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0xa1, 0xb1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(dsi_ctx, 121);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
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

/* --------------------------- */
/* FHD+ (1080x2412)          */
/* --------------------------- */

static void panel_amb670_on_fhd(struct mipi_dsi_multi_context *dsi_ctx)
{
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9e,
		0x11, 0x00, 0x00, 0xab, 0x30, 0x80, 0x0c, 0x90, 0x05, 0xa0, 0x00,
		0x43, 0x02, 0xd0, 0x02, 0xd0, 0x02, 0x00, 0x02, 0xc2, 0x00, 0x20,
		0x06, 0x89, 0x00, 0x0a, 0x00, 0x0f, 0x01, 0xd2, 0x01, 0x24, 0x18,
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
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0xa1, 0xb1);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(dsi_ctx, 121);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
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
/* post_on: second command pass sent from prepare(), after on() returns   */
/* ---------------------------------------------------------------------- */

static void panel_amb670_post_on_qhd(struct mipi_dsi_multi_context *dsi_ctx)
{
	/* CASET/PASET Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2a, 0x00, 0x00, 0x05, 0x9f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x2b, 0x00, 0x00, 0x0c, 0x8f);

	/* Scaler Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc3, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* FFC Setting 171.3MHz WQ MIPI 1363.2Mbps */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x10, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x50, 0x60,
		0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50,
		0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60, 0x50, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* OPLUS ADFR OFF */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x02, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* TSP_SYNC1 Fixed Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* TSP_SYNC3 Fixed Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* 120hz Transition */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
}

static void panel_amb670_post_on_fhd(struct mipi_dsi_multi_context *dsi_ctx)
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
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* FFC Setting 171.3MHz FHD MIPI 825.6Mbps */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xc5,
		0x29, 0x14, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x84, 0xca,
		0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
		0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* OPLUS ADFR OFF */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x02, 0x60);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* TSP_SYNC1 Fixed Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* TSP_SYNC3 Fixed Setting */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);

	/* 120hz Transition */
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
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

static int panel_amb670_prepare(struct drm_panel *panel)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_ctx(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	const struct panel_amb670_mode_desc *m;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	panel_amb670_reset(ctx);

	m = &ctx->desc->modes[ctx->mode_id];

	/* DSC slice geometry depends on which mode we're bringing up */
	ctx->dsc.slice_width = m->slice_width;
	ctx->dsc.slice_height = m->slice_height;
	ctx->dsc.slice_count = m->slice_count;

	m->on(&dsi_ctx);
	ret = dsi_ctx.accum_err;
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);
		return ret;
	}

	/* Second pass: CASET/PASET, scaler, FFC, ADFR-off, TSP_SYNC1/3,
	 * 120Hz-transition - see comment on post_on's declaration. */
	m->post_on(&dsi_ctx);
	ret = dsi_ctx.accum_err;
	if (ret < 0) {
		dev_err(dev, "Failed post_on sequence: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(panel_amb670_supplies), ctx->supplies);
		return ret;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(dev, "failed to transmit PPS: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		return ret;
	}

	msleep(28); /* TODO: Is this panel-dependent? */

	return 0;
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

/* ---------------------------------------------------------------------- */
/* Modes / desc table                                                     */
/* ---------------------------------------------------------------------- */

static const struct panel_amb670_mode_desc panel_amb670_modes[PANEL_AMB670_MODE_COUNT] = {
	[PANEL_AMB670_MODE_QHD] = {
		.mode = {
			.clock = (1440 + 88 + 8 + 57) * (3216 + 8 + 8 + 8) * 120 / 1000,
			.hdisplay = 1440,
			.hsync_start = 1440 + 88,
			.hsync_end = 1440 + 88 + 8,
			.htotal = 1440 + 88 + 8 + 57,
			.vdisplay = 3216,
			.vsync_start = 3216 + 8,
			.vsync_end = 3216 + 8 + 8,
			.vtotal = 3216 + 8 + 8 + 8,
			.width_mm = 70,
			.height_mm = 156,
			.type = DRM_MODE_TYPE_DRIVER,
		},
		.on = panel_amb670_on_qhd,
		.post_on = panel_amb670_post_on_qhd,
		.slice_width = 720,
		.slice_height = 24,
		.slice_count = 2,
	},
	[PANEL_AMB670_MODE_FHD] = {
		.mode = {
			.clock = (1080 + 64 + 8 + 50) * (2412 + 2 + 2 + 8) * 120 / 1000,
			.hdisplay = 1080,
			.hsync_start = 1080 + 64,
			.hsync_end = 1080 + 64 + 8,
			.htotal = 1080 + 64 + 8 + 50,
			.vdisplay = 2412,
			.vsync_start = 2412 + 2,
			.vsync_end = 2412 + 2 + 2,
			.vtotal = 2412 + 2 + 2 + 8,
			.width_mm = 70,
			.height_mm = 156,
			.type = DRM_MODE_TYPE_DRIVER,
		},
		.on = panel_amb670_on_fhd,
		.post_on = panel_amb670_post_on_fhd,
		.slice_width = 540,
		.slice_height = 36,
		.slice_count = 2,
	},
};

static const struct panel_amb670_desc panel_amb670_desc = {
	.modes = panel_amb670_modes,
};

static int panel_amb670_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_ctx(panel);
	const struct drm_display_mode *m = &ctx->desc->modes[ctx->mode_id].mode;
	struct drm_display_mode *mode;

	/* Only the single mode picked at probe() time (via fhd_mode) is ever
	 * registered - see the file header comment for why both can't be
	 * listed simultaneously on this DSI host driver. */
	mode = drm_mode_duplicate(connector->dev, m);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u\n",
			m->hdisplay, m->vdisplay);
		return -ENOMEM;
	}

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = m->width_mm;
	connector->display_info.height_mm = m->height_mm;

	return 1;
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
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct panel_samsung_amb670yf08_fs_1440_3216_dsc, panel,
				   &panel_amb670_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->desc = of_device_get_match_data(dev);
	if (!ctx->desc)
		return -ENODEV;

	ctx->mode_id = fhd_mode ? PANEL_AMB670_MODE_FHD : PANEL_AMB670_MODE_QHD;
	dev_info(dev, "probing in %s mode\n",
		 fhd_mode ? "FHD+ (1080x2412)" : "QHD+ (1440x3216, native)");

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

	/* This panel only supports DSC; unconditionally enable it.
	 *
	 * Seed slice_width/height/count with the SELECTED mode's values
	 * here at probe time (not just QHD's) - the DSI host driver
	 * validates hdisplay % slice_width during mode probing/validation,
	 * which runs right after probe() and well before .prepare() is ever
	 * called, against whatever single mode get_modes() registered. */
	dsi->dsc = &ctx->dsc;
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;
	ctx->dsc.convert_rgb = true;
	ctx->dsc.slice_width = ctx->desc->modes[ctx->mode_id].slice_width;
	ctx->dsc.slice_height = ctx->desc->modes[ctx->mode_id].slice_height;
	ctx->dsc.slice_count = ctx->desc->modes[ctx->mode_id].slice_count;

	/* TO-DO Verifie using  2  . */
	dsi->dsc_slice_per_pkt = 1;

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
		.data = &panel_amb670_desc,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_amb670_of_match);

static struct mipi_dsi_driver panel_amb670_driver = {
	.probe = panel_amb670_probe,
	.remove = panel_amb670_remove,
	.driver = {
		.name = "panel-panel-samsung-amb670yf08-fs-1440-3216-dsc",
		.of_match_table = panel_amb670_of_match,
	},
};
module_mipi_dsi_driver(panel_amb670_driver);

MODULE_AUTHOR("Degdag Mohamed <degdagmohamed@gmail.com>");
MODULE_DESCRIPTION("DRM driver for samsung AMB670YF08 FS dsc cmd mode panel (QHD+/FHD+)");
MODULE_LICENSE("GPL");
