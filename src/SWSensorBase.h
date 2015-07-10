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

#ifndef ST_SWSENSOR_BASE_H
#define ST_SWSENSOR_BASE_H

#include <string.h>
#include <poll.h>

#include "SensorBase.h"

#define ST_SENSOR_FUSION_RESOLUTION(maxRange)		(maxRange / (1 << 24))
#define ST_SW_SENSOR_BASE_MAX_FLUSH_EVENTS		(10)

class SWSensorBase;

/*
 * class SWSensorBase
 */
class SWSensorBase : public SensorBase {
protected:
	bool dependency_resolution;
	bool dependency_range;
	bool dependency_delay;
	bool dependency_name;

	DependencyID id_sensor_trigger;
	int trigger_write_pipe_fd, trigger_read_pipe_fd;
	struct pollfd android_pollfd;

	SensorBaseData *sensors_tmp_data;

public:
	SWSensorBase(const char *name, int handle, int sensor_type,
			bool use_dependency_resolution, bool use_dependency_range,
			bool use_dependency_delay, bool use_dependency_name);
	virtual ~SWSensorBase();

	virtual int Enable(int handle, bool enable, bool lock_en_mutex);

	virtual int AddSensorDependency(SensorBase *p);
	virtual void RemoveSensorDependency(SensorBase *p);

	virtual void ReceiveDataFromDependency(int handle, SensorBaseData *data);

	virtual int FlushData(int handle, bool lock_en_mutex);
	virtual void ProcessFlushData(int handle, int64_t timestamp);

	virtual void ThreadDataTask();

	bool hasDataChannels() { return true; }
};


/*
 * class SWSensorBaseWithPollrate
 */
class SWSensorBaseWithPollrate : public SWSensorBase {
public:
	SWSensorBaseWithPollrate(const char *name, int handle, int sensor_type,
			bool use_dependency_resolution, bool use_dependency_range,
			bool use_dependency_delay, bool use_dependency_name);
	virtual ~SWSensorBaseWithPollrate();

	virtual int SetDelay(int handle, int64_t period_ns, int64_t timeout, bool lock_en_mutex);
	virtual int FlushData(int handle, bool lock_en_mutex);
	virtual void WriteDataToPipe(int64_t hw_pollrate);
};

#endif /* ST_SWSENSOR_BASE_H */
