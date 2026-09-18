// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct panel_samsung_amb670yf08_fs_1440_3216_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct regulator *vddio;
	struct regulator *vci;
};

static inline
struct panel_samsung_amb670yf08_fs_1440_3216_dsc *to_panel_samsung_amb670yf08_fs_1440_3216_dsc(struct drm_panel *panel)
{
	return container_of(panel, struct panel_samsung_amb670yf08_fs_1440_3216_dsc, panel);
}

static void panel_samsung_amb670yf08_fs_1440_3216_dsc_reset(struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_on(struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	mipi_dsi_dcs_write_seq(dsi, 0x9e,
			       0x11, 0x00, 0x00, 0xab, 0x30, 0x80, 0x09, 0x6c,
			       0x04, 0x38, 0x00, 0x24, 0x02, 0x1c, 0x02, 0x1c,
			       0x02, 0x00, 0x02, 0x3b, 0x00, 0x20, 0x03, 0x35,
			       0x00, 0x07, 0x00, 0x0e, 0x03, 0x34, 0x02, 0xd4,
			       0x18, 0x00, 0x10, 0xf0, 0x07, 0x10, 0x20, 0x00,
			       0x06, 0x0f, 0x0f, 0x33, 0x0e, 0x1c, 0x2a, 0x38,
			       0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
			       0x7d, 0x7e, 0x02, 0x02, 0x22, 0x00, 0x2a, 0x40,
			       0x2a, 0xbe, 0x3a, 0xfc, 0x3a, 0xfa, 0x3a, 0xf8,
			       0x3b, 0x38, 0x3b, 0x78, 0x3b, 0xb6, 0x4b, 0xb6,
			       0x4b, 0xf4, 0x4b, 0xf4, 0x6c, 0x34, 0x84, 0x74,
			       0x00);
	mipi_dsi_dcs_write_seq(dsi, 0x9d, 0x01);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(12000, 13000);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq(dsi, 0xc5,
			       0x29, 0x14, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50,
			       0x05, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
			       0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
			       0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84,
			       0xca);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_MEMORY_START);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to nop: %d\n", ret);
		return ret;
	}
	usleep_range(17000, 18000);

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_MEMORY_START);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to nop: %d\n", ret);
		return ret;
	}
	usleep_range(17000, 18000);

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_MEMORY_START);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to nop: %d\n", ret);
		return ret;
	}
	usleep_range(17000, 18000);

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_MEMORY_START);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to nop: %d\n", ret);
		return ret;
	}
	usleep_range(17000, 18000);

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_MEMORY_START);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to nop: %d\n", ret);
		return ret;
	}
	msleep(40);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0xa1, 0xb1);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	msleep(121);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq(dsi, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq(dsi, 0x60, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq(dsi, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq(dsi, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq(dsi, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq(dsi, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_column_address(dsi, 0x0000, 0x0437);
	if (ret < 0) {
		dev_err(dev, "Failed to set column address: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_page_address(dsi, 0x0000, 0x096b);
	if (ret < 0) {
		dev_err(dev, "Failed to set page address: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xc3, 0x89);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x24, 0xc3);
	mipi_dsi_dcs_write_seq(dsi, 0xc3,
			       0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00,
			       0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00,
			       0xfd);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x3a, 0xc3);
	mipi_dsi_dcs_write_seq(dsi, 0xc3,
			       0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00,
			       0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00,
			       0xb0);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x50, 0xc3);
	mipi_dsi_dcs_write_seq(dsi, 0xc3,
			       0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00,
			       0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00,
			       0x43);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x61, 0xc3);
	mipi_dsi_dcs_write_seq(dsi, 0xc3,
			       0xcc, 0xcc, 0xcc, 0xcc, 0xc0, 0xfe, 0x00, 0xfe,
			       0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe,
			       0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x2b, 0xf6);
	mipi_dsi_dcs_write_seq(dsi, 0xf6, 0x60, 0x63, 0x69);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x46, 0xf4);
	mipi_dsi_dcs_write_seq(dsi, 0xf4, 0x08);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x18, 0xb1);
	mipi_dsi_dcs_write_seq(dsi, 0xb1, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0d, 0x63);
	mipi_dsi_dcs_write_seq(dsi, 0x63, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0c, 0x63);
	mipi_dsi_dcs_write_seq(dsi, 0x63, 0x20);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x28);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x52, 0x1f);
	mipi_dsi_dcs_write_seq(dsi, 0x1f, 0x02);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x54, 0x1f);
	mipi_dsi_dcs_write_seq(dsi, 0x1f, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_off(struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(21);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(101);

	return 0;
}

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_prepare(struct drm_panel *panel)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_panel_samsung_amb670yf08_fs_1440_3216_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;
	
	ret = regulator_enable(ctx->vddio);
	if (ret) {
		dev_err(dev, "failed to enable vddio regulator: %d\n", ret);
		return ret;
	}
	
	ret = regulator_enable(ctx->vci);
	if (ret) {
		dev_err(dev, "failed to enable vci regulator: %d\n", ret);
		return ret;
	}

	panel_samsung_amb670yf08_fs_1440_3216_dsc_reset(ctx);

	ret = panel_samsung_amb670yf08_fs_1440_3216_dsc_on(ctx);
	if (ret < 0) {
		regulator_disable(ctx->vci);
		regulator_disable(ctx->vddio);
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}
	
	// CASET/PASET Setting 
	mipi_dsi_dcs_write_seq(ctx->dsi, 0x2a, 0x00, 0x00, 0x04, 0x37);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0x2b, 0x00, 0x00, 0x09, 0x6b);
	
	// Scaler Setting 
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc3, 0x89);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x24, 0xc3);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc3, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd, 0x00, 0xfd);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x3a, 0xc3);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc3, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xb0);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x50, 0xc3);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc3, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43, 0x00, 0x43);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x61, 0xc3);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc3, 0xcc, 0xcc, 0xcc, 0xcc, 0xc0, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	// FFC Setting 171.3MHz FHD MIPI 825.6Mbps
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x31, 0xc5);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xc5, 0x29, 0x14, 0x00, 0x00, 0x00, 0x11, 0x10, 0x50, 0x05, 0x84, 0xca, 0x84, 0xca, 0x84,
					0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca,
					0x84, 0xca, 0x84, 0xca, 0x84, 0xca, 0x84, 0xca);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	// OPLUS ADFR OFF 
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x02, 0x60);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0x60, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf2, 0x00, 0x01);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	// TSP_SYNC1 Fixed Setting
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x22, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x3a, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x26, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	// TSP_SYNC3 Fixed Setting
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x24, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x21);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x38, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x02);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x2a, 0xb9);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb9, 0x00, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	// 120hz Transition
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xbd, 0x21);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x16, 0xf2);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf2, 0x1b, 0x50);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x08, 0xcb);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xcb, 0x24);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0x60, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x10, 0xbd);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x16, 0xbd);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xbd, 0x77);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x14, 0xbd);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xbd, 0x00);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xb0, 0x00, 0x63, 0xcb);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xcb, 0x08);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xbd, 0x23, 0x02);
	mipi_dsi_dcs_write_seq(ctx->dsi, 0xf0, 0x5a, 0x5a);
	
	
	
	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(panel->dev, "failed to transmit PPS: %d\n", ret);
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

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_unprepare(struct drm_panel *panel)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = to_panel_samsung_amb670yf08_fs_1440_3216_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = panel_samsung_amb670yf08_fs_1440_3216_dsc_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_disable(ctx->vci);
	regulator_disable(ctx->vddio);

	return 0;
}

static const struct drm_display_mode panel_samsung_amb670yf08_fs_1440_3216_dsc_mode = {
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
};

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_get_modes(struct drm_panel *panel,
							       struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &panel_samsung_amb670yf08_fs_1440_3216_dsc_mode);
}

static const struct drm_panel_funcs panel_samsung_amb670yf08_fs_1440_3216_dsc_panel_funcs = {
	.prepare = panel_samsung_amb670yf08_fs_1440_3216_dsc_prepare,
	.unprepare = panel_samsung_amb670yf08_fs_1440_3216_dsc_unprepare,
	.get_modes = panel_samsung_amb670yf08_fs_1440_3216_dsc_get_modes,
};

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	//dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	//dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	//dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	//dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_ops = {
	.update_status = panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_update_status,
	.get_brightness = panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_get_brightness,
};

static struct backlight_device *
panel_samsung_amb670yf08_fs_1440_3216_dsc_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 1433,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &panel_samsung_amb670yf08_fs_1440_3216_dsc_bl_ops, &props);
}

static int panel_samsung_amb670yf08_fs_1440_3216_dsc_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
		
	ctx->vddio = devm_regulator_get(dev, "vddio");
	if (IS_ERR(ctx->vddio))
		return dev_err_probe(dev, PTR_ERR(ctx->vddio), "failed to get vddio regulator\n");
	
	ctx->vci = devm_regulator_get(dev, "vci");
	if (IS_ERR(ctx->vci))
		return dev_err_probe(dev, PTR_ERR(ctx->vddio), "failed to get vci regulator\n");

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

	drm_panel_init(&ctx->panel, dev, &panel_samsung_amb670yf08_fs_1440_3216_dsc_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panel_samsung_amb670yf08_fs_1440_3216_dsc_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;

	/* TODO: Pass slice_per_pkt = 2 */
	ctx->dsc.slice_height = 36;
	ctx->dsc.slice_width = 540;
	ctx->dsi->dsc_slice_per_pkt = 1; //works 1
	ctx->dsc.slice_count = 2; //works 2
	ctx->dsc.convert_rgb = true; //works true
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	 ctx->dsc.block_pred_enable = true;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void panel_samsung_amb670yf08_fs_1440_3216_dsc_remove(struct mipi_dsi_device *dsi)
{
	struct panel_samsung_amb670yf08_fs_1440_3216_dsc *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id panel_samsung_amb670yf08_fs_1440_3216_dsc_of_match[] = {
	{ .compatible = "panel,samsung-amb670yf08-fs-1440-3216-dsc" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_samsung_amb670yf08_fs_1440_3216_dsc_of_match);

static struct mipi_dsi_driver panel_samsung_amb670yf08_fs_1440_3216_dsc_driver = {
	.probe = panel_samsung_amb670yf08_fs_1440_3216_dsc_probe,
	.remove = panel_samsung_amb670yf08_fs_1440_3216_dsc_remove,
	.driver = {
		.name = "panel-panel-samsung-amb670yf08-fs-1440-3216-dsc",
		.of_match_table = panel_samsung_amb670yf08_fs_1440_3216_dsc_of_match,
	},
};
module_mipi_dsi_driver(panel_samsung_amb670yf08_fs_1440_3216_dsc_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for samsung AMB670YF08 FS dsc cmd mode panel");
MODULE_LICENSE("GPL");
