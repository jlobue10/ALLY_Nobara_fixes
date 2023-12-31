// SPDX-License-Identifier: GPL-2.0
//
// CS35L41 ALSA HDA Property driver
//
// Copyright 2023 Cirrus Logic, Inc.
//
// Author: Stefan Binding <sbinding@opensource.cirrus.com>

#include <linux/dmi.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "cs35l41_hda_property.h"

/*
 * Device CLSA010(0/1) doesn't have _DSD so a gpiod_get by the label reset won't work.
 * And devices created by serial-multi-instantiate don't have their device struct
 * pointing to the correct fwnode, so acpi_dev must be used here.
 * And devm functions expect that the device requesting the resource has the correct
 * fwnode.
 */
static int lenovo_legion_no_acpi(struct cs35l41_hda *cs35l41, struct device *physdev, int id,
				 const char *hid)
{
	struct cs35l41_hw_cfg *hw_cfg = &cs35l41->hw_cfg;

	/* check I2C address to assign the index */
	cs35l41->index = id == 0x40 ? 0 : 1;
	cs35l41->channel_index = 0;
	cs35l41->reset_gpio = gpiod_get_index(physdev, NULL, 0, GPIOD_OUT_HIGH);
	cs35l41->speaker_id = cs35l41_get_speaker_id(physdev, 0, 0, 2);
	hw_cfg->spk_pos = cs35l41->index;
	hw_cfg->gpio2.func = CS35L41_INTERRUPT;
	hw_cfg->gpio2.valid = true;
	hw_cfg->valid = true;

	if (strcmp(hid, "CLSA0100") == 0) {
		hw_cfg->bst_type = CS35L41_EXT_BOOST_NO_VSPK_SWITCH;
	} else if (strcmp(hid, "CLSA0101") == 0) {
		hw_cfg->bst_type = CS35L41_EXT_BOOST;
		hw_cfg->gpio1.func = CS35l41_VSPK_SWITCH;
		hw_cfg->gpio1.valid = true;
	}

	return 0;
}

/*
 * The CSC3551 is used in almost the entire ASUS ROG laptop range in 2023, this is likely to
 * also include many non ROG labelled laptops. It is also used with either I2C connection or
 * SPI connection. The SPI connected versions may be missing a chip select GPIO and require
 * an DSD table patch.
 */
static int asus_rog_2023_spkr_id2(struct cs35l41_hda *cs35l41, struct device *physdev, int id,
				const char *hid)
{
	struct cs35l41_hw_cfg *hw_cfg = &cs35l41->hw_cfg;

	/* check SPI or I2C address to assign the index */
	cs35l41->index = (id == 0 || id == 0x40) ? 0 : 1;
	cs35l41->channel_index = 0;
	cs35l41->speaker_id = cs35l41_get_speaker_id(physdev, 0, 0, 2);
	hw_cfg->spk_pos = cs35l41->index;
	hw_cfg->bst_type = CS35L41_EXT_BOOST;
	hw_cfg->gpio1.func = CS35l41_VSPK_SWITCH;
	hw_cfg->gpio1.valid = true;
	hw_cfg->gpio2.func = CS35L41_INTERRUPT;
	hw_cfg->gpio2.valid = true;

	if (strcmp(cs35l41->acpi_subsystem_id, "10431473") == 0
		|| strcmp(cs35l41->acpi_subsystem_id, "10431483") == 0
		|| strcmp(cs35l41->acpi_subsystem_id, "10431493") == 0) {
		cs35l41->reset_gpio = gpiod_get_index(physdev, NULL, 1, GPIOD_OUT_HIGH);
	} else {
		cs35l41->reset_gpio = gpiod_get_index(physdev, NULL, 0, GPIOD_OUT_HIGH);
	}

	hw_cfg->valid = true;

	return 0;
}

static int asus_rog_2023_ally_fix(struct cs35l41_hda *cs35l41, struct device *physdev, int id,
				const char *hid)
{
	const char *rog_ally_bios_ver = dmi_get_system_info(DMI_BIOS_VERSION);
	const char *rog_ally_bios_num = rog_ally_bios_ver + 6; // Dropping the RC71L. part before the number
	int rog_ally_bios_int;
	kstrtoint(rog_ally_bios_num, 10, &rog_ally_bios_int);
	if(rog_ally_bios_int >= 330){
		printk(KERN_INFO "DSD properties exist in the %d BIOS. Not applying DSD override...\n", rog_ally_bios_int);
		return -ENOENT; //Patch not applicable. Exiting...
	}

	struct cs35l41_hw_cfg *hw_cfg = &cs35l41->hw_cfg;

	dev_info(cs35l41->dev, "Adding DSD properties for %s\n", cs35l41->acpi_subsystem_id);

	cs35l41->index = id == 0x40 ? 0 : 1;
	cs35l41->channel_index = 0;
	cs35l41->reset_gpio = gpiod_get_index(physdev, NULL, 0, GPIOD_OUT_HIGH);
	cs35l41->speaker_id = cs35l41_get_speaker_id(physdev, 0, 0, 2);
	hw_cfg->spk_pos = cs35l41->index;
	hw_cfg->gpio1.func = CS35L41_NOT_USED;
	hw_cfg->gpio1.valid = true;
	hw_cfg->gpio2.func = CS35L41_INTERRUPT;
	hw_cfg->gpio2.valid = true;
	hw_cfg->bst_type = CS35L41_INT_BOOST;
	hw_cfg->bst_ind = 1000; /* 1,000nH Inductance value */
	hw_cfg->bst_ipk = 4500; /* 4,500mA peak current */
	hw_cfg->bst_cap = 24; /* 24 microFarad cap value */
	hw_cfg->valid = true;

	return 0;
}

struct cs35l41_prop_model {
	const char *hid;
	const char *ssid;
	int (*add_prop)(struct cs35l41_hda *cs35l41, struct device *physdev, int id,
			const char *hid);
};

const struct cs35l41_prop_model cs35l41_prop_model_table[] = {
	{ "CLSA0100", NULL, lenovo_legion_no_acpi },
	{ "CLSA0101", NULL, lenovo_legion_no_acpi },
	{ "CSC3551", "10431433", asus_rog_2023_spkr_id2 }, // ASUS GS650P - i2c
	{ "CSC3551", "10431463", asus_rog_2023_spkr_id2 }, // ASUS GA402X/N - i2c
	{ "CSC3551", "10431473", asus_rog_2023_spkr_id2 }, // ASUS GU604V - spi, reset gpio 1
	{ "CSC3551", "10431483", asus_rog_2023_spkr_id2 }, // ASUS GU603V - spi, reset gpio 1
	{ "CSC3551", "10431493", asus_rog_2023_spkr_id2 }, // ASUS GV601V - spi, reset gpio 1
	{ "CSC3551", "10431573", asus_rog_2023_spkr_id2 }, // ASUS GZ301V - spi, reset gpio 0
	{ "CSC3551", "104317F3", asus_rog_2023_ally_fix }, // ASUS ROG ALLY - i2c
	{ "CSC3551", "10431B93", asus_rog_2023_spkr_id2 }, // ASUS G614J - spi, reset gpio 0
	{ "CSC3551", "10431CAF", asus_rog_2023_spkr_id2 }, // ASUS G634J - spi, reset gpio 0
	{ "CSC3551", "10431C9F", asus_rog_2023_spkr_id2 }, // ASUS G614JI -spi, reset gpio 0
	{ "CSC3551", "10431D1F", asus_rog_2023_spkr_id2 }, // ASUS G713P - i2c
	{ "CSC3551", "10431F1F", asus_rog_2023_spkr_id2 }, // ASUS H7604JV - spi, reset gpio 0
	{}
};

int cs35l41_add_dsd_properties(struct cs35l41_hda *cs35l41, struct device *physdev, int id,
			       const char *hid)
{
	const struct cs35l41_prop_model *model;

	for (model = cs35l41_prop_model_table; model->hid > 0; model++) {
		if (!strcmp(model->hid, hid) &&
		    (!model->ssid ||
		     (cs35l41->acpi_subsystem_id &&
		      !strcmp(model->ssid, cs35l41->acpi_subsystem_id))))
			return model->add_prop(cs35l41, physdev, id, hid);
	}

	return -ENOENT;
}
