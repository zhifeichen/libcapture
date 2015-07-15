
#include "css_cmd_route.h"
#include "css_protocol_package.h"
#include "css.h"

int AddUser(int id, int roomid, css_stream_t* stream);
int RemoveUser(int id);
int DispatchUserData(css_stream_t* stream, char* buf, int len);

#define DCL_CMD_HANDLER(cmd) \
	static void do_##cmd(css_stream_t* stream, char* package, ssize_t length);

#define IMP_CMD_HANDLER(cmd) \
	static void do_##cmd(css_stream_t* stream, char* package, ssize_t length)

#define DSP_CMD_HANDLER(cmd) \
	case cmd:				 \
	do_##cmd(stream, package, length);\
	break;

#define CSS_RT_GET_CMD(cmd) css_proto_package_decode(package, length, (JNetCmd_Header*) &cmd)
#define CSS_RT_FREE_PACKAGE() free(package)
#define CSS_RT_CLOSE_STREAM() css_stream_close(stream, css_cmd_delete_stream)

DCL_CMD_HANDLER(cc_user_login_info)
DCL_CMD_HANDLER(cc_start_send_frame)
DCL_CMD_HANDLER(cc_media_sample)
//DCL_CMD_HANDLER(sv_cmd_restartcomputer)

static void css_cmd_delete_stream(css_stream_t* stream);

static void css_cmd_delete_stream(css_stream_t* stream)
{
	if(stream)
		free(stream);
}

IMP_CMD_HANDLER(cc_user_login_info)
{
	JUser_Login_Info cmd;
	int ret;
	do{
		ret = CSS_RT_GET_CMD(cmd);
		if (ret <= 0) {
			break;
		}
		if (AddUser(cmd.UserId, cmd.RoomId, stream) >= 0){
			DispatchUserData(stream, package, length);
			return;
		}
		else {
			break;
			//CSS_RT_FREE_PACKAGE();
			//return;/* do not close stream, wait next cmd */
		}
	}while(0);

	CSS_RT_FREE_PACKAGE();
	CSS_RT_CLOSE_STREAM();
}

IMP_CMD_HANDLER(cc_start_send_frame)
{
	JStart_Send_Frame cmd;
	int ret;

	do{
		ret = CSS_RT_GET_CMD(cmd);
		if (ret <= 0) {
			break;
		}
		DispatchUserData(stream, package, length);
		return;
	}while(0);

	CSS_RT_FREE_PACKAGE();
	CSS_RT_CLOSE_STREAM();
}

IMP_CMD_HANDLER(cc_media_sample)
{
	JMedia_sample cmd;
	int ret;

	do{
		ret = CSS_RT_GET_CMD(cmd);
		if (ret <= 0) {
			break;
		}
		DispatchUserData(stream, package, length);
		return;
	}while(0);

	CSS_RT_FREE_PACKAGE();
	CSS_RT_CLOSE_STREAM();
}

//IMP_CMD_HANDLER(sv_cmd_restartcomputer)
//{
//	JNetCmd_RestartComputer cmd;
//	int ret;
//
//	do{
//		ret = CSS_RT_GET_CMD(cmd);
//		if (ret <= 0) {
//			break;
//		}
//	}while(0);
//
//	CSS_RT_FREE_PACKAGE();
//	CSS_RT_CLOSE_STREAM();
//}


int css_cmd_dispatch(css_stream_t* stream, char* package, ssize_t length)
{
	int ret = -1;

	JNetCmd_Header header;

	/* stop read first, start read later if necessary */
	//css_stream_read_stop(stream);

	if (length <= 0) {
		if (package)
			free(package);
		if (stream->data){
			DispatchUserData(stream, NULL, length);
		}
		else {
			css_stream_close(stream, css_cmd_delete_stream);
		}
		return (int)length;
	}

	ret = css_proto_package_header_decode(package, length, &header);
	if(ret <= 0){
		free(package);
		css_stream_close(stream, css_cmd_delete_stream);
		return ret;
	}

	switch (header.I_CmdId) {
	DSP_CMD_HANDLER(cc_user_login_info)
	DSP_CMD_HANDLER(cc_start_send_frame)
	DSP_CMD_HANDLER(cc_media_sample)

	default:
		free(package);
		css_stream_close(stream, css_cmd_delete_stream);
		break;
	}

	return ret;
}