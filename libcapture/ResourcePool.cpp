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
	CResourcePool::GetInstance().PostCollect();
}

CResourcePool::CResourcePool():
m_listResource(),
m_pLoop(NULL)
{
}

CResourcePool::~CResourcePool()
{
}

int CResourcePool::Init(uv_loop_t* loop)
{
	m_pLoop = loop;
	uv_mutex_init(&m_mtxList);
	return 0;
}

int CResourcePool::Uninit(void)
{
	DoCollect(0);
	uv_mutex_destroy(&m_mtxList);
	return 0;
}

int CResourcePool::PostCollect(void)
{
	int ret;
	uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (!req) return UV_ENOMEM;
	req->data = this;
	if ((ret = uv_queue_work(m_pLoop, req, DoNothing, DoCollect)) < 0){
		free(req);
	}
	return ret;
}

void CResourcePool::DoCollect(uv_work_t* req, int status)
{
	CResourcePool* p = (CResourcePool*)req->data;
	p->DoCollect(status);
	free(req);
}

void CResourcePool::DoCollect(int status)
{
	std::list<CResource*>::iterator it;
	uv_mutex_lock(&m_mtxList);
	it = m_listResource.begin();
	while (it != m_listResource.end()){
		CResource* r = *it;
		std::list<CResource*>::iterator it1 = it;
		it++;
		if (r->m_bCanCollection){
			m_listResource.erase(it1);
			delete r;
		}
	}
	uv_mutex_unlock(&m_mtxList);
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
		r = new CAudioDecoder();
		break;
	case e_rsc_videodecoder:
		r = new CVideoDecoder();
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

CResourcePool& CResourcePool::GetInstance(void)
{
	static CResourcePool gPool;
	return gPool;
}