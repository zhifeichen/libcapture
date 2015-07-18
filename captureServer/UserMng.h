#ifndef __USER_MNG_H__
#define __USER_MNG_H__

#include "css_stream.h"
#include <map>

//==================================================
// refrence buf_t
//
struct cc_buf_t : public uv_buf_t
{
	static cc_buf_t* Get(char* base, uint32_t len){ return new cc_buf_t(base, len); }
	void AddRef(void){ InterlockedIncrement(&ref_); }
	void Release(void){ InterlockedDecrement(&ref_); if (!ref_){ delete this; } }
private:
	cc_buf_t(char* b, uint32_t l) : ref_(1){
		base = b;
		len = l;
	}
	virtual ~cc_buf_t(){ if(base) free(base); }
	uint32_t ref_;
};

class UserClient
{
public:
	UserClient(int id, int roomid, css_stream_t* stream);
	~UserClient();

	UserClient*& next(void){ return next_; }
	int& id(void){ return id_; }
	int& roomid(void) { return roomid_; }
	css_stream_t*& stream(void){ return stream_; }

	int Send(cc_buf_t* buf);
	void Close(void);
private:
	static void Send_cb(css_write_req_t* req, int status);
	static void Close_cb(css_stream_t* stream);

	int id_;
	int roomid_;
	css_stream_t* stream_;
	UserClient* next_;
};

class UserRoom
{
public:
	explicit UserRoom(int roomid);
	~UserRoom();

	int AddClient(UserClient* uc);
	int RemoveClient(UserClient* uc);
	int RemoveClientByStream(css_stream_t* stream);
	void AnnounceUserLogout(int id);
	int DispatchUserData(css_stream_t* stream, char* buf, int len);
	UserClient* FindUser(int id);
	int& roomid(void){ return roomid_; }

	int SendBackOtherOnlineUser(UserClient* uc);
	
private:
	static void Send_cb(css_write_req_t* req, int status);


	UserClient* clients_;
	int clients_cnt_;
	int roomid_;
};

class RoomMng
{
public:
	RoomMng();
	~RoomMng();

	int AddUser(int id, int roomid, css_stream_t* stream);
	int RemoveUser(int id);

private:
	std::map<int, UserRoom*> room_map_;
	typedef std::map<int, UserRoom*>::iterator roomit_t;

	UserClient* FindUser(int id);
	UserRoom* FindRoom(int roomid);
};

int AddUser(int id, int roomid, css_stream_t* stream);
int RemoveUser(int id);
int DispatchUserData(css_stream_t* stream, char* buf, int len);


#endif