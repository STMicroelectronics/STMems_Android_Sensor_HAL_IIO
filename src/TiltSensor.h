/*
 * Copyright (C) 2015-2016 STMicroelectronics
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ST_TILT_SENSOR_H
#define ST_TILT_SENSOR_H

#include "HWSensorBase.h"

/*
 * class TiltSensor
 */
class TiltSensor : public HWSensorBase {
public:
	TiltSensor(HWSensorBaseCommonData *data, const char *name, int handle,
				unsigned int hw_fifo_len, float power_consumption);
	virtual ~TiltSensor();

	virtual int SetDelay(int handle, int64_t period_ns, int64_t timeout, bool lock_en_mute);
	virtual void ProcessData(SensorBaseData *data);
	void ProcessEvent(struct device_iio_events *event_data);
};

#endif /* ST_TILT_SENSOR_H */
