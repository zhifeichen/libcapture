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

class CVideoDecoder : public CResource
{
	uint8_t			   *iobuffer;
	AVIOContext		   *avio;

	AVFormatContext    *pFormatCtx;
	int				    videoStream;
	AVCodecContext     *pCodecCtxOrig;
	AVCodecContext     *pCodecCtx;
	AVCodec            *pCodec;
	AVFrame            *pFrame;
	AVFrame			   *pFrameYUV;
	AVPacket            packet;
	int                 frameFinished;
	float               aspect_ratio;
	struct SwsContext  *sws_ctx;
	SDL_Renderer	   *renderer;

	SDL_Texture        *texture;
	SDL_Window         *screen;
	SDL_Rect            rect_src, rect_dst;
	SDL_Event           event;

	std::deque<cc_src_sample_t> buf_deque;
	HWND			hWin;

	// uv:
	uv_loop_t		*pLoop;
	uv_mutex_t		queue_mutex;
	uv_cond_t		queue_not_empty;

	static int fill_iobuffer(void * opaque, uint8_t *buf, int bufsize);
	int fill_iobuffer(uint8_t *buf, int bufsize);

	bool            bInit;
	bool			bStop;
	bool			bOpen;
	bool			bStarting;
	int				iFrame;
	uv_work_t		decode_worker_req;
	static void decode_worker(uv_work_t* req);
	static void after_decode(uv_work_t* req, int status);
	void decode(void);

private:
	friend class CResourcePool;
	/* only get point through CResourcePool */
	CVideoDecoder(uv_loop_t* loop);
	virtual ~CVideoDecoder();

public:
	int init(HWND w);
	int resizewindow(void);
	int open(void);
	int start();
	int stop();
	int close();
	int finit();
	int put(cc_src_sample_t buf);
	int put(uint8_t* buf, int len);
};

typedef void(*DECODE_FRAME_FN)(AVFrame*, int, void*);

class CVideoDecoder2 : public CResource
{
	AVCodecID				codecId;
	AVCodec				   *pCodec;
	AVCodecContext		   *pCodecCtx;
	AVCodecParserContext   *pCodecParserCtx;
	AVPacket			   *pPacket;
	AVFrame				   *pFrame;

	struct SwsContext	   *pConvertCtx;
	AVFrame				   *pFrameYUV;

	uv_loop_t			   *pLoop;

	std::deque<AVPacket*>	packetQueue;
	uv_mutex_t			   *pQueueMutex;
	uv_cond_t			   *pQueueNotEmpty;

	bool					bInit;
    bool                    bStop;

	uv_work_t				decodeWorkerReq;
	static void DecodeWorker(uv_work_t* req);
	static void AfterDecode(uv_work_t* req, int status);
	void Decode(void);

    DECODE_FRAME_FN         fnCallback;
    void                   *pUserData;
private:
	friend class CResourcePool;
	/* only get point through CResourcePool */
	CVideoDecoder2(uv_loop_t* loop);
	virtual ~CVideoDecoder2();

    void Finit(void);
public:
	int Init(void);
    int SetFrameCallback(DECODE_FRAME_FN cb, void* data);
    int Put(const uint8_t* buf, int len);
    int Stop(void);
    void Close(CLOSERESOURCECB cb = NULL);
};

#endif //__DECODE_VIDEO_H__