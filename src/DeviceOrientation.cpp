/*
 * STMicroelectronics DeviceOrientation Sensor Class
 *
 * Copyright 2017 STMicroelectronics Inc.
 * Author: Lorenzo Bianconi - <lorenzo.bianconi@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "DeviceOrientation.h"

DeviceOrientation::DeviceOrientation(HWSensorBaseCommonData *data,
				     const char *name,
				     struct device_iio_sampling_freqs *sfa,
				     int handle, unsigned int hw_fifo_len,
				     float power_consumption, bool wakeup)
				: HWSensorBaseWithPollrate(data, name, sfa, handle,
							   SENSOR_TYPE_DEVICE_ORIENTATION,
							   hw_fifo_len, power_consumption)
{
	sensor_t_data.stringType = SENSOR_STRING_TYPE_DEVICE_ORIENTATION;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
}

void DeviceOrientation::ProcessData(SensorBaseData *data)
{
#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: p=%f, timestamp=%" PRIu64 "ns, "
	      "deltatime=%" PRIu64 "ns (sensor type: %d).", sensor_t_data.name,
	      data->raw[0], data->timestamp,
	      data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	sensor_event.data[0] = data->raw[0];
	sensor_event.timestamp = data->timestamp;

	HWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	HWSensorBaseWithPollrate::ProcessData(data);
}
