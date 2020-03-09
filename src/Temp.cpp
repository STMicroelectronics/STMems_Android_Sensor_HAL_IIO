/*
 * STMicroelectronics Temp Sensor Class
 *
 * Copyright 2016 STMicroelectronics Inc.
 * Author: Lorenzo Bianconi - <lorenzo.bianconi@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "Temp.h"

Temp::Temp(HWSensorBaseCommonData *data, const char *name,
	   struct device_iio_sampling_freqs *sfa,
	   int handle, unsigned int hw_fifo_len,
	   float power_consumption, bool wakeup)
	: HWSensorBaseWithPollrate(data, name, sfa, handle,
				   SENSOR_TYPE_AMBIENT_TEMPERATURE,
				   hw_fifo_len, power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_AMBIENT_TEMPERATURE;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
#else /* CONFIG_ST_HAL_ANDROID_VERSION */
	(void)wakeup;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = fabs(data->channels[0].scale);
	sensor_t_data.maxRange = sensor_t_data.resolution * (pow(2, data->channels[0].bits_used) - 1);
}

void Temp::ProcessData(SensorBaseData *data)
{
#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: t=%f, timestamp=%" PRIu64 "ns, "
	      "deltatime=%" PRIu64 "ns (sensor type: %d).", sensor_t_data.name,
	      data->raw[0], data->timestamp,
	      data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

	sensor_event.relative_humidity = data->raw[0];
	sensor_event.timestamp = data->timestamp;

	HWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	HWSensorBaseWithPollrate::ProcessData(data);
}
