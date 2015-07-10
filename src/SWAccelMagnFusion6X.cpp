/*
 * STMicroelectronics Accel-Magn Fusion 6X Sensor Class
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

#include "SWAccelMagnFusion6X.h"

extern "C" {
	#include "iNemoEngineGeoMagAPI.h"
}

SWAccelMagnFusion6X::SWAccelMagnFusion6X(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_ST_ACCEL_MAGN_FUSION6X,
			false, false, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
	sensor_t_data.maxDelay = FREQUENCY_TO_US(CONFIG_ST_HAL_MIN_FUSION_POLLRATE);
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = ST_SENSOR_FUSION_RESOLUTION(1.0f);
	sensor_t_data.maxRange = 1.0f;

	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ACCELEROMETER;
	dependencies_type_list[SENSOR_DEPENDENCY_ID_1] = SENSOR_TYPE_MAGNETIC_FIELD;
	id_sensor_trigger = SENSOR_DEPENDENCY_ID_1;
}

SWAccelMagnFusion6X::~SWAccelMagnFusion6X()
{

}

int SWAccelMagnFusion6X::CustomInit()
{
	iNemoEngine_GeoMag_API_Initialization(100);

	return 0;
}

int SWAccelMagnFusion6X::Enable(int handle, bool enable, bool lock_en_mutex)
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
		if (enable)
			sensor_global_enable = android::elapsedRealtimeNano();
		else
			sensor_global_disable = android::elapsedRealtimeNano();
	}

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

int SWAccelMagnFusion6X::SetDelay(int handle, int64_t period_ns, int64_t timeout, bool lock_en_mutex)
{
	int err;

	if ((period_ns > FREQUENCY_TO_NS(CONFIG_ST_HAL_MIN_FUSION_POLLRATE)) && (period_ns != INT64_MAX))
		period_ns = FREQUENCY_TO_NS(CONFIG_ST_HAL_MIN_FUSION_POLLRATE);

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	err = SWSensorBaseWithPollrate::SetDelay(handle, period_ns, timeout, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_lock(&enable_mutex);

		return err;
	}

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

void SWAccelMagnFusion6X::ProcessData(SensorBaseData *data)
{
	unsigned int i;
	int err, nomaxdata = 10;
	SensorBaseData accel_data;
	iNemoGeoMagSensorsData sdata;

	do {
		err = GetLatestValidDataFromDependency(SENSOR_DEPENDENCY_ID_0, &accel_data, data->timestamp);
		if (err < 0) {
			usleep(10);
			nomaxdata--;
			continue;
		}

	} while ((nomaxdata >= 0) && (err < 0));

	if (nomaxdata > 0) {
		int64_t delta_ms;

		delta_ms = (data->timestamp - sensor_event.timestamp) / 1000000;
		memcpy(sdata.accel, accel_data.raw, sizeof(sdata.accel));
		memcpy(sdata.magn, data->processed, sizeof(sdata.magn));
		iNemoEngine_GeoMag_API_Run(delta_ms, &sdata);
	}

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data from trigger: x=%f y=%f z=%f, timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
				sensor_t_data.name, data->raw[0], data->raw[1], data->raw[2],
				data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	sensor_event.timestamp = data->timestamp;
	outdata.timestamp = data->timestamp;
	outdata.flush_event_handle = data->flush_event_handle;
	outdata.accuracy = data->accuracy;
	outdata.pollrate_ns = data->pollrate_ns;

	for (i = 0; i < push_data.num; i++) {
		if (!push_data.sb[i]->ValidDataToPush(outdata.timestamp))
			continue;

		switch (push_data.sb[i]->GetType()) {
		case SENSOR_TYPE_ROTATION_VECTOR:
		case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
			err = iNemoEngine_GeoMag_API_Get_Quaternion(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_ORIENTATION:
			err = iNemoEngine_GeoMag_API_Get_Hpr(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_LINEAR_ACCELERATION:
			err = iNemoEngine_GeoMag_API_Get_LinAcc(outdata.processed);
			if (err < 0)
				continue;

			break;

		case SENSOR_TYPE_GRAVITY:
			err = iNemoEngine_GeoMag_API_Get_Gravity(outdata.processed);
			if (err < 0)
				continue;

			break;

#ifdef CONFIG_ST_HAL_VIRTUAL_GYRO_ENABLED
		case SENSOR_TYPE_GYROSCOPE:
			err = iNemoEngine_GeoMag_API_Get_VirtualGyro(outdata.processed);
			if (err < 0)
				continue;

			break;
#endif /* CONFIG_ST_HAL_VIRTUAL_GYRO_ENABLED */
		default:
			return;
		}

		push_data.sb[i]->ReceiveDataFromDependency(sensor_t_data.handle, &outdata);
	}
}
