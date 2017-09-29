/*
 * Copyright (C) 2017 STMicroelectronics
 * Author: Lorenzo Bianconi - <lorenzo.bianconi@st.com>
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

#ifndef ST_DYNAMICSENSOR_PROXY_H
#define ST_DYNAMICSENSOR_PROXY_H

#include <DynamicSensorManager.h>
#include <SensorEventCallback.h>

#include "SensorHAL.h"
#include "SensorBase.h"

using android::sp;

namespace android {
	namespace SensorHalExt {
		class BaseSensorObject;
		class DynamicSensorManager;
		class SensorEventCallback;
	}
}

using android::SensorHalExt::BaseSensorObject;
using android::SensorHalExt::DynamicSensorManager;
using android::SensorHalExt::SensorEventCallback;

#define DYNAMIC_SENSOR_HANDLE_COUNT	0xf0000

class DynamicSensorProxy : public SensorBase, public SensorEventCallback {
public:
	DynamicSensorProxy(STSensorHAL_data *hal_data, int index, int handle);
	int Enable(int handle, bool enable, bool lock_en_mutex);
	int SetDelay(int handle, int64_t period_ns, int64_t timeout,
		     bool lock_en_mutex);
	int flush(int handle);
	int submitEvent(sp<BaseSensorObject> source, const sensors_event_t &e);
private:
	DynamicSensorManager *manager;
};
#endif /* ST_DYNAMICSENSOR_PROXY_H */
