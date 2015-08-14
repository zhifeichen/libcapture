#include "ResourcePool.h"
#include "MsgSocket.h"
#include "AacEncoder.h"
#include "X264Encoder.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"

CResource::CResource(E_RESOURCE_TYPE type):
m_eType(type),
m_bCanCollection(false)
{
}

CResource::~CResource()
{
}

void CResource::Release(void)
{
	m_bCanCollection = true;
	CResourcePool::GetInstance()->PostCollect();
}

CResourcePool* CResourcePool::gPool = NULL;

CResourcePool::CResourcePool():
m_listResource(),
m_pLoop(NULL),
m_bStop(false)
{
}

CResourcePool::~CResourcePool()
{
}

int CResourcePool::Init(uv_loop_t* loop)
{
    int ret;
	m_pLoop = loop;
	uv_mutex_init(&m_mtxList);
    uv_cond_init(&m_cndList);
    uv_sem_init(&m_semClose, 1);
    uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
    if(!req) return UV_ENOMEM;
    req->data = this;
    if((ret = uv_queue_work(m_pLoop, req, DoCollect, AfterCollect)) < 0){
        free(req);
    }
    return ret;
}

int CResourcePool::Uninit(void)
{
    uv_sem_wait(&m_semClose);
	uv_mutex_destroy(&m_mtxList);
    uv_cond_destroy(&m_cndList);
    uv_sem_destroy(&m_semClose);
    gPool = NULL;
    delete this;
	return 0;
}

int CResourcePool::PostCollect(void)
{
	int ret = 0;
    uv_cond_signal(&m_cndList);
	return ret;
}

void CResourcePool::DoCollect(uv_work_t* req)
{
	CResourcePool* p = (CResourcePool*)req->data;
	p->DoCollect();
}

void CResourcePool::DoCollect(void)
{
	std::list<CResource*>::iterator it;
    while(!m_bStop){
        uv_mutex_lock(&m_mtxList);
        if(m_listResource.size() == 0 || 1){
            int ret = uv_cond_timedwait(&m_cndList, &m_mtxList, (uint64_t)(1000 * 1e6));
            if(ret == UV_ETIMEDOUT){
                uv_mutex_unlock(&m_mtxList);
                continue;
            }
        }
        it = m_listResource.begin();
        while(it != m_listResource.end()){
            CResource* r = *it;
            std::list<CResource*>::iterator it1 = it;
            it++;
            if(r->m_bCanCollection){
                m_listResource.erase(it1);
                delete r;
            }
        }
        uv_mutex_unlock(&m_mtxList);
    }
}

void CResourcePool::AfterCollect(uv_work_t* req, int status)
{
    CResourcePool* p = (CResourcePool*)req->data;
    free(req);
    uv_sem_post(&p->m_semClose);
}

int CResourcePool::Close(void)
{
    int ret = GetResourceCount();
    if(ret > 0){
        return ret;
    }
    m_bStop = true;
    return 0;
}

void DoClose(uv_work_t* req)
{
    CResourcePool* p = (CResourcePool*)req->data;
    while(p->Close()){ 
        ::Sleep(1000); 
        p->PostCollect(); 
    }
}

void AfterClose(uv_work_t* req, int s)
{
    CResourcePool* p = (CResourcePool*)req->data;
    free(req);
}

void CResourcePool::PostClose(void)
{
    uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
    if(!req) return ;
    req->data = this;
    if(uv_queue_work(m_pLoop, req, DoClose, AfterClose) < 0){
        free(req);
    }
}

CResource* CResourcePool::Get(E_RESOURCE_TYPE type)
{
	CResource* r = NULL;
	switch (type){
	case e_rsc_msgsocket:
		r = new CMsgSocket(m_pLoop);
		break;
	case e_rsc_aacencoder:
		r = new CAacEncoder(m_pLoop);
		break;
	case e_rsc_x264encoder:
		r = new CX264Encoder(m_pLoop);
		break;
	case e_rsc_audiodecoder:
		r = new CAudioDecoder(m_pLoop);
		break;
	case e_rsc_videodecoder:
		r = new CVideoDecoder(m_pLoop);
		break;
    case e_rsc_videodecoder2:
        r = new CVideoDecoder2(m_pLoop);
        break;
	default:
		break;
	}
	if (r){
		uv_mutex_lock(&m_mtxList);
		m_listResource.push_back(r);
		uv_mutex_unlock(&m_mtxList);
	}
	return r;
}

CResourcePool* CResourcePool::GetInstance(void)
{
	//static CResourcePool *gPool;
    if(!gPool){
        gPool = new CResourcePool();
    }
	return gPool;
}