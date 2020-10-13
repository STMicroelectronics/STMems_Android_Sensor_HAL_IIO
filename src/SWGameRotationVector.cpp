/*
 * STMicroelectronics SW Game Rotation Vector Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWGameRotationVector.h"

SWGameRotationVector::SWGameRotationVector(const char *name, int handle) :
		SWSensorBaseWithPollrate(name, handle, SENSOR_TYPE_GAME_ROTATION_VECTOR,
			true, true, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_GAME_ROTATION_VECTOR;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_GYRO_FUSION6X;
	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
	supportsSensorAdditionalInfo = true;
	sensor_t_data.flags |= SENSOR_FLAG_ADDITIONAL_INFO;
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
}

SWGameRotationVector::~SWGameRotationVector()
{

}

void SWGameRotationVector::ProcessData(SensorBaseData *data)
{
	/*
	 * Game rotation vector
	 *
	 * Underlying physical sensors: Accelerometer and Gyroscope (no Magnetometer)
	 * Reporting-mode: Continuous
	 * getDefaultSensor(SENSOR_TYPE_GAME_ROTATION_VECTOR) returns a non-wake-up sensor
	 * A game rotation vector sensor is similar to a rotation vector sensor but not using the geomagnetic field.
	 * Therefore the Y axis doesn't point north but instead to some other reference. That reference is allowed
	 * to drift by the same order of magnitude as the gyroscope drifts around the Z axis.
	 * See the Rotation vector sensor for details on how to set sensors_event_t.data[0-3].
	 * This sensor does not report an estimated heading accuracy: sensors_event_t.data[4]
	 * is reserved and should be set to 0.
	 * In an ideal case, a phone rotated and returned to the same real-world orientation should
	 * report the same game rotation vector.
	 *
	 * TODO:
	 * This sensor must be based on a gyroscope and an accelerometer. It cannot use magnetometer
	 * as an input, besides, indirectly, through estimation of the gyroscope bias.
	 */
	memcpy(sensor_event.data, data->processed, SENSOR_DATA_4AXIS_ACCUR * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
int SWGameRotationVector::getSensorAdditionalInfoPayLoadFramesArray(additional_info_event_t **array_sensorAdditionalInfoPLFrames)
{
	additional_info_event_t* p_custom_SAI_Placement_event = nullptr;

	// place for ODM/OEM to fill custom_SAI_Placement_event
	// p_custom_SAI_Placement_event = &custom_SAI_Placement_event

	return UseCustomAINFOSensorPlacementPLFramesArray(array_sensorAdditionalInfoPLFrames, p_custom_SAI_Placement_event);
}
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
