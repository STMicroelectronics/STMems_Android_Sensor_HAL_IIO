/* IIO - useful set of util functionality
 *
 * Copyright (c) 2008 Jonathan Cameron
 * Modified by Denis Ciocca <denis.ciocca@st.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <utils/Log.h>

#include "iio_utils.h"
#include "common_data.h"

#define FORMAT_SCAN_ELEMENTS_DIR		"%s/scan_elements"
#define FORMAT_TYPE_FILE			"%s_type"
#define FORMAT_EVENT_ELEMENT_DIR		"%s/events"
#define SELFTEST_POSITIVE_RESULT		"pass"
#define SELFTEST_NEGATIVE_RESULT		"fail"

const char *iio_dir = "/sys/bus/iio/devices/";
const char *iio_sampling_frequency_available_filename = "sampling_frequency_available";
const char *iio_sampling_frequency_filename = "sampling_frequency";
const char *iio_max_delivery_rate_filename = "max_delivery_rate";
const char *iio_hw_fifo_enabled = "hwfifo_enabled";
const char *iio_hw_fifo_length = "hwfifo_watermark_max";
const char *iio_hw_fifo_watermark = "hwfifo_watermark";
const char *iio_hw_fifo_flush = "hwfifo_flush";
const char *iio_buffer_enable = "buffer/enable";
const char *iio_buffer_length = "buffer/length";
const char *iio_selftest_available_filename = "selftest_available";
const char *iio_selftest_filename = "selftest";
const char *iio_injection_mode_enable = "injection_mode";
const char *iio_injection_sensors_filename = "injection_sensors";

static int write_sysfs_int(char *filename, char *basedir, int val);
static int write_sysfs_int_and_verify(char *filename, char *basedir, int val);
static int write_sysfs_float_and_verify(char *filename, char *basedir, float val);
static int read_sysfs_posint(char *filename, char *basedir);
static int read_sysfs_string(char *filename, char *basedir, char *str);
static int write_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len);
static int write_sysfs_string(char *filename, char *basedir, char *val);


static inline int check_syfs_file(const char *filename, const char *basedir)
{
	size_t stringlen = strlen(basedir) + strlen(filename) + 2;
	char fpath[PATH_MAX];
	struct stat info;

	snprintf(fpath, stringlen, "%s/%s", basedir, filename);
	return stat((char *)fpath, &info);
}

/**
 * iio_utils_break_up_name() - Extract generic name from full channel name
 * @full_name: the full channel name.
 * @generic_name: the output generic channel name.
 **/
static inline int iio_utils_break_up_name(const char *full_name, char **generic_name)
{
	char *w, *r, *current, *working;

	current = strdup(full_name);
	working = strtok(current, "_\0");
	w = working;
	r = working;

	while (*r != '\0') {
		if (!isdigit(*r)) {
			*w = *r;
			w++;
		}
		r++;
	}

	*w = '\0';
	*generic_name = strdup(working);
	free(current);

	return 0;
}

/**
 * iio_utils_get_type() - Find and process _type attribute data
 * @is_signed: output whether channel is signed.
 * @bytes: output how many bytes the channel storage occupies.
 * @bits_used: number of bits of data.
 * @shift: shift value to get right data.
 * @mask: output a bit mask for the raw data.
 * @be: big endian.
 * @device_dir: the iio device directory.
 * @name: the channel name.
 * @generic_name: the channel type name.
 **/
static inline int iio_utils_get_type(unsigned *is_signed, unsigned *bytes,
	unsigned *bits_used, unsigned *shift, uint64_t *mask, unsigned *be,
			const char *device_dir, const char *name, const char *generic_name)
{
	DIR *dp;
	int ret;
	FILE *sysfsfp;
	unsigned padint;
	const struct dirent *ent;
	char signchar, endianchar;
	char *scan_el_dir, *builtname, *builtname_generic, *filename = 0;

	ret = asprintf(&scan_el_dir, FORMAT_SCAN_ELEMENTS_DIR, device_dir);
	if (ret < 0)
		return -ENOMEM;

	ret = asprintf(&builtname, FORMAT_TYPE_FILE, name);
	if (ret < 0) {
		ret = -ENOMEM;
		goto error_free_scan_el_dir;
	}

	ret = asprintf(&builtname_generic, FORMAT_TYPE_FILE, generic_name);
	if (ret < 0) {
		ret = -ENOMEM;
		goto error_free_builtname;
	}

	dp = opendir(scan_el_dir);
	if (dp == NULL) {
		ret = -errno;
		goto error_free_builtname_generic;
	}

	while (ent = readdir(dp), ent != NULL)
		/*
		 * Do we allow devices to override a generic name with
		 * a specific one?
		 */
		if ((strcmp(builtname, ent->d_name) == 0) ||
				(strcmp(builtname_generic, ent->d_name) == 0)) {
			ret = asprintf(&filename, "%s/%s", scan_el_dir, ent->d_name);
			if (ret < 0) {
				ret = -ENOMEM;
				goto error_closedir;
			}

			sysfsfp = fopen(filename, "r");
			if (sysfsfp == NULL) {
				printf("failed to open %s\n", filename);
				ret = -errno;
				goto error_free_filename;
			}

			ret = fscanf(sysfsfp, "%ce:%c%u/%u>>%u", &endianchar,
					&signchar, bits_used, &padint, shift);
			if (ret < 0) {
				ret = -errno;
				goto error_close_sysfsfp;
			}

			*be = (endianchar == 'b');
			*bytes = padint / 8;
			if (*bits_used == 64)
				*mask = ~0;
			else
				*mask = (1ULL << *bits_used) - 1;

			if (signchar == 's')
				*is_signed = 1;
			else
				*is_signed = 0;

			fclose(sysfsfp);
			sysfsfp = 0;

			free(filename);
			filename = 0;
		}

error_close_sysfsfp:
	if (sysfsfp)
		fclose(sysfsfp);
error_free_filename:
	if (filename)
		free(filename);
error_closedir:
	closedir(dp);
error_free_builtname_generic:
	free(builtname_generic);
error_free_builtname:
	free(builtname);
error_free_scan_el_dir:
	free(scan_el_dir);
	return ret;
}

/*
 * iio_utils_get_param_float() - Get float parameter from generic sysfs file
 * @output: read value.
 * @param_name: sysfs filename to read.
 * @device_dir: IIO device sysfs directory.
 */
static inline int iio_utils_get_param_float(float *output, const char *param_name,
		const char *device_dir, const char *name, const char *generic_name)
{
	DIR *dp;
	int ret;
	FILE *sysfsfp;
	char *filename = NULL;
	const struct dirent *ent;
	char *builtname, *builtname_generic;

	ret = asprintf(&builtname, "%s_%s", name, param_name);
	if (ret < 0)
		return -ENOMEM;

	ret = asprintf(&builtname_generic, "%s_%s", generic_name, param_name);
	if (ret < 0) {
		ret = -ENOMEM;
		goto error_free_builtname;
	}

	dp = opendir(device_dir);
	if (dp == NULL) {
		ret = -errno;
		goto error_free_builtname_generic;
	}

	while (ent = readdir(dp), ent != NULL) {
		if ((strcmp(builtname, ent->d_name) == 0) ||
					(strcmp(builtname_generic, ent->d_name) == 0)) {
			ret = asprintf(&filename, "%s/%s", device_dir, ent->d_name);
			if (ret < 0) {
				ret = -ENOMEM;
				goto error_closedir;
			}

			sysfsfp = fopen(filename, "r");
			if (!sysfsfp) {
				ret = -errno;
				goto error_free_filename;
			}

			fscanf(sysfsfp, "%f", output);
			break;
		}
	}

error_free_filename:
	if (filename)
		free(filename);
error_closedir:
	closedir(dp);
error_free_builtname_generic:
	free(builtname_generic);
error_free_builtname:
	free(builtname);
	return ret;
}

/**
 * iio_utils_sort_channel_array_by_index() - Reorder so that the array is in index order
 * @ci_array: channel struct to reorder.
 * @cnt: number of channels available.
 **/
static inline void iio_utils_sort_channel_array_by_index(struct iio_channel_info **ci_array, int cnt)
{
	int x, y;
	struct iio_channel_info temp;

	for (x = 0; x < cnt; x++) {
		for (y = 0; y < (cnt - 1); y++) {
			if ((*ci_array)[y].index > (*ci_array)[y+1].index) {
				temp = (*ci_array)[y + 1];
				(*ci_array)[y + 1] = (*ci_array)[y];
				(*ci_array)[y] = temp;
			}
		}
	}
}

static int iio_utils_enable_event_channels(const char *device_dir, bool enable)
{
	char *event_el_dir, *filename;
	const struct dirent *ent;
	FILE *sysfsfp;
	int err = 0;
	DIR *dp;

	err = asprintf(&event_el_dir, FORMAT_EVENT_ELEMENT_DIR, device_dir);
	if (err < 0)
		return -ENOMEM;

	dp = opendir(event_el_dir);
	if (!dp) {
		err = (errno == ENOENT) ? 0 : -errno;
		free(event_el_dir);
		return err;
	}

	while (ent = readdir(dp), ent != NULL) {
		if (!strcmp(ent->d_name + strlen(ent->d_name) - strlen("_en"), "_en")) {
			err = asprintf(&filename, "%s/%s", event_el_dir, ent->d_name);
			if (err < 0) {
				err = -ENOMEM;
				goto error_close_dir;
			}

			sysfsfp = fopen(filename, "r+");
			if (!sysfsfp) {
				err = -errno;
				free(filename);
				goto error_close_dir;
			}

			fprintf(sysfsfp, "%d", enable);
			fclose(sysfsfp);
			free(filename);
		}
	}

error_close_dir:
	closedir(dp);
	free(event_el_dir);

	return err;
}

int iio_utils_enable_sensor(const char *device_dir, bool enable)
{
	int err;

	err = check_syfs_file(iio_buffer_enable, device_dir);
	if (!err) {
		err = write_sysfs_int_and_verify((char *)iio_buffer_enable,
						 (char *)device_dir, (int)enable);
		if (err < 0)
			return err;
	} else if (errno != ENOENT) {
		return err;
	}

	return iio_utils_enable_event_channels(device_dir, enable);
}

int iio_utils_support_injection_mode(const char *device_dir)
{
	int err;
	char injectors[30];

	err = read_sysfs_posint((char *)iio_injection_mode_enable, (char *)device_dir);
	if (err < 0) {
		err = read_sysfs_string((char *)iio_injection_sensors_filename, (char *)device_dir, injectors);
		if (err < 0)
			return err;

		return 1;
	}

	return 0;
}

int iio_utils_set_injection_mode(const char *device_dir, bool enable)
{
	return write_sysfs_int_and_verify((char *)iio_injection_mode_enable, (char *)device_dir, (int)enable);
}

int iio_utils_inject_data(const char *device_dir, uint8_t *data, int len, enum iio_chan_type device_type)
{
	char *injection_filename;

	switch (device_type) {
		case IIO_ACCEL:
			injection_filename = (char *)"in_accel_injection_raw";
			break;
		case IIO_MAGN:
			injection_filename = (char *)"in_magn_injection_raw";
			break;
		case IIO_ANGL_VEL:
			injection_filename = (char *)"in_anglvel_injection_raw";
			break;
		case IIO_PRESSURE:
			injection_filename = (char *)"in_press_injection_raw";
			break;
		default:
			return -EINVAL;
	}

	return write_sysfs_byte((char *)injection_filename, (char *)device_dir, data, len);
}

int iio_utils_get_hw_fifo_length(const char *device_dir)
{
	int err, len;

	len = read_sysfs_posint((char *)iio_hw_fifo_length, (char *)device_dir);
	if (len < 0)
		return 0;

	err = write_sysfs_int((char *)iio_buffer_length, (char *)device_dir, 2 * len);
	if (err < 0)
		return err;

	/* used for compatibility with old iio API */
	err = check_syfs_file(iio_hw_fifo_enabled, device_dir);
	if (err < 0 && errno == ENOENT)
		return len;

	err = write_sysfs_int((char *)iio_hw_fifo_enabled, (char *)device_dir, 1);
	if (err < 0) {
		ALOGE("Failed to enable hw fifo: %s/%s.", (char *)device_dir, (char *)iio_hw_fifo_enabled);
		return err;
	}

	return len;
}

int iio_utils_set_hw_fifo_watermark(const char *device_dir, unsigned int watermark)
{
	return write_sysfs_int((char *)iio_hw_fifo_watermark, (char *)device_dir, watermark);
}

int iio_utils_hw_fifo_flush(const char *device_dir)
{
	return write_sysfs_int((char *)iio_hw_fifo_flush, (char *)device_dir, 1);
}

int iio_utils_set_scale(const char *device_dir, float value, enum iio_chan_type device_type)
{
	int err;
	char *scale_file_name;

	switch (device_type) {
		case IIO_ACCEL:
			scale_file_name = (char *)"in_accel_x_scale";
			break;
		case IIO_MAGN:
			scale_file_name = (char *)"in_magn_x_scale";
			break;
		case IIO_ANGL_VEL:
			scale_file_name = (char *)"in_anglvel_x_scale";
			break;
		case IIO_PRESSURE:
			scale_file_name = (char *)"in_press_scale";
			break;
		default:
			return -EINVAL;
	}

	err = write_sysfs_float_and_verify(scale_file_name, (char *)device_dir, value);
	if (err < 0)
		return err;

	return 0;
}

int iio_utils_get_scale_available(const char *device_dir, struct iio_scale_available *sa, enum iio_chan_type device_type)
{
	int err;
	FILE *fp;
	char *tmp_name, *avl_name, *pch, *res, line[200];

	sa->num_available = 0;

	switch (device_type) {
		case IIO_ACCEL:
			avl_name = (char *)"in_accel_scale_available";
			break;
		case IIO_MAGN:
			avl_name = (char *)"in_magn_scale_available";
			break;
		case IIO_ANGL_VEL:
			avl_name = (char *)"in_anglvel_scale_available";
			break;
		case IIO_PRESSURE:
			avl_name = (char *)"in_press_scale_available";
			break;
		case IIO_HUMIDITYRELATIVE:
			avl_name = (char *)"in_humidityrelative_scale_available";
			break;
		case IIO_TEMP:
			avl_name = (char *)"in_temp_scale_available";
			break;
		default:
			return -EINVAL;
	}

	err = asprintf(&tmp_name, "%s/%s", device_dir, avl_name);
	if (err < 0)
		return err;

	fp = fopen(tmp_name, "r");
	if (fp == NULL) {
		err = 0;
		goto open_file_error;
	}

	res = fgets(line, sizeof(line), fp);
	if (res == NULL) {
		ALOGE("Scale available file format error: \"%s\".", tmp_name);
		err = -EINVAL;
		goto read_error;
	}

	pch = strtok(line," ");
	while (pch != NULL) {
		sa->values[sa->num_available] = atof(pch);
		pch = strtok(NULL, " ");
		sa->num_available++;

		if (sa->num_available >= IIO_UTILS_SCALE_AVAILABLE)
			break;
	}

read_error:
	fclose(fp);
open_file_error:
	free(tmp_name);
	return err < 0 ? err : 0;
}

int iio_utils_get_sampling_frequency_available(const char *device_dir,
					struct iio_sampling_frequency_available *sfa)
{
	int err;
	FILE *fp;
	char *tmp_name, *pch, *res, line[200];

	sfa->num_available = 0;

	err = asprintf(&tmp_name, "%s/%s", device_dir, iio_sampling_frequency_available_filename);
	if (err < 0)
		return err;

	fp = fopen(tmp_name, "r");
	if (fp == NULL) {
		ALOGE("Failed to open sampling frequency available file: \"%s\".", tmp_name);
		err = -errno;
		goto tmp_name_free;
	}

	res = fgets(line, sizeof(line), fp);
	if (res == NULL) {
		ALOGE("Sampling frequency file format error: \"%s\".", tmp_name);
		err = -EINVAL;
		goto close_file;
	}

	pch = strtok(line," ,.");
	while (pch != NULL) {
		sfa->hz[sfa->num_available] = atoi(pch);
		pch = strtok(NULL, " ,.");
		sfa->num_available++;

		if (sfa->num_available >= IIO_UTILS_MAX_SAMP_FREQ_AVAILABLE)
			break;
	}

close_file:
	fclose(fp);
tmp_name_free:
	free(tmp_name);
	return err < 0 ? err : 0;
}

int iio_utils_set_sampling_frequency(const char *device_dir, unsigned int frequency)
{
	int err;

	err = check_syfs_file(iio_sampling_frequency_filename, device_dir);
	if (err < 0 && errno == ENOENT)
		return 0;

	return write_sysfs_int_and_verify((char *)iio_sampling_frequency_filename, (char *)device_dir, frequency);
}

int iio_utils_set_max_delivery_rate(const char *device_dir, unsigned int deelay)
{
	int err;

	err = check_syfs_file(iio_max_delivery_rate_filename, device_dir);
	if (err < 0 && errno == ENOENT)
		return 0;

	return write_sysfs_int_and_verify((char *)iio_max_delivery_rate_filename, (char *)device_dir, deelay);
}

int iio_utils_build_channel_array(const char *device_dir,
				struct iio_channel_info **ci_array, int *counter)
{
	DIR *dp;
	FILE *sysfsfp;
	int ret, count = 0, i;
	const struct dirent *ent;
	char *scan_el_dir, *filename;
	struct iio_channel_info *current;

	*counter = 0;
	ret = asprintf(&scan_el_dir, FORMAT_SCAN_ELEMENTS_DIR, device_dir);
	if (ret < 0)
		return -ENOMEM;

	dp = opendir(scan_el_dir);
	if (dp == NULL) {
		ret = -errno;
		goto error_free_name;
	}

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name + strlen(ent->d_name)
						- strlen("_en"), "_en") == 0) {
			ret = asprintf(&filename,
					"%s/%s", scan_el_dir, ent->d_name);
			if (ret < 0) {
				ret = -ENOMEM;
				goto error_close_dir;
			}

			sysfsfp = fopen(filename, "r+");
			if (sysfsfp == NULL) {
				ret = -errno;
				free(filename);
				goto error_close_dir;
			}

			fprintf(sysfsfp, "%d", 1);
			rewind(sysfsfp);

			fscanf(sysfsfp, "%u", &ret);
			if (ret == 1)
				(*counter)++;

			fclose(sysfsfp);
			free(filename);
		}
	}

	*ci_array = malloc(sizeof(**ci_array) * (*counter));
	if (*ci_array == NULL) {
		ret = -ENOMEM;
		goto error_close_dir;
	}

	rewinddir(dp);

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name + strlen(ent->d_name)
						- strlen("_en"), "_en") == 0) {
			current = &(*ci_array)[count++];
			ret = asprintf(&filename, "%s/%s", scan_el_dir, ent->d_name);
			if (ret < 0) {
				ret = -ENOMEM;
				/* decrement count to avoid freeing name */
				count--;
				goto error_cleanup_array;
			}

			sysfsfp = fopen(filename, "r");
			if (sysfsfp == NULL) {
				free(filename);
				ret = -errno;
				goto error_cleanup_array;
			}

			fscanf(sysfsfp, "%u", &current->enabled);
			fclose(sysfsfp);

			if (!current->enabled) {
				free(filename);
				count--;
				continue;
			}

			current->scale = 1.0f;
			current->offset = 0.0f;
			current->name = strndup(ent->d_name,
					strlen(ent->d_name) - strlen("_en"));
			if (current->name == NULL) {
				free(filename);
				ret = -ENOMEM;
				goto error_cleanup_array;
			}

			/* Get the generic and specific name elements */
			ret = iio_utils_break_up_name(current->name,
							&current->generic_name);
			if (ret) {
				free(filename);
				goto error_cleanup_array;
			}

			ret = asprintf(&filename, "%s/%s_index",
						scan_el_dir, current->name);
			if (ret < 0) {
				free(filename);
				ret = -ENOMEM;
				goto error_cleanup_array;
			}

			sysfsfp = fopen(filename, "r");
			fscanf(sysfsfp, "%u", &current->index);
			fclose(sysfsfp);
			free(filename);

			/* Find the scale */
			ret = iio_utils_get_param_float(&current->scale, "scale",
				device_dir, current->name, current->generic_name);
			if (ret < 0)
				goto error_cleanup_array;

			ret = iio_utils_get_param_float(&current->offset,
					"offset", device_dir, current->name,
							current->generic_name);
			if (ret < 0)
				goto error_cleanup_array;

			ret = iio_utils_get_type(&current->is_signed,
						&current->bytes,
						&current->bits_used,
						&current->shift,
						&current->mask,
						&current->be,
						device_dir,
						current->name,
						current->generic_name);
		}
	}

	closedir(dp);
	iio_utils_sort_channel_array_by_index(ci_array, *counter);

	return 1;

error_cleanup_array:
	for (i = count - 1; i >= 0; i--)
		free((*ci_array)[i].name);

	free(*ci_array);
error_close_dir:
	closedir(dp);
error_free_name:
	free(scan_el_dir);
	return ret;
}

int iio_utils_get_devices_name(struct iio_device devices[], unsigned int max_list)
{
	DIR *dp;
	FILE *nameFile;
	size_t string_len;
	const struct dirent *ent;
	int err, number, numstrlen;
	unsigned int device_num = 0;
	char thisname[IIO_MAX_NAME_LENGTH], *filename;

	dp = opendir(iio_dir);
	if (dp == NULL)
		return -ENODEV;

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name, ".") != 0 &&
				strcmp(ent->d_name, "..") != 0 &&
				strlen(ent->d_name) > strlen("iio:device") &&
				strncmp(ent->d_name, "iio:device", strlen("iio:device")) == 0) {

			sscanf(ent->d_name + strlen("iio:device"), "%d", &number);
			numstrlen = strlen(ent->d_name) - strlen("iio:device");

			/* verify the next character is not a colon */
			if (strncmp(ent->d_name + strlen("iio:device") + numstrlen, ":", 1) != 0) {
				string_len = strlen(iio_dir) + strlen("iio:device") + numstrlen + 6;

				filename = (char *)malloc(string_len * sizeof(char));
				if (filename == NULL) {
					closedir(dp);
					return -ENOMEM;
				}

				err = snprintf(filename, string_len, "%s%s%d/name", iio_dir, "iio:device", number);
				if (err < 0) {
					free(filename);
					closedir(dp);
					return -EINVAL;
				}

				nameFile = fopen(filename, "r");
				if (!nameFile) {
					free(filename);
					continue;
				}

				free(filename);
				fscanf(nameFile, "%s", thisname);
				fclose(nameFile);

				memcpy(devices[device_num].name, thisname, strlen(thisname));
				devices[device_num].name[strlen(thisname)] = '\0';

				devices[device_num].dev_num = number;
				device_num++;

				if (device_num >= max_list) {
					closedir(dp);
					return (int)device_num;
				}
			}
		}
	}
	closedir(dp);

	return (int)device_num;
}

static inline int _write_sysfs_int(char *filename, char *basedir, int val, int verify)
{
	char *temp;
	FILE *sysfsfp;
	size_t stringlen;
	int ret = 0, test;

	stringlen = strlen(basedir) + strlen(filename) + 2;

	temp = (char *)malloc(stringlen * sizeof(char));
	if (temp == NULL)
		return -ENOMEM;

	ret = snprintf(temp, stringlen, "%s/%s", basedir, filename);
	if (ret < 0) {
		ret = -EINVAL;
		goto error_free;
	}

	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	fprintf(sysfsfp, "%d", val);
	fclose(sysfsfp);

	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			ret = -errno;
			goto error_free;
		}
		fscanf(sysfsfp, "%d", &test);
		fclose(sysfsfp);

		if (test != val) {
			ALOGE("Failed to write \"%d\" to \"%s/%s\" file.", val, basedir, filename);
			ret = -1;
		}
	}

error_free:
	free(temp);
	return ret;
}

int write_sysfs_int(char *filename, char *basedir, int val)
{
	return _write_sysfs_int(filename, basedir, val, 0);
}

int write_sysfs_int_and_verify(char *filename, char *basedir, int val)
{
	return _write_sysfs_int(filename, basedir, val, 1);
}

static inline int _write_sysfs_float(char *filename, char *basedir, float val, int verify)
{
	char *temp;
	float test;
	int ret = 0;
	FILE *sysfsfp;
	size_t stringlen;

	stringlen = strlen(basedir) + strlen(filename) + 2;

	temp = (char *)malloc(stringlen * sizeof(char));
	if (temp == NULL)
		return -ENOMEM;

	ret = snprintf(temp, stringlen, "%s/%s", basedir, filename);
	if (ret < 0) {
		ret = -EINVAL;
		goto error_free;
	}

	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	fprintf(sysfsfp, "%f", val);
	fclose(sysfsfp);

	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			ret = -errno;
			goto error_free;
		}

		fscanf(sysfsfp, "%f", &test);
		fclose(sysfsfp);

		if (test != val) {
			ALOGE("Failed to write \"%f\" to \"%s/%s\" file.",
							val, basedir, filename);
			ret = -1;
		}
	}

error_free:
	free(temp);
	return ret;
}

int write_sysfs_float_and_verify(char *filename, char *basedir, float val)
{
	return _write_sysfs_float(filename, basedir, val, 1);
}

static int read_sysfs_posint(char *filename, char *basedir)
{
	int ret;
	char *temp;
	FILE  *sysfsfp;
	size_t stringlen;

	stringlen = strlen(basedir) + strlen(filename) + 2;

	temp = (char *)malloc(stringlen * sizeof(char));
	if (temp == NULL)
		return -ENOMEM;

	ret = snprintf(temp, stringlen, "%s/%s", basedir, filename);
	if (ret < 0) {
		ret = -EINVAL;
		goto error_free;
	}

	sysfsfp = fopen(temp, "r");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	fscanf(sysfsfp, "%d\n", &ret);
	fclose(sysfsfp);

error_free:
	free(temp);
	return ret;
}

int iio_utils_get_selftest_available(const char *device_dir, char list[][20])
{
	FILE *fp;
	int err, elements = 0;
	char *tmp_name, *pch, *res, line[200];

	err = asprintf(&tmp_name, "%s/%s", device_dir, iio_selftest_available_filename);
	if (err < 0)
		return err;

	fp = fopen(tmp_name, "r");
	if (fp == NULL) {
		err = -errno;
		goto tmp_name_free;
	}

	res = fgets(line, sizeof(line), fp);
	if (res == NULL) {
		err = -EINVAL;
		goto close_file;
	}

	pch = strtok(line," ,.");
	while (pch != NULL) {
		memcpy(list[elements], pch, strlen(pch) + 1);
		pch = strtok(NULL, " ,.");
		elements++;
	}

close_file:
	fclose(fp);
tmp_name_free:
	free(tmp_name);
	return err < 0 ? err : elements;
}

int iio_utils_execute_selftest(const char *device_dir, char *mode)
{
	int err;
	char result[20];

	err = write_sysfs_string((char *)iio_selftest_filename, (char *)device_dir, mode);
	if (err < 0)
		return err;

	err = read_sysfs_string((char *)iio_selftest_filename, (char *)device_dir, result);
	if (err < 0)
		return err;

	err = strncmp(result, SELFTEST_POSITIVE_RESULT, strlen(SELFTEST_POSITIVE_RESULT));
	if (err == 0)
		return 1;

	err = strncmp(result, SELFTEST_NEGATIVE_RESULT, strlen(SELFTEST_NEGATIVE_RESULT));
	if (err == 0)
		return 0;

	return -EINVAL;
}

static int _write_sysfs_string(char *filename, char *basedir, char *val, int verify)
{
	char *temp;
	int ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	sprintf(temp, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -EIO;
		goto error_free;
	}

	fprintf(sysfsfp, "%s", val);
	fclose(sysfsfp);
	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			ret = -EIO;
			goto error_free;
		}

		fscanf(sysfsfp, "%s", temp);
		fclose(sysfsfp);
		if (strcmp(temp, val) != 0)
			ret = -1;
	}

error_free:
	free(temp);
	return ret;
}

int write_sysfs_string(char *filename, char *basedir, char *val)
{
	return _write_sysfs_string(filename, basedir, val, 0);
}

static int read_sysfs_string(char *filename, char *basedir, char *str)
{
	char *temp;
	int ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	snprintf(temp, strlen(basedir) + strlen(filename) + 2, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "r");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	ret = fscanf(sysfsfp, "%s\n", str);
	if (ret < 0)
		ret = -errno;

	fclose(sysfsfp);

error_free:
	free(temp);
	return ret;
}

static int write_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len)
{
	char *temp;
	int ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	snprintf(temp, strlen(basedir) + strlen(filename) + 2, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	ret = fwrite(data, len, 1, sysfsfp);
	fclose(sysfsfp);

error_free:
	free(temp);
	return ret <= 0 ? -errno : (int)len;
}


#if 0
static int read_sysfs_float(char *filename, char *basedir, float *val);
static int write_sysfs_string_and_verify(char *filename, char *basedir, char *val);
static int write_sysfs_float(char *filename, char *basedir, float val);
static int write_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len);
static int read_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len);
static int write_sysfs_ulong_and_verify(char *filename, char *basedir, unsigned long val);

static int read_sysfs_float(char *filename, char *basedir, float *val)
{
	char *temp;
	float ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	sprintf(temp, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "r");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	fscanf(sysfsfp, "%f\n", val);
	fclose(sysfsfp);

error_free:
	free(temp);
	return ret;
}

int write_sysfs_float(char *filename, char *basedir, float val)
{
	return _write_sysfs_float(filename, basedir, val, 0);
}

static int read_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len)
{
	char *temp;
	int ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	sprintf(temp, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "r");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	ret = fread(data, len, 1, sysfsfp);
	fclose(sysfsfp);

error_free:
	free(temp);
	return ret <= 0 ? -errno : (int)len;
}

static int write_sysfs_byte(char *filename, char *basedir, uint8_t *data, size_t len)
{
	char *temp;
	int ret = 0;
	FILE  *sysfsfp;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	sprintf(temp, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	ret = fwrite(data, len, 1, sysfsfp);
	fclose(sysfsfp);

error_free:
	free(temp);
	return ret <= 0 ? -errno : (int)len;
}

int write_sysfs_string_and_verify(char *filename, char *basedir, char *val)
{
	return _write_sysfs_string(filename, basedir, val, 1);
}

static inline int _write_sysfs_ulong_and_verify(char *filename, char *basedir,
						unsigned long val, int verify)
{
	char *temp;
	int ret = 0;
	FILE *sysfsfp;
	unsigned long test;

	temp = (char *)malloc(strlen(basedir) + strlen(filename) + 2);
	if (temp == NULL)
		return -ENOMEM;

	sprintf(temp, "%s/%s", basedir, filename);
	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}

	fprintf(sysfsfp, "%lu", val);
	fclose(sysfsfp);

	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			ret = -errno;
			goto error_free;
		}
		fscanf(sysfsfp, "%lu", &test);
		fclose(sysfsfp);

		if (test != val) {
			ALOGE("Failed to write \"%lu\" to \"%s/%s\" file.",
							val, basedir, filename);
			ret = -1;
		}
	}

error_free:
	free(temp);
	return ret;
}

int write_sysfs_ulong_and_verify(char *filename, char *basedir, unsigned long val)
{
	return _write_sysfs_ulong_and_verify(filename, basedir, val, 1);
}

static int find_type_by_name(char *name, const char *type)
{
	DIR *dp;
	FILE *nameFile;
	int number, numstrlen;
	const struct dirent *ent;
	char thisname[IIO_MAX_NAME_LENGTH], *filename;

	dp = opendir(iio_dir);
	if (dp == NULL)
		return -ENODEV;

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name, ".") != 0 &&
				strcmp(ent->d_name, "..") != 0 &&
				strlen(ent->d_name) > strlen(type) &&
				strncmp(ent->d_name, type, strlen(type)) == 0) {

			sscanf(ent->d_name + strlen(type), "%d", &number);
			numstrlen = strlen(ent->d_name) - strlen("iio:device");

			/* verify the next character is not a colon */
			if (strncmp(ent->d_name + strlen(type) + numstrlen,
								":", 1) != 0) {
				filename = (char *)malloc(strlen(iio_dir)
						+ strlen(type)
						+ numstrlen
						+ 6);
				if (filename == NULL) {
					closedir(dp);
					return -ENOMEM;
				}

				sprintf(filename, "%s%s%d/name", iio_dir,
								type, number);
				nameFile = fopen(filename, "r");
				if (!nameFile) {
					free(filename);
					continue;
				}

				free(filename);
				fscanf(nameFile, "%s", thisname);
				fclose(nameFile);

				if (strcmp(name, thisname) == 0) {
					closedir(dp);
					return number;
				}
			}
		}
	}
	closedir(dp);

	return -ENODEV;
}
#endif
