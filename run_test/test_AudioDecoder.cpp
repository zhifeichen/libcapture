#include "css_test.h"
#include "AudioDecoder.h"
#include "ResourcePool.h"

TEST_IMPL(AudioDecoder)
{
    uv_loop_t* loop = uv_default_loop();
    CResourcePool::GetInstance().Init(loop);
    CAudioDecoder* dec = dynamic_cast<CAudioDecoder*>(CResourcePool::GetInstance().Get(e_rsc_audiodecoder));
    dec->init(loop);
    //dec->open();
    dec->stop();
    printf("active handle: %d\n", uv_loop_alive(loop));
    uv_run(loop, UV_RUN_DEFAULT);
    printf("stop!\n");
    printf("Resource count: %d\n", CResourcePool::GetInstance().GetResourceCount());
}