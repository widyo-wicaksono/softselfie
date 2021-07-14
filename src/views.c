/*
 * views.c
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */

#include <wav_player.h>
#include <storage.h>

#include "views.h"
#include "util_gen.h"
#include "edc_defines.h"

#define COUNTER_STR_LEN 3
#define FILE_PREFIX "IMAGE"
#define STR_ERROR "Error"
#define STR_OK "OK"
#define STR_FILE_PROTOCOL "file://"
#define SOUND_APP	"sounds/aperture.wav"
#define SOUND_FOCUS	"sounds/focus.wav"

#define POPUP_BUTTON_STYLE	"popup_button/default"
#define POPUP_BUTTON_PART	"button1"

//#define CAMERA_DIRECTORY2	"/opt/storage/sdcard/softselfie\0"
#define CAMERA_DIRECTORY2	"/opt/usr/media/softselfie\0"


int giImageUtilError = 0;
/*
int g_iMediaConnectResult = 0;
int g_iMediaForEach = 0;
int g_iMediaDisconnectResult = 0;
int g_iMKDir =0;
char g_cFilepath[PATH_MAX] = {0};
*/

static void _exit_show_warning_popup(Evas_Object *navi, const char *caption, const char *text);

void globalVarInit(){
	s_info.win = NULL;
	s_info.conform = NULL;
	s_info.navi = NULL;
	s_info.navi_item = NULL;
	s_info.layout = NULL;
	s_info.popup = NULL;
	s_info.preview_canvas = NULL;
	s_info.camera = NULL;
	s_info.camera_enabled = false;
	s_info.timer = NULL;
	s_info.view_finder_timer = NULL;
	s_info.media_content_folder = NULL;
	s_info.prev_width=640;
	s_info.prev_height=480;
	s_info.bIsAppPaused = 0;
	s_info.iCamIndex = 1;
	s_info.iEffect = 13;
	s_info.iUIState = 0;
	s_info.iEffect = 13;
	//void* pRes = ecore_thread_global_data_set("cam", (void*)s_info.iCamIndex,NULL);
}


static bool _storage_cb(int storage_id, storage_type_e type, storage_state_e state,
                        const char *path, void *user_data)
{
    if (STORAGE_TYPE_INTERNAL == type) {
        int *internal_storage_id = (int *)user_data;
        *internal_storage_id = storage_id;

        /* Internal storage found, stop the iteration. */
        return false;
    } else {
        /* Continue iterating over storages. */
        return true;
    }
}


void stopViewFinderTimer(){
	if(s_info.view_finder_timer !=NULL){
		ecore_timer_del(s_info.view_finder_timer);
		s_info.view_finder_timer=NULL;
	}
}

Eina_Bool blurFormTimer_cb(void *data)
{
	stopViewFinderTimer();
	elm_object_signal_emit(s_info.layout, "hide_viewfinder", "hide_viewfinder");
	return ECORE_CALLBACK_CANCEL;
}

void startViewFinderTimer()
{
	stopViewFinderTimer();
	if(s_info.view_finder_timer==NULL){
		s_info.view_finder_timer = ecore_timer_add(2.0, blurFormTimer_cb, NULL);
	}
}

void startViewFinderTimerEx(float fDur)
{
	stopViewFinderTimer();
	if(s_info.view_finder_timer==NULL){
		s_info.view_finder_timer = ecore_timer_add(fDur, blurFormTimer_cb, NULL);
	}
}
void view_destroy(void)
{
	if (s_info.win == NULL)
		return;
	cameraStopPreviewTimer();
	stopViewFinderTimer();
	free(s_info.media_content_folder);
	ecore_thread_global_data_del("blur");
	//ecore_thread_global_data_del("camindex");
	evas_object_del(s_info.win);
}

Evas_Object *view_create_win(const char *pkg_name)
{
	Evas_Object *win = NULL;

	/*
	 * Window
	 * Create and initialise elm_win.
	 * elm_win is mandatory to manipulate the window.
	 */
	win = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_conformant_set(win, EINA_TRUE);

	return win;
}

Evas_Object *view_create_conformant(Evas_Object *win)
{
	/*
	 * Conformant
	 * Create and initialise elm_conformant.
	 * elm_conformant is mandatory for base GUI to have proper size
	 * when indicator or virtual keypad is visible.
	 */
	Evas_Object *conform = NULL;

	if (win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "window is NULL.");
		return NULL;
	}

	conform = elm_conformant_add(win);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_TRANSPARENT);

	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conform);

	evas_object_show(conform);

	return conform;
}

static Evas_Object *_view_create_bg(void)
{
	Evas_Object *bg = elm_bg_add(s_info.conform);
	elm_object_part_content_set(s_info.conform, "elm.swallow.bg", bg);
	return bg;
}

static void _app_navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *top = elm_naviframe_top_item_get(s_info.navi);

	if (top == elm_naviframe_bottom_item_get(s_info.navi)){
		//elm_win_lower(s_info.win);
		if(s_info.camera){
			camera_state_e cur_state = CAMERA_STATE_NONE;
			int res = camera_get_state(s_info.camera, &cur_state);
			if (res == CAMERA_ERROR_NONE) {
				if(cur_state!=CAMERA_STATE_CAPTURING){
					_exit_show_warning_popup(s_info.navi, "Exit App", "Do you wish to exit ?");
				}
			}
		}
	}
	else
		elm_naviframe_item_pop(s_info.navi);
}

/**
 * @brief Add and configure naviframe to conform.
 * @return Naviframe object pointer
 */
static Evas_Object *_app_navi_add(void)
{
	Evas_Object *navi = elm_naviframe_add(s_info.conform);
	if (!navi) {
		dlog_print(DLOG_ERROR, LOG_TAG, "elm_naviframe_add() failed");
		return NULL;
	}
	elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);

	eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK, _app_navi_back_cb, NULL);
	elm_object_part_content_set(s_info.conform, "elm.swallow.content", navi);
	return navi;
}
/*
static bool _folder_cb(media_folder_h folder, void *user_data)
{
	char *name = NULL;
	int ret = MEDIA_CONTENT_ERROR_NONE;
	int len = 0;

	ret = media_folder_get_name(folder, &name);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[%s:%d] media_folder_get_name() error: %s",
				__FILE__, __LINE__, get_error_message(ret));
		return false;
	}

	len = strlen(CAMERA_DIRECTORY_NAME2);
	char * image_folder_name = NULL;
	lowerCase(name, &image_folder_name, strlen(name));

	if (strncmp(CAMERA_DIRECTORY_NAME2, image_folder_name, len) || len != strlen(name)) {
		free(name);
		free(image_folder_name);
		return true;
	}

	ret = media_folder_get_path(folder, &s_info.media_content_folder);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[%s:%d] media_folder_get_path() error: %s",
				__FILE__, __LINE__, get_error_message(ret));
		free(image_folder_name);
		free(name);
		return false;
	}

	free(image_folder_name);
	free(name);
	return false;
}
*/

/**
 * @brief List all media folders from media content db
 * @return false in case of any problems, otherwise true
 */
/*
static bool _list_media_folders(void)
{
	int ret = MEDIA_CONTENT_ERROR_NONE;

	ret = media_content_connect();
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[%s:%d] media_content_connect() error: %s",
				__FILE__, __LINE__, get_error_message(ret));
		return false;
	}
	//g_iMediaConnectResult = ret;

	ret = media_folder_foreach_folder_from_db(NULL, _folder_cb, NULL);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[%s:%d] media_folder_foreach_folder_from_db() error: %s",
				__FILE__, __LINE__, get_error_message(ret));
		return false;
	}
	//g_iMediaForEach = ret;

	ret = media_content_disconnect();
	//if (media_content_disconnect() != MEDIA_CONTENT_ERROR_NONE) {
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Error occurred when disconnecting from media db!");
		return false;
	}
	//g_iMediaDisconnectResult = ret;

	if(s_info.media_content_folder==NULL){
		//g_iMKDir =  mkdir(CAMERA_DIRECTORY, 0777);
		mkdir(CAMERA_DIRECTORY, 0777);
		s_info.media_content_folder=(char*)malloc(sizeof(CAMERA_DIRECTORY));
		strcpy(s_info.media_content_folder,CAMERA_DIRECTORY);
	}
	return true;
}
*/
bool _list_media_folders2(void)
{
	 /* Get the path to the Images directory: */

	/* 1. Get internal storage id. */
	int internal_storage_id = -1;

	int error_code = storage_foreach_device_supported(_storage_cb, &internal_storage_id);
	if (STORAGE_ERROR_NONE != error_code) {
		return false;
	}

	/* 2. Get the path to the Images directory. */
	error_code = storage_get_directory(internal_storage_id, STORAGE_DIRECTORY_IMAGES, &s_info.media_content_folder);
	return true;
}

bool _list_media_folders3(void)
{
	//g_iMKDir =  mkdir(CAMERA_DIRECTORY2, 0777);
	mkdir(CAMERA_DIRECTORY2, 0777);
	s_info.media_content_folder=(char*)malloc(sizeof(CAMERA_DIRECTORY2));
	strcpy(s_info.media_content_folder,CAMERA_DIRECTORY2);
	return true;
}
/**
 * @brief Check if dir->d_name has IMAGE prefix.
 * @param[in] dir "dirent" structure with d_name to check
 * @return strncmp String compare result (0 on equal)
 */
static int _main_view_image_file_filter(const struct dirent *dir)
{
	return strncmp(dir->d_name, FILE_PREFIX, sizeof(FILE_PREFIX) - 1) == 0;
}

/**
 * @brief Gets the last file with IMAGE prefix on given path.
 * @param[out] file_path Full image path output
 * @param[in] size Maximum path string length
 * @return Returned file_path length
 */
static size_t _main_view_get_last_file_path(char *file_path, size_t size)
{
	int n = -1;
	struct dirent **namelist = NULL;
	int ret_size = 0;

	n = scandir(s_info.media_content_folder, &namelist, _main_view_image_file_filter, alphasort);

	if (n > 0) {
		ret_size = snprintf(file_path, size, "%s/%s", s_info.media_content_folder, namelist[n - 1]->d_name);

		/* Need to go through array to clear it */
		int i;
		for (i = 0; i < n; ++i) {
			/* The last file in sorted array will be taken */
			free(namelist[i]);
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "No files or failed to make scandir");
	}

	free(namelist);
	return ret_size;
}

static void _main_view_popup_close_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (s_info.popup) {
		evas_object_del(s_info.popup);
		s_info.popup = NULL;
	}
}


/**
 * @brief Show warning popup.
 * @param[in] navi Naviframe to create popup
 * @param[in] caption Popup caption
 * @param[in] text Popup message text
 * @param[in] button_text Text on popup confirmation button
 */
static void _main_view_show_warning_popup(Evas_Object *navi,
		const char *caption, const char *text, const char *button_text)
{
	Evas_Object *button = NULL;
	Evas_Object *popup = elm_popup_add(navi);
	if (!popup) {
		dlog_print(DLOG_ERROR, LOG_TAG, "popup is not created");
		return;
	}
	elm_object_part_text_set(popup, "title,text", caption);
	elm_object_text_set(popup, text);
	evas_object_show(popup);

	button = elm_button_add(popup);
	if (!button) {
		dlog_print(DLOG_ERROR, LOG_TAG, "button is not created");
		return;
	}
	elm_object_style_set(button, POPUP_BUTTON_STYLE);
	elm_object_text_set(button, button_text);
	elm_object_part_content_set(popup, POPUP_BUTTON_PART, button);
	evas_object_smart_callback_add(button, "clicked", _main_view_popup_close_cb, NULL);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _main_view_popup_close_cb, NULL);

	s_info.popup = popup;
}

/**
 * @brief Smart callback on thumbnail click.
 * @param[in] data User data
 * @param[in] obj Target object
 * @param[in] event_info Event information
 */
static void _main_view_thumbnail_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	int ret = 0;
	app_control_h app_control = NULL;
	char file_path[PATH_MAX] = { '\0' };
	char file_path_prepared[PATH_MAX + sizeof(STR_FILE_PROTOCOL)] = { '\0' };

	if (_main_view_get_last_file_path(file_path, sizeof(file_path)) == 0){
		_main_view_show_warning_popup(s_info.navi, STR_ERROR, "no filepath", STR_OK);
		return;
	}

	ret = app_control_create(&app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_control_create() failed.");
		return;
	}

	strncpy(file_path_prepared, STR_FILE_PROTOCOL, PATH_MAX + sizeof(STR_FILE_PROTOCOL));
	strncat(file_path_prepared, file_path, PATH_MAX + sizeof(STR_FILE_PROTOCOL));
	ret = app_control_set_uri(app_control, file_path_prepared);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_control_set_uri() failed.");
		app_control_destroy(app_control);
		return;
	}

	char *sModelName =NULL;
	get_device_model_name(&sModelName);

	if(strncmp(sModelName,"SM-Z2",5)==0){
		if(sModelName!=NULL)
			free(sModelName);
		ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "app_control_set_operation() failed.");
			app_control_destroy(app_control);
			return;
		}

		ret = app_control_set_mime(app_control, IMAGE_MIME_TYPE);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "app_control_set_mime() failed.");
			app_control_destroy(app_control);
			return;
		}

		ret = app_control_set_app_id (app_control, "tizen.gallery");
		if (ret != APP_CONTROL_ERROR_NONE){
		   dlog_print(DLOG_ERROR, LOG_TAG, "app_control_set_app_id() is failed. err = %d", ret);
		   app_control_destroy(app_control);
		   return;
		}

	}
	else{
		if(sModelName!=NULL)
			free(sModelName);
		bool isSucceeded = false;
		ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW );
		if (ret == APP_CONTROL_ERROR_NONE){
			isSucceeded = true;
		}
		if(!isSucceeded){
			app_control_destroy(app_control);
			_main_view_show_warning_popup(s_info.navi, STR_ERROR, "Failed to lauch image viewer", STR_OK);
			return;
		}
	}

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_control_send_launch_request() failed.");

		if (ret == APP_CONTROL_ERROR_APP_NOT_FOUND) {
			_main_view_show_warning_popup(s_info.navi, STR_ERROR,
					"Image viewing application wasn't found", STR_OK);
		} else {
			_main_view_show_warning_popup(s_info.navi, STR_ERROR,
					"Image viewing application initialisation failed", STR_OK);
		}
	}

	app_control_destroy(app_control);
}

/**
 * @brief Set the thumbnail content to specified file.
 * @param[in] file_path Path to the file to set
 */
static void _main_view_thumbnail_set(const char *file_path)
{
	Evas_Object *img = elm_object_part_content_get(s_info.layout, "thumbnail");
	if (!img) {
		img = elm_image_add(s_info.layout);
		elm_object_part_content_set(s_info.layout, "thumbnail", img);
		evas_object_smart_callback_add(img, "clicked", _main_view_thumbnail_click_cb, NULL);
	}
	elm_image_file_set(img, file_path, NULL);
	elm_object_signal_emit(s_info.layout, "default", "thumbnail_background");
}

/**
 * @brief Load last file thumbnail.
 */
static void _main_view_thumbnail_load(void)
{
	char file_path[PATH_MAX] = { '\0' };
	if (_main_view_get_last_file_path(file_path, sizeof(file_path)))
		_main_view_thumbnail_set(file_path);
	else
		elm_object_signal_emit(s_info.layout, "no_image", "thumbnail_background");
}

static void _main_view_destroy_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	cameraStopPreviewTimer();
	cameraStopPreview(s_info.camera);
	cameraDestroy(&s_info.camera);
}

static void _main_aperture_cb(void *interval, Evas_Object *obj, const char *emission, const char *source)
{
	int static iFlagWav=0;
	if(iFlagWav!=0)
		wav_player_start(_get_resource_path(SOUND_APP), SOUND_TYPE_MEDIA, NULL, NULL, NULL);
	else {
		iFlagWav=1;
	}
	int tmp = s_info.iEffect / 10;
	s_info.iEffect = tmp*10 + (int)interval;

	//ecore_thread_global_data_set("blur", interval,NULL);
	ecore_thread_global_data_set("blur", (void*) s_info.iEffect,NULL);
}

static void _main_view_rate_button_cb(void *data, Evas_Object *obj, const char *emission, const char *source){
	app_control_h service = NULL;
	int ret = -1;
	if(app_control_create(&service)==APP_CONTROL_ERROR_NONE){
		if(app_control_set_app_id(service, "org.tizen.tizenstore")== APP_CONTROL_ERROR_NONE){
			//if(app_control_set_uri(service, "tizenstore://ProductDetail/com.camera.prolens")== APP_CONTROL_ERROR_NONE){
			if(app_control_set_uri(service, "tizenstore://ProductDetail/com.camera.softselfie")== APP_CONTROL_ERROR_NONE){
				if(app_control_set_operation(service, APP_CONTROL_OPERATION_VIEW)==APP_CONTROL_ERROR_NONE){
					if(app_control_send_launch_request(service, NULL, NULL)==APP_CONTROL_ERROR_NONE){
						ret = 0;
					}
				}
			}
		}
		app_control_destroy(service);
	}
	if(ret!=0){

	}
	return;
}



static void _exit_popup_close_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *popup =(Evas_Object *)data;
	if(popup){
		evas_object_del(popup);
	}
	ui_app_exit();
}

static void _not_exit_popup_close_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *popup =(Evas_Object *)data;
	if(popup){
		evas_object_del(popup);
	}
}


static void _exit_show_warning_popup(Evas_Object *navi, const char *caption, const char *text)
{
	Evas_Object *button = NULL;
	Evas_Object *button_cancel = NULL;
	Evas_Object *popup = elm_popup_add(navi);
	if (!popup) {
		dlog_print(DLOG_ERROR, LOG_TAG, "popup is not created");
		return;
	}
	elm_object_part_text_set(popup, "title,text", caption);
	elm_object_text_set(popup, text);
	evas_object_show(popup);

	button = elm_button_add(popup);
	if (!button) {
		dlog_print(DLOG_ERROR, LOG_TAG, "button is not created");
		return;
	}
	button_cancel = elm_button_add(popup);
	if (!button_cancel) {
		dlog_print(DLOG_ERROR, LOG_TAG, "button is not created");
		return;
	}
	elm_object_style_set(button, POPUP_BUTTON_STYLE);
	elm_object_text_set(button, "Exit");
	elm_object_style_set(button_cancel, POPUP_BUTTON_STYLE);
	elm_object_text_set(button_cancel, "Cancel");
	//elm_object_part_content_set(popup, POPUP_BUTTON_PART, button);
	elm_object_part_content_set(popup, POPUP_BUTTON_PART, button_cancel);
	elm_object_part_content_set(popup, "button2", button);
	//elm_object_part_content_set(popup, "button2", button_cancel);

	evas_object_smart_callback_add(button, "clicked", _exit_popup_close_cb, popup);
	evas_object_smart_callback_add(button_cancel, "clicked", _not_exit_popup_close_cb, popup);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _not_exit_popup_close_cb, popup);
}

static void _exit_show_warning_popup2(Evas_Object *navi, const char *caption, const char *text)
{
	Evas_Object *button = NULL;
	Evas_Object *popup = elm_popup_add(navi);
	if (!popup) {
		dlog_print(DLOG_ERROR, LOG_TAG, "popup is not created");
		return;
	}
	elm_object_part_text_set(popup, "title,text", caption);
	elm_object_text_set(popup, text);
	evas_object_show(popup);

	button = elm_button_add(popup);
	if (!button) {
		dlog_print(DLOG_ERROR, LOG_TAG, "button is not created");
		return;
	}
	elm_object_style_set(button, POPUP_BUTTON_STYLE);
	elm_object_text_set(button, "Exit");
	elm_object_part_content_set(popup, "button1", button);

	evas_object_smart_callback_add(button, "clicked", _exit_popup_close_cb, popup);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _exit_popup_close_cb, popup);
}

static void _main_view_exit(void *data, Evas_Object *obj, void *event_info)
{
	/*
	if(s_info.iUIState>0)
	{
		camera_state_e cur_state = CAMERA_STATE_NONE;
		//int res = camera_get_state((camera_h)data, &cur_state);
		int res = camera_get_state(s_info.camera, &cur_state);
		if (res == CAMERA_ERROR_NONE) {
			if(cur_state!=CAMERA_STATE_CAPTURING){
				//_exit_show_warning_popup(s_info.navi, "Exit App", "Do you wish to exit ?");
				cameraStopPreview(s_info.camera);
				cameraStopPreviewTimer();
				elm_object_signal_emit(s_info.layout, EVENT_BACK_FRONT_PAGE, "splashscreen");
				s_info.iUIState--;
			}
		}
	}
	else{
		_exit_show_warning_popup(s_info.navi, "Exit App", "Do you wish to exit ?");
	}
	*/
	camera_state_e cur_state = CAMERA_STATE_NONE;
	int res = camera_get_state(s_info.camera, &cur_state);
	if (res == CAMERA_ERROR_NONE) {
		if(cur_state!=CAMERA_STATE_CAPTURING){
			_exit_show_warning_popup(s_info.navi, "Exit App", "Do you wish to exit ?");
		}
	}
}

static void _main_view_capture_cb(camera_image_data_s *image, camera_image_data_s *postview, camera_image_data_s *thumbnail, void *user_data)
{
	size_t size;
	char filename[PATH_MAX] = { '\0' };

	size = _get_file_path(filename, sizeof(filename), s_info.media_content_folder);
	if (size == 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "_main_view_get_filename() failed");
		return;
	}
/*
	memset(g_cFilepath,0,PATH_MAX);
	strcpy(g_cFilepath, filename);
*/
	int res =modify_final_image(image, filename, s_info.camera, s_info.iCamIndex);
	if(res!=0){
		//_main_view_show_warning_popup(s_info.navi, STR_ERROR, "Error while saving image. Make sure you have enough free space", STR_OK);
		/*
		if(res==IMAGE_UTIL_ERROR_INVALID_PARAMETER){
			giImageUtilError = IMAGE_UTIL_ERROR_INVALID_PARAMETER;
		}
		else if(res==IMAGE_UTIL_ERROR_OUT_OF_MEMORY){

		}
		else if(res==IMAGE_UTIL_ERROR_NO_SUCH_FILE){

		}
		*/
		giImageUtilError = res;
	}
	else{
		_main_view_thumbnail_set(filename);
	}
}

static void _main_view_capture_completed_cb(void *data)
{
	struct view_info * pInfo = (struct view_info *)data;
	if (!pInfo->camera_enabled) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Camera hasn't been initialised.");
		return;
	}
	elm_object_part_text_set(s_info.layout, "timer_text", "");
	evas_object_show(s_info.layout);
	if(s_info.bIsAppPaused){
		cameraStopPreview(s_info.camera);
	}
	else
		cameraStartPreviewTimer();
	if(giImageUtilError!=0){

		char sErr[128] = {0};
		//strncpy(sErr, 128, "Saving Error Code %d. Please report this error via email or store comment",giImageUtilError );
		snprintf(sErr, 128, "Saving Error Code [%d]. Please report this error via email or store comment",giImageUtilError);
		giImageUtilError=0;
		_main_view_show_warning_popup(s_info.navi, STR_ERROR, sErr, STR_OK);
	}
}

void cameraCapture_cb(void *data, Evas_Object *obj, const char *emission, const char *source){
	if (s_info.camera_enabled) {
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(s_info.camera, &cur_state);
		if (res != CAMERA_ERROR_NONE) {
			return;
		}
		if (cur_state != CAMERA_STATE_PREVIEW) {
			return;
		}
		elm_object_part_text_set(s_info.layout, "timer_text", "saving ...");
		evas_object_show(s_info.layout);
		camera_start_capture(s_info.camera, _main_view_capture_cb,_main_view_capture_completed_cb, data);
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Camera hasn't been initialised.");
	}
}

void cameraSwtich_cb(void *data, Evas_Object *obj, const char *emission, const char *source){
	//cameraSwitch();
	int iErr,iErrCode;
	if(cameraSwitchEx(&iErr, &iErrCode)!=0)
		//_main_view_show_warning_popup(s_info.navi, STR_ERROR, "Camera failed, probably in use", STR_OK);
		_exit_show_warning_popup2(s_info.navi, STR_ERROR, "Camera failed, probably in use");
}

void cameraStartPreview_cb(void *data, Evas_Object *obj, const char *emission, const char *source){
	//s_info.iUIState++;
	cameraStartPreviewTimer();
}

void exitViaSplash_cb(void *data, Evas_Object *obj, const char *emission, const char *source){
	_exit_show_warning_popup(s_info.navi, "Exit App", "Do you wish to exit ?");
}

static void _main_mode_cb(void *interval, Evas_Object *obj, const char *emission, const char *source)
{
	/*
	int static iFlagWav=0;
	if(iFlagWav!=0)
		wav_player_start(_get_resource_path(SOUND_APP), SOUND_TYPE_MEDIA, NULL, NULL, NULL);
	else {
		iFlagWav=1;
	}
	*/
	wav_player_start(_get_resource_path(SOUND_FOCUS), SOUND_TYPE_MEDIA, NULL, NULL, NULL);
	int tmp = s_info.iEffect % 10;
	s_info.iEffect = (int)interval + tmp;

	//ecore_thread_global_data_set("blur", interval,NULL);
	startViewFinderTimer();
	ecore_thread_global_data_set("blur", (void*)s_info.iEffect,NULL);
}

static void _main_view_register_cbs(void)
{
	//elm_object_signal_callback_add(s_info.layout, EVENT_APT01_SELECTED, "*", _main_aperture_cb, (void *)1);
	//elm_object_signal_callback_add(s_info.layout, EVENT_APT02_SELECTED, "*", _main_aperture_cb, (void *)2);
	//elm_object_signal_callback_add(s_info.layout, EVENT_APT03_SELECTED, "*", _main_aperture_cb, (void *)3);
	//elm_object_signal_callback_add(s_info.layout, EVENT_APT04_SELECTED, "*", _main_aperture_cb, (void *)4);

	elm_object_signal_callback_add(s_info.layout, EVENT_APT01_SELECTED, "*", _main_aperture_cb, (void *)4);
	elm_object_signal_callback_add(s_info.layout, EVENT_APT02_SELECTED, "*", _main_aperture_cb, (void *)3);
	elm_object_signal_callback_add(s_info.layout, EVENT_APT03_SELECTED, "*", _main_aperture_cb, (void *)2);
	elm_object_signal_callback_add(s_info.layout, EVENT_APT04_SELECTED, "*", _main_aperture_cb, (void *)1);
	elm_object_signal_callback_add(s_info.layout, EVENT_BLUR_RAD_SELECTED, "*", _main_mode_cb, (void *)10);
	elm_object_signal_callback_add(s_info.layout, EVENT_BLUR_VER_SELECTED, "*", _main_mode_cb, (void *)20);
	elm_object_signal_callback_add(s_info.layout, EVENT_BLUR_HOR_SELECTED, "*", _main_mode_cb, (void *)30);
	elm_object_signal_callback_add(s_info.layout, EVENT_SHUTTER_CLICKED, "*", cameraCapture_cb, (void*)&s_info);
	elm_object_signal_callback_add(s_info.layout, EVENT_CAM_SWITCHED, "*", cameraSwtich_cb, NULL);
	elm_object_signal_callback_add(s_info.layout, EVENT_CAM_START_PREVIEW, "*", cameraStartPreview_cb, NULL);
	elm_object_signal_callback_add(s_info.layout, EVENT_SPLASH_EXIT, "*", exitViaSplash_cb, NULL);
	elm_object_signal_callback_add(s_info.layout, EVENT_RATE_CLICKED, "*",_main_view_rate_button_cb,NULL);
	//eext_object_event_callback_add(s_info.layout, EEXT_CALLBACK_BACK, _main_view_exit, s_info.camera);
}

static Evas_Object *_main_view_add(void)
{
	//_list_media_folders();
	_list_media_folders3();

	s_info.layout = elm_layout_add(s_info.navi);
	if (!s_info.layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "elm_layout_add() failed");
		return NULL;
	}
	elm_layout_theme_set(s_info.layout, GRP_MAIN, "application", "default");
	evas_object_event_callback_add(s_info.layout, EVAS_CALLBACK_FREE, _main_view_destroy_cb, NULL);

	elm_layout_file_set(s_info.layout, _get_resource_path(EDJ_MAIN), GRP_MAIN);
	elm_object_signal_emit(s_info.layout, "mouse,clicked,1", "timer_5");
	//elm_object_signal_emit(s_info.layout, "mouse,clicked,1", "blur_radial");

	s_info.preview_canvas = evas_object_image_filled_add(evas_object_evas_get(s_info.layout));
	if (!s_info.preview_canvas) {
		dlog_print(DLOG_ERROR, LOG_TAG, "_main_view_add() failed");
		evas_object_del(s_info.layout);
		return NULL;
	}

	elm_object_part_content_set(s_info.layout, "elm.swallow.content", s_info.preview_canvas);

	//s_info.camera_enabled =cameraInit(s_info.iCamIndex);
	int iErr, iErrCode;
	s_info.camera_enabled =cameraInitEx(s_info.iCamIndex, &iErr, &iErrCode);
	//debug begin
/*
			char buff[32]={0};
			sprintf(buff,"Res:%d x %d", s_info.prev_width, s_info.prev_height);
			_main_view_show_warning_popup(s_info.navi, "ERROR", buff, "OK");
*/
	//debug end

	_main_view_thumbnail_load();
	_main_view_register_cbs();

	s_info.navi_item = elm_naviframe_item_push(s_info.navi, NULL, NULL, NULL, s_info.layout, NULL);
	elm_naviframe_item_title_enabled_set(s_info.navi_item, EINA_FALSE, EINA_FALSE);

	return s_info.layout;
}

Eina_Bool main_view_create(){

	elm_config_accel_preference_set("opengl");

	/* Create the window */
	s_info.win = view_create_win(PACKAGE);
	if (s_info.win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a window.");
		return EINA_FALSE;
	}

	/* Create the conformant */
	s_info.conform = view_create_conformant(s_info.win);
	if (s_info.conform == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a conformant");
		return EINA_FALSE;
	}

	/* Add new background to conformant */
	if (!_view_create_bg()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create background");
		return EINA_FALSE;
	}

	/* Add naviframe */
	s_info.navi = _app_navi_add();
	if (!s_info.navi) {
		dlog_print(DLOG_ERROR, LOG_TAG, "_app_navi_add() failed");
		return EINA_FALSE;
	}

	/* Add main view to naviframe */
	Evas_Object *view = _main_view_add();

	if (!view)
		dlog_print(DLOG_ERROR, LOG_TAG, "main_view_add() failed");

	evas_object_show(s_info.win);

	if (!s_info.camera_enabled) {
		dlog_print(DLOG_ERROR, LOG_TAG, "_main_view_start_preview failed");
		//_main_view_show_warning_popup(s_info.navi, STR_ERROR, "Camera in use, please restart your phone", STR_OK);
		_exit_show_warning_popup2(s_info.navi, "Exit App", "Camera in use, please restart your phone");
	}
	cameraStartPreviewTimer();
	char cResult[2048] = {0};
	//sprintf(cResult,"media_connect:[%d]\n media_foreach:[%d]\n media_disconnect:[%d]\n mkdir:[%d]\n path:%s", g_iMediaConnectResult, g_iMediaForEach, g_iMediaDisconnectResult, g_iMKDir, s_info.media_content_folder);
	//_main_view_show_warning_popup(s_info.navi, STR_ERROR, cResult, STR_OK);
	//startViewFinderTimerEx(4.0);

	return EINA_TRUE;
}

void setAppPauseState(int bState){
	s_info.bIsAppPaused = bState;
}

