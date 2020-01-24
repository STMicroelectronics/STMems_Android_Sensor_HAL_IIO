/*
 * STMicroelectronics device iio utils core for SensorHAL
 *
 * Copyright 2019 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <iostream>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "utils.h"

#define SELFTEST_POSITIVE_RESULT                "pass"
#define SELFTEST_NEGATIVE_RESULT                "fail"

/* IIO SYSFS interface */
static const char *device_iio_dir = "/sys/bus/iio/devices/";
static const char *device_iio_sfa_filename = "sampling_frequency_available";
static const char *device_iio_sf_filename = "sampling_frequency";
static const char *device_iio_hw_fifo_enabled = "hwfifo_enabled";
static const char *device_iio_hw_fifo_length = "hwfifo_watermark_max";
static const char *device_iio_hw_fifo_watermark = "hwfifo_watermark";
static const char *device_iio_max_delivery_rate_filename = "max_delivery_rate";
static const char *device_iio_hw_fifo_flush = "hwfifo_flush";
static const char *device_iio_buffer_enable = "buffer/enable";
static const char *device_iio_event_dir = "%s/events";
static const char *device_iio_buffer_length = "buffer/length";
static const char *device_iio_device_name = "iio:device";
static const char *device_iio_injection_mode_enable = "injection_mode";
static const char *device_iio_injection_sensors_filename = "injection_sensors";
static const char *device_iio_scan_elements_en = "_en";
static const char *device_iio_selftest_available_filename = "selftest_available";
static const char *device_iio_selftest_filename = "selftest";

int device_iio_utils::sysfs_opendir(const char *name, DIR **dp)
{
	struct stat sb;
	DIR *tmp;

	/*
	 * Check if path exists, if a component of path does not exist,
	 * or path is an empty string return ENOENT
	 * If path is not accessible return EACCES
	 */
	if (stat(name, &sb) == -1) {
		return -errno;
	}

	/* Check if directory */
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		return -EINVAL;
	}

	/* Open sysfs directory */
	tmp = opendir(name);
	if (tmp == NULL)
		return -errno;

	*dp = tmp;

	return 0;
}

int device_iio_utils::sysfs_write_int(char *file, int val)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%d", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_write_uint(char *file, unsigned int val)
{
	FILE *fp;

	fp = fopen(file, "r+");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%u", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_write_float(char *file, float val)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%f", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_write_str(char *file, char *str)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%s", str);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_read_int(char *file, int *val)
{
	FILE *fp;
	int ret;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -errno;

	ret = fscanf(fp, "%d\n", val);
	fclose(fp);

	return ret;
}

int device_iio_utils::sysfs_read_uint(char *file, unsigned int *val)
{
	FILE *fp;
	int ret;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -errno;

	ret = fscanf(fp, "%u\n", val);
	fclose(fp);

	return ret;
}

int device_iio_utils::sysfs_read_float(char *file, float *val)
{
	FILE *fp;
	int ret;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -errno;

	ret = fscanf(fp, "%f", val);
	fclose(fp);

	return ret;
}

int device_iio_utils::sysfs_read_str(char *file, char *str, int len)
{
	FILE *fp;
	int err = 0;

	fp = fopen(file, "r");
	if (fp == NULL)
		return -errno;

	if (fgets(str, len, fp) == NULL)
		err = -errno;

	fclose(fp);

	return err;
}

int device_iio_utils::check_file(char *filename)
{
	struct stat info;

	return stat(filename, &info);
}

int device_iio_utils::sysfs_enable_channels(const char *device_dir, bool enable)
{
	char dir[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char filename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	const struct dirent *ent;
	FILE *sysfsfp;
	DIR *dp;
	int ret = 0;

	/* check in scan_elements dir entry all enable file flag */
	if (strlen(device_dir) + strlen("scan_elements") + 1 >
	    DEVICE_IIO_MAX_FILENAME_LEN)
		return -ENOMEM;

	sprintf(dir, "%s/scan_elements", device_dir);
	ret = sysfs_opendir(dir, &dp);
	if (ret)
		return ret;

	while (ent = readdir(dp), ent != NULL) {
		if (strlen(dir) + strlen(ent->d_name) >
		    DEVICE_IIO_MAX_FILENAME_LEN)
		continue;

		if (!strcmp(ent->d_name + strlen(ent->d_name) - strlen("_en"),
			    "_en")) {
			sprintf(filename, "%s/%s", dir, ent->d_name);
			sysfsfp = fopen(filename, "r+");
			if (!sysfsfp) {
				ret = -errno;
				goto out;
			}

			fprintf(sysfsfp, "%d", enable);
			fclose(sysfsfp);
		}
	}

out:
	closedir(dp);

	return ret;
}

int device_iio_utils::get_device_by_name(const char *name)
{
	struct dirent *ent;
	int number, numstrlen;
	FILE *deviceFile;
	DIR *dp;
	char dname[DEVICE_IIO_MAX_NAME_LENGTH];
	char dfilename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	int ret;
	int fnamelen;

	ret = sysfs_opendir(device_iio_dir, &dp);
	if (ret)
		return ret;

	for (ent = readdir(dp); ent; ent = readdir(dp)) {
		if (strlen(ent->d_name) <= strlen(device_iio_device_name) ||
		    strstr(ent->d_name, "."))
			continue;

		if (strncmp(ent->d_name, device_iio_device_name,
			    strlen(device_iio_device_name)) == 0) {
			numstrlen = sscanf(ent->d_name +
					   strlen(device_iio_device_name),
					   "%d", &number);
			fnamelen = numstrlen + strlen(device_iio_dir) +
				   strlen(device_iio_device_name);
			if (fnamelen > DEVICE_IIO_MAX_FILENAME_LEN)
				continue;

			sprintf(dfilename,
				"%s%s%d/name",
				device_iio_dir,
				device_iio_device_name,
				number);
			deviceFile = fopen(dfilename, "r");
			if (!deviceFile)
				continue;

			ret = fscanf(deviceFile, "%s", dname);
			if (ret <= 0) {
				fclose(deviceFile);
				break;
			}

			if (strncmp(name, dname, strlen(dname)) == 0) {
				fclose(deviceFile);
				closedir(dp);
				return number;
			}

		fclose(deviceFile);
		}
	}

	closedir(dp);

	return -ENODEV;
}

int device_iio_utils::get_devices_name(struct device_iio_type_name devices[],
				       unsigned int max_list)
{
	DIR *dp;
	FILE *nameFile;
	size_t string_len, iio_len;
	const struct dirent *ent;
	int err, number, numstrlen;
	unsigned int device_num = 0;
	char thisname[DEVICE_IIO_MAX_FILENAME_LEN], *filename;

	err = sysfs_opendir(device_iio_dir, &dp);
	if (err)
		return err;

	iio_len = strlen("iio:device");
	while (ent = readdir(dp), ent != NULL) {
		if (!strstr(ent->d_name, ".") &&
		    (strlen(ent->d_name) > iio_len) &&
		    strncmp(ent->d_name, "iio:device", iio_len) == 0) {
			sscanf(ent->d_name + iio_len, "%d", &number);
			numstrlen = strlen(ent->d_name) - iio_len;

			if (strncmp(ent->d_name + iio_len + numstrlen, ":", 1) != 0) {
				string_len = strlen(device_iio_dir) + iio_len +
					     numstrlen + 6;

				filename = (char *)malloc(string_len);
				if (filename == NULL) {
					closedir(dp);
					return -ENOMEM;
				}

				err = snprintf(filename, string_len,
					       "%s%s%d/name", device_iio_dir,
					       "iio:device", number);
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

				memcpy(devices[device_num].name, thisname,
				       strlen(thisname));
				devices[device_num].name[strlen(thisname)] = '\0';

				devices[device_num].num = number;
				device_num++;

				if (device_num >= max_list)
					goto close_dp;
			}
		}
	}

close_dp:
	closedir(dp);

	return (int)device_num;
}

int device_iio_utils::scan_channel(const char *device_dir,
				   struct device_iio_info_channel **data,
				   int *counter)
{
	DIR *dp;
	int ret, i, x, y;
	unsigned int temp;
	const struct dirent *ent;
	char dname[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char dfilename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	struct device_iio_info_channel *current, ch_ord;
	const char *tmp;

	memset(dname, 0, DEVICE_IIO_MAX_FILENAME_LEN + 1);
	memset(dfilename, 0, DEVICE_IIO_MAX_FILENAME_LEN + 1);
	snprintf(dname, DEVICE_IIO_MAX_FILENAME_LEN, "%s/scan_elements",
		 device_dir);

	ret = sysfs_opendir(dname, &dp);
	if (ret)
		return ret;

	*counter = 0;

	/* Count how many channel info in scan_elements */
	for (ent = readdir(dp); ent; ent = readdir(dp))  {
		tmp = &ent->d_name[strlen(ent->d_name) -
		      strlen(device_iio_scan_elements_en)];
		if (strncmp(tmp, device_iio_scan_elements_en,
		            strlen(device_iio_scan_elements_en)) == 0) {
			if (strlen(dname) + strlen(ent->d_name) + 1 >
			    DEVICE_IIO_MAX_FILENAME_LEN) {
				ret = -ENOMEM;
				goto error_cleanup_array;
			}

			/* open all scan_element xxx_en files and enable it */
			sprintf(dfilename,  "%s/%s", dname, ent->d_name);
			sysfs_write_uint(dfilename, ENABLE_CHANNEL);

			/* Check for scan enabled */
			ret = sysfs_read_uint(dfilename, &temp);
			if (ret > 0 && temp == 1) {
				/* Allocate new channel and populate */
				*data = (struct device_iio_info_channel *)
					realloc(*data, ((*counter) + 1) * sizeof(device_iio_info_channel));
				if (!*data) {
					ret = -ENOMEM;
					goto error_cleanup_array;
				}

				current = &(*data)[*counter];
				current->enabled = temp;
				current->scale = 1.0f;
				current->offset = 0.0f;
				current->name = strndup(ent->d_name,
							strlen(ent->d_name) -
							strlen(device_iio_scan_elements_en));
				if (current->name == NULL) {
					ret = -ENOMEM;
					goto error_cleanup_array;
				}

				sprintf(dfilename,  "%s/%s_index", dname,
					current->name);
				sysfs_read_uint(dfilename, &current->index);

				sprintf(dfilename,  "%s/%s_scale", device_dir,
					current->name);
				ret = sysfs_read_float(dfilename, &current->scale);
				if (!ret)
					goto error_cleanup_array;

				sprintf(dfilename,  "%s/%s_offset", device_dir,
					current->name);
				ret = sysfs_read_float(dfilename,
						       &current->offset);
				if (!ret)
					goto error_cleanup_array;

				ret = get_type(current, device_dir,
					       current->name, "in");
				current->location = 0;
				(*counter)++;
			}
		}
	}

	closedir(dp);

	/* reorder index in channel array */
	for (x = 0; x < (*counter); x++) {
		for (y = 0; y < ((*counter) - 1); y++) {
			if ((*data)[y].index > (*data)[y+1].index) {
				ch_ord = (*data)[y + 1];
				(*data)[y + 1] = (*data)[y];
				(*data)[y] = ch_ord;
			}
		}
	}

	return 1;

error_cleanup_array:
	if (*data) {
		for (i = *counter - 1; i >= 0; i--) {
			if ((*data)[i].name)
				free((*data)[i].name);
		}

		free(*data);
	}

	closedir(dp);

	return ret;
}

int device_iio_utils::enable_events(const char *device_dir, bool enable)
{
	char event_el_dir[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char filename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	const struct dirent *ent;
	FILE *sysfsfp;
	int err = 0;
	DIR *dp;

	err = sprintf(event_el_dir, device_iio_event_dir, device_dir);
	if (err < 0)
		return err;

	dp = opendir(event_el_dir);
	if (!dp) {
		err = (errno == ENOENT) ? 0 : -errno;
		return err;
	}

	while (ent = readdir(dp), ent != NULL) {
		if (!strcmp(ent->d_name +
		    strlen(ent->d_name) - strlen("_en"), "_en")) {
			err = sprintf(filename, "%s/%s",
				      event_el_dir, ent->d_name);
			if (err < 0) {
				err = -ENOMEM;
				goto error_close_dir;
			}

			sysfsfp = fopen(filename, "r+");
			if (!sysfsfp) {
				err = -errno;
				goto error_close_dir;
			}

			/* enable sensor event */
			fprintf(sysfsfp, "%d", enable);
			fclose(sysfsfp);
		}
	}

error_close_dir:
	closedir(dp);

	return err;
}


int device_iio_utils::enable_sensor(char *device_dir, bool enable)
{
	char enable_file[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	int err;

	err = check_file(device_dir);
	if (!err) {
		sprintf(enable_file, "%s/%s", device_dir, device_iio_buffer_enable);
		err = sysfs_write_int(enable_file, enable);
	} else if (err != -ENOENT) {
		/* permission error */
		return err;
	}

	return enable_events(device_dir, enable);
}

int device_iio_utils::get_sampling_frequency_available(char *device_dir,
				struct device_iio_sampling_freqs *sfa)
{
	int err = 0;
	char sf_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *pch;
	char line[100];

	sfa->length = 0;

	err = sprintf(sf_filaname,
		      "%s/%s",
		      device_dir,
		      device_iio_sfa_filename);
	if (err < 0)
		return err;

	err = sysfs_read_str(sf_filaname, line, sizeof(line));
	if (err < 0)
		return err;

	pch = strtok(line," ,");
	while (pch != NULL) {
		sfa->freq[sfa->length] = atof(pch);
		pch = strtok(NULL, " ,");
		sfa->length++;
		if (sfa->length >= DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE)
			break;
	}

	return err < 0 ? err : 0;
}

int device_iio_utils::get_hw_fifo_length(const char *device_dir)
{
	int ret;
	int len;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* read <iio:devicex>/hwfifo_watermark_max */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_length);
	if (ret < 0)
		return 0;

	ret = sysfs_read_int(tmp_filaname, &len);
	if (ret < 0 || len <= 0)
		return 0;

	/* write "len * 2" -> <iio:devicex>/buffer/length */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_buffer_length);
	if (ret < 0)
		goto out;

	ret = sysfs_write_int(tmp_filaname, 2 * len);
	if (ret < 0)
		goto out;;

	/* write "1" -> <iio:devicex>/hwfifo_enabled */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_enabled);
	if (ret < 0)
		goto out;

	/* used for compatibility with old iio API */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		goto out;

	ret = sysfs_write_int(tmp_filaname, 1);
	if (ret < 0) {
		ALOGE("Failed to enable hw fifo: %s.", tmp_filaname);
	}

out:
	return len;
}

int device_iio_utils::set_sampling_frequency(char *device_dir,
					     unsigned int frequency)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "frequency" -> <iio:devicex>/sampling_frequency */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_sf_filename);
	if (ret < 0)
		return -ENOMEM;

	/* it's ok if file not exists */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		return 0;

	return sysfs_write_int(tmp_filaname, frequency);
}

int device_iio_utils::set_max_delivery_rate(const char *device_dir,
					    unsigned int delay)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "delay" -> <iio:devicex>/max_delivery_rate */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir,
		       device_iio_max_delivery_rate_filename);

	if (ret < 0)
		return -ENOMEM;

	/* it's ok if file not exists */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		return 0;

	return sysfs_write_int(tmp_filaname, delay);
}

int device_iio_utils::set_hw_fifo_watermark(char *device_dir,
					    unsigned int watermark)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "watermark" -> <iio:devicex>/hwfifo_watermark */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_watermark);

	if (ret < 0)
		return -ENOMEM;

	/* it's ok if file not exists */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		return 0;

	return sysfs_write_int(tmp_filaname, watermark);
}

int device_iio_utils::hw_fifo_flush(char *device_dir)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "1" -> <iio:devicex>/hwfifo_flush */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_flush);
	if (ret < 0)
		return -ENOMEM;

	/* it's ok if file not exists */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		return 0;

	return sysfs_write_int(tmp_filaname, 1);
}

int device_iio_utils::set_scale(const char *device_dir,
				float value,
				device_iio_chan_type_t device_type)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *scale_filename;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			scale_filename = (char *)"in_accel_x_scale";
			break;
		case DEVICE_IIO_GYRO:
			scale_filename = (char *)"in_anglvel_x_scale";
			break;
		case DEVICE_IIO_MAGN:
			scale_filename = (char *)"in_magn_x_scale";
			break;
		case DEVICE_IIO_PRESSURE:
			scale_filename = (char *)"in_press_scale";
			break;
		case DEVICE_IIO_TEMP:
			scale_filename = (char *)"in_temp_scale";
			break;
		default:
			return -EINVAL;
	}

	/* write scale -> <iio:devicex>/in_<device_type>_x_scale */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, scale_filename);

	return ret < 0 ? -ENOMEM : sysfs_write_float(tmp_filaname, value);
}

int device_iio_utils::get_scale(const char *device_dir, float *value,
				device_iio_chan_type_t device_type)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *scale_filename;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			scale_filename = (char *)"in_accel_x_scale";
			break;
		case DEVICE_IIO_GYRO:
			scale_filename = (char *)"in_anglvel_x_scale";
			break;
		case DEVICE_IIO_MAGN:
			scale_filename = (char *)"in_magn_x_scale";
			break;
		case DEVICE_IIO_PRESSURE:
			scale_filename = (char *)"in_press_scale";
			break;
		case DEVICE_IIO_TEMP:
			scale_filename = (char *)"in_temp_scale";
			break;
		default:
			return -EINVAL;
	}

	/* read <iio:devicex>/in_<device_type>_x_scale */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, scale_filename);

	return ret < 0 ? -ENOMEM : sysfs_read_float(tmp_filaname, value);
}

int device_iio_utils::get_type(struct device_iio_info_channel *channel,
			       const char *device_dir, const char *name,
			       const char *pre_name)
{
	DIR *dp;
	int ret;
	FILE *sysfsfp;
	unsigned int padint;
	const struct dirent *ent;
	char signchar, endianchar;
	char dir[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char type_name[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char name_pre_name[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char filename[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	/* Check string len */
	if (strlen(device_dir) + strlen("scan_elements") + 1 >
	    DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	if (strlen(name) + strlen("_type") + 1 >
	    DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	if (strlen(pre_name) + strlen("_type") + 1 >
	    DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(dir, "%s/scan_elements", device_dir);
	sprintf(type_name, "%s_type", name);
	sprintf(name_pre_name, "%s_type", pre_name);

	ret = sysfs_opendir(dir, &dp);
	if (ret)
		return ret;

	while (ent = readdir(dp), ent != NULL) {
		if ((strcmp(type_name, ent->d_name) == 0) ||
		    (strcmp(name_pre_name, ent->d_name) == 0)) {
			sprintf(filename, "%s/%s", dir, ent->d_name);
			sysfsfp = fopen(filename, "r");
			if (sysfsfp == NULL)
				continue;

			/* scan format like "le:s16/16>>0" */
			ret = fscanf(sysfsfp, "%ce:%c%u/%u>>%u",
				     &endianchar,
				     &signchar,
				     &channel->bits_used,
				     &padint,
				     &channel->shift);
			if (ret < 0) {
				fclose(sysfsfp);

				continue;
			}

			channel->be = (endianchar == 'b');
			channel->sign = (signchar == 's');
			channel->bytes = (padint >> 3);

			if (channel->bits_used == 64)
				channel->mask = ~0;
			else
				channel->mask = (1 << channel->bits_used) - 1;

			fclose(sysfsfp);
		}
	}

	closedir(dp);

	return 0;
}

int device_iio_utils::get_available_scales(const char *device_dir,
				   struct device_iio_scales *sa,
				   device_iio_chan_type_t device_type)
{
	int err = 0;
	FILE *fp;
	char *avl_name;
	char *pch;
	char tmp_name[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char line[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	sa->length = 0;

	/* check on all supported sensor type */
	switch (device_type) {
		case DEVICE_IIO_ACC:
			avl_name = (char *)"in_accel_scale_available";
			break;
		case DEVICE_IIO_GYRO:
			avl_name = (char *)"in_anglvel_scale_available";
			break;
		case DEVICE_IIO_TEMP:
			avl_name = (char *)"in_temp_scale_available";
			break;
		case DEVICE_IIO_MAGN:
			avl_name = (char *)"in_magn_scale_available";
			break;
		case DEVICE_IIO_PRESSURE:
			avl_name = (char *)"in_press_scale_available";
			break;
		case DEVICE_IIO_HUMIDITYRELATIVE:
			avl_name = (char *)"in_humidityrelative_scale_available";
			break;
		default:
			return -EINVAL;
	}

	/* check string len */
	if (strlen(device_dir) +
	    strlen(avl_name) + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -ENOMEM;

	sprintf(tmp_name, "%s/%s", device_dir, avl_name);

	/* if scale not available not report error */
	fp = fopen(tmp_name, "r");
	if (fp == NULL) {
		err = 0;
		return err;
	}

	if (!fgets(line, sizeof(line), fp)) {
		err = -EINVAL;

		goto fpclose;
	}

	pch = strtok(line," ");
	while (pch != NULL) {
		sa->scales[sa->length] = atof(pch);
		pch = strtok(NULL, " ");
		sa->length++;

		if (sa->length >= DEVICE_IIO_SCALE_AVAILABLE)
			break;
	}

fpclose:
	fclose(fp);

	return err;
}

/* Sensor data injection mode */
int device_iio_utils::support_injection_mode(const char *device_dir)
{
	int err;
	char injectors[30];
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	/* Check string len */
	if (strlen(device_dir) +
	    strlen(device_iio_injection_mode_enable) +
	    1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(tmp_filaname,
		"%s/%s",
		device_dir,
		device_iio_injection_mode_enable);
	err = sysfs_read_int(tmp_filaname, &err);
	if (err < 0) {
		sprintf(tmp_filaname,
			"%s/%s",
			device_dir,
			device_iio_injection_sensors_filename);
		err = sysfs_read_str(tmp_filaname,
				     injectors,
				     sizeof(injectors));
		if (err < 0)
			return err;

		return 1;
	}

	return 0;
}

int device_iio_utils::set_injection_mode(const char *device_dir, bool enable)
{
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	int ret = 0;

	/* write "enable" -> <iio:devicex>/injection_mode */
	ret = snprintf(tmp_filaname,
		       DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s",
		       device_dir,
		       device_iio_injection_mode_enable);
	if (ret < 0)
		return -ENOMEM;

	return ret < 0 ? -ENOMEM : sysfs_write_int(tmp_filaname, enable);
}

int device_iio_utils::inject_data(const char *device_dir, unsigned char *data,
				  int len, device_iio_chan_type_t device_type)
{
	char *injection_filename;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	FILE  *sysfsfp;
	int ret = 0;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			injection_filename = (char *)"in_accel_injection_raw";
			break;
		case DEVICE_IIO_GYRO:
			injection_filename =
				(char *)"in_anglvel_injection_raw";
			break;
		case DEVICE_IIO_MAGN:
			injection_filename = (char *)"in_magn_injection_raw";
			break;
		case DEVICE_IIO_PRESSURE:
			injection_filename = (char *)"in_press_injection_raw";
			break;
		case DEVICE_IIO_TEMP:
			injection_filename = (char *)"in_temp_injection_raw";
			break;
		default:
			return -EINVAL;
	}

	/* Check string len */
	if (strlen(device_dir) +
		strlen(injection_filename) + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(tmp_filaname, "%s/%s", device_dir, injection_filename);

	sysfsfp = fopen(tmp_filaname, "w");
	if (sysfsfp == NULL)
		return -errno;

	ret = fwrite(data, len, 1, sysfsfp);
	fclose(sysfsfp);

	return ret;
}

int device_iio_utils::get_selftest_available(const char *device_dir,
					     char list[][20])
{
	FILE *fp;
	int err, elements = 0;
	char *pch, *res, line[200];
	char sf_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	err = sprintf(sf_filaname, "%s/%s", device_dir,
		      device_iio_selftest_available_filename);
	if (err < 0)
		return err;

	fp = fopen(sf_filaname, "r");
	if (fp == NULL)
		return -errno;

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

        return err < 0 ? err : elements;
}

int device_iio_utils::execute_selftest(const char *device_dir, char *mode)
{
	int err;
	char result[20];
	char sf_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	err = sprintf(sf_filaname, "%s/%s", device_dir,
		      device_iio_selftest_filename);
	if (err < 0)
		return err;

	err = sysfs_write_str(sf_filaname, mode);
	if (err < 0)
		return err;

	err = sysfs_read_str(sf_filaname, result, 20);
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
