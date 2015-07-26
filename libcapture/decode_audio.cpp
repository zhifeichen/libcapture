#include "stdafx.h"
#include "decode_audio.h"

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

/* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*/
void  fill_audio(void *udata, Uint8 *stream, int len){
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len>audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}
//-----------------

audio_decoder::audio_decoder()
: pFormatCtx(NULL), pCodecCtxOrig(NULL)
, pCodecCtx(NULL), pCodec(NULL)
, pFrame(NULL), sws_ctx(NULL), pFrameYUV(NULL)
, bmp(NULL), screen(NULL)
, renderer(NULL)
, bStop(true), bOpen(false), bStarting(false)
, iFrame(0)
, CResource(e_rsc_audiodecode)
{
}

audio_decoder::~audio_decoder()
{
}

int audio_decoder::init(uv_loop_t* loop)
{
	int ret = 0;
	pLoop = loop;

    uv_mutex_init(&queue_mutex);
    uv_cond_init(&queue_not_empty);

	// Register all formats and codecs
	av_register_all();

	if (ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
	}

	return ret;
}

int audio_decoder::finit()
{
	int ret = 0;
	uv_mutex_destroy(&queue_mutex);
	uv_cond_destroy(&queue_not_empty);
	return ret;
}

int audio_decoder::open(void)
{
	int ret = 0;
	bOpen = true;
	pFormatCtx = avformat_alloc_context();
	const int BUF_LEN = (1024 * 200);
	iobuffer = (unsigned char *)av_malloc(BUF_LEN);
	avio = avio_alloc_context(iobuffer, BUF_LEN, 0, this, fill_iobuffer, NULL, NULL);
	pFormatCtx->pb = avio;
	pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;
	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0){
		bOpen = false;
		return -1; // Couldn't open file
	}

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL)<0){
		bOpen = false;
		return -1; // Couldn't find stream information
	}

	// Find the first video stream
	audioStream = -1;
	unsigned int i;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
	if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
		audioStream = i;
		break;
	}
	if (audioStream == -1){
		bOpen = false;
		return -1; // Didn't find a video stream
	}
	/*AV_SAMPLE_FMT_S16;*/
	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[audioStream]->codec;
	if (!pCodecCtxOrig){
		fprintf(stderr, "Unsupported codec!\n");
		bOpen = false;
		return -1; // Codec not found
	}
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		bOpen = false;
		return -1; // Codec not found
	}

	// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy codec context");
		bOpen = false;
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0){
		bOpen = false;
		return -1; // Could not open codec
	}

	// Allocate audio frame
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	av_init_packet(&packet);
	out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	//Out Audio Param
	out_channel_layout = AV_CH_LAYOUT_STEREO;
	out_nb_samples = 1024;
	out_sample_fmt = AV_SAMPLE_FMT_S16;
	out_sample_rate = 44100;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size
	out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = 1024/*out_nb_samples*/;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = pCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, NULL)<0){
		printf("can't open audio.\n");
		return -1;
	}

	int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);

	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
	swr_init(au_convert_ctx);


	bOpen = true;
	//bStop = false;
	//start();
	return ret;
}

int audio_decoder::start()
{
	int ret = 0;
	if (!bStop || bStarting) return 0;
	decode_worker_req.data = this;
	bStop = false;
	//bOpen = true;
	bStarting = true;

	ret = uv_queue_work(pLoop, &decode_worker_req, decode_worker, after_decode);
	return ret;
}

void audio_decoder::decode(void)
{
	// Read frames and save first five frames to disk
	//if (!bOpen && buf_deque.size() >2){
		if (open() < 0) 
			return;
	//}

	int ret;

	while (!bStop && (ret = av_read_frame(pFormatCtx, &packet)) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == audioStream) {
			ret = avcodec_decode_audio4(pCodecCtx, pFrame, &frameFinished, &packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				break;
			}
			if (frameFinished > 0){
				swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data, pFrame->nb_samples);

				//FIX:FLAC,MP3,AAC Different number of samples
				if (wanted_spec.samples != pFrame->nb_samples){
					SDL_CloseAudio();
					out_nb_samples = pFrame->nb_samples;
					out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
					wanted_spec.samples = out_nb_samples;
					SDL_OpenAudio(&wanted_spec, NULL);
				}
			}
			//Set audio buffer (PCM data)
			audio_chunk = (Uint8 *)out_buffer;
			//Audio buffer length
			audio_len = out_buffer_size;

			audio_pos = audio_chunk;
			//Play
			SDL_PauseAudio(0);
			while (audio_len>0)//Wait until finish
				SDL_Delay(1);
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			SDL_Quit();
			return;
			break;
		default:
			break;
		}

	}
	if (ret < 0){
		const char* err = SDL_GetError();
		printf("%s", err);
	}
}

int audio_decoder::stop()
{
	int ret = 0;
	bStop = true;
	return ret;
}

int audio_decoder::close()
{
	int ret = 0;
	SDL_DestroyTexture(bmp);
	// Free the YUV frame
	av_frame_free(&pFrame);

    av_freep(iobuffer);

	// Close the codec
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

    finit();

	bOpen = false;
	bStarting = false;
	return ret;
}

int audio_decoder::put(cc_src_sample_t buf)
{
	int ret = 0;
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() > 50){
		uv_mutex_unlock(&queue_mutex);
		free(buf.buf);
		return ret;
	}
	buf_deque.push_back(buf);
	iFrame++;
	uv_mutex_unlock(&queue_mutex);
	uv_cond_signal(&queue_not_empty);

	if (!bOpen && buf_deque.size() > 0){
		open();
	}
	return ret;
}

static FILE* fd = NULL;

int audio_decoder::put(uint8_t* buf, int len)
{
	int ret = 0;
	
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() > 5000){
		uv_mutex_unlock(&queue_mutex);
		//free(buf);
		return ret;
	}
	cc_src_sample_t s;
	s.buf[0] = (uint8_t*)malloc(len);
	s.len = len;
	memcpy(s.buf[0], buf, s.len);
	buf_deque.push_back(s);
	iFrame++;
	uv_mutex_unlock(&queue_mutex);
	uv_cond_signal(&queue_not_empty);

	if (!bOpen && buf_deque.size() > 3){
		start();
		bOpen = true;
	}
	return ret;
}

int audio_decoder::fill_iobuffer(uint8_t *buf, int bufsize)
{
	int ret = 0;
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() == 0){
		//uv_mutex_unlock(&queue_mutex);
		int ret = uv_cond_timedwait(&queue_not_empty, &queue_mutex, (uint64_t)(1000 * 1e6));
		if (ret == UV_ETIMEDOUT){
			uv_mutex_unlock(&queue_mutex);
			//MessageBox(NULL, TEXT("BUFFER EMPTY!"), TEXT("remote preview"), MB_OK);
			return 1;
		}
	}
	int real_len = 0;
	int buf_rest = bufsize;
	int isize = buf_deque.size();
	for (int i = 0; i < isize; i++){
		cc_src_sample_t s = buf_deque.front();
		if (buf_rest >= s.len){
			memcpy(buf+real_len, s.buf[0], s.len);

			size_t wt = 0;
			if (!fd){
				fd = fopen("test.aac", "wb");
			}
			if (fd) {
				wt = fwrite(s.buf[0], 1, s.len, fd);
			}

			free(s.buf[0]);
			real_len += s.len;
			buf_rest -= s.len;
			buf_deque.pop_front();
		}
		else {
			break;
		}
	}
	uv_mutex_unlock(&queue_mutex);
	return real_len;
}

// static function
int audio_decoder::fill_iobuffer(void * opaque, uint8_t *buf, int bufsize)
{
	audio_decoder* self = (audio_decoder*)opaque;
	return self->fill_iobuffer(buf, bufsize);
}

// static encode worker
void audio_decoder::decode_worker(uv_work_t* req)
{
	audio_decoder* self = (audio_decoder*)req->data;
	self->decode();
}

void audio_decoder::after_decode(uv_work_t* req, int status)
{
	audio_decoder* self = (audio_decoder*)req->data;
	self->close();
}