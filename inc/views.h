/*
 * views.h
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */

#ifndef VIEWS_H_
#define VIEWS_H_
#endif /* VIEWS_H_ */

#ifndef CAMERA_HANDLER_H_
#define CAMERA_HANDLER_H_
#endif /* CAMERA_HANDLER_H_ */

#include "camera_handler.h"

#ifndef CAMERA_HANDLER_H_
#include <app.h>
#include <Elementary.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "softselfie"

#if !defined(PACKAGE)
#define PACKAGE "com.camera.softselfie"
#endif


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

	char *media_content_folder;
	int prev_width;
	int prev_height;
	int bIsAppPaused;
	int iCamIndex;
	int iEffect;
	int iUIState;
} s_info;
#endif

void globalVarInit();
void setAppPauseState(int bState);
void view_destroy(void);
Eina_Bool main_view_create();
