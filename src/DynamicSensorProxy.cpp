/*
 * STMicroelectronics DynamicSensorProxy Sensor Class
 *
 * Copyright (C) 2017 STMicroelectronics
 * Author: Lorenzo Bianconi - <lorenzo.bianconi@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include "DynamicSensorProxy.h"

int DynamicSensorProxy::submitEvent(sp<BaseSensorObject> source,
				    const sensors_event_t &e)
{
#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_VERBOSE)
	ALOGD("%s: received event sensor %d\n", GetName(), e.sensor);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */
	
	return write(write_pipe_fd, &e, sizeof(sensors_event_t));
}

DynamicSensorProxy::DynamicSensorProxy(STSensorHAL_data *hal_data, int index)
	: SensorBase("Dynamic Sensor Proxy", index,
		     SENSOR_TYPE_DYNAMIC_SENSOR_META)
{
	manager = DynamicSensorManager::createInstance(index,
			DYNAMIC_SENSOR_HANDLE_COUNT, this);
	sensor_t_data = manager->getDynamicMetaSensor();

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_VERBOSE)
	ALOGD("%s: created Dynamic Sensor Proxy, handle: %d (sensor type: %d).",
	      GetName(), GetHandle(), GetType());
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	GetSensor_tData(&hal_data->sensor_t_list[index - 1]);
	hal_data->android_pollfd[index - 1].fd = GetFdPipeToRead();
	hal_data->android_pollfd[index - 1].events = POLLIN;
}

int DynamicSensorProxy::Enable(int handle, bool enable, bool lock_en_mutex)
{
	if (manager->owns(handle))
		return manager->activate(handle, enable);
	else
		return -EINVAL;
}

int DynamicSensorProxy::setDelay(int handle, int64_t sampling_period_ns,
				 int64_t max_report_latency_ns)
{
	if (manager->owns(handle))
		return manager->batch(handle, sampling_period_ns,
				      max_report_latency_ns);
	else
		return -EINVAL;
}

int DynamicSensorProxy::flush(int handle)
{
	if (manager->owns(handle))
		return manager->flush(handle);
	else
		return -EINVAL;
}

