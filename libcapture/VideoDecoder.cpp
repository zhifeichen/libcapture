#include "stdafx.h"
#include "VideoDecoder.h"

CVideoDecoder::CVideoDecoder(uv_loop_t* loop)
: pFormatCtx(NULL), pCodecCtxOrig(NULL)
, pCodecCtx(NULL), pCodec(NULL)
, pFrame(NULL), sws_ctx(NULL), pFrameYUV(NULL)
, texture(NULL), screen(NULL)
, renderer(NULL), hWin(NULL)
, bStop(true), bOpen(false), bStarting(false), bInit(false)
, iFrame(0)
, pLoop(loop)
, CResource(e_rsc_videodecoder)
{
}

CVideoDecoder::~CVideoDecoder()
{
}

int CVideoDecoder::init(HWND w)
{
	int ret = 0;
	if (!bInit){
		hWin = w;

		uv_mutex_init(&queue_mutex);
		uv_cond_init(&queue_not_empty);

		// Register all formats and codecs
		av_register_all();

		if (ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
			fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		}
		bInit = true;
	}

	return ret;
}

int CVideoDecoder::resizewindow(void)
{
	if (hWin){
		RECT rc;
		GetClientRect(hWin, &rc);
		SDL_SetWindowSize(screen, rc.right - rc.left, rc.bottom - rc.top);
		rect_dst.x = rect_dst.y = 0;
		rect_dst.w = rc.right - rc.left;
		rect_dst.h = rc.bottom - rc.top;
		return 0;
	}
	return -1;
}

int CVideoDecoder::finit()
{
	int ret = 0;
	if (bInit){
		uv_mutex_destroy(&queue_mutex);
		uv_cond_destroy(&queue_not_empty);
		hWin = NULL;
		bInit = false;
	}
	return ret;
}

int CVideoDecoder::open(void)
{
	int ret = 0;
	bOpen = true;
	pFormatCtx = avformat_alloc_context();
	const int BUF_LEN = (1024 * 200);
	iobuffer = (unsigned char *)av_malloc(BUF_LEN);
	avio = avio_alloc_context(iobuffer, BUF_LEN, 0, this, fill_iobuffer, NULL, NULL);
	pFormatCtx->pb = avio;
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
	videoStream = -1;
	unsigned int i;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
	if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
		videoStream = i;
		break;
	}
	if (videoStream == -1){
		bOpen = false;
		return -1; // Didn't find a video stream
	}
	/*AV_SAMPLE_FMT_S16;*/
	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
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

	// Allocate video frame
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	// Make a screen to put our video
	if (hWin){
		screen = SDL_CreateWindowFrom(hWin);
		RECT rc;
		GetClientRect(hWin, &rc);
		SDL_SetWindowSize(screen, rc.right - rc.left, rc.bottom - rc.top);
		rect_dst.x = rect_dst.y = 0;
		rect_dst.w = rc.right - rc.left;
		rect_dst.h = rc.bottom - rc.top;
	} else
		screen = SDL_CreateWindow("",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			pCodecCtx->width, pCodecCtx->height,
			0);
	renderer = SDL_CreateRenderer(screen, -1, 0);
	if (!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		bOpen = false;
		return -1;;
	}

	// Allocate a place to put our YUV image on that screen
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV /*SDL_PIXELFORMAT_YV12*/,
		SDL_TEXTUREACCESS_STREAMING,
		pCodecCtx->width, pCodecCtx->height);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		);

	int numBytes = avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width,
		pCodecCtx->height);
	uint8_t* buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	avpicture_fill((AVPicture *)pFrameYUV, buffer, PIX_FMT_YUV420P,
		pCodecCtx->width, pCodecCtx->height);

	bOpen = true;
	//bStop = false;
	//start();
	return ret;
}

int CVideoDecoder::start()
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

void CVideoDecoder::decode(void)
{
	// Read frames and save first five frames to disk
	//if (!bOpen && buf_deque.size() >2){
		if (open() < 0) 
			return;
	//}

	rect_src.x = 0;
	rect_src.y = 0;
	rect_src.w = pCodecCtx->width;
	rect_src.h = pCodecCtx->height;
	int ret = -1;

	while (!bStop && (ret = av_read_frame(pFormatCtx, &packet)) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {

				if (1){
					// Convert the image into YUV format that SDL uses
					sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height,
						pFrameYUV->data, pFrameYUV->linesize);

					//SDL_UpdateTexture(bmp, &rect_src, pFrameYUV->data[0], pFrameYUV->linesize[0]);
					SDL_UpdateYUVTexture(texture, &rect_src,
						pFrameYUV->data[0], pFrameYUV->linesize[0],
						pFrameYUV->data[1], pFrameYUV->linesize[1],
						pFrameYUV->data[2], pFrameYUV->linesize[2]);
				} else 
					SDL_UpdateTexture(texture, &rect_src, pFrame->data[0], pFrame->linesize[0]);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, &rect_src, &rect_dst);
				SDL_RenderPresent(renderer);
				//SDL_Delay(66);
			}
			
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

int CVideoDecoder::stop()
{
	int ret = 0;
	bStop = true;
	if (!bStarting)
		close();
	return ret;
}

int CVideoDecoder::close()
{
	int ret = 0;
	if (texture)
		SDL_DestroyTexture(texture);
	// Free the YUV frame
	if (pFrame)
		av_frame_free(&pFrame);

	// Close the codec
	if (pCodecCtx)
		avcodec_close(pCodecCtx);
	if (pCodecCtxOrig)
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	if (pFormatCtx)
		avformat_close_input(&pFormatCtx);

	bOpen = false;
	bStarting = false;
	finit();
	Release();
	return ret;
}

int CVideoDecoder::put(cc_src_sample_t buf)
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

int CVideoDecoder::put(uint8_t* buf, int len)
{
	int ret = 0;
	
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() > 5000){
		uv_mutex_unlock(&queue_mutex);
		free(buf);
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

int CVideoDecoder::fill_iobuffer(uint8_t *buf, int bufsize)
{
	int ret = 0;
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() == 0){
		//uv_mutex_unlock(&queue_mutex);
		int ret = uv_cond_timedwait(&queue_not_empty, &queue_mutex, (uint64_t)(1000 * 1e6));
		if (ret == UV_ETIMEDOUT){
			uv_mutex_unlock(&queue_mutex);
			//MessageBox(NULL, TEXT("BUFFER EMPTY!"), TEXT("remote preview"), MB_OK);
			return 0;
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
				fd = fopen("test.264", "wb");
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
int CVideoDecoder::fill_iobuffer(void * opaque, uint8_t *buf, int bufsize)
{
	CVideoDecoder* self = (CVideoDecoder*)opaque;
	return self->fill_iobuffer(buf, bufsize);
}

// static encode worker
void CVideoDecoder::decode_worker(uv_work_t* req)
{
	CVideoDecoder* self = (CVideoDecoder*)req->data;
	self->decode();
}

void CVideoDecoder::after_decode(uv_work_t* req, int status)
{
	CVideoDecoder* self = (CVideoDecoder*)req->data;
	self->close();
}


////////////////////////////
// CVideoDecoder2
//
CVideoDecoder2::CVideoDecoder2(uv_loop_t* loop)
: CResource(e_rsc_videodecoder)
, codecId(AV_CODEC_ID_H264)
, pLoop(loop)
, pCodec(NULL), pCodecCtx(NULL), pCodecParserCtx(NULL)
, packetQueue()
, pPacket(NULL), pFrame(NULL), pFrameYUV(NULL)
, pConvertCtx(NULL)
, queueMutex(NULL), queueNotEmpty(NULL)
, bInit(false)
{
}

CVideoDecoder2::~CVideoDecoder2()
{
}

int CVideoDecoder2::Init(void)
{
	int ret = -1;
	while (!bInit){
		queueMutex = (uv_mutex_t*)malloc(sizeof(uv_mutex_t));
		if (!queueMutex){
			ret = -1;
			break;
		}
		ret = uv_mutex_init(queueMutex);
		if (ret < 0){
			break;
		}
		queueNotEmpty = (uv_cond_t*)malloc(sizeof(uv_cond_t));
		if (!queueNotEmpty){
			ret = -1;
			break;
		}
		ret = uv_cond_init(queueNotEmpty);
		if (ret < 0){
			break;
		}

		// Register all codecs
		avcodec_register_all();
		pCodec = avcodec_find_decoder(codecId);
		if (!pCodec){
			ret = -1;
			break;
		}
		pCodecCtx = avcodec_alloc_context3(pCodec);
		if (!pCodecCtx){
			ret = -1;
			break;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
			ret = -1;
			break;
		}

		pCodecParserCtx = av_parser_init(codecId);
		if (!pCodecParserCtx){
			ret = -1;
			break;
		}
		bInit = true;
	}
	if (!bInit){
		Finit();
		return -1;
	} else {
		return 0;
	}
}

void CVideoDecoder2::Finit(void)
{
    uv_mutex_destroy(queueMutex);
    uv_cond_destroy(queueNotEmpty);
	sws_freeContext(pConvertCtx);
	av_parser_close(pCodecParserCtx);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	avcodec_close(pCodecCtx);
	av_freep(pCodecCtx);
}