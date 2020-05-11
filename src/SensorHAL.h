/*
 * Copyright (C) 2015-2016 STMicroelectronics
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ST_SENSOR_HAL_H
#define ST_SENSOR_HAL_H

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <poll.h>

#include "SensorBase.h"
#include "SelfTest.h"
#include "common_data.h"

#ifdef CONFIG_ST_HAL_DIRECT_REPORT_SENSOR
#include <unordered_map>
#include <utils/Mutex.h>
#include "RingBuffer.h"
#endif /* CONFIG_ST_HAL_DIRECT_REPORT_SENSOR */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)		(int)((sizeof(a) / sizeof(*(a))) / \
					static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif

/*
 * IIO driver sensors names
 */
#define ST_SENSORS_LIST_0				"test"
#define ST_SENSORS_LIST_1				"lsm6ds3"
#define ST_SENSORS_LIST_2				"lis3mdl"
#define ST_SENSORS_LIST_3				"lsm303dlhc"
#define ST_SENSORS_LIST_4				"lis3dh"
#define ST_SENSORS_LIST_5				"lsm330d"
#define ST_SENSORS_LIST_6				"lsm330dl"
#define ST_SENSORS_LIST_7				"lsm330dlc"
#define ST_SENSORS_LIST_8				"lis331dlh"
#define ST_SENSORS_LIST_9				"lsm303dl"
#define ST_SENSORS_LIST_10				"lsm303dlh"
#define ST_SENSORS_LIST_11				"lsm303dlm"
#define ST_SENSORS_LIST_12				"lsm330"
#define ST_SENSORS_LIST_13				"lsm303agr"
#define ST_SENSORS_LIST_14				"lis2ds12"
#define ST_SENSORS_LIST_15				"lis2dg"
#define ST_SENSORS_LIST_16				"l3g4200d"
#define ST_SENSORS_LIST_17				"l3gd20"
#define ST_SENSORS_LIST_18				"lps331ap"
#define ST_SENSORS_LIST_19				"lps25h"
#define ST_SENSORS_LIST_20				"lsm6dsm"
#define ST_SENSORS_LIST_21				"lsm6ds33"
#define ST_SENSORS_LIST_22				"lsm6ds3h"
#define ST_SENSORS_LIST_23				"lsm303ah"
#define ST_SENSORS_LIST_24				"lps22hb"
#define ST_SENSORS_LIST_25				"lis2hh12"
#define ST_SENSORS_LIST_26				"hts221"
#define ST_SENSORS_LIST_27				"lis2dw12"
#define ST_SENSORS_LIST_28				"lis2mdl"
#define ST_SENSORS_LIST_29				"lsm9ds1"
#define ST_SENSORS_LIST_30				"lsm6dso"
#define ST_SENSORS_LIST_31				"lsm6dsl"
#define ST_SENSORS_LIST_32				"asm330lhh"
#define ST_SENSORS_LIST_33				"lis2dh"
#define ST_SENSORS_LIST_34				"lps33hw"
#define ST_SENSORS_LIST_35				"lps35hw"
#define ST_SENSORS_LIST_36				"lps22hd"
#define ST_SENSORS_LIST_37				"lis3dhh"
#define ST_SENSORS_LIST_38				"lsm6ds0"
#define ST_SENSORS_LIST_40				"ism330dlc"
#define ST_SENSORS_LIST_41				"lps22hh"
#define ST_SENSORS_LIST_42				"iis2mdc"
#define ST_SENSORS_LIST_43				"iis2dh"
#define ST_SENSORS_LIST_44				"ism303dac"
#define ST_SENSORS_LIST_45				"iis3dhhc"
#define ST_SENSORS_LIST_46				"lsm6dsr"
#define ST_SENSORS_LIST_47				"lps27hhw"
#define ST_SENSORS_LIST_48				"lsm6dso32"

/*
 * IIO driver sensors suffix for sensors
 */
#define ACCEL_NAME_SUFFIX_IIO				"_accel"
#define MAGN_NAME_SUFFIX_IIO				"_magn"
#define GYRO_NAME_SUFFIX_IIO				"_gyro"
#define SIGN_MOTION_NAME_SUFFIX_IIO			"_sign_motion"
#define STEP_DETECTOR_NAME_SUFFIX_IIO			"_step_d"
#define STEP_COUNTER_NAME_SUFFIX_IIO			"_step_c"
#define TILT_NAME_SUFFIX_IIO				"_tilt"
#define PRESSURE_NAME_SUFFIX_IIO			"_press"
#define RHUMIDITY_NAME_SUFFIX_IIO			"_rh"
#define TEMP_NAME_SUFFIX_IIO				"_temp"
#define WRIST_GESTURE_NAME_SUFFIX_IIO			"_wrist"
#define GLANCE_GESTURE_NAME_SUFFIX_IIO			"_glance"

#define ST_HAL_WAKEUP_SUFFIX_IIO			"_wk"
#define ST_HAL_PICKUP_SUFFIX_IIO			"_pickup"
#define ST_HAL_MOTION_SUFFIX_IIO			"_motion"
#define ST_HAL_NO_MOTION_SUFFIX_IIO			"_no_motion"
#define ST_HAL_DEVICE_ORIENTATION_SUFFIX_IIO		"_dev_orientation"

#define ST_HAL_NEW_SENSOR_SUPPORTED(DRIVER_NAME, ANDROID_SENSOR_TYPE, IIO_SENSOR_TYPE, ANDROID_NAME, POWER_CONSUMPTION) \
	{ \
	.driver_name = DRIVER_NAME, \
	.android_name = ANDROID_NAME, \
	.android_sensor_type = ANDROID_SENSOR_TYPE,\
	.iio_sensor_type = IIO_SENSOR_TYPE, \
	.power_consumption = POWER_CONSUMPTION,\
	},

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_MARSHMALLOW_VERSION)
#define ST_HAL_IIO_DEVICE_API_VERSION			SENSORS_DEVICE_API_VERSION_1_4
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#if (CONFIG_ST_HAL_ANDROID_VERSION == ST_HAL_LOLLIPOP_VERSION)
#define ST_HAL_IIO_DEVICE_API_VERSION			SENSORS_DEVICE_API_VERSION_1_3
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#if (CONFIG_ST_HAL_ANDROID_VERSION == ST_HAL_KITKAT_VERSION)
#define ST_HAL_IIO_DEVICE_API_VERSION			SENSORS_DEVICE_API_VERSION_1_1
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#if defined(CONFIG_ST_HAL_HAS_GEOMAG_FUSION) && \
				(defined(CONFIG_ST_HAL_GEOMAG_ROT_VECTOR_AP_ENABLED) || \
				defined(CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_6X) || \
				defined(CONFIG_ST_HAL_VIRTUAL_GYRO_ENABLED) || \
				defined(CONFIG_ST_HAL_ORIENTATION_AP_ENABLED_6X))
#define ST_HAL_NEEDS_GEOMAG_FUSION			1
#endif /* CONFIG_ST_HAL_GAME_ROT_VECTOR_AP_ENABLED */

#if defined(CONFIG_ST_HAL_HAS_6AX_FUSION) && \
				(defined(CONFIG_ST_HAL_GAME_ROT_VECTOR_AP_ENABLED) || \
				defined(CONFIG_ST_HAL_GRAVITY_AP_ENABLED_6X) || \
				defined(CONFIG_ST_HAL_LINEAR_AP_ENABLED_6X))
#define ST_HAL_NEEDS_6AX_FUSION				1
#endif /* CONFIG_ST_HAL_GAME_ROT_VECTOR_AP_ENABLED */

#if defined(CONFIG_ST_HAL_HAS_9AX_FUSION) && \
				(defined(CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_9X) || \
				defined(CONFIG_ST_HAL_ORIENTATION_AP_ENABLED_9X) || \
				defined(CONFIG_ST_HAL_GRAVITY_AP_ENABLED_9X) || \
				defined(CONFIG_ST_HAL_LINEAR_AP_ENABLED_9X))
#define ST_HAL_NEEDS_9AX_FUSION				1
#endif /* CONFIG_ST_HAL_GAME_ROT_VECTOR_AP_ENABLED */


struct STSensorHAL_data {
	sensors_poll_device_1 poll_device;

	pthread_t *data_threads;
	pthread_t *events_threads;
	SensorBase *sensor_classes[ST_HAL_IIO_MAX_DEVICES];

	int last_handle;

	unsigned int sensor_available;
	struct sensor_t *sensor_t_list;

#ifdef CONFIG_ST_HAL_HAS_SELFTEST_FUNCTIONS
	SelfTest *self_test;
#endif /* CONFIG_ST_HAL_HAS_SELFTEST_FUNCTIONS */

	struct pollfd android_pollfd[ST_HAL_IIO_MAX_DEVICES];

#ifdef CONFIG_ST_HAL_DIRECT_REPORT_SENSOR
	int mDirectChannelHandle;
	android::Mutex mDirectChannelLock;
#endif /* CONFIG_ST_HAL_DIRECT_REPORT_SENSOR */

} typedef STSensorHAL_data;

#endif /* ST_SENSOR_HAL_H */
