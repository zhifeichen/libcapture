#ifndef __DECODE_AUDIO_H__
#define __DECODE_AUDIO_H__

#include <deque>
#include "uv/uv.h"
#include "ResourcePool.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __cplusplus
}
#endif

#include "libcapture.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

class CAudioDecoder : public CResource
{
	uint8_t			*iobuffer;
	AVIOContext		*avio;

	AVFormatContext *pFormatCtx;
	int             audioStream;
	AVCodecContext  *pCodecCtxOrig;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame;
	AVFrame			*pFrameYUV;
	AVPacket        packet;
	int             frameFinished;
	float           aspect_ratio;
	struct SwsContext *sws_ctx;
	struct SwrContext *au_convert_ctx;
	SDL_Renderer	*renderer;

	SDL_AudioSpec wanted_spec;
	SDL_Texture     *bmp;
	SDL_Window      *screen;
	SDL_Rect        rect_src, rect_dst;
	SDL_Event       event;

	std::deque<cc_src_sample_t> buf_deque;

	// uv:
	uv_loop_t		*pLoop;
	uv_mutex_t		queue_mutex;
	uv_cond_t		queue_not_empty;

	//Out Audio Param
	uint64_t		out_channel_layout;
	int				out_nb_samples;
	AVSampleFormat	out_sample_fmt;
	int				out_sample_rate;
	int				out_channels;
	//Out Buffer Size
	int				out_buffer_size;
	uint8_t			*out_buffer;

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
	CAudioDecoder(uv_loop_t* loop);
	~CAudioDecoder();

public:
	int init(void);
	int open(void);
	int start();
	int stop();
	int close();
	int finit();
	int put(cc_src_sample_t buf);
	int put(uint8_t* buf, int len);
};

#endif //__DECODE_AUDIO_H__