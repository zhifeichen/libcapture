#include "css_test.h"
#include "VideoDecoder.h"
#include "ResourcePool.h"

TEST_IMPL(VideoDecoder2)
{
    uv_loop_t* loop = uv_default_loop();
    CResourcePool::GetInstance()->Init(loop);
    CVideoDecoder2* dec = dynamic_cast<CVideoDecoder2*>(CResourcePool::GetInstance()->Get(e_rsc_videodecoder2));
    dec->Init();
    //dec->open();
    dec->Stop();
    printf("active handle: %d\n", uv_loop_alive(loop));
    printf("Resource count: %d\n", CResourcePool::GetInstance()->GetResourceCount());
    CResourcePool::GetInstance()->PostClose();
    uv_run(loop, UV_RUN_DEFAULT);
    printf("stop!\n");
    printf("active handle: %d\n", uv_loop_alive(loop));
    printf("Resource count: %d\n", CResourcePool::GetInstance()->GetResourceCount());
    CResourcePool::GetInstance()->Uninit();
}