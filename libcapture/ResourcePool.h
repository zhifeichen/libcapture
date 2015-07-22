#ifndef __RESOURCE_POOL_H__
#define __RESOURCE_POOL_H__

#include <list>
#include "uv/uv.h"

typedef enum {
	e_rsc_msgsocket,
	e_rsc_aacencode,
	e_rsc_x264encode,
	e_rsc_audiodecode,
	e_rsc_videodecode
}E_RESOURCE_TYPE;

class CResource
{
	friend class CResourcePool;
public:
	CResource(E_RESOURCE_TYPE type);
	virtual ~CResource();

	void Release(void);

private:
	E_RESOURCE_TYPE m_eType;
	bool m_bCanCollection;
};

class CResourcePool
{
public:
	static CResourcePool& GetInstance(void);
	CResource* Get(E_RESOURCE_TYPE type);
	int Init(uv_loop_t* loop);
private:
	CResourcePool();
	~CResourcePool();
	CResourcePool(const CResourcePool&);
	CResourcePool& operator=(const CResourcePool&);

	std::list<CResource*> m_listResource;
	uv_loop_t* m_pLoop;
};


#endif // __RESOURCE_POOL_H__