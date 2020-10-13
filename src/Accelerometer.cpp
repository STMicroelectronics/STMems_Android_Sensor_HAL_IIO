/*
 * STMicroelectronics Accelerometer Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "Accelerometer.h"

#ifdef CONFIG_ST_HAL_ACCEL_CALIB_ENABLED
#define CALIBRATION_FREQUENCY	25
#define CALIBRATION_PERIOD_MS	(1000.0f / CALIBRATION_FREQUENCY)

extern "C" {
	#include "STAccCalibration_API.h"
}
#endif /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */

Accelerometer::Accelerometer(HWSensorBaseCommonData *data, const char *name,
		struct device_iio_sampling_freqs *sfa, int handle,
		unsigned int hw_fifo_len, float power_consumption, bool wakeup) :
			HWSensorBaseWithPollrate(data, name, sfa, handle,
			SENSOR_TYPE_ACCELEROMETER, hw_fifo_len, power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_ACCELEROMETER;
#ifdef CONFIG_ST_HAL_DIRECT_REPORT_SENSOR
	/* Support Direct Channel in Android O */
	sensor_t_data.flags |= (SENSOR_FLAG_CONTINUOUS_MODE |
				SENSOR_FLAG_DIRECT_CHANNEL_ASHMEM |
				(SENSOR_DIRECT_RATE_VERY_FAST <<
				 SENSOR_FLAG_SHIFT_DIRECT_REPORT));
#else /* CONFIG_ST_HAL_DIRECT_REPORT_SENSOR */
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_DIRECT_REPORT_SENSOR */

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
#else /* CONFIG_ST_HAL_ANDROID_VERSION */
	(void)wakeup;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = data->channels[0].scale;
	sensor_t_data.maxRange = sensor_t_data.resolution * (pow(2, data->channels[0].bits_used - 1) - 1);

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
	supportsSensorAdditionalInfo = true;
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
}

Accelerometer::~Accelerometer()
{

}

int Accelerometer::Enable(int handle, bool enable, bool lock_en_mutex)
{
#ifdef CONFIG_ST_HAL_ACCEL_CALIB_ENABLED
	int err;

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	err = HWSensorBaseWithPollrate::Enable(handle, enable, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		return err;
	}

	if (enable)
		ST_AccCalibration_API_Init(CALIBRATION_PERIOD_MS);
	else
		ST_AccCalibration_API_DeInit(CALIBRATION_PERIOD_MS);

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
#else /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */
	return HWSensorBaseWithPollrate::Enable(handle, enable, lock_en_mutex);
#endif /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */
}

void Accelerometer::ProcessData(SensorBaseData *data)
{
	float tmp_raw_data[SENSOR_DATA_3AXIS];
#ifdef CONFIG_ST_HAL_ACCEL_CALIB_ENABLED
	STAccCalibration_Input acc_cal_input;
	STAccCalibration_Output acc_cal_output;
#endif /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */

	memcpy(tmp_raw_data, data->raw, SENSOR_DATA_3AXIS * sizeof(float));

	data->raw[0] = SENSOR_X_DATA(tmp_raw_data[0], tmp_raw_data[1], tmp_raw_data[2], CONFIG_ST_HAL_ACCEL_ROT_MATRIX);
	data->raw[1] = SENSOR_Y_DATA(tmp_raw_data[0], tmp_raw_data[1], tmp_raw_data[2], CONFIG_ST_HAL_ACCEL_ROT_MATRIX);
	data->raw[2] = SENSOR_Z_DATA(tmp_raw_data[0], tmp_raw_data[1], tmp_raw_data[2], CONFIG_ST_HAL_ACCEL_ROT_MATRIX);

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: x=%f y=%f z=%f, timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
				sensor_t_data.name, data->raw[0], data->raw[1], data->raw[2],
				data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

#ifdef CONFIG_ST_HAL_FACTORY_CALIBRATION
	data->raw[0] = (data->raw[0] - factory_offset[0]) * factory_scale[0];
	data->raw[1] = (data->raw[1] - factory_offset[1]) * factory_scale[1];
	data->raw[2] = (data->raw[2] - factory_offset[2]) * factory_scale[2];
	data->accuracy = SENSOR_STATUS_ACCURACY_HIGH;
#else /* CONFIG_ST_HAL_FACTORY_CALIBRATION */
	data->accuracy = SENSOR_STATUS_UNRELIABLE;
#endif /* CONFIG_ST_HAL_FACTORY_CALIBRATION */

#ifdef CONFIG_ST_HAL_ACCEL_CALIB_ENABLED
	acc_cal_input.timestamp = data->timestamp;
	acc_cal_input.acc_raw[0] = data->raw[0];
	acc_cal_input.acc_raw[1] = data->raw[1];
	acc_cal_input.acc_raw[2] = data->raw[2];

	ST_AccCalibration_API_Run(&acc_cal_output, &acc_cal_input);

	data->processed[0] = acc_cal_output.acc_cal[0];
	data->processed[1] = acc_cal_output.acc_cal[1];
	data->processed[2] = acc_cal_output.acc_cal[2];
	data->offset[0] = acc_cal_output.offset[0];
	data->offset[1] = acc_cal_output.offset[1];
	data->offset[2] = acc_cal_output.offset[2];
	data->accuracy = acc_cal_output.accuracy;
#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": ACCEL-CALIB (accuracy %d - offs: x=%f y=%f z=%f) received new sensor data: x=%f y=%f z=%f (norm is %f), timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
				sensor_t_data.name, data->accuracy, data->offset[0], data->offset[1], data->offset[2], data->processed[0], data->processed[1], data->processed[2],
				sqrtf(data->processed[0]*data->processed[0] + data->processed[1]*data->processed[1] + data->processed[2]*data->processed[2]), data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */
#else /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */
	data->processed[0] = data->raw[0];
	data->processed[1] = data->raw[1];
	data->processed[2] = data->raw[2];

	data->accuracy = SENSOR_STATUS_UNRELIABLE;
#endif /* CONFIG_ST_HAL_ACCEL_CALIB_ENABLED */

	sensor_event.acceleration.x = data->processed[0];
	sensor_event.acceleration.y = data->processed[1];
	sensor_event.acceleration.z = data->processed[2];
	sensor_event.acceleration.status = data->accuracy;
	sensor_event.timestamp = data->timestamp;

	HWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	HWSensorBaseWithPollrate::ProcessData(data);
}


#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
int Accelerometer::getSensorAdditionalInfoPayLoadFramesArray(additional_info_event_t **array_sensorAdditionalInfoPLFrames)
{
	additional_info_event_t* p_custom_SAI_Placement_event = nullptr;

	// place for ODM/OEM to fill custom_SAI_Placement_event
	// p_custom_SAI_Placement_event = &custom_SAI_Placement_event

/*  //Custom Placement example
	additional_info_event_t custom_SAI_Placement_event;
	custom_SAI_Placement_event = {
		.type = AINFO_SENSOR_PLACEMENT,
		.serial = 0,
		.data_float = {-1, 0, 0, 1, 0, -1, 0, 2, 0, 0, -1, 3},
	};
	p_custom_SAI_Placement_event = &custom_SAI_Placement_event;
*/

	return UseCustomAINFOSensorPlacementPLFramesArray(array_sensorAdditionalInfoPLFrames, p_custom_SAI_Placement_event);

}
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
