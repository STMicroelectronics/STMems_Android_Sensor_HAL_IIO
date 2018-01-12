/*
 * STMicroelectronics Accelerometer Uncalibrated Sensor Class
 *
 * Copyright 2017 STMicroelectronics Inc.
 * Author: Armando Visconti <armando.visconti@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWAccelerometerUncalibrated.h"

SWAccelerometerUncalibrated::SWAccelerometerUncalibrated(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED,
			true, true, true, true)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ACCELEROMETER;
	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;
}

SWAccelerometerUncalibrated::~SWAccelerometerUncalibrated()
{

}

void SWAccelerometerUncalibrated::ProcessData(SensorBaseData *data)
{
#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data from trigger: x=%f y=%f z=%f, timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
				sensor_t_data.name, data->raw[0], data->raw[1], data->raw[2],
				data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	memcpy(sensor_event.uncalibrated_accelerometer.uncalib, data->raw, SENSOR_DATA_3AXIS * sizeof(float));
	memcpy(sensor_event.uncalibrated_accelerometer.bias, data->offset, SENSOR_DATA_3AXIS * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}
