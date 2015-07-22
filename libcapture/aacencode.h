#ifndef __AAC_ENCODE_H__
#define __AAC_ENCODE_H__

#include <stdint.h>
#include "libcapture.h"
#include <deque>
#include "uv/uv.h"
#include "Streams.h"
#include "ResourcePool.h"

#ifdef __cplusplus
extern "C"{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"
}
#endif

typedef void(*NALCALLBACK)(cc_src_sample_t sample, void* user_data);

class aacenc : public CResource
{

	int i_frame;
	int i_frame_size;

	CMediaType m_mt;

	AVCodecContext* m_pCodecCtx;
	AVCodec* m_pCodec;

	AVFrame* m_pFrame;
	uint8_t* frame_buf;

	bool b_stop;
	uv_loop_t* p_loop;
	uv_work_t enc_worker_req;

	void* p_user_data;
	NALCALLBACK fn_cb;

	uv_mutex_t queue_mutex;
	uv_cond_t queue_not_empty;
	std::deque<cc_src_sample_t> sample_queue;

	static void encode_worker(uv_work_t* req);
	static void after_encode(uv_work_t* req, int status);
	void encode();
	int close();
	int encode_delay();

public:
	aacenc(uv_loop_t* loop);
	~aacenc();
	int set_param(const CMediaType* mt = 0);
	int open();
	int put_sample(cc_src_sample_t sample);
	int start_encode(NALCALLBACK cb, void* data);
	int stop_encode();
};

#endif //__AAC_ENCODE_H__