#define LOG_NDEBUG 0
#define LOG_TAG "Camera"
#include "aw/CDX_Debug.h"

#include "V4L2.h"
#include "camera.h"

typedef struct CameraParm
{
	int deviceid;
	int framerate;
	int width;
	int height;
}CameraParm;


int CameraGetOneframe(void* v4l2ctx, struct v4l2_buffer *buffer)
{
	int ret;

	/* Wait till FPS timeout expires, or thread exit message is received. */
	ret = v4l2WaitCameraReady(v4l2ctx);
	if (ret != 0)
	{
		LOGE("wait v4l2 buffer time out");
		return __LINE__;
	}
	
	// get one video frame
	memset(buffer, 0, sizeof(struct v4l2_buffer));
	ret = getPreviewFrame(v4l2ctx, buffer);
	if (ret != 0)
	{
		return ret;
	}

	return 0;
}

void CameraReturnOneframe(void* v4l2ctx, int id)
{
	releasePreviewFrame(v4l2ctx, id);
	return;
}

void *CreateCameraContext()
{
	return CreateV4l2Context();
}

void DestroyCameraContext(void* v4l2ctx)
{

	DestroyV4l2Context(v4l2ctx);
}

int OpenCamera(void* v4l2ctx)
{
	return openCameraDevice(v4l2ctx);
}

void CloseCamera(void* v4l2ctx)
{
	closeCameraDevice(v4l2ctx);
}

int ConfigureCamera(void* v4l2ctx, int *width, int *height, int framerate)
{
	int ret = 0;
	int pix_fmt = V4L2_PIX_FMT_NV12;
	//int pix_fmt = V4L2_PIX_FMT_YUYV;
	int mframerate = framerate;
	struct v4l2_streamparm params;

	// set capture mode

	memset(&params, 0, sizeof(params));
	params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	params.parm.capture.timeperframe.numerator = 1;
	params.parm.capture.timeperframe.denominator = mframerate;
	params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	params.parm.capture.capturemode = V4L2_MODE_VIDEO;

	ret =v4l2setCaptureParams(v4l2ctx, &params);
	if (ret)
		return ret;

	// set v4l2 device parameters
	ret = v4l2SetVideoParams(v4l2ctx, width, height, pix_fmt);
	if (ret)
		return ret;

	// set fps
	//ret =v4l2setCaptureParams(v4l2ctx,&params);
	//if (ret)
	//	return ret;

	return 0;
}

int StartCamera(void* v4l2ctx)
{
	int ret = 0;
	int mbuffernuber = BUFFER_NUMBER;
	
	// v4l2 request buffers
	ret =v4l2ReqBufs(v4l2ctx, &mbuffernuber);
	if (ret)
			return ret;

	// v4l2 query buffers
	ret =v4l2QueryBuf(v4l2ctx);
	if (ret)
			return ret;
	
	// stream on the v4l2 device
	ret =v4l2StartStreaming(v4l2ctx);
	if (ret)
			return ret;

    return 0;
}

int StopCamera(void* v4l2ctx)
{
	LOGV("stopCamera");
	
	// v4l2 device stop stream
	v4l2StopStreaming(v4l2ctx);

	// v4l2 device unmap buffers
    v4l2UnmapBuf(v4l2ctx);

    return 0;
}
