/*
 * util_gen.c
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */

#include "util_gen.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef FILE_PREFIX
#define FILE_PREFIX "IMAGE"
#endif

void lowerCase(char * input, char ** output, int len){
	char * buff = (char*)calloc(len+1,sizeof(char));
	for(int i=0;i<len;i++){
		buff[i] = tolower(input[i]);
	}
	*output=buff;
}

void get_device_model_name(char** smodel) {
	char *value;
	int ret;

	ret = system_info_get_platform_string("tizen.org/system/model_name", &value);
	if (ret == SYSTEM_INFO_ERROR_NONE){
		*smodel = value;
	}
}

/**
 * @brief Get resource absolute path.
 * @param[in] file_path Resource path relative to resources directory
 * @return Resource absolute path
 */
const char *_get_resource_path(const char *file_path)
{
	static char res_path[PATH_MAX] = { '\0' };
	static char res_folder_path[PATH_MAX] = { '\0' };
	if (res_folder_path[0] == '\0') {
		char *resource_path_buf = app_get_resource_path();
		strncpy(res_folder_path, resource_path_buf, PATH_MAX - 1);
		free(resource_path_buf);
	}
	snprintf(res_path, PATH_MAX, "%s%s", res_folder_path, file_path);
	return res_path;
}

size_t _get_file_path(char *file_path, size_t size, char * media_content_folder)
{
	int chars = 0;
	struct tm localtime = { 0 };
	time_t rawtime = time(NULL);

	if (!file_path) {
		//dlog_print(DLOG_ERROR, LOG_TAG, "file_path is NULL");
		return 0;
	}

	if (localtime_r(&rawtime, &localtime) == NULL)
		return 0;

	chars = snprintf(file_path, size, "%s/%s_%04i-%02i-%02i_%02i:%02i:%02i.jpg",
	//chars = snprintf(file_path, size, "%s/%04i-%02i-%02i_%02i:%02i:%02i.jpg",
			media_content_folder, FILE_PREFIX,
			//media_content_folder,
			localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
			localtime.tm_hour, localtime.tm_min, localtime.tm_sec);

	return chars;
}
