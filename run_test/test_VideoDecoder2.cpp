#include "css_test.h"
#include "VideoDecoder.h"
#include "ResourcePool.h"

void DoFrame(AVFrame* frame, int status, void* data)
{
    static int cnt = 0;
    if(frame){
        printf("get frame %d\n", cnt++);
        av_frame_free(&frame);
    } else{
        printf("DoFrame received NULL frame, status: %d\n", status);
        CVideoDecoder2* dec = (CVideoDecoder2*)data;
        dec->Stop();
        printf("Resource count: %d\n", CResourcePool::GetInstance()->GetResourceCount());
        CResourcePool::GetInstance()->PostClose();
    }
}

void DoPutBuf(CVideoDecoder2* dec)
{
    FILE* fd = fopen("./testResource/bigbuckbunny_480x272.h264", "rb");
    if(fd){
        uint8_t buf[5 * 1024];
        int len;
        do{
            len = fread(buf, sizeof uint8_t, 5 * 1024, fd);
            if(len > 0){
                dec->Put(buf, len);
            }
        } while(feof(fd) == 0);
        fclose(fd);
    }
}

TEST_IMPL(VideoDecoder2)
{
    uv_loop_t* loop = uv_default_loop();
    CResourcePool::GetInstance()->Init(loop);
    CVideoDecoder2* dec = dynamic_cast<CVideoDecoder2*>(CResourcePool::GetInstance()->Get(e_rsc_videodecoder2));
    dec->Init();
    dec->SetFrameCallback(DoFrame, dec);
    DoPutBuf(dec);
    //dec->Stop();
    printf("active handle: %d\n", uv_loop_alive(loop));
    printf("Resource count: %d\n", CResourcePool::GetInstance()->GetResourceCount());
    //CResourcePool::GetInstance()->PostClose();
    uv_run(loop, UV_RUN_DEFAULT);
    printf("stop!\n");
    printf("active handle: %d\n", uv_loop_alive(loop));
    printf("Resource count: %d\n", CResourcePool::GetInstance()->GetResourceCount());
    CResourcePool::GetInstance()->Uninit();
}