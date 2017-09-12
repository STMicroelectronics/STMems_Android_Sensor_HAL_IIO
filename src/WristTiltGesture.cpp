/*
 * STMicroelectronics Tilt Sensor Class
 *
 * Copyright 2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "WristTiltGesture.h"

WristTiltGesture::WristTiltGesture(HWSensorBaseCommonData *data, const char *name, int handle,
		unsigned int hw_fifo_len, float power_consumption) :
		HWSensorBase(data, name, handle, SENSOR_TYPE_TILT_DETECTOR, hw_fifo_len,
			power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_LOLLIPOP_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_WRIST_TILT_GESTURE;
	sensor_t_data.flags |= SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = 1.0f;
	sensor_t_data.maxRange = 1.0f;
}

WristTiltGesture::~WristTiltGesture()
{

}

int WristTiltGesture::SetDelay(int __attribute__((unused))handle,
				int64_t __attribute__((unused))period_ns,
				int64_t __attribute__((unused))timeout,
				bool __attribute__((unused))lock_en_mute)
{
	return 0;
}

void WristTiltGesture::ProcessData(SensorBaseData *data)
{
	sensor_event.data[0] = 1.0f;
	sensor_event.timestamp = data->timestamp;

	HWSensorBase::WriteDataToPipe(0);
	HWSensorBase::ProcessData(data);
}
