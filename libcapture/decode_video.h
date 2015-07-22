#ifndef __DECODE_VIDEO_H__
#define __DECODE_VIDEO_H__

#include <deque>
#include "uv/uv.h"
#include "ResourcePool.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __cplusplus
}
#endif

#include "libcapture.h"

class video_decoder : public CResource
{
	uint8_t			*iobuffer;
	AVIOContext		*avio;

	AVFormatContext *pFormatCtx;
	int             videoStream;
	AVCodecContext  *pCodecCtxOrig;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame;
	AVFrame			*pFrameYUV;
	AVPacket        packet;
	int             frameFinished;
	float           aspect_ratio;
	struct SwsContext *sws_ctx;
	SDL_Renderer	*renderer;

	SDL_Texture     *bmp;
	SDL_Window      *screen;
	SDL_Rect        rect_src, rect_dst;
	SDL_Event       event;

	std::deque<cc_src_sample_t> buf_deque;
	HWND			hWin;

	// uv:
	uv_loop_t		*pLoop;
	uv_mutex_t		queue_mutex;
	uv_cond_t		queue_not_empty;

	static int fill_iobuffer(void * opaque, uint8_t *buf, int bufsize);
	int fill_iobuffer(uint8_t *buf, int bufsize);

	bool			bStop;
	bool			bOpen;
	bool			bStarting;
	int				iFrame;
	uv_work_t		decode_worker_req;
	static void decode_worker(uv_work_t* req);
	static void after_decode(uv_work_t* req, int status);
	void decode(void);

public:
	video_decoder();
	~video_decoder();

	int init(uv_loop_t* loop, HWND w);
	int resizewindow(void);
	int open(void);
	int start();
	int stop();
	int close();
	int finit();
	int put(cc_src_sample_t buf);
	int put(uint8_t* buf, int len);
};

#endif //__DECODE_VIDEO_H__