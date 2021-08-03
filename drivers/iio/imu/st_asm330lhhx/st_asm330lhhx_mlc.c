// SPDX-License-Identifier: GPL-2.0-only
/*
 * STMicroelectronics st_asm330lhhx machine learning core driver
 *
 * Copyright 2021 STMicroelectronics Inc.
 *
 * Tesi Mario <mario.tesi@st.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/buffer.h>
#include <linux/iio/events.h>
#include <linux/firmware.h>

#include <linux/platform_data/st_sensors_pdata.h>

#include "st_asm330lhhx.h"

#define ST_ASM330LHHX_MLC_LOADER_VERSION		"0.3"

/* number of machine learning core available on device hardware */
#define ST_ASM330LHHX_MLC_NUMBER			8
#define ST_ASM330LHHX_FSM_NUMBER			16

#ifdef CONFIG_IIO_ASM330LHHX_MLC_BUILTIN_FIRMWARE
static const u8 st_asm330lhhx_mlc_fw[] = {
	#include "st_asm330lhhx_mlc.fw"
};
DECLARE_BUILTIN_FIRMWARE(ASM330LHHX_MLC_FIRMWARE_NAME, st_asm330lhhx_mlc_fw);
#else /* CONFIG_IIO_ASM330LHHX_MLC_BUILTIN_FIRMWARE */
#define ASM330LHHX_MLC_FIRMWARE_NAME	"st_asm330lhhx_mlc.bin"
#endif /* CONFIG_IIO_ASM330LHHX_MLC_BUILTIN_FIRMWARE */

#ifdef CONFIG_IIO_ST_ASM330LHHX_MLC_PRELOAD
static const u8 mlcdata[] = {
	/*
	 * Machine Learning Core Tool v1.2.0.0 Beta, ASM330LHHX
	 * 6D position recognition
	 * source:
	 * https://github.com/STMicroelectronics/STMems_Machine_Learning_Core/blob/master/application_examples/asm330lhhx/6D%20position%20recognition/asm330lhhx_six_d_position.ucf
	 */
	0x10, 0x00, 0x11, 0x00, 0x01, 0x80, 0x05, 0x00, 0x17, 0x40,
	0x02, 0x11, 0x08, 0xEA, 0x09, 0x58, 0x02, 0x11, 0x08, 0xEB,
	0x09, 0x03, 0x02, 0x11, 0x08, 0xEC, 0x09, 0x62, 0x02, 0x11,
	0x08, 0xED, 0x09, 0x03, 0x02, 0x11, 0x08, 0xEE, 0x09, 0x00,
	0x02, 0x11, 0x08, 0xEF, 0x09, 0x00, 0x02, 0x11, 0x08, 0xF0,
	0x09, 0x0A, 0x02, 0x11, 0x08, 0xF2, 0x09, 0x10, 0x02, 0x11,
	0x08, 0xFA, 0x09, 0x3C, 0x02, 0x11, 0x08, 0xFB, 0x09, 0x03,
	0x02, 0x11, 0x08, 0xFC, 0x09, 0x6E, 0x02, 0x11, 0x08, 0xFD,
	0x09, 0x03, 0x02, 0x11, 0x08, 0xFE, 0x09, 0x7A, 0x02, 0x11,
	0x08, 0xFF, 0x09, 0x03, 0x02, 0x31, 0x08, 0x3C, 0x09, 0x3F,
	0x02, 0x31, 0x08, 0x3D, 0x09, 0x00, 0x02, 0x31, 0x08, 0x3E,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x3F, 0x09, 0x84, 0x02, 0x31,
	0x08, 0x40, 0x09, 0x00, 0x02, 0x31, 0x08, 0x41, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x42, 0x09, 0x00, 0x02, 0x31, 0x08, 0x43,
	0x09, 0x88, 0x02, 0x31, 0x08, 0x44, 0x09, 0x00, 0x02, 0x31,
	0x08, 0x45, 0x09, 0x00, 0x02, 0x31, 0x08, 0x46, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x47, 0x09, 0x8C, 0x02, 0x31, 0x08, 0x48,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x49, 0x09, 0x00, 0x02, 0x31,
	0x08, 0x4A, 0x09, 0x00, 0x02, 0x31, 0x08, 0x4B, 0x09, 0x04,
	0x02, 0x31, 0x08, 0x4C, 0x09, 0x00, 0x02, 0x31, 0x08, 0x4D,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x4E, 0x09, 0x00, 0x02, 0x31,
	0x08, 0x4F, 0x09, 0x08, 0x02, 0x31, 0x08, 0x50, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x51, 0x09, 0x00, 0x02, 0x31, 0x08, 0x52,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x53, 0x09, 0x0C, 0x02, 0x31,
	0x08, 0x54, 0x09, 0x00, 0x02, 0x31, 0x08, 0x55, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x56, 0x09, 0x1F, 0x02, 0x31, 0x08, 0x57,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x6E, 0x09, 0x00, 0x02, 0x31,
	0x08, 0x6F, 0x09, 0x00, 0x02, 0x31, 0x08, 0x70, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x71, 0x09, 0x00, 0x02, 0x31, 0x08, 0x72,
	0x09, 0x00, 0x02, 0x31, 0x08, 0x73, 0x09, 0x00, 0x02, 0x31,
	0x08, 0x74, 0x09, 0x00, 0x02, 0x31, 0x08, 0x75, 0x09, 0x00,
	0x02, 0x31, 0x08, 0x76, 0x09, 0x00, 0x02, 0x31, 0x08, 0x77,
	0x09, 0x00, 0x01, 0x00, 0x12, 0x00, 0x01, 0x80, 0x17, 0x40,
	0x02, 0x0D, 0x08, 0x00, 0x09, 0xCD, 0x09, 0x34, 0x09, 0x05,
	0x09, 0x02, 0x09, 0x00, 0x09, 0x08, 0x09, 0x00, 0x09, 0x00,
	0x09, 0xCD, 0x09, 0x34, 0x09, 0x03, 0x09, 0x04, 0x09, 0x04,
	0x09, 0x08, 0x09, 0x00, 0x09, 0x00, 0x09, 0xCD, 0x09, 0x34,
	0x09, 0x05, 0x09, 0x0C, 0x09, 0x14, 0x09, 0x0B, 0x09, 0x00,
	0x09, 0x00, 0x09, 0xCD, 0x09, 0x34, 0x09, 0x00, 0x09, 0x08,
	0x09, 0x08, 0x09, 0x09, 0x09, 0x00, 0x09, 0x00, 0x09, 0xCD,
	0x09, 0x34, 0x09, 0x03, 0x09, 0x08, 0x09, 0x10, 0x09, 0x0B,
	0x09, 0x00, 0x09, 0x00, 0x09, 0xCD, 0x09, 0x34, 0x09, 0x00,
	0x09, 0x0C, 0x09, 0x08, 0x09, 0x09, 0x09, 0x00, 0x09, 0x00,
	0x09, 0xCD, 0x09, 0x34, 0x09, 0x00, 0x09, 0x0E, 0x09, 0x04,
	0x09, 0x09, 0x09, 0x00, 0x09, 0x00, 0x09, 0xCD, 0x09, 0x34,
	0x09, 0x01, 0x09, 0x04, 0x09, 0x0C, 0x09, 0x0B, 0x09, 0x00,
	0x09, 0x00, 0x01, 0x80, 0x17, 0x00, 0x04, 0x00, 0x05, 0x10,
	0x02, 0x01, 0x01, 0x00, 0x12, 0x44, 0x01, 0x80, 0x60, 0x15,
	0x01, 0x00, 0x10, 0x20, 0x11, 0x00, 0x5E, 0x02, 0x01, 0x80,
	0x0D, 0x01, 0x01, 0x00,
};

static const struct firmware st_asm330lhhx_mlc_preload = {
		.size = sizeof(mlcdata),
		.data = mlcdata
};
#endif /* CONFIG_IIO_ST_ASM330LHHX_MLC_PRELOAD */

static
struct iio_dev *st_asm330lhhx_mlc_alloc_iio_dev(struct st_asm330lhhx_hw *hw,
					enum st_asm330lhhx_sensor_id id);

static const unsigned long st_asm330lhhx_mlc_available_scan_masks[] = {
	0x1, 0x0
};
static const unsigned long st_asm330lhhx_fsm_available_scan_masks[] = {
	0x1, 0x0
};

static inline int
st_asm330lhhx_read_page_locked(struct st_asm330lhhx_hw *hw, unsigned int addr,
			       void *val, unsigned int len)
{
	int err;

	st_asm330lhhx_set_page_access(hw, true, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	err = regmap_bulk_read(hw->regmap, addr, val, len);
	st_asm330lhhx_set_page_access(hw, false, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	mutex_unlock(&hw->page_lock);

	return err;
}

static inline int
st_asm330lhhx_write_page_locked(struct st_asm330lhhx_hw *hw, unsigned int addr,
				unsigned int *val, unsigned int len)
{
	int err;

	mutex_lock(&hw->page_lock);
	st_asm330lhhx_set_page_access(hw, true, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	err = regmap_bulk_write(hw->regmap, addr, val, len);
	st_asm330lhhx_set_page_access(hw, false, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	mutex_unlock(&hw->page_lock);

	return err;
}

static inline int
st_asm330lhhx_update_page_bits_locked(struct st_asm330lhhx_hw *hw,
				      unsigned int addr, unsigned int mask,
				      unsigned int val)
{
	int err;

	mutex_lock(&hw->page_lock);
	st_asm330lhhx_set_page_access(hw, true, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	err = regmap_update_bits(hw->regmap, addr, mask, val);
	st_asm330lhhx_set_page_access(hw, false, ST_ASM330LHHX_REG_FUNC_CFG_MASK);
	mutex_unlock(&hw->page_lock);

	return err;
}

static int st_asm330lhhx_mlc_enable_sensor(struct st_asm330lhhx_sensor *sensor,
					   bool enable)
{
	struct st_asm330lhhx_hw *hw = sensor->hw;
	int i, id, err = 0;

	if (sensor->status == ST_ASM330LHHX_MLC_ENABLED) {
		int value;

		value = enable ? hw->mlc_config->mlc_int_mask : 0;
		err = st_asm330lhhx_write_page_locked(hw,
					hw->mlc_config->mlc_int_addr,
					&value, 1);
		if (err < 0)
			return err;

		/*
		 * enable mlc core
		 * only one mlc so not need to check if other running
		 */
		err = st_asm330lhhx_update_page_bits_locked(hw,
				ST_ASM330LHHX_EMB_FUNC_EN_B_ADDR,
				ST_ASM330LHHX_MLC_EN_MASK,
				ST_ASM330LHHX_SHIFT_VAL(enable,
						ST_ASM330LHHX_MLC_EN_MASK));
		if (err < 0)
			return err;

		dev_info(sensor->hw->dev,
			"Enabling MLC sensor %d to %d (INT %x)\n",
			sensor->id, enable, value);
	} else if (sensor->status == ST_ASM330LHHX_FSM_ENABLED) {
		int value[2];

		value[0] = enable ? hw->mlc_config->fsm_int_mask[0] : 0;
		value[1] = enable ? hw->mlc_config->fsm_int_mask[1] : 0;
		err = st_asm330lhhx_write_page_locked(hw,
					hw->mlc_config->fsm_int_addr[0],
					&value[0], 2);
		if (err < 0)
			return err;

		/* enable fsm core */
		for (i = 0; i < ST_ASM330LHHX_FSM_NUMBER; i++) {
			id = st_asm330lhhx_fsm_sensor_list[i];
			if (hw->enable_mask & BIT(id))
				break;
		}

		/* check for any other fsm already enabled */
		if (enable || i == ST_ASM330LHHX_FSM_NUMBER) {
			err = st_asm330lhhx_update_page_bits_locked(hw,
					ST_ASM330LHHX_EMB_FUNC_EN_B_ADDR,
					ST_ASM330LHHX_FSM_EN_MASK,
					ST_ASM330LHHX_SHIFT_VAL(enable,
						ST_ASM330LHHX_FSM_EN_MASK));
			if (err < 0)
				return err;
		}

		dev_info(sensor->hw->dev,
			"Enabling FSM sensor %d to %d (INT %x-%x)\n",
			sensor->id, enable, value[0], value[1]);
	} else {
		dev_err(hw->dev, "invalid sensor configuration\n");
		err = -ENODEV;

		return err;
	}

	if (enable)
		hw->enable_mask |= BIT(sensor->id);
	else
		hw->enable_mask &= ~BIT(sensor->id);

	return err < 0 ? err : 0;
}

static int st_asm330lhhx_mlc_write_event_config(struct iio_dev *iio_dev,
					 const struct iio_chan_spec *chan,
					 enum iio_event_type type,
					 enum iio_event_direction dir,
					 int state)
{
	struct st_asm330lhhx_sensor *sensor = iio_priv(iio_dev);

	return st_asm330lhhx_mlc_enable_sensor(sensor, state);
}

static int st_asm330lhhx_mlc_read_event_config(struct iio_dev *iio_dev,
					const struct iio_chan_spec *chan,
					enum iio_event_type type,
					enum iio_event_direction dir)
{
	struct st_asm330lhhx_sensor *sensor = iio_priv(iio_dev);
	struct st_asm330lhhx_hw *hw = sensor->hw;

	return !!(hw->enable_mask & BIT(sensor->id));
}

/* parse and program mlc fragments */
static int st_asm330lhhx_program_mlc(const struct firmware *fw,
				     struct st_asm330lhhx_hw *hw,
				     u8 *mlc_mask, u16 *fsm_mask)
{
	uint8_t mlc_int = 0, mlc_num = 0, fsm_num = 0, skip = 0;
	int int_pin, reg, val, ret, i = 0;
	uint8_t fsm_int[2] = { 0, 0 };
	bool stmc_page = false;

	while (i < fw->size) {
		reg = fw->data[i++];
		val = fw->data[i++];

		if (reg == 0x01 && val == 0x80) {
			stmc_page = true;
		} else if (reg == 0x01 && val == 0x00) {
			stmc_page = false;
		} else if (stmc_page) {
			switch (reg) {
			case ST_ASM330LHHX_MLC_INT1_ADDR:
			case ST_ASM330LHHX_MLC_INT2_ADDR:
				mlc_int |= val;
				mlc_num++;
				skip = 1;
				break;
			case ST_ASM330LHHX_FSM_INT1_A_ADDR:
			case ST_ASM330LHHX_FSM_INT2_A_ADDR:
				fsm_int[0] |= val;
				fsm_num++;
				skip = 1;
				break;
			case ST_ASM330LHHX_FSM_INT1_B_ADDR:
			case ST_ASM330LHHX_FSM_INT2_B_ADDR:
				fsm_int[1] |= val;
				fsm_num++;
				skip = 1;
				break;
			case ST_ASM330LHHX_EMB_FUNC_EN_B_ADDR:
				skip = 1;
				break;
			default:
				break;
			}
		}

		if (!skip) {
			ret = regmap_write(hw->regmap, reg, val);
			if (ret) {
				dev_err(hw->dev, "regmap_write fails\n");

				return ret;
			}
		}

		skip = 0;

		if (mlc_num >= ST_ASM330LHHX_MLC_NUMBER ||
		    fsm_num >= ST_ASM330LHHX_FSM_NUMBER)
			break;
	}

	hw->mlc_config->bin_len = fw->size;

	ret = st_asm330lhhx_of_get_pin(hw, &int_pin);
	if (ret < 0) {
		struct st_sensors_platform_data *pdata;
		struct device *dev = hw->dev;

		pdata = (struct st_sensors_platform_data *)dev->platform_data;
		int_pin = pdata ? pdata->drdy_int_pin : 1;
	}

	if (mlc_num) {
		hw->mlc_config->mlc_int_mask = mlc_int;
		*mlc_mask = mlc_int;

		hw->mlc_config->mlc_int_addr = (int_pin == 1 ?
					    ST_ASM330LHHX_MLC_INT1_ADDR :
					    ST_ASM330LHHX_MLC_INT2_ADDR);

		hw->mlc_config->status |= ST_ASM330LHHX_MLC_ENABLED;
		hw->mlc_config->mlc_configured += mlc_num;
	}

	if (fsm_num) {
		hw->mlc_config->fsm_int_mask[0] = fsm_int[0];
		hw->mlc_config->fsm_int_mask[1] = fsm_int[1];
		*fsm_mask = (u16)(((u16)fsm_int[1] << 8) | fsm_int[0]);

		hw->mlc_config->fsm_int_addr[0] = (int_pin == 1 ?
					    ST_ASM330LHHX_FSM_INT1_A_ADDR :
					    ST_ASM330LHHX_FSM_INT2_A_ADDR);
		hw->mlc_config->fsm_int_addr[1] = (int_pin == 1 ?
					    ST_ASM330LHHX_FSM_INT1_B_ADDR :
					    ST_ASM330LHHX_FSM_INT2_B_ADDR);

		hw->mlc_config->status |= ST_ASM330LHHX_FSM_ENABLED;
		hw->mlc_config->fsm_configured += fsm_num;
	}

	return fsm_num + mlc_num;
}

static void st_asm330lhhx_mlc_update(const struct firmware *fw, void *context)
{
	struct st_asm330lhhx_hw *hw = context;
	enum st_asm330lhhx_sensor_id id;
	u16 fsm_mask = 0;
	u8 mlc_mask = 0;
	int ret, i;

	if (!fw) {
		dev_err(hw->dev, "could not get binary firmware\n");
		return;
	}

	ret = st_asm330lhhx_program_mlc(fw, hw, &mlc_mask, &fsm_mask);
	if (ret > 0) {
		dev_info(hw->dev, "MLC loaded (%d) MLC %x FSM %x-%x\n",
			 ret, mlc_mask,
			 (fsm_mask >> 8) & 0xFF, fsm_mask & 0xFF);

		for (i = 0; i < ST_ASM330LHHX_MLC_NUMBER; i++) {
			if (mlc_mask & BIT(i)) {
				id = st_asm330lhhx_mlc_sensor_list[i];
				hw->iio_devs[id] =
					st_asm330lhhx_mlc_alloc_iio_dev(hw, id);
				if (!hw->iio_devs[id])
					goto release;

				ret = iio_device_register(hw->iio_devs[id]);
				if (ret)
					goto release;
			}
		}

		for (i = 0; i < ST_ASM330LHHX_FSM_NUMBER; i++) {
			if (fsm_mask & BIT(i)) {
				id = st_asm330lhhx_fsm_sensor_list[i];
				hw->iio_devs[id] =
					st_asm330lhhx_mlc_alloc_iio_dev(hw, id);
				if (!hw->iio_devs[id])
					goto release;

				ret = iio_device_register(hw->iio_devs[id]);
				if (ret)
					goto release;
			}
		}
	}

release:
	if (hw->preload_mlc) {
		hw->preload_mlc = 0;

		return;
	}

	release_firmware(fw);
}

static int st_asm330lhhx_mlc_flush_all(struct st_asm330lhhx_hw *hw)
{
	struct st_asm330lhhx_sensor *sensor_mlc;
	struct iio_dev *iio_dev;
	int ret = 0, id;

	for (id = ST_ASM330LHHX_ID_MLC_0; id < ST_ASM330LHHX_ID_MAX; id++) {
		iio_dev = hw->iio_devs[id];
		if (!iio_dev)
			continue;

		sensor_mlc = iio_priv(iio_dev);
		ret = st_asm330lhhx_mlc_enable_sensor(sensor_mlc, false);
		if (ret < 0)
			break;

		iio_device_unregister(iio_dev);
		kfree(iio_dev->channels);
		iio_device_free(iio_dev);
		hw->iio_devs[id] = NULL;
	}

	return ret;
}

static ssize_t st_asm330lhhx_mlc_info(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct st_asm330lhhx_sensor *sensor = iio_priv(dev_get_drvdata(dev));
	struct st_asm330lhhx_hw *hw = sensor->hw;

	return scnprintf(buf, PAGE_SIZE, "mlc %02x fsm %02x\n",
			 hw->mlc_config->mlc_configured,
			 hw->mlc_config->fsm_configured);
}

static ssize_t st_asm330lhhx_mlc_get_version(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "mlc loader Version %s\n",
			 ST_ASM330LHHX_MLC_LOADER_VERSION);
}

static ssize_t st_asm330lhhx_mlc_upload_firmware(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct st_asm330lhhx_sensor *sensor = iio_priv(dev_get_drvdata(dev));
	int err;

	err = request_firmware_nowait(THIS_MODULE, true,
				      ASM330LHHX_MLC_FIRMWARE_NAME,
				      dev, GFP_KERNEL,
				      sensor->hw,
				      st_asm330lhhx_mlc_update);

	return err < 0 ? err : size;
}

static ssize_t st_asm330lhhx_mlc_flush(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	struct st_asm330lhhx_sensor *sensor = iio_priv(dev_get_drvdata(dev));
	struct st_asm330lhhx_hw *hw = sensor->hw;
	int ret;

	ret = st_asm330lhhx_mlc_flush_all(hw);
	memset(hw->mlc_config, 0, sizeof(*hw->mlc_config));

	return ret < 0 ? ret : size;
}

static IIO_DEVICE_ATTR(mlc_info, S_IRUGO,
		       st_asm330lhhx_mlc_info, NULL, 0);
static IIO_DEVICE_ATTR(mlc_flush, S_IWUSR,
		       NULL, st_asm330lhhx_mlc_flush, 0);
static IIO_DEVICE_ATTR(mlc_version, S_IRUGO,
		       st_asm330lhhx_mlc_get_version, NULL, 0);
static IIO_DEVICE_ATTR(load_mlc, S_IWUSR,
		       NULL, st_asm330lhhx_mlc_upload_firmware, 0);

static struct attribute *st_asm330lhhx_mlc_event_attributes[] = {
	&iio_dev_attr_mlc_info.dev_attr.attr,
	&iio_dev_attr_mlc_version.dev_attr.attr,
	&iio_dev_attr_load_mlc.dev_attr.attr,
	&iio_dev_attr_mlc_flush.dev_attr.attr,
	NULL,
};

static const struct attribute_group st_asm330lhhx_mlc_event_attribute_group = {
	.attrs = st_asm330lhhx_mlc_event_attributes,
};

static const struct iio_info st_asm330lhhx_mlc_event_info = {
	.attrs = &st_asm330lhhx_mlc_event_attribute_group,
	.read_event_config = st_asm330lhhx_mlc_read_event_config,
	.write_event_config = st_asm330lhhx_mlc_write_event_config,
};

static const struct iio_info st_asm330lhhx_mlc_x_event_info = {
	.read_event_config = st_asm330lhhx_mlc_read_event_config,
	.write_event_config = st_asm330lhhx_mlc_write_event_config,
};

static
struct iio_dev *st_asm330lhhx_mlc_alloc_iio_dev(struct st_asm330lhhx_hw *hw,
						enum st_asm330lhhx_sensor_id id)
{
	struct st_asm330lhhx_sensor *sensor;
	struct iio_chan_spec *channels;
	struct iio_dev *iio_dev;

	/* devm management only for ST_ASM330LHHX_ID_MLC */
	if (id == ST_ASM330LHHX_ID_MLC)
		iio_dev = devm_iio_device_alloc(hw->dev, sizeof(*sensor));
	else
		iio_dev = iio_device_alloc(sizeof(*sensor));

	if (!iio_dev)
		return NULL;

	iio_dev->modes = INDIO_DIRECT_MODE;
	iio_dev->dev.parent = hw->dev;

	sensor = iio_priv(iio_dev);
	sensor->id = id;
	sensor->hw = hw;

	switch (id) {
	case ST_ASM330LHHX_ID_MLC: {
		const struct iio_chan_spec st_asm330lhhx_mlc_channels[] = {
			ST_ASM330LHHX_EVENT_CHANNEL(IIO_ACTIVITY, thr),
		};

		channels = devm_kzalloc(hw->dev,
					sizeof(st_asm330lhhx_mlc_channels),
					GFP_KERNEL);
		if (!channels)
			return NULL;

		memcpy(channels, st_asm330lhhx_mlc_channels,
		       sizeof(st_asm330lhhx_mlc_channels));

		iio_dev->available_scan_masks =
			st_asm330lhhx_mlc_available_scan_masks;
		iio_dev->channels = channels;
		iio_dev->num_channels = ARRAY_SIZE(st_asm330lhhx_mlc_channels);
		iio_dev->info = &st_asm330lhhx_mlc_event_info;
		scnprintf(sensor->name, sizeof(sensor->name), "asm330lhhx_mlc");
		break;
	}
	case ST_ASM330LHHX_ID_MLC_0:
	case ST_ASM330LHHX_ID_MLC_1:
	case ST_ASM330LHHX_ID_MLC_2:
	case ST_ASM330LHHX_ID_MLC_3:
	case ST_ASM330LHHX_ID_MLC_4:
	case ST_ASM330LHHX_ID_MLC_5:
	case ST_ASM330LHHX_ID_MLC_6:
	case ST_ASM330LHHX_ID_MLC_7: {
		const struct iio_chan_spec st_asm330lhhx_mlc_x_ch[] = {
			ST_ASM330LHHX_EVENT_CHANNEL(IIO_ACTIVITY, thr),
		};

		channels = kzalloc(sizeof(st_asm330lhhx_mlc_x_ch), GFP_KERNEL);
		if (!channels)
			return NULL;

		memcpy(channels, st_asm330lhhx_mlc_x_ch,
		       sizeof(st_asm330lhhx_mlc_x_ch));

		iio_dev->available_scan_masks =
			st_asm330lhhx_mlc_available_scan_masks;
		iio_dev->channels = channels;
		iio_dev->num_channels = ARRAY_SIZE(st_asm330lhhx_mlc_x_ch);
		iio_dev->info = &st_asm330lhhx_mlc_x_event_info;
		scnprintf(sensor->name, sizeof(sensor->name),
			  "asm330lhhx_mlc_%d", id - ST_ASM330LHHX_ID_MLC_0);
		sensor->outreg_addr = ST_ASM330LHHX_REG_MLC0_SRC_ADDR +
				id - ST_ASM330LHHX_ID_MLC_0;
		sensor->status = ST_ASM330LHHX_MLC_ENABLED;
		sensor->pm = ST_ASM330LHHX_NO_MODE;
		break;
	}
	case ST_ASM330LHHX_ID_FSM_0:
	case ST_ASM330LHHX_ID_FSM_1:
	case ST_ASM330LHHX_ID_FSM_2:
	case ST_ASM330LHHX_ID_FSM_3:
	case ST_ASM330LHHX_ID_FSM_4:
	case ST_ASM330LHHX_ID_FSM_5:
	case ST_ASM330LHHX_ID_FSM_6:
	case ST_ASM330LHHX_ID_FSM_7:
	case ST_ASM330LHHX_ID_FSM_8:
	case ST_ASM330LHHX_ID_FSM_9:
	case ST_ASM330LHHX_ID_FSM_10:
	case ST_ASM330LHHX_ID_FSM_11:
	case ST_ASM330LHHX_ID_FSM_12:
	case ST_ASM330LHHX_ID_FSM_13:
	case ST_ASM330LHHX_ID_FSM_14:
	case ST_ASM330LHHX_ID_FSM_15: {
		const struct iio_chan_spec st_asm330lhhx_fsm_x_ch[] = {
			ST_ASM330LHHX_EVENT_CHANNEL(IIO_ACTIVITY, thr),
		};

		channels = kzalloc(sizeof(st_asm330lhhx_fsm_x_ch), GFP_KERNEL);
		if (!channels)
			return NULL;

		memcpy(channels, st_asm330lhhx_fsm_x_ch,
		       sizeof(st_asm330lhhx_fsm_x_ch));

		iio_dev->available_scan_masks =
			st_asm330lhhx_fsm_available_scan_masks;
		iio_dev->channels = channels;
		iio_dev->num_channels = ARRAY_SIZE(st_asm330lhhx_fsm_x_ch);
		iio_dev->info = &st_asm330lhhx_mlc_x_event_info;
		scnprintf(sensor->name, sizeof(sensor->name),
			  "asm330lhhx_fsm_%d", id - ST_ASM330LHHX_ID_FSM_0);
		sensor->outreg_addr = ST_ASM330LHHX_FSM_OUTS1_ADDR +
				id - ST_ASM330LHHX_ID_FSM_0;
		sensor->status = ST_ASM330LHHX_FSM_ENABLED;
		sensor->pm = ST_ASM330LHHX_NO_MODE;
		break;
	}
	default:
		dev_err(hw->dev, "invalid sensor id %d\n", id);

		return NULL;
	}

	iio_dev->name = sensor->name;

	return iio_dev;
}

int st_asm330lhhx_mlc_check_status(struct st_asm330lhhx_hw *hw)
{
	struct st_asm330lhhx_sensor *sensor;
	u8 i, mlc_status, id, event[16];
	struct iio_dev *iio_dev;
	__le16 __fsm_status = 0;
	u16 fsm_status;
	int err = 0;

	if (hw->mlc_config->status & ST_ASM330LHHX_MLC_ENABLED) {
		err = st_asm330lhhx_read_locked(hw,
					ST_ASM330LHHX_MLC_STATUS_MAINPAGE,
					(void *)&mlc_status, 1);
		if (err)
			return err;

		if (mlc_status) {
			for (i = 0; i < ST_ASM330LHHX_MLC_NUMBER; i++) {
				id = st_asm330lhhx_mlc_sensor_list[i];
				if (!(hw->enable_mask & BIT(id)))
					continue;

				if (mlc_status & BIT(i)) {
					iio_dev = hw->iio_devs[id];
					if (!iio_dev) {
						err = -ENOENT;

						return err;
					}

					sensor = iio_priv(iio_dev);
					err = st_asm330lhhx_read_page_locked(hw,
						sensor->outreg_addr,
						(void *)&event[i], 1);
					if (err)
						return err;

					iio_push_event(iio_dev, (u64)event[i],
						       iio_get_time_ns(iio_dev));

					dev_info(hw->dev,
						 "MLC %d Status %x MLC EVENT %llx\n",
						 id, mlc_status, (u64)event[i]);
				}
			}
		}
	}

	if (hw->mlc_config->status & ST_ASM330LHHX_FSM_ENABLED) {
		err = st_asm330lhhx_read_locked(hw,
					ST_ASM330LHHX_FSM_STATUS_A_MAINPAGE,
					(void *)&__fsm_status, 2);
		if (err)
			return err;

		fsm_status = le16_to_cpu(__fsm_status);
		if (fsm_status) {
			for (i = 0; i < ST_ASM330LHHX_FSM_NUMBER; i++) {
				id = st_asm330lhhx_fsm_sensor_list[i];
				if (!(hw->enable_mask & BIT(id)))
					continue;

				if (fsm_status & BIT(i)) {
					iio_dev = hw->iio_devs[id];
					if (!iio_dev) {
						err = -ENOENT;

						return err;
					}

					sensor = iio_priv(iio_dev);
					err = st_asm330lhhx_read_page_locked(hw,
						sensor->outreg_addr,
						(void *)&event[i], 1);
					if (err)
						return err;

					iio_push_event(iio_dev, (u64)event[i],
						       iio_get_time_ns(iio_dev));

					dev_info(hw->dev,
						 "FSM %d Status %x FSM EVENT %llx\n",
						 id, mlc_status, (u64)event[i]);
				}
			}
		}
	}

	return err;
}

static int st_asm330lhhx_of_get_mlc_int_pin(struct st_asm330lhhx_hw *hw,
					    int *pin)
{
	struct device_node *np = hw->dev->of_node;

	if (!np)
		return -EINVAL;

	return of_property_read_u32(np, "st,mlc-int-pin", pin);
}

int st_asm330lhhx_mlc_probe(struct st_asm330lhhx_hw *hw)
{
	int int_pin;
	int ret;

	ret = st_asm330lhhx_of_get_mlc_int_pin(hw, &int_pin);
	if (ret < 0) {
		dev_info(hw->dev, "missing mlc-int-pin, using default\n");
		int_pin = hw->int_pin;
	}

	if (int_pin != 1 && int_pin != 2) {
		dev_err(hw->dev, "invalid mlc interrupt configuration\n");
		return -EINVAL;
	}

	hw->iio_devs[ST_ASM330LHHX_ID_MLC] =
		st_asm330lhhx_mlc_alloc_iio_dev(hw, ST_ASM330LHHX_ID_MLC);
	if (!hw->iio_devs[ST_ASM330LHHX_ID_MLC])
		return -ENOMEM;

	hw->mlc_config = devm_kzalloc(hw->dev,
				      sizeof(struct st_asm330lhhx_mlc_config_t),
				      GFP_KERNEL);
	if (!hw->mlc_config)
		return -ENOMEM;

	hw->mlc_config->mlc_int_pin = int_pin;

	return 0;
}

int st_asm330lhhx_mlc_remove(struct device *dev)
{
	struct st_asm330lhhx_hw *hw = dev_get_drvdata(dev);

	return st_asm330lhhx_mlc_flush_all(hw);
}
EXPORT_SYMBOL(st_asm330lhhx_mlc_remove);

int st_asm330lhhx_mlc_init_preload(struct st_asm330lhhx_hw *hw)
{
#ifdef CONFIG_IIO_ST_ASM330LHHX_MLC_PRELOAD
	hw->preload_mlc = 1;
	st_asm330lhhx_mlc_update(&st_asm330lhhx_mlc_preload, hw);
#endif /* CONFIG_IIO_ST_ASM330LHHX_MLC_PRELOAD */

	return 0;
}
