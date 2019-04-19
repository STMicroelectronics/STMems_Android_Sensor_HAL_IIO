/*
 * STMicroelectronics Step Counter Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "StepCounter.h"

StepCounter::StepCounter(HWSensorBaseCommonData *data, const char *name,
		int handle, unsigned int hw_fifo_len, float power_consumption, bool wakeup) :
			HWSensorBase(data, name, handle,
			SENSOR_TYPE_STEP_COUNTER, hw_fifo_len, power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_STEP_COUNTER;
	sensor_t_data.flags |= SENSOR_FLAG_ON_CHANGE_MODE;

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
#else /* CONFIG_ST_HAL_ANDROID_VERSION */
	(void)wakeup;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = 1.0f;
	sensor_t_data.maxRange = pow(2, data->channels[0].bits_used) - 1;
}

StepCounter::~StepCounter()
{

}

int StepCounter::Enable(int handle, bool enable, bool lock_en_mutex)
{
	int err;
	bool old_status;

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	old_status = GetStatus(false);

	err = HWSensorBase::Enable(handle, enable, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		return err;
	}

	if (GetStatus(false) && !old_status)
		last_data_timestamp = 0;

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

int StepCounter::SetDelay(int handle, int64_t period_ns, int64_t timeout, bool lock_en_mutex)
{
	int err;
	int64_t min_pollrate_ns;
	int64_t restore_min_period_ms;

	if (lock_en_mutex)
		pthread_mutex_lock(&enable_mutex);

	err = HWSensorBase::SetDelay(handle, period_ns, timeout, false);
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		return err;
	}

	restore_min_period_ms = period_ns;
	sensors_pollrates[handle] = period_ns;
	min_pollrate_ns = GetMinPeriod(false);

	err = device_iio_utils::set_max_delivery_rate(common_data.device_iio_sysfs_path,
						      NS_TO_MS(min_pollrate_ns));
	if (err < 0) {
		if (lock_en_mutex)
			pthread_mutex_unlock(&enable_mutex);

		sensors_pollrates[handle] = restore_min_period_ms;

		return err;
	}

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_INFO)
	if (handle == sensor_t_data.handle)
		ALOGD("\"%s\": changed max delivery rate time to %dms (sensor type: %d).",
			sensor_t_data.name, (int)NS_TO_MS(min_pollrate_ns), sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	if (lock_en_mutex)
		pthread_mutex_unlock(&enable_mutex);

	return 0;
}

void StepCounter::ProcessData(SensorBaseData *data)
{
	sensor_event.u64.step_counter = (uint64_t)data->raw[0];
	sensor_event.timestamp = data->timestamp;

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: s=%f, timestamp=%" PRIu64 "ns (sensor type: %d).", sensor_t_data.name, data->raw[0], data->timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	HWSensorBase::WriteDataToPipe(0);
	HWSensorBase::ProcessData(data);
}
