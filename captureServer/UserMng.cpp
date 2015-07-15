#include "UserMng.h"
#include "css_protocol_package.h"


//==================================================
// user client class
//
UserClient::UserClient(int id, int roomid, css_stream_t* stream)
: id_(id)
, roomid_(roomid)
, stream_(stream)
, next_(NULL)
{
}

UserClient::~UserClient()
{
}

int UserClient::Send(cc_buf_t* buf)
{
	css_write_req_t* req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
	css_stream_write_bufs(stream_, req, buf, 1, Send_cb);
	return 0;
}

void UserClient::Send_cb(css_write_req_t* req, int status)
{
	cc_buf_t* buf = static_cast<cc_buf_t*>(req->bufs);
	if (buf)
		buf->Release();
	free(req);
}

void UserClient::Close(void)
{
	stream_->data = this;
	css_stream_close(stream_, Close_cb);
}

void UserClient::Close_cb(css_stream_t* stream)
{
	UserClient* uc = (UserClient*)stream->data;
	free(stream);
	delete uc;
}

//==================================================
// user room class
//
UserRoom::UserRoom(int roomid)
: clients_(NULL)
, clients_cnt_(0)
, roomid_(roomid)
{
}

UserRoom::~UserRoom()
{
}

int UserRoom::AddClient(UserClient* uc)
{
	if (!clients_){
		clients_ = uc;
	}
	else {
		uc->next() = clients_;
		clients_ = uc;
	}
	clients_cnt_++;
	return clients_cnt_;
}

int UserRoom::RemoveClient(UserClient* uc)
{
	if (!uc) return 0;
	UserClient* tmp = clients_;
	bool removed = false;
	if (tmp == uc){
		clients_ = uc->next();
		uc->next() = NULL;
		clients_cnt_--;
		removed = true;
	}
	else{
		while (tmp){
			if (uc == tmp->next()){
				tmp->next() = uc->next();
				uc->next() = NULL;
				clients_cnt_--;
				removed = true;
			}
			tmp = tmp->next();
		}
	}
	if (removed){
		AnnounceUserLogout(uc->id());
	}
	uc->Close();
	return 0;
}

int UserRoom::RemoveClientByStream(css_stream_t* stream)
{
	UserClient* tmp = clients_;
	while (tmp){
		if (tmp->stream() == stream){
			return RemoveClient(tmp);
		}
		tmp = tmp->next();
	}
	return -1;
}

void UserRoom::AnnounceUserLogout(int id)
{
	if (!clients_cnt_){// no user is login
		return;
	}
	JUser_Logout_Info msg;
	JUser_Logout_Info_init(&msg);
	msg.UserId = id;
	msg.RoomId = roomid_;

	int len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&msg);
	if (len < 0){
		goto fail2;
	}
	char* buf = (char*)malloc(len);
	if (!buf){
		goto fail2;
	}
	int ret = css_proto_package_encode(buf, len, (JNetCmd_Header*)&msg);
	if (ret <= 0){
		goto fail1;
	}
	cc_buf_t* cc_buf = cc_buf_t::Get(buf, len);
	if (!cc_buf){
		goto fail1;
	}

	UserClient* uc = clients_;
	while (uc){
		css_write_req_t* req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
		if (req){
			cc_buf->AddRef();
			if (css_stream_write_bufs(uc->stream(), req, cc_buf, 1, Send_cb) < 0){
				cc_buf->Release();
			}
		}
		uc = uc->next();
	}
	goto fail0;
fail1:
	free(buf);
fail2:
	return;
fail0:
	cc_buf->Release();
	return;
}

void UserRoom::Send_cb(css_write_req_t* req, int status)
{
	cc_buf_t* buf = static_cast<cc_buf_t*>(req->bufs);
	if (buf){
		buf->Release();
	}
	free(req);
}

int UserRoom::SendBackOtherOnlineUser(UserClient* uc)
{
	UserClient* tmp = clients_;
	if (tmp == uc){
		return 0;
	}
	
	while (tmp){
		if (tmp != uc){
			JUser_Login_Info msg;
			JUser_Login_Info_init(&msg);
			msg.UserId = tmp->id();
			msg.RoomId = tmp->roomid();
			int len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&msg);
			if (len < 0){
				goto fail2;
			}
			char* buf = (char*)malloc(len);
			if (!buf){
				goto fail2;
			}
			int ret = css_proto_package_encode(buf, len, (JNetCmd_Header*)&msg);
			if (ret <= 0){
				goto fail1;
			}
			cc_buf_t* cc_buf = cc_buf_t::Get(buf, len);
			if (!cc_buf){
				goto fail1;
			}

			css_write_req_t* req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
			if (req){
				cc_buf->AddRef();
				if (css_stream_write_bufs(uc->stream(), req, cc_buf, 1, Send_cb) < 0){
					cc_buf->Release();
				}
			}
			goto fail0;
		fail1:
			free(buf);
		fail2:
			continue;
		fail0:
			cc_buf->Release();
			continue;
		}
		tmp = tmp->next();
	}
	return 0;
}

int UserRoom::DispatchUserData(css_stream_t* stream, char* buf, int len)
{
	if (len < 0){
		RemoveClientByStream(stream);
	}
	else{
		UserClient* tmp = clients_;
		cc_buf_t* buf_tmp = cc_buf_t::Get(buf, len);
		while (tmp){
			if (tmp->stream() != stream){
				buf_tmp->AddRef();
				if (tmp->Send(buf_tmp) < 0){
					buf_tmp->Release();
				}
			}
			tmp = tmp->next();
		}
		buf_tmp->Release();
	}
	return 0;
}

UserClient* UserRoom::FindUser(int id)
{
	UserClient* tmp = clients_;
	while (tmp){
		if (tmp->id() == id){
			return tmp;
		}
		else {
			tmp = tmp->next();
		}
	}
	return NULL;
}


//==================================================
// room manager class
//
RoomMng::RoomMng()
{
}

RoomMng::~RoomMng()
{
}

int RoomMng::AddUser(int id, int roomid, css_stream_t* stream)
{
	UserClient* user = FindUser(id);
	if (user){
		return 0;
	}
	user = new UserClient(id, roomid, stream);
	if (!user){
		return -1;
	}
	UserRoom* ur = FindRoom(roomid);
	if (ur){
		ur->AddClient(user);
		stream->data = ur;
	}
	else {
		ur = new UserRoom(roomid);
		if (!ur){
			delete user;
			return -1;
		}
		else{
			room_map_.insert({ roomid, ur });
			ur->AddClient(user);
			stream->data = ur;
		}
	}
	ur->SendBackOtherOnlineUser(user);
	return 0;
}

int RoomMng::RemoveUser(int id)
{
	UserClient* user = FindUser(id);
	if (!user){
		return 0;
	}
	UserRoom* ur = FindRoom(user->roomid());
	if (!ur){// finded user, so couldn't finded user room
		delete user;
		return -1;
	}
	ur->RemoveClient(user);
	delete user;
	// TODO: remove room when user is empty;
	return 0;
}

UserClient* RoomMng::FindUser(int id)
{
	UserClient* user = NULL;
	roomit_t it;
	for (it = room_map_.begin(); it != room_map_.end(); it++){
		UserRoom* ur = it->second;
		if (ur){
			if (user = ur->FindUser(id)){
				return user;
			}
		}
	}
	return NULL;
}

UserRoom* RoomMng::FindRoom(int roomid)
{
	UserRoom* ur = NULL;
	roomit_t it;
	for (it = room_map_.begin(); it != room_map_.end(); it++){
		ur = it->second;
		if (ur->roomid() == roomid){
			return ur;
		}
	}
	return NULL;
}

//==================================================
// static object and function
//
static RoomMng gRoomMng;

//==================================================
// public api
//
int AddUser(int id, int roomid, css_stream_t* stream)
{
	return gRoomMng.AddUser(id, roomid, stream);
}

int RemoveUser(int id)
{
	return gRoomMng.RemoveUser(id);
}

int DispatchUserData(css_stream_t* stream, char* buf, int len)
{
	UserRoom* ur = (UserRoom*)stream->data;
	return ur->DispatchUserData(stream, buf, len);
}