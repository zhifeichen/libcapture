#ifndef __X264_ENCODE_H__
#define __X264_ENCODE_H__

#include <stdint.h>
#include "libcapture.h"
#include <deque>
#include "uv\uv.h"
#include "ResourcePool.h"

#ifdef __cplusplus
extern "C"{
#include "x264.h"
}
#endif

typedef void(*NALCALLBACK)(cc_src_sample_t sample, void* user_data);

class CX264Encoder : public CResource
{
	int width, height;
	x264_param_t param;
	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_t *h;
	int i_frame;
	int i_frame_size;
	x264_nal_t *nal;
	int i_nal;

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
	CX264Encoder(uv_loop_t* loop);
	~CX264Encoder();
	int set_param(x264_param_t* param = 0);
	int open();
	int put_sample(cc_src_sample_t sample);
	int start_encode(NALCALLBACK cb, void* data);
	int stop_encode();
};

#endif //__X264_ENCODE_H__