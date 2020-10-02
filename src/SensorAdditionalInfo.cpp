/*
 * STMicroelectronics SensorAdditionalInfo Class
 *
 * Copyright 2019 STMicroelectronics Inc.
 * Author: Matteo Dameno - <matteo.dameno@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */


#include "SensorAdditionalInfo.h"

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)

#include <stdlib.h>

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_OREO_VERSION)
#include <log/log.h>
#else
#include <cutils/log.h>
#endif /* use log/log.h start from android 8 major version */

SensorAdditionalInfoEvent::SensorAdditionalInfoEvent(int32_t payload_type, int32_t serial)
{
	sensor_additional_info_event.type = payload_type;
	sensor_additional_info_event.serial = serial;
};


SensorAdditionalInfoEvent::~SensorAdditionalInfoEvent()
{
	//delete or so;
}

const additional_info_event_t* SensorAdditionalInfoEvent::getBeginFrameEvent()
{
	static const additional_info_event_t sensor_additional_info_beginFrame = {
		.type = AINFO_BEGIN,
		.serial = 0,
	};
	return &sensor_additional_info_beginFrame;
}

const additional_info_event_t* SensorAdditionalInfoEvent::getEndFrameEvent()
{
	static const additional_info_event_t sensor_additional_info_endFrame = {
		.type = AINFO_END,
		.serial = 0,
	};
	return &sensor_additional_info_endFrame;
}

const additional_info_event_t* SensorAdditionalInfoEvent::getDefaultSensorPlacementFrameEvent()
{
	static const additional_info_event_t sensor_additional_info_defaultSensorPlacementFrame = {
		.type = AINFO_SENSOR_PLACEMENT,
		.serial = 0,
		.data_float = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
	};
	return &sensor_additional_info_defaultSensorPlacementFrame;
}

void SensorAdditionalInfoEvent::incrementEventSerial()
{
	sensor_additional_info_event.serial++;
}

#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
