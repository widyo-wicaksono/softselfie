/*
 * util_gen.h
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */

#ifndef UTIL_GEN_H_
#define UTIL_GEN_H_



#endif /* UTIL_GEN_H_ */

#ifndef PATH_MAX
#define PATH_MAX        4096
#endif

#include <app.h>
#include <stdlib.h>
#include <ctype.h>
#include <system_info.h>

void lowerCase(char * input, char ** output, int len);
void get_device_model_name(char** smodel);
const char *_get_resource_path(const char *file_path);
size_t _get_file_path(char *file_path, size_t size, char * media_content_folder);
