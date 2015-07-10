/*
 * STMicroelectronics Accel-Magn-Gyro Fusion 9X Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#define __STDC_LIMIT_MACROS
#define __STDINT_LIMITS

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWAccelMagnGyroFusion9X.h"

extern "C" {
	#include "iNemoEngineProAPI.h"
}

SWAccelMagnGyroFusion9X::SWAccelMagnGyroFusion9X(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_ST_ACCEL_MAGN_GYRO_FUSION9X,
			false, false, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
	sensor_t_data.maxDelay = FREQUENCY_TO_US(CONFIG_ST_HAL_MIN_FUSION_POLLRATE);
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = ST_SENSOR_FUSION_RESOLUTION(1.0f);
	sensor_t_data.maxRange = 1.0f;

	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ACCELEROMETER;
	dependencies_type_list[SENSOR_DEPENDENCY_ID_1] = SENSOR_TYPE_GEOMAGNETIC_FIELD;
	dependencies_type_list[SENSOR_DEPENDENCY_ID_2] = SENSOR_TYPE_GYROSCOPE;
	id_sensor_trigger = SENSOR_DEPENDENCY_ID_2;
}

SWAccelMagnGyroFusion9X::~SWAccelMagnGyroFusion9X()
{

}

int SWAccelMagnGyroFusion9X::CustomInit()
{
	iNemoEngine_API_Initialization(NULL, NULL);

	return 0;
}

int SWAccelMagnGyroFusion9X::Enable(int handle, bool enable, bool lock_en_mutex)
{
	int err;
	bool old_status;
	bool old_status_no_handle;

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	old_status = GetStatus(false);
	old_status_no_handle = GetStatusExcludeHandle(handle);

	err = SWSensorBaseWithPollrate::Enable(handle, enable, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		return err;
	}

	if ((enable && !old_status) || (!enable && !old_status_no_handle)) {
		if (enable) {
			sensor_event.timestamp = 0;
			sensor_global_enable = android::elapsedRealtimeNano();
		} else
			sensor_global_disable = android::elapsedRealtimeNano();

		iNemoEngine_API_enable9X(enable);
	}

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

int SWAccelMagnGyroFusion9X::SetDelay(int handle, int64_t period_ns, int64_t timeout, bool lock_en_mutex)
{
	int err;

	if ((period_ns > FREQUENCY_TO_NS(CONFIG_ST_HAL_MIN_FUSION_POLLRATE)) && (period_ns != INT64_MAX))
		period_ns = FREQUENCY_TO_NS(CONFIG_ST_HAL_MIN_FUSION_POLLRATE);

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	err = SWSensorBaseWithPollrate::SetDelay(handle, period_ns, timeout, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		return err;
	}

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

void SWAccelMagnGyroFusion9X::ProcessData(SensorBaseData *data)
{
	unsigned int i;
	SensorBaseData accel_data, magn_data;
	int err, nomaxdata_accel = 10, nomaxdata_magn = 10;
	iNemoSensorsData sdata;

	do {
		err = GetLatestValidDataFromDependency(SENSOR_DEPENDENCY_ID_0, &accel_data, data->timestamp);
		if (err < 0) {
			usleep(10);
			nomaxdata_accel--;
			continue;
		}

	} while ((nomaxdata_accel >= 0) && (err < 0));

	do {
		err = GetLatestValidDataFromDependency(SENSOR_DEPENDENCY_ID_1, &magn_data, data->timestamp);
		if (err < 0) {
			usleep(10);
			nomaxdata_magn--;
			continue;
		}

	} while ((nomaxdata_magn >= 0) && (err < 0));

	if ((nomaxdata_accel > 0) && (nomaxdata_magn > 0)) {
		memcpy(sdata.accel, accel_data.raw, sizeof(sdata.accel));
		memcpy(sdata.magn, magn_data.processed, sizeof(sdata.magn));
		memcpy(sdata.gyro, data->processed, sizeof(sdata.gyro));

		iNemoEngine_API_Run(data->timestamp - sensor_event.timestamp, &sdata);
	}

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data from trigger: x=%f y=%f z=%f, timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
				sensor_t_data.name, data->raw[0], data->raw[1], data->raw[2],
				data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	sensor_event.timestamp = data->timestamp;
	outdata.timestamp = data->timestamp;
	outdata.flush_event_handle = data->flush_event_handle;
	outdata.accuracy = data->accuracy < magn_data.accuracy ? data->accuracy : magn_data.accuracy;
	outdata.pollrate_ns = data->pollrate_ns;

	for (i = 0; i < push_data.num; i++) {
		if (!push_data.sb[i]->ValidDataToPush(outdata.timestamp))
			continue;

		switch (push_data.sb[i]->GetType()) {
		case SENSOR_TYPE_ROTATION_VECTOR:
			err = iNemoEngine_API_Get_Quaternion(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_ORIENTATION:
			err = iNemoEngine_API_Get_Euler_Angles(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_GRAVITY:
			err = iNemoEngine_API_Get_Gravity(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_LINEAR_ACCELERATION:
			err = iNemoEngine_API_Get_Linear_Acceleration(outdata.processed);
			if (err < 0)
				continue;

			break;

		default:
			return;
		}

		push_data.sb[i]->ReceiveDataFromDependency(sensor_t_data.handle, &outdata);
	}
}
