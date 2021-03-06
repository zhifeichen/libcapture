#include "stdafx.h"
#include "X264Encoder.h"

CX264Encoder::CX264Encoder(uv_loop_t* loop)
: b_stop(true)
, h(NULL)
, nal(NULL)
, width(320), height(240)
, i_frame(0)
, i_frame_size(0)
, i_nal(0)
, p_loop(loop)
, p_user_data(NULL)
, fn_cb(NULL)
, CResource(e_rsc_x264encoder)
{
	uv_mutex_init(&queue_mutex);
	uv_cond_init(&queue_not_empty);
}

CX264Encoder::~CX264Encoder()
{
	uv_mutex_destroy(&queue_mutex);
	uv_cond_destroy(&queue_not_empty);
}

int CX264Encoder::set_param(x264_param_t* p)
{
	int ret = -1;
	ret = x264_param_default_preset(&param, "ultrafast", NULL);

	param.i_csp = X264_CSP_I420;// X264_CSP_I420;// X264_CSP_RGB;
	param.i_width = width;
	param.i_height = height;
	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.i_nal_hrd = X264_NAL_HRD_VBR;

	ret = x264_param_apply_profile(&param, "high");
	return ret;
}

int CX264Encoder::open()
{
	int ret = -1;
	if (!h ){
		if (h = x264_encoder_open(&param)){
			return ret;
		}
	}
	else{
		return 0;
	}
	return ret;
}

int CX264Encoder::start_encode(NALCALLBACK cb, void* data)
{
	int ret = -1;
	if (!b_stop) return 0;
	enc_worker_req.data = this;
	b_stop = false;
	p_user_data = data;
	fn_cb = cb;
	ret = uv_queue_work(p_loop, &enc_worker_req, encode_worker, after_encode);
	return ret;
}

int CX264Encoder::stop_encode()
{
	if (b_stop) return 0;
	b_stop = true;
	return 0;
}

int CX264Encoder::put_sample(cc_src_sample_t sample)
{
	uv_mutex_lock(&queue_mutex);
	if (sample_queue.size() > 50){
		uv_mutex_unlock(&queue_mutex);
		free(sample.buf[0]);
		return 0;
	}
	sample_queue.push_back(sample);
	printf("%d", sample_queue.size());
	uv_mutex_unlock(&queue_mutex);
	uv_cond_signal(&queue_not_empty);
	return 0;
}

int CX264Encoder::encode_delay()
{
	int ret = -1;
	ret = x264_encoder_delayed_frames(h);
	return ret;
}

int CX264Encoder::close()
{
	int ret = 0;
	if (h){
		x264_encoder_close(h);
		x264_picture_clean(&pic);
	}
	return ret;
}

static void convert_rgb(uint8_t* dst, uint8_t* src, int w, int h)
{
	int i;
	int pd = 0;
	int l = w * 3;
	for (i = h - 1; i > -1; i--){
		memcpy(dst + pd, src + i*l, l);
		pd += l;
	}
}

static FILE* fd = NULL;

void CX264Encoder::encode()
{
	//MessageBox(NULL, TEXT("1"), TEXT("remote preview"), MB_OK);
	if (x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height) < 0){
		return;
	}
	//MessageBox(NULL, TEXT("2"), TEXT("remote preview"), MB_OK);
	while (!b_stop){
		uv_mutex_lock(&queue_mutex);
		if (sample_queue.size() == 0){
			//uv_mutex_unlock(&queue_mutex);
			int ret = uv_cond_timedwait(&queue_not_empty, &queue_mutex, (uint64_t)(5000 * 1e6));
			if (ret == UV_ETIMEDOUT){
				uv_mutex_unlock(&queue_mutex);
				MessageBox(NULL, TEXT("BUFFER EMPTY!"), TEXT("remote preview"), MB_OK);
				return;
			}
		}

		
		cc_src_sample_t buf = sample_queue.front();
		pic.i_pts = i_frame++;
		int64_t ticks_per_frame = buf.end - buf.start;
		TCHAR m[248];
		swprintf(m, TEXT("len: %d; width: %d; height: %d"), buf.len, param.i_width, param.i_height);
		//MessageBox(NULL, m, TEXT("remote preview"), MB_OK);
		if (param.i_csp == X264_CSP_RGB){
			convert_rgb(pic.img.plane[0], buf.buf[0], param.i_width, param.i_height);
			//memcpy(pic.img.plane[0], buf.buf[0], buf.line[0]*param.i_height);
		}
		else {
			//for (int i = 0; i < buf.iplan; i++){
			//	memcpy(pic.img.plane[i], buf.buf[i], buf.line[i]);
			//}
			memcpy(pic.img.plane[0], buf.buf[0], buf.line[0] * param.i_height);

			memcpy(pic.img.plane[1], buf.buf[1], buf.line[1] * param.i_height / 2);

			memcpy(pic.img.plane[2], buf.buf[2], buf.line[2] * param.i_height / 2);
		}
		//MessageBox(NULL, TEXT("3"), TEXT("remote preview"), MB_OK);
		i_frame_size = x264_encoder_encode(h, &nal, &i_nal, &pic, &pic_out);
		//MessageBox(NULL, TEXT("4"), TEXT("remote preview"), MB_OK);
		if (i_frame_size < 0)
			break;
		else if (i_frame_size)
		{
			//TODO:encode success
			if (fn_cb){
				int i;
				for (i = 0; i < i_nal; i++){
					cc_src_sample_t out_smple;
					out_smple.sync = !!pic_out.b_keyframe;
					out_smple.start = pic_out.i_pts;
					out_smple.end = pic_out.i_pts + ticks_per_frame;
					out_smple.buf[0] = (uint8_t*)malloc(nal[i].i_payload);
					out_smple.len = nal[i].i_payload;
					out_smple.iplan = 1;
					memcpy(out_smple.buf[0], nal[i].p_payload, nal[i].i_payload);
					//MessageBox(NULL, TEXT("5"), TEXT("remote preview"), MB_OK);
					fn_cb(out_smple, p_user_data);
					if (!fd){
						fd = fopen("src.264", "wb");
						if (fd)
							;// MessageBox(NULL, TEXT("create file!"), TEXT("x264 encoder"), MB_OK);
						else
							MessageBox(NULL, TEXT("create file false!"), TEXT("x264 encoder"), MB_OK);
					}
					if (fd){
						fwrite(out_smple.buf[0], 1, out_smple.len, fd);
					}
					free(out_smple.buf[0]);
				}
			}
		}
		//MessageBox(NULL, TEXT("6"), TEXT("remote preview"), MB_OK);
		sample_queue.pop_front();
		for (int i = 0; i < buf.iplan; i++){
			free(buf.buf[i]);
		}
		uv_mutex_unlock(&queue_mutex);
	}

	while (encode_delay()){
		i_frame_size = x264_encoder_encode(h, &nal, &i_nal, NULL, &pic_out);
		if (i_frame_size < 0)
			break;
		else if (i_frame_size)
		{
			//TODO:encode success
		}
	}
}

// static encode worker
void CX264Encoder::encode_worker(uv_work_t* req)
{
	CX264Encoder* self = (CX264Encoder*)req->data;
	self->encode();
}

void CX264Encoder::after_encode(uv_work_t* req, int status)
{
	CX264Encoder* self = (CX264Encoder*)req->data;
	self->close();
}


///////////////////////////////////
// CX264Encoder2
//

int CX264Encoder2::Put(AVFrame* frame)
{
    uv_mutex_lock(pQueueMutex);
    if(queueFrame.size() > 50){
        uv_mutex_unlock(pQueueMutex);
        av_frame_free(&frame);
        return 0;
    }
    queueFrame.push_back(frame);
    printf("%d", queueFrame.size());
    uv_mutex_unlock(pQueueMutex);
    uv_cond_signal(pQueueNotEmpty);
    return 0;
}

void CX264Encoder2::Encode(void)
{
    int ret;
    int gotPic;
    while(!bStop){
        uv_mutex_lock(pQueueMutex);
        if(queueFrame.size() == 0){
            ret = uv_cond_timedwait(pQueueNotEmpty, pQueueMutex, (uint64_t)(500 * 1e6));
            if(ret == UV_ETIMEDOUT){
                uv_mutex_unlock(pQueueMutex);
                MessageBox(NULL, TEXT("BUFFER EMPTY!"), TEXT("remote preview"), MB_OK);
                continue;
            }
        }

        AVFrame* frame = queueFrame.front();
        queueFrame.pop_front();
        uv_mutex_unlock(pQueueMutex);

        /* encode the image */
        AVPacket *pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
        av_init_packet(pkt);
        ret = avcodec_encode_video2(pCodecCtx, pkt, frame, &gotPic);
        if(ret < 0) {
            printf("Error encoding frame\n");
            break;
        }
        if(gotPic) {
            //printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt->size);
            //framecnt++;
            //fwrite(pkt->data, 1, pkt->size, fp_out);
            if(m_fnCb){
                m_fnCb(pkt, m_pUserdata);
            }
        } else{
            av_free_packet(pkt);
            av_freep(pkt);
        }
    }
}

// static encode worker
void CX264Encoder2::EncodeWorker(uv_work_t* req)
{
    CX264Encoder2* self = (CX264Encoder2*)req->data;
    self->Encode();
}

void CX264Encoder2::AfterEncode(uv_work_t* req, int status)
{
    CX264Encoder2* self = (CX264Encoder2*)req->data;
    self->Close();
}