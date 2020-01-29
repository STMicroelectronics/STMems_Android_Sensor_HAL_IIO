Index
=====
	* Introduction
	* Software architecture and Integration details
	* STM proprietary libraries
	* More information
	* Copyright


Introduction
=========
The STM Android sensor Hardware Abstraction Layer (*HAL*) defines a standard interface for STM sensors allowing Android to be agnostic about [lower-level driver implementations](https://github.com/STMicroelectronics/STMems_Linux_IIO_drivers/tree/linux-4.4.y-gh) . The HAL library is packaged into modules (.so) file and loaded by the Android system at the appropriate time. For more information see [AOSP HAL Interface](https://source.android.com/devices/sensors/hal-interface.html)

STM Sensor HAL is leaning on [Linux IIO framework](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/iio) to gather data from sensor device drivers and to forward samples to the Android Framework

Currently supported sensors are:

### Inertial Module Unit (IMU):

> LSM330, LSM330DLC, LSM6DS3, LSM6DS3H, LSM6DSM, LSM6DSL, LSM6DS0, LSM9DS0, LSM9DS1, LSM330D, LSM330DL, ISM330DLC, LSM6DSO, ASM330LHH, LSM6DSR, LSM6DSO32

### eCompass:

> LSM303AGR, LSM303AH, LSM303DLHC, LSM303DLH, LSM303DLM, ISM303DAC

### Accelerometer:

> LIS2DS12, LIS2HH12, LIS3DH, LIS3DHH, LIS2DW12, LIS331DLH, LIS2DG, LIS2DH, LIS2DH12, IIS2DH, IIS3DHHC

### Gyroscope:

> L3GD20, L3GD20H, L3G4200D

### Magnetometer:

> LIS3MDL, LIS2MDL, IIS2MDC

### Pressure and Temperature:

> LPS22HB, LPS22HD, LPS25H, LPS331AP, LPS33HW, LPS35HW, LPS22HH, LPS27HHW

### Humidity and Temperature:

> HTS221

Software architecture and Integration details
=============

STM Sensor HAL is written in *C++* language using object-oriented design. For each hw sensor there is a custom class file
(*Accelerometer.cpp*, *Magnetometer.cpp*, *Gyroscope.cpp*, *Pressure.cpp*, *Temp.cpp* and *RHumidity.cpp*) which extends the common base class (*SensorBase.cpp*).

Copy the HAL source code into *<AOSP_DIR\>/hardware/STMicroelectronics/SensorHAL_IIO* folder. During building process Android will include automatically the SensorHAL Android.mk.
In *<AOSP_DIR\>/device/<vendor\>/<board\>/device.mk* add package build information:

	PRODUCT_PACKAGES += sensors.{TARGET_BOARD_PLATFORM}

	Note: device.mk can not read $(TARGET_BOARD_PLATFORM) variable, read and replace the value from your BoardConfig.mk (e.g. PRODUCT_PACKAGES += sensors.msm8974 for Nexus 5)

To compile the SensorHAL_IIO just build AOSP source code from *$TOP* folder

	$ cd <AOSP_DIR>
	$ source build/envsetup.sh
	$ lunch <select target platform>
	$ make V=99

The compiled library will be placed in *<AOSP_DIR\>/out/target/product/<board\>/system/vendor/lib/hw/sensor.{TARGET_BOARD_PLATFORM}.so*

To configure sensor the Sensor HAL IIO use mm utility from HAL root folder (only for Android version up to M included)

	$mm sensors-defconfig       (default configuration)
or

	$mm sensors-menuconfig

otherwise after the initial build of Android and SensorHAL, move to SensorHAL path and load initial Android settings

	"source android_data_config"

than run make command with one of the following options:

	"make sensors-defconfig"	Set to default configuration

	"make sensors-menuconfig"	Text based color menus, radiolists & dialogs.

	"make sensors-cleanconf"	Delete hal_config, hal_config.old and configuration.h files.


For more information on compiling an Android project, please consult the [AOSP website](https://source.android.com/source/requirements.html)



STM proprietary libraries
================

STM proprietary libraries are used to define composite sensors based on hardware (accelerometer, gyroscope, magnetometer) or to provide sensor calibration

### SENSOR_FUSION:
> The STM Sensor Fusion library is a complete 9-axis/6-axis solution which combines the measurements from a 3-axis gyroscope, a 3-axis magnetometer and a 3-axis accelerometer to provide a robust absolute orientation vector and game orientation vector

### GEOMAG_FUSION:
> The STM GeoMag Fusion library is a complete 6-axis solution which combines the measurements from a 3-axis magnetometer and a 3-axis accelerometer to provide a robust geomagnetic orientation vector

### GBIAS:
> The STM Gbias Calibration library provides an efficient gyroscope bias runtime compensation

### MAGCALIB:
> The STM Magnetometer Calibration library provides an accurate magnetometer Hard Iron (HI) and Soft Iron (SI) runtime compensation

### ACC_CALIB:
> The STM Accelerometer Calibration library provide an afficient accelerometer offset runtime compensation

To enable STM proprietary libraries please use mm utility

	$mm sensors-menuconfig

The release of STM proprietary libraries is subject to signature of a License User Agreement (LUA); please contact an STMicroelectronics sales office and representatives for further information.


Copyright
========
Copyright (C) 2019 STMicroelectronics

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
