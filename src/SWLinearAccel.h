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

#ifndef ST_SWLINEAR_ACCEL_H
#define ST_SWLINEAR_ACCEL_H

#include "SWSensorBase.h"

class SWLinearAccel : public SWSensorBaseWithPollrate {
public:
	SWLinearAccel(const char *name, int handle);
	~SWLinearAccel();

	virtual int CustomInit();
	virtual void ProcessData(SensorBaseData *data);
};

#endif /* ST_SWLINEAR_ACCEL_H */
