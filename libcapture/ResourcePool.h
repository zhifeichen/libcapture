#ifndef __RESOURCE_POOL_H__
#define __RESOURCE_POOL_H__

#include <list>
#include "uv/uv.h"

typedef enum {
	e_rsc_msgsocket,
	e_rsc_aacencoder,
	e_rsc_x264encoder,
	e_rsc_audiodecoder,
	e_rsc_videodecoder,
    e_rsc_videodecoder2
}E_RESOURCE_TYPE;

class CResource;
typedef void(*CLOSERESOURCECB)(CResource* r);

class CResource
{
	friend class CResourcePool;
protected:
	CResource(E_RESOURCE_TYPE type);
	virtual ~CResource();

	void Release(void);
    virtual void Close(CLOSERESOURCECB cb = NULL);

private:
	E_RESOURCE_TYPE m_eType;
	bool m_bCanCollection;
    CLOSERESOURCECB m_fnCB;
};

class CResourcePool
{
public:
	static CResourcePool* GetInstance(void);
	CResource* Get(E_RESOURCE_TYPE type);
	int Init(uv_loop_t* loop);
	int Uninit(void);
	int PostCollect(void);
    int Close(void);
    void PostClose(void);

	int GetResourceCount(void){ return m_listResource.size(); }
private:
	CResourcePool();
	~CResourcePool();
	CResourcePool(const CResourcePool&);
	CResourcePool& operator=(const CResourcePool&);

    static void DoCollect(uv_work_t*);
	static void AfterCollect(uv_work_t* req, int status);
	void DoCollect(void);

	std::list<CResource*> m_listResource;
	uv_mutex_t  m_mtxList;
    uv_cond_t   m_cndList;
    uv_sem_t    m_semClose;
	uv_loop_t  *m_pLoop;
    bool        m_bStop;

    static CResourcePool *gPool;
};


#endif // __RESOURCE_POOL_H__