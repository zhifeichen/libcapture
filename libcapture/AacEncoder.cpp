#include "stdafx.h"
#include "AacEncoder.h"

CAacEncoder::CAacEncoder(uv_loop_t* loop)
: b_stop(true)
, i_frame(0)
, i_frame_size(0)
, p_loop(loop)
, p_user_data(NULL)
, fn_cb(NULL)
, CResource(e_rsc_aacencoder)
{
	uv_mutex_init(&queue_mutex);
	uv_cond_init(&queue_not_empty);
}

CAacEncoder::~CAacEncoder()
{
	uv_mutex_destroy(&queue_mutex);
	uv_cond_destroy(&queue_not_empty);
}

int CAacEncoder::set_param(const CMediaType* mt)
{
	int ret = -1;
	if (mt){
		m_mt = *mt;
		return 0;
	}
	return ret;
}

int CAacEncoder::open()
{
	int ret = -1;

	WAVEFORMATEX *pai = (WAVEFORMATEX *)m_mt.pbFormat;

	av_register_all();

	m_pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!m_pCodec){
		printf("Can not find encoder!\n");
		return -1;
	}
	m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
	//pCodecCtx->codec_id = fmt->audio_codec;
	m_pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	if (pai->wBitsPerSample == 16){
		m_pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;//AV_SAMPLE_FMT_U8
	} else if (pai->wBitsPerSample == 8) {
		m_pCodecCtx->sample_fmt = AV_SAMPLE_FMT_U8;
	}
	m_pCodecCtx->sample_rate = pai->nSamplesPerSec;
	m_pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	m_pCodecCtx->channels = pai->nChannels;
	m_pCodecCtx->bit_rate = 64000;

	if (avcodec_open2(m_pCodecCtx, m_pCodec, NULL) < 0){
		printf("Failed to open encoder!\n");
		return -1;
	}

	m_pFrame = av_frame_alloc();
	m_pFrame->nb_samples = m_pCodecCtx->frame_size;
	m_pFrame->format = m_pCodecCtx->sample_fmt;

    // 4096 byte
	i_frame_size = av_samples_get_buffer_size(NULL, m_pCodecCtx->channels, m_pCodecCtx->frame_size, m_pCodecCtx->sample_fmt, 1);
	frame_buf = (uint8_t *)av_malloc(i_frame_size);
	avcodec_fill_audio_frame(m_pFrame, m_pCodecCtx->channels, m_pCodecCtx->sample_fmt, (const uint8_t*)frame_buf, i_frame_size, 1);
	return ret;
}

int CAacEncoder::start_encode(NALCALLBACK cb, void* data)
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

int CAacEncoder::stop_encode()
{
	if (b_stop) return 0;
	b_stop = true;
	return 0;
}

int CAacEncoder::put_sample(cc_src_sample_t sample)
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

int CAacEncoder::encode_delay()
{
	int ret = -1;
	return ret;
}

int CAacEncoder::close()
{
	int ret = 0;
	
	return ret;
}

static FILE* fd = NULL;

void CAacEncoder::encode()
{
	//MessageBox(NULL, TEXT("1"), TEXT("remote preview"), MB_OK);
	
	//MessageBox(NULL, TEXT("2"), TEXT("remote preview"), MB_OK);
	int pos = 0;
	int got_frame = 0;
	AVPacket pkt;
	av_new_packet(&pkt, i_frame_size);
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
		i_frame++;
		int64_t ticks_per_frame = buf.end - buf.start;

		if (pos > 0){
			memcpy(frame_buf + pos, buf.buf[0], i_frame_size - pos);
			int ret = avcodec_encode_audio2(m_pCodecCtx, &pkt, m_pFrame, &got_frame);
			if (ret < 0)
				break;
			else if (got_frame == 1){
				//TODO:encode success
				if (fn_cb){

					cc_src_sample_t out_smple;
					out_smple.sync = !!0;
					out_smple.start = 0;
					out_smple.end = 0;
					out_smple.buf[0] = (uint8_t*)malloc(pkt.size);
					out_smple.len = pkt.size;
					out_smple.iplan = 1;
					memcpy(out_smple.buf[0], pkt.data, pkt.size);
					//MessageBox(NULL, TEXT("5"), TEXT("remote preview"), MB_OK);
					fn_cb(out_smple, p_user_data);
					if (!fd){
						fd = fopen("src.aac", "wb");
						if (fd)
							MessageBox(NULL, TEXT("create file!"), TEXT("aac encoder"), MB_OK);
						else
							MessageBox(NULL, TEXT("create file false!"), TEXT("aac encoder"), MB_OK);
					}
					if (fd){
						fwrite(out_smple.buf[0], 1, out_smple.len, fd);
					}
					free(out_smple.buf[0]);
				}
				av_free_packet(&pkt);
			}
		}
		while (pos <= buf.len - i_frame_size){
			memcpy(frame_buf, buf.buf[0] + pos, i_frame_size);
			pos += i_frame_size;
			int ret = avcodec_encode_audio2(m_pCodecCtx, &pkt, m_pFrame, &got_frame);
			if (ret < 0)
				break;
			else if (got_frame == 1){
				//TODO:encode success
				if (fn_cb){

					cc_src_sample_t out_smple;
					out_smple.sync = !!0;
					out_smple.start = 0;
					out_smple.end = 0;
					out_smple.buf[0] = (uint8_t*)malloc(pkt.size);
					out_smple.len = pkt.size;
					out_smple.iplan = 1;
					memcpy(out_smple.buf[0], pkt.data, pkt.size);
					//MessageBox(NULL, TEXT("5"), TEXT("remote preview"), MB_OK);
					fn_cb(out_smple, p_user_data);
					if (!fd){
						fd = fopen("src.aac", "wb");
						if (fd)
							;//MessageBox(NULL, TEXT("create file!"), TEXT("aac encoder"), MB_OK);
						else
							MessageBox(NULL, TEXT("create file false!"), TEXT("aac encoder"), MB_OK);
					}
					if (fd){
						fwrite(out_smple.buf[0], 1, out_smple.len, fd);
					}
					free(out_smple.buf[0]);
				}
				av_free_packet(&pkt);
			}
		}
		if (buf.len - pos > 0){
			memcpy(frame_buf, buf.buf[0] + pos, buf.len - pos);
			pos = buf.len - pos;
		}
		else {
			pos = 0;
		}
		//MessageBox(NULL, TEXT("6"), TEXT("remote preview"), MB_OK);
		sample_queue.pop_front();
		for (int i = 0; i < buf.iplan; i++){
			free(buf.buf[i]);
		}
		uv_mutex_unlock(&queue_mutex);
	}

	while (encode_delay()){
		i_frame_size = 0;
		if (i_frame_size < 0)
			break;
		else if (i_frame_size)
		{
			//TODO:encode success
		}
	}
}

// static encode worker
void CAacEncoder::encode_worker(uv_work_t* req)
{
	CAacEncoder* self = (CAacEncoder*)req->data;
	self->encode();
}

void CAacEncoder::after_encode(uv_work_t* req, int status)
{
	CAacEncoder* self = (CAacEncoder*)req->data;
	self->close();
}