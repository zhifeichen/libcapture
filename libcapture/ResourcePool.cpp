#include "ResourcePool.h"
#include "MsgSocket.h"
#include "aacencode.h"
#include "x264encode.h"
#include "decode_audio.h"
#include "decode_video.h"

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
	return 0;
}

CResource* CResourcePool::Get(E_RESOURCE_TYPE type)
{
	CResource* r = NULL;
	switch (type){
	case e_rsc_msgsocket:
		r = new CMsgSocket(m_pLoop);
		break;
	case e_rsc_aacencode:
		r = new aacenc(m_pLoop);
		break;
	case e_rsc_x264encode:
		r = new x264enc(m_pLoop);
		break;
	case e_rsc_audiodecode:
		r = new audio_decoder();
		break;
	case e_rsc_videodecode:
		r = new video_decoder();
		break;
	default:
		break;
	}
	if (r){
		m_listResource.push_back(r);
	}
	return r;
}

CResourcePool& CResourcePool::GetInstance(void)
{
	static CResourcePool gPool;
	return gPool;
}