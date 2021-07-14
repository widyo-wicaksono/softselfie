/*
 * camera_handler.h
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */

#ifndef CAMERA_HANDLER_H_
#define CAMERA_HANDLER_H_
#endif /* CAMERA_HANDLER_H_ */

#include "util_gen.h"
#ifndef UTIL_GEN_H_
#include <app.h>
#endif

#include <Elementary.h>
#include <efl_extension.h>
#include <dlog.h>
#include <camera.h>

#include <media_content.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "softselfie"

#if !defined(PACKAGE)
#define PACKAGE "com.camera.softselfie"
#endif

#define CAMERA_DIRECTORY	"/opt/usr/media/Images\0"
//#define CAMERA_DIRECTORY2	"/opt/storage/sdcard/softselfie\0"
//#define CAMERA_DIRECTORY	"/Internal0/Images\0"
#define CAMERA_DIRECTORY_NAME2	"images"
#define IMAGE_MIME_TYPE		"image/*"

struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *navi;
	Elm_Object_Item *navi_item;
	Evas_Object *layout;
	Evas_Object *popup;
	Evas_Object *preview_canvas;
	camera_h camera;
	Eina_Bool camera_enabled;
	Ecore_Timer *timer;
	Ecore_Timer *view_finder_timer;

	char *media_content_folder;
	int prev_width;
	int prev_height;
	int bIsAppPaused;
	int iCamIndex;
	int iEffect;
	int iUIState;
} s_info;

int cameraDestroy(camera_h *pcamera);
int cameraStopPreview(camera_h camera);
void cameraStopPreviewTimer();
int modify_final_image(camera_image_data_s *image, const char *filename, camera_h cam_handle, int iCamIndex);
void cameraStartPreviewTimer();
int prepare_camera(camera_h camera);
Eina_Bool cameraInit(int iCamIndex);
Eina_Bool cameraInitEx(int iCamIndex, int *pError, int *pErrCode);
int cameraSwitch();
int cameraSwitchEx(int * pErr, int * pErrCode);
int cameraIsSettingReadyToBeChanged(camera_h camera);
