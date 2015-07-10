/*
 * STMicroelectronics SW Geomagnetic Rotation Vector Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWGeoMagRotationVector.h"

SWGeoMagRotationVector::SWGeoMagRotationVector(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR,
			true, true, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_MAGN_FUSION6X;
	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;
}

SWGeoMagRotationVector::~SWGeoMagRotationVector()
{

}

void SWGeoMagRotationVector::ProcessData(SensorBaseData *data)
{
	memcpy(sensor_event.data, data->processed, SENSOR_DATA_4AXIS * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}
