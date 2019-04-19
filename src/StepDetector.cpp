/*
 * STMicroelectronics Step Detector Base Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "StepDetector.h"

StepDetector::StepDetector(HWSensorBaseCommonData *data, const char *name,
		int handle, unsigned int hw_fifo_len,
		float power_consumption, bool wakeup) :
			HWSensorBase(data, name, handle,
			SENSOR_TYPE_STEP_DETECTOR, hw_fifo_len, power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_STEP_DETECTOR;
	sensor_t_data.flags |= SENSOR_FLAG_SPECIAL_REPORTING_MODE;

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
#else /* CONFIG_ST_HAL_ANDROID_VERSION */
	(void)wakeup;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = 1.0f;
	sensor_t_data.maxRange = 1.0f;
}

StepDetector::~StepDetector()
{

}

int StepDetector::SetDelay(int __attribute__((unused))handle,
				int64_t __attribute__((unused))period_ns,
				int64_t __attribute__((unused))timeout,
				bool __attribute__((unused))lock_en_mute)
{
	return 0;
}

void StepDetector::ProcessData(SensorBaseData *data)
{
	sensor_event.data[0] = 1.0f;
	sensor_event.timestamp = data->timestamp;

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: timestamp=%" PRIu64 "ns (sensor type: %d).", sensor_t_data.name, data->timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	HWSensorBase::WriteDataToPipe(0);
	HWSensorBase::ProcessData(data);
}

void StepDetector::ProcessEvent(struct device_iio_events *event_data)
{
	sensor_event.data[0] = 1.0f;
	sensor_event.timestamp = event_data->event_timestamp;

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor event: timestamp=%" PRIu64 "ns (sensor type: %d).", sensor_t_data.name, sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	HWSensorBase::WriteDataToPipe(0);
	HWSensorBase::ProcessEvent(event_data);
}
