/*
 * STMicroelectronics SW Linear Acceleration Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWLinearAccel.h"

SWLinearAccel::SWLinearAccel(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_LINEAR_ACCELERATION,
			true, false, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_LINEAR_ACCELERATION;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#ifdef CONFIG_ST_HAL_LINEAR_AP_ENABLED_9X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_MAGN_GYRO_FUSION9X;
#endif /* CONFIG_ST_HAL_LINEAR_AP_ENABLED_9X */

#ifdef CONFIG_ST_HAL_LINEAR_AP_ENABLED_6X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_GYRO_FUSION6X;
#endif /* CONFIG_ST_HAL_LINEAR_AP_ENABLED_6X */

	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
	supportsSensorAdditionalInfo = true;
	sensor_t_data.flags |= SENSOR_FLAG_ADDITIONAL_INFO;
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
}

SWLinearAccel::~SWLinearAccel()
{

}

int SWLinearAccel::CustomInit()
{
	bool valid;
	float maxRange = 0;

	valid = GetDependencyMaxRange(SENSOR_TYPE_ACCELEROMETER, &maxRange);
	if (!valid)
		return -EINVAL;

	sensor_t_data.maxRange = maxRange;

	return 0;
}

void SWLinearAccel::ProcessData(SensorBaseData *data)
{
	memcpy(sensor_event.data, data->processed, SENSOR_DATA_3AXIS * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
int SWLinearAccel::getSensorAdditionalInfoPayLoadFramesArray(additional_info_event_t **array_sensorAdditionalInfoPLFrames)
{
	additional_info_event_t* p_custom_SAI_Placement_event = nullptr;

	// place for ODM/OEM to fill custom_SAI_Placement_event
	// p_custom_SAI_Placement_event = &custom_SAI_Placement_event

	return UseCustomAINFOSensorPlacementPLFramesArray(array_sensorAdditionalInfoPLFrames, p_custom_SAI_Placement_event);
}
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
