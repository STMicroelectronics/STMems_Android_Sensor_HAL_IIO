/*
 * Copyright (C) 2019 STMicroelectronics
 * Author: Matteo Dameno - <matteo.dameno@st.com>
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

#ifndef ST_SENSOR_ADDITIONAL_INFO_H
#define ST_SENSOR_ADDITIONAL_INFO_H

#include "common_data.h"

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)

#include <hardware/sensors.h>

/*
 * class SensorAdditionalInfoEvent
 *
 *
 */
class SensorAdditionalInfoEvent {
private:
	additional_info_event_t sensor_additional_info_event;

protected:

public:
	SensorAdditionalInfoEvent(int32_t payload_type, int32_t serial);
	~SensorAdditionalInfoEvent();

	static const additional_info_event_t* getBeginFrameEvent();
	static const additional_info_event_t* getEndFrameEvent();
	static const additional_info_event_t* getDefaultSensorPlacementFrameEvent();
	void incrementEventSerial();
};

#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

#endif /* ST_SENSOR_ADDITIONAL_INFO_H */
