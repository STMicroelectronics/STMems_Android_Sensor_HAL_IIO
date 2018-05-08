/*
 * STMicroelectronics SW Rotation Vector Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWRotationVector.h"

SWRotationVector::SWRotationVector(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_ROTATION_VECTOR,
			true, true, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_ROTATION_VECTOR;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#ifdef CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_9X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_MAGN_GYRO_FUSION9X;
#endif /* CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_9X */

#ifdef CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_6X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_MAGN_FUSION6X;
#endif /* CONFIG_ST_HAL_ROT_VECTOR_AP_ENABLED_6X */

	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;
}

SWRotationVector::~SWRotationVector()
{

}

void SWRotationVector::ProcessData(SensorBaseData *data)
{
	/*
	 * The heading error must be less than estimated_accuracy 95% of the time.
	 * This sensor must use a gyroscope as the main orientation change input.
	 */
	memcpy(sensor_event.data, data->processed, SENSOR_DATA_4AXIS_ACCUR * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}
