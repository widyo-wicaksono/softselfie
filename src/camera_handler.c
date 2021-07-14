/*
 * camera_handler.c
 *
 *  Created on: Apr 10, 2017
 *      Author: Wicaksono
 */
#include "camera_handler.h"
#include <image_util.h>

/*
#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

*/

int cameraStopPreview(camera_h camera){
	if(camera!=NULL){
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(s_info.camera, &cur_state);

		if (res != CAMERA_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera state. Error: %d", res);
			return -1;
		}
		if (cur_state == CAMERA_STATE_PREVIEW){
			res=camera_stop_preview(camera);
			return res;
		}
		else if(cur_state == CAMERA_STATE_CAPTURED){
			res = camera_start_preview(camera);
			if(res == CAMERA_ERROR_NONE){
				res=camera_stop_preview(camera);
				return res;
			}
		}
		else if (cur_state == CAMERA_STATE_CREATED){
			return CAMERA_ERROR_NONE;
		}
	}
	return -1;
}

int cameraResumePreview(camera_h camera){
	if(camera!=NULL){
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(camera, &cur_state);

		if (res != CAMERA_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera state. Error: %d", res);
			return res;
		}
		if (cur_state != CAMERA_STATE_PREVIEW) {
			res = camera_start_preview(s_info.camera);
			return res;
		}
		return 0;
	}
	else
		return -1;
}

int cameraDestroy(camera_h *pcamera){
	if(*pcamera!=NULL){
		int res=0;
		if(cameraStopPreview(*pcamera) == CAMERA_ERROR_NONE){
			res = camera_destroy(*pcamera);
			*pcamera=NULL;
			return res;
		}
		else
			return -1;
	}
	return -1;
}

void cameraStopPreviewTimer()
{
	if(s_info.timer !=NULL){
		ecore_timer_del(s_info.timer);
		s_info.timer=NULL;
	}
}

Eina_Bool cameraTimer_cb(void *data)
{
	cameraStopPreviewTimer();
	cameraResumePreview(s_info.camera);
	return ECORE_CALLBACK_CANCEL;
}

void cameraStartPreviewTimer()
{
	cameraStopPreviewTimer();
	if(s_info.timer==NULL){
		s_info.timer = ecore_timer_add(0.5, cameraTimer_cb, NULL);
	}
}

void _update_media_db(const char * filename)
{
	if (media_content_connect() == MEDIA_CONTENT_ERROR_NONE)
	{
		if (media_content_scan_file(filename) == MEDIA_CONTENT_ERROR_NONE)
		{
			dlog_print(DLOG_INFO, LOG_TAG, "Media database updated with the new file.");
		}
		media_content_disconnect();
	}
}

unsigned char * manualRotate270(unsigned char * rgb_data, int width, int height){

	unsigned long iTotalPixel = width*height;
	unsigned long iTotalBytesInImage = iTotalPixel*3;
	unsigned long iNewWidth = height;
	unsigned long iNewHeight = width;
	unsigned char * newDimension = (unsigned char *)malloc(sizeof(unsigned char)*iTotalBytesInImage);
	unsigned long iLineLenInByte = width*3;
	unsigned long lCurrentNewIndex=0;
	unsigned long lCurrentOldIndex=0;
	for(unsigned long x=0;x<iNewHeight;x++){
		unsigned long ulColumnIndex = iLineLenInByte - (x*3);
		for(unsigned long y=0;y<iNewWidth;y++){
			lCurrentNewIndex = (iNewWidth*x*3)+y*3;
			lCurrentOldIndex = ulColumnIndex + (iLineLenInByte*y);
			newDimension[lCurrentNewIndex]= rgb_data[lCurrentOldIndex];
			newDimension[lCurrentNewIndex+1]=rgb_data[lCurrentOldIndex+1];
			newDimension[lCurrentNewIndex+2]=rgb_data[lCurrentOldIndex+2];
		}
	}
	free(rgb_data);
	return newDimension;
}

unsigned char * manualRotate90(unsigned char * rgb_data, int width, int height){

	unsigned long iTotalPixel = width*height;
	unsigned long iTotalBytesInImage = iTotalPixel*3;
	unsigned long iNewWidth = height;
	unsigned long iNewHeight = width;
	unsigned char * newDimension = (unsigned char *)malloc(sizeof(unsigned char)*iTotalBytesInImage);
	unsigned long iLineLenInByte = width*3;
	unsigned long lCurrentNewIndex=0;
	unsigned long lCurrentOldIndex=0;
	for(unsigned long x=0;x<iNewHeight;x++){
		unsigned long ulColumnIndex = (x*3) + (iTotalBytesInImage - iLineLenInByte);
		for(unsigned long y=0;y<iNewWidth;y++){
			lCurrentNewIndex = (iNewWidth*x*3)+y*3;
			lCurrentOldIndex = ulColumnIndex - (iLineLenInByte*y);
			newDimension[lCurrentNewIndex]= rgb_data[lCurrentOldIndex];
			newDimension[lCurrentNewIndex+1]=rgb_data[lCurrentOldIndex+1];
			newDimension[lCurrentNewIndex+2]=rgb_data[lCurrentOldIndex+2];
		}
	}
	free(rgb_data);
	return newDimension;
}

unsigned char * nv12ToRGBEx(unsigned char* luma, int iLumaSize, unsigned char* chroma, int iChromaSize, int width, int height){

	unsigned int iPixelCount = height*width;
	unsigned char * rgb_data = (unsigned char *)malloc(iPixelCount*3);
	float R=0;
	float G=0;
	float B=0;

	int chromaConstanta =  width;
	int iPixLine = 0;
	int temp;

	int iHorizontalOffset = 0;
	int iVerticalOffset = 0;

	for(unsigned int i=0;i<iPixelCount;i++){
		temp = i / chromaConstanta;
		if(temp!=iPixLine)
			iPixLine = temp;

		if(iPixLine % 2==0){
			iVerticalOffset =  (chromaConstanta * iPixLine)/2;
		}
		else {
			iVerticalOffset = (chromaConstanta * (iPixLine-1))/2;
		}
		temp = i % chromaConstanta;
		if(temp % 2 ==0){
			iHorizontalOffset = iVerticalOffset + temp;
		}
		else{
			iHorizontalOffset = iVerticalOffset + (temp-1);
		}

		R = 1.164*(luma[i] - 16) + 1.596*(chroma[iHorizontalOffset+1] - 128);
		if(R>255){R=255;}if(R<0){R=0;}

		G = 1.164*(luma[i] - 16) - 0.813*(chroma[iHorizontalOffset+1] - 128) - 0.391*(chroma[iHorizontalOffset] - 128);
		if(G>255){G=255;}if(G<0){G=0;}

		B = 1.164*(luma[i] - 16) + 2.018*(chroma[iHorizontalOffset] - 128);
		if(B>255){B=255;}if(B<0){B=0;}
		R=round(R);
		G=round(G);
		B=round(B);

		rgb_data[i*3] = (char)(int)R;
		rgb_data[i*3+1] = (char)(int)G;
		rgb_data[i*3+2] = (char)(int)B;
	}
	return rgb_data;
}

void oldblend2(unsigned char * r1, unsigned char * g1, unsigned char * b1, unsigned char * r2, unsigned char * g2, unsigned char * b2, float alpha)
{
	// how much should we take from each color?
	float alpha2 = (1-alpha);

	*r1 = (unsigned char)round((*r1*alpha2  + *r2 * alpha));
	*g1 = (unsigned char)round((*g1*alpha2  + *g2 * alpha));
	*b1 = (unsigned char)round((*b1*alpha2  + *b2 * alpha));

}

void superFastBlur2(unsigned char *pix, int w, int h, int mode, int r1, int xc, int yc, int area, int radius){

	if (radius<1) return;
	int wm=w-1;
	int hm=h-1;
	int wh=w*h;

	int r2 = r1+area;
	float alpha;
	int jarak=0;

	int div=radius+radius+1;

	unsigned char *r=(unsigned char*)malloc(sizeof(unsigned char)*wh);
	unsigned char *g=(unsigned char*)malloc(sizeof(unsigned char)*wh);
	unsigned char *b=(unsigned char*)malloc(sizeof(unsigned char)*wh);

	int rsum,gsum,bsum,x,y,i,p,p1,p2,yp,yi,yw;
	int *vMIN = (int*)malloc(sizeof(int)* MAX(w,h));
	int *vMAX = (int*)malloc(sizeof(int)* MAX(w,h));
	unsigned char *dv=(unsigned char*)malloc(sizeof(unsigned char)*256*div);
	for (i=0;i<256*div;i++) dv[i]=(i/div);

	yw=yi=0;

	for (y=0;y<h;y++){
		rsum=gsum=bsum=0;
		for(i=-radius;i<=radius;i++){
			p = (yi + MIN(wm, MAX(i,0))) * 3;
			rsum += pix[p];
			gsum += pix[p+1];
			bsum += pix[p+2];
		}
		for (x=0;x<w;x++){
			r[yi]=dv[rsum];
			g[yi]=dv[gsum];
			b[yi]=dv[bsum];

			if(y==0){
				vMIN[x]=MIN(x+radius+1,wm);
				vMAX[x]=MAX(x-radius,0);
			}
			p1 = (yw+vMIN[x])*3;
			p2 = (yw+vMAX[x])*3;

			rsum += pix[p1] - pix[p2];
			gsum += pix[p1+1] - pix[p2+1];
			bsum += pix[p1+2] - pix[p2+2];

			yi++;
		}
		yw+=w;
	}

	for (x=0;x<w;x++){
		rsum=gsum=bsum=0;
		yp=-radius*w;
		for(i=-radius;i<=radius;i++){
			yi=MAX(0,yp)+x;
			rsum+=r[yi];
			gsum+=g[yi];
			bsum+=b[yi];
			yp+=w;
		}
		yi=x;
		for (y=0;y<h;y++){
			switch(mode){
				case 0:
					jarak = (int) sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc));
					break;
				case 1:
					jarak = (int) abs(yc - y);
					break;
				case 2:
					jarak = (int) abs(xc - x);
					break;
			}

			if(jarak>r1 && jarak<r2){
				alpha = ((float)abs(jarak-r1))/(float)area;
				oldblend2(&pix[yi*3],&pix[yi*3 + 1],&pix[yi*3 + 2],&dv[rsum],&dv[gsum],&dv[bsum],alpha);
				//pix[yi*3] = 0;
				//pix[yi*3 + 1] = 0;
				//pix[yi*3 + 2] = 0;
			}
			else if(jarak<=r1) {
				//alpha = 0;
			}
			else {
				pix[yi*3] = dv[rsum];
				pix[yi*3 + 1] = dv[gsum];
				pix[yi*3 + 2] = dv[bsum];
			}
			if(x==0){
				vMIN[y]=MIN(y+radius+1,hm)*w;
				vMAX[y]=MAX(y-radius,0)*w;
			}
			p1=x+vMIN[y];
			p2=x+vMAX[y];

			rsum+=r[p1]-r[p2];
			gsum+=g[p1]-g[p2];
			bsum+=b[p1]-b[p2];

			yi+=w;
		}
	}

	free(r);
	free(g);
	free(b);
	free(vMIN);
	free(vMAX);
	free(dv);
}

int modify_final_image(camera_image_data_s *image, const char *filename, camera_h cam_handle, int iCamIndex)
{
	if (image->format == CAMERA_PIXEL_FORMAT_NV12)
	{
		dlog_print(DLOG_INFO, LOG_TAG, "We've got NV12 data - data [%p], length [%d], width [%d], height [%d]",
			image->data, image->size, image->width, image->height);

		unsigned long iImSize = image->width*image->height;
		unsigned char * rgb_data = nv12ToRGBEx(image->data, iImSize, image->data+iImSize, iImSize/2, image->width, image->height);

		//int cam_effect = (int)ecore_thread_global_data_find("blur");
		//int iBlurMode = cam_effect%10;

		int cam_effect = (int)ecore_thread_global_data_find("blur");
		int iBlurMode = cam_effect%10;
		if(iBlurMode<1 && iBlurMode>4)
			iBlurMode=3;
		int iMode = cam_effect/ 10 -1;
		if(iMode<0 && iMode>2)
			iMode=0;

		if(iBlurMode<1 && iBlurMode>4)
			iBlurMode=3;

		unsigned long r1=0;
		int radius=18;

		if(iBlurMode==1){
			r1 = (image->height/8)*2;
			if(image->height==1200)
				radius = 8*5;
			else if(image->height==480){
				radius = 8*2;
			}
		}
		else if(iBlurMode==2){
			r1 = (image->height/7)*2;
			//radius = 6*5;
			if(image->height==1200)
				radius = 6*5;
			else if(image->height==480)
				radius = 6*2;

		}
		else if(iBlurMode==3){
			r1 = (image->height/6)*2;
			//radius = 4*5;
			if(image->height==1200)
				radius = 4*5;
			else if(image->height==480)
				radius = 4*2;
		}
		else if(iBlurMode==4){
			r1 =(image->height/5)*2;
			//radius = 2*5;
			if(image->height==1200)
				radius = 2*5;
			else if(image->height==480)
				radius = 2*2;
		}

		if(iMode==1){
			if(image->height==1200)
				radius = radius * 5 *2 ;
			else if(image->height==480)
				radius = radius * 2 *2 ;
		}

		//superFastBlur2(rgb_data,  image->width, image->height,0,r1,image->width/2,image->height/2,image->height/6, radius);
		superFastBlur2(rgb_data,  image->width, image->height,iMode,r1,image->width/2,image->height/2,image->height/6, radius);

		unsigned int width=0;
		unsigned int height=0;
		int lens_orientation = 0;
		if (camera_attr_get_lens_orientation(cam_handle, &lens_orientation)== CAMERA_ERROR_NONE) {
			if(lens_orientation==90 && iCamIndex==0){
				rgb_data = manualRotate90(rgb_data, image->width, image->height);
				height = image->width;
				width = image->height;
			}
			else if(lens_orientation==90 && iCamIndex==1){
				rgb_data = manualRotate270(rgb_data, image->width, image->height);
				height = image->width;
				width = image->height;
			}
			else{
				width=image->width;
				height=image->height;
			}

			//else if(lens_orientation==90 && iCamIndex==1){

			//}
		}
		//int iRes = image_util_encode_jpeg(rgb_data, image->height, image->width, IMAGE_UTIL_COLORSPACE_RGB888 , 100, filename);
		int iRes = image_util_encode_jpeg(rgb_data, width, height, IMAGE_UTIL_COLORSPACE_RGB888 , 100, filename);
		free(rgb_data);
		if(iRes== IMAGE_UTIL_ERROR_NONE)
		{
			dlog_print(DLOG_INFO, LOG_TAG, "Image saved successfully.");
			_update_media_db(filename);

			return 0;
		}
		else{
			dlog_print(DLOG_ERROR, LOG_TAG, "Error occurred when saving image!");
			return iRes;
		}
		dlog_print(DLOG_INFO, LOG_TAG, "The modifications have been applied.");
		return 0;
	}
	else
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "Wrong image format!");
		return -1;
	}
}

unsigned char * nv12ToRGB(camera_preview_data_s *frame){

	unsigned int iPixelCount = frame->height*frame->width;
	unsigned char * rgb_data = (unsigned char *)malloc(iPixelCount*3);
	float R=0;
	float G=0;
	float B=0;

	int chromaConstanta =  frame->width;
	int iPixLine = 0;
	int temp;

	int iHorizontalOffset = 0;
	int iVerticalOffset = 0;

	for(unsigned int i=0;i<iPixelCount;i++){
		temp = i / chromaConstanta;
		if(temp!=iPixLine)
			iPixLine = temp;

		if(iPixLine % 2==0){
			iVerticalOffset =  (chromaConstanta * iPixLine)/2;
		}
		else {
			iVerticalOffset = (chromaConstanta * (iPixLine-1))/2;
		}
		temp = i % chromaConstanta;
		if(temp % 2 ==0){
			iHorizontalOffset = iVerticalOffset + temp;
		}
		else{
			iHorizontalOffset = iVerticalOffset + (temp-1);
		}

		R = 1.164*(frame->data.double_plane.y[i] - 16) + 1.596*(frame->data.double_plane.uv[iHorizontalOffset+1] - 128);
		if(R>255){R=255;}if(R<0){R=0;}

		G = 1.164*(frame->data.double_plane.y[i] - 16) - 0.813*(frame->data.double_plane.uv[iHorizontalOffset+1] - 128) - 0.391*(frame->data.double_plane.uv[iHorizontalOffset] - 128);
		if(G>255){G=255;}if(G<0){G=0;}

		B = 1.164*(frame->data.double_plane.y[i] - 16) + 2.018*(frame->data.double_plane.uv[iHorizontalOffset] - 128);
		if(B>255){B=255;}if(B<0){B=0;}
		R=round(R);
		G=round(G);
		B=round(B);

		rgb_data[i*3] = (char)(int)R;
		rgb_data[i*3+1] = (char)(int)G;
		rgb_data[i*3+2] = (char)(int)B;
	}
	return rgb_data;
}



void RGBTonv12(camera_preview_data_s *frame, unsigned char * rgb_data){

	unsigned int iPixelCount = frame->height*frame->width;

	int chromaConstanta =  frame->width;
	int iPixLine = 0;
	int temp;

	int iChromaIndex=0;
	for(unsigned int i=0;i<iPixelCount;i++){
		int R = rgb_data[i*3];
		int G = rgb_data[i*3+1];
		int B = rgb_data[i*3+2];

		frame->data.double_plane.y[i] =  (0.257 * R) + (0.504 * G) + (0.098 * B) + 16;

		temp = i / chromaConstanta;
		if(temp!=iPixLine)
			iPixLine = temp;
		if(iPixLine%2==0){
			if(i%2==0){
				frame->data.double_plane.uv[iChromaIndex] = -1*(0.148 * R) - (0.291 * G) + (0.439 * B) + 128;
				frame->data.double_plane.uv[iChromaIndex+1] = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128;

				iChromaIndex=iChromaIndex+2;
			}
		}
	}
}

void RGBTonv12Ex(unsigned char* luma, int iLumaSize, unsigned char* chroma, int iChromaSize, int width, int height, unsigned char * rgb_data){

	unsigned int iPixelCount = height*width;

	int chromaConstanta =  width;
	int iPixLine = 0;
	int temp;

	int iChromaIndex=0;
	for(unsigned int i=0;i<iPixelCount;i++){
		int R = rgb_data[i*3];
		int G = rgb_data[i*3+1];
		int B = rgb_data[i*3+2];

		luma[i] =  (0.257 * R) + (0.504 * G) + (0.098 * B) + 16;

		temp = i / chromaConstanta;
		if(temp!=iPixLine)
			iPixLine = temp;
		if(iPixLine%2==0){
			if(i%2==0){
				chroma[iChromaIndex] = -1*(0.148 * R) - (0.291 * G) + (0.439 * B) + 128;
				chroma[iChromaIndex+1] = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128;

				iChromaIndex=iChromaIndex+2;
			}
		}
	}
}



void _camera_preview_callback(camera_preview_data_s *frame, void *user_data)
{
	int cam_effect = (int)ecore_thread_global_data_find("blur");
	int iBlurMode = cam_effect%10;
	if(iBlurMode<1 && iBlurMode>4)
		iBlurMode=3;
	int iMode = cam_effect/ 10 -1;
	if(iMode<0 && iMode>2)
		iMode=0;

	unsigned long r1=0;
	int radius=4;

	if(iBlurMode==1){
		r1 = (frame->height/8)*2;
		radius = 8;
	}
	else if(iBlurMode==2){
		r1 = (frame->height/7)*2;
		radius = 6;
	}
	else if(iBlurMode==3){
		r1 = (frame->height/6)*2;
		radius = 4;
	}
	else if(iBlurMode==4){
		r1 =(frame->height/5)*2;
		radius = 2;
	}

	if(iMode==1)
		radius = radius *2;

	unsigned char * rgb_data = nv12ToRGB(frame);
	//superFastBlur2(rgb_data,  frame->width, frame->height,0,r1,frame->width/2,frame->height/2,frame->height/6, radius);
	superFastBlur2(rgb_data,  frame->width, frame->height,iMode,r1,frame->width/2,frame->height/2,frame->height/6, radius);
	RGBTonv12(frame,rgb_data);
	free(rgb_data);
}


int prepare_camera(camera_h camera)
{
	if ((camera_set_capture_format(camera, CAMERA_PIXEL_FORMAT_NV12) == CAMERA_ERROR_NONE)
				&& (camera_set_preview_cb(camera, _camera_preview_callback, NULL) == CAMERA_ERROR_NONE))
	{
		dlog_print(DLOG_INFO, LOG_TAG, "The camera has been prepared for an image modification.");
		return 0;
	}
	else
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "Error occurred when preparing camera!");
		return -1;
	}
}

static void rotate_image_preview(camera_h camera, int lens_orientation)
{
	int display_rotation_angle = (lens_orientation) % 360;

	camera_rotation_e rotation = CAMERA_ROTATION_NONE;
	switch (display_rotation_angle) {
	case 0:
		break;
	case 90:
		rotation = CAMERA_ROTATION_270;
		break;
	case 180:
		rotation = CAMERA_ROTATION_180;
		break;
	case 270:
		rotation = CAMERA_ROTATION_90;
		break;
	default:
		dlog_print(DLOG_ERROR, LOG_TAG, "Wrong lens_orientation value");
		return;
	}
	camera_set_display_rotation(camera, rotation);
}

static void rotate_image_preview2(camera_h camera, int lens_orientation)
{
	int display_rotation_angle = (lens_orientation) % 360;

	camera_rotation_e rotation = CAMERA_ROTATION_NONE;
	switch (display_rotation_angle) {
	case 0:
		break;
	case 90:
		rotation = CAMERA_ROTATION_90;
		break;
	case 180:
		rotation = CAMERA_ROTATION_180;
		break;
	case 270:
		rotation = CAMERA_ROTATION_270;
		break;
	default:
		dlog_print(DLOG_ERROR, LOG_TAG, "Wrong lens_orientation value");
		return;
	}

	camera_set_display_rotation(camera, rotation);
}

static bool _camera_supported_preview_resolution_cb(int width, int height, void *user_data){

	struct view_info * pData = (struct view_info *) user_data;
	if(width < 480 && height < 360){
		pData->prev_width = width;
		pData->prev_height = height;
		return true;
	}
	else
		return false;
}

Eina_Bool cameraInit(int iCamIndex)
{
	int lens_orientation = 0;
	//int result = camera_create(CAMERA_DEVICE_CAMERA0, &s_info.camera);
	int result = camera_create(iCamIndex, &s_info.camera);
	if (result != CAMERA_ERROR_NONE || !s_info.preview_canvas)
		return false;

	result = camera_set_display(s_info.camera, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(s_info.preview_canvas));
	if (result != CAMERA_ERROR_NONE || !s_info.preview_canvas){
		return false;
	}

	camera_set_display_mode(s_info.camera, CAMERA_DISPLAY_MODE_LETTER_BOX);
	//camera_set_display_mode(s_info.camera, CAMERA_DISPLAY_MODE_FULL);
	camera_set_display_visible(s_info.camera, true);

	if(camera_attr_set_image_quality(s_info.camera, 100)!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set image quality");
	}

	camera_foreach_supported_preview_resolution(s_info.camera, _camera_supported_preview_resolution_cb, &s_info);
	if (camera_set_preview_resolution(s_info.camera,s_info.prev_width, s_info.prev_height)!= CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get set preview resolution");
		return false;
	}

	if(camera_attr_set_preview_fps(s_info.camera,CAMERA_ATTR_FPS_AUTO )!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set PREVIEW FPS");
	}
	if (camera_attr_get_lens_orientation(s_info.camera, &lens_orientation)== CAMERA_ERROR_NONE) {
		if(iCamIndex==CAMERA_DEVICE_CAMERA0){
			rotate_image_preview(s_info.camera, lens_orientation);
		}
		else if(iCamIndex==CAMERA_DEVICE_CAMERA1){
			rotate_image_preview2(s_info.camera, lens_orientation);
			camera_set_display_flip(s_info.camera, CAMERA_FLIP_HORIZONTAL);
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera lens attribute");
	}

	result = prepare_camera(s_info.camera);
	if(result==-1)
		return false;
	return true;
}

Eina_Bool cameraInitEx(int iCamIndex, int *pError, int *pErrCode)
{
	/*
	 Error value
	 0 = OK
	 -1 = Fail, unknown
	 -2 = Cam create fail
	 -3 = Others
	 */
	*pError = -2;
	int lens_orientation = 0;
	int result = camera_create(iCamIndex, &s_info.camera);
	if (result != CAMERA_ERROR_NONE || !s_info.preview_canvas){
		*pError = -2;
		*pErrCode = result;
		return false;
	}

	result = camera_set_display(s_info.camera, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(s_info.preview_canvas));
	if (result != CAMERA_ERROR_NONE || !s_info.preview_canvas){
		*pError = -3;
		*pErrCode = result;
		return false;
	}

	int width,height;
	if(iCamIndex==0){
		width = 1600;
		height = 1200;
	}
	else {
		width = 640;
		height = 480;
	}

	if(camera_set_capture_resolution(s_info.camera, width, height)!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set camera resolution");
		*pError = -3;
		*pErrCode = result;
		return false;
	}

	result = camera_set_display_mode(s_info.camera, CAMERA_DISPLAY_MODE_LETTER_BOX);
	if (result != CAMERA_ERROR_NONE) {
		*pError = -3;
		*pErrCode = result;
		return false;
	}
	result = camera_set_display_visible(s_info.camera, true);
	if (result != CAMERA_ERROR_NONE) {
		*pError = -3;
		*pErrCode = result;
		return false;
	}

	if(camera_attr_set_image_quality(s_info.camera, 100)!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set image quality");
	}

	if(camera_foreach_supported_preview_resolution(s_info.camera, _camera_supported_preview_resolution_cb, &s_info)!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set resolution callback");
	}
	if(camera_set_preview_resolution(s_info.camera,s_info.prev_width, s_info.prev_height)!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set preview resolution");
	}

	if(camera_attr_set_preview_fps(s_info.camera,CAMERA_ATTR_FPS_AUTO )!=CAMERA_ERROR_NONE){
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot set PREVIEW FPS");
	}
	if (camera_attr_get_lens_orientation(s_info.camera, &lens_orientation)== CAMERA_ERROR_NONE) {
		if(iCamIndex==CAMERA_DEVICE_CAMERA0){
			rotate_image_preview(s_info.camera, lens_orientation);
		}
		else if(iCamIndex==CAMERA_DEVICE_CAMERA1){
			rotate_image_preview2(s_info.camera, lens_orientation);
			camera_set_display_flip(s_info.camera, CAMERA_FLIP_HORIZONTAL);
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera lens attribute");
	}

	result = prepare_camera(s_info.camera);
	if(result==-1)
		return false;
	return true;
}

int cameraSwitch(){
	if(s_info.camera!=NULL){
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(s_info.camera, &cur_state);

		if (res != CAMERA_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera state. Error: %d", res);
			return res;
			}
		if (cur_state == CAMERA_STATE_PREVIEW) {
			cameraStopPreviewTimer();
			cameraStopPreview(s_info.camera);
			cameraDestroy(&s_info.camera);
			if(s_info.iCamIndex){
				s_info.iCamIndex=0;
			}
			else
				s_info.iCamIndex=1;

			cameraInit(s_info.iCamIndex);
			cameraStartPreviewTimer();
		}
		return 0;
	}
	else
		return -1;
}


int cameraSwitchEx(int * pErr, int * pErrCode){
	if(s_info.camera!=NULL){
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(s_info.camera, &cur_state);

		if (res != CAMERA_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera state. Error: %d", res);
			return res;
		}
		if (cur_state == CAMERA_STATE_PREVIEW) {
			cameraStopPreviewTimer();
			cameraStopPreview(s_info.camera);
			cameraDestroy(&s_info.camera);
			if(s_info.iCamIndex){
				s_info.iCamIndex=0;
			}
			else
				s_info.iCamIndex=1;
			res = cameraInitEx(s_info.iCamIndex,pErr,pErrCode);
			if(res==1)
				cameraStartPreviewTimer();
			else{
				return -1;
			}
		}
		return 0;
	}
	else
		return -1;
}

int cameraIsSettingReadyToBeChanged(camera_h camera){
	if(camera!=NULL){
		camera_state_e cur_state = CAMERA_STATE_NONE;
		int res = camera_get_state(s_info.camera, &cur_state);

		if (res != CAMERA_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Cannot get camera state. Error: %d", res);
			return res;
		}
		else {
			return 0;
		}
	}
	return -1;
}
