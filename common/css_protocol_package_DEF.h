#ifndef PDU_
#error Macro PDU_ not defined.
#endif

#ifndef Byte_
#error Macro Byte_ not defined.
#endif

#ifndef Int32_
#error Macro Int32_ not defined.
#endif

#ifndef UInt16_
#error Macro UInt16_ not defined.
#endif

#ifndef UInt32_
#error Macro UInt32_ not defined.
#endif

#ifndef UInt64_
#error Macro UInt64_ not defined.
#endif

#ifndef FixedString_
#error Macro FixedString_ not defined.
#endif

#ifndef OctetString_
#error Macro OctetString_ not defined.
#endif

#ifndef SkipBlock_
#error Macro SkipBlock_ not defined.
#endif

#ifndef ByteArray_
#error Macro ByteArray_ not defined.
#endif


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


//协议头
#define PACKET_HEADER_	\
	UInt32_(I_CmdLen) \
	UInt32_(I_CmdId) \
	UInt32_(I_SeqNum) \
	UInt32_(I_SrcAddr) \
	UInt32_(I_DestAddr)

/*------------------------------------------------------------------------*\
 * 通用的分析数据头 
\*------------------------------------------------------------------------*/

/* 协议头 */
PDU_(JNetCmd_Header,
	 sv_cmd_header,
	 0xffffffff,
	 PACKET_HEADER_
	 SkipBlock_((This->I_CmdLen-20))
	 )

/*------------------------------------------------------------------------*\
 * 客户端申请临时ID 
\*------------------------------------------------------------------------*/

/* Interface:	进入虚拟网 */
PDU_(JVlanInterface_EnterVlan,
	 vlan_interface_enter_vlan,
	 0x01000001,
	 PACKET_HEADER_
	 UInt32_(ClientIp)
	 )
	 
/* Interface:	进入虚拟网,获取客户端ID的响应 */
PDU_(JVlanInterface_EnterVlanResp,
	 vlan_interface_enter_vlan_resp,
	 0x81000001,
	 PACKET_HEADER_
	 UInt32_(State)
	 UInt32_(ClientId)
	 )

/*------------------------------------------------------------------------*\
 *  报警上传
\*------------------------------------------------------------------------*/

PDU_(JNetCmd_AlarmUpMsgServer,
	 sv_cmd_alarmupmsg_server,
	 0xA00B0001,
	 PACKET_HEADER_
	 UInt64_(DvrEquipId)
	 UInt16_(ChannelNo)
	 SkipBlock_(1)			/* AlarmType */
	 SkipBlock_(1)			/*AlarmNum */
	 SkipBlock_(23)			/*Time */
	 SkipBlock_(1)			/*DeviceID */
	 Byte_(warnType)	
	 SkipBlock_(20)			/*ChannelName */
	 SkipBlock_(20)			/*DetectorName */
	 SkipBlock_(20)			/*AlarmBoxName */
	 UInt32_(ConnectType)	/*ConnectType */
	 UInt32_(DvrIp)
	 FixedString_(DvrName,20)
	 UInt32_(DvrPort)
	 UInt32_(TranSvrIp)	
	 UInt32_(TranSvrPort)
	 UInt32_(TranSvrDwId)
	 UInt32_(DvrType)
	 UInt64_(AlarmEventIDIndex)
	 SkipBlock_((This->I_CmdLen-173))
	 )

PDU_(JNetCmd_AlarmUpMsgServer_Ex,
	 sv_cmd_alarmupmsg_server_ex,
	 0x000B0001,
	 PACKET_HEADER_
	 UInt32_(AlarmInfoXmlLen)
	 FixedString_(AlarmInfoXml,This->AlarmInfoXmlLen)
	 UInt32_(RelationInfoXmlLen)
	 FixedString_(RelationInfoXml,This->RelationInfoXmlLen)
	 )

/*------------------------------------------------------------------------*\
 * 发送报警信息
\*------------------------------------------------------------------------*/
PDU_(JNetCmd_AlarmUpMsg,
	 sv_cmd_alarmupmsg,
	 0x000B0003,
	 PACKET_HEADER_
	 UInt64_(cssid)
	 UInt32_(status)
	 FixedString_(alarmTime,23)
	 )


PDU_(JNetCmd_AlarmUpMsg_Resp,
	 sv_cmd_alarmupmsg_resp,
	 0x800B0003,
	 PACKET_HEADER_
	 Int32_(State)
	 )

/*------------------------------------------------------------------------*\
 * 录像和录像的响应
\*------------------------------------------------------------------------*/

#if TW_MANAGER_VER >= TW_VER_0210
PDU_(JNetCmd_PreviewConnect, 
	 sv_cmd_preview_connect,
	 0x00050101,
	 PACKET_HEADER_
	 FixedString_(Token,128)
	 UInt64_(EquipId)
	 UInt16_(ChannelNo)
	 Int32_(CodingFlowType)
	 )
#else
PDU_(JNetCmd_PreviewConnect, 
	 sv_cmd_preview_connect,
	 0x00050101,
	 PACKET_HEADER_
	 FixedString_(Token,128)
	 UInt16_(ChannelNo)
	 )
#endif

PDU_(JNetCmd_PreviewConnect_Ex, 
	 sv_cmd_preview_connect_ex,
	 0x00050104,
	 PACKET_HEADER_
	 FixedString_(Token,128)
	 UInt64_(EquipId)
	 UInt16_(ChannelNo)
	 Int32_(CodingFlowType)	/*码流类型*/
	 )


PDU_(JNetCmd_PreviewConnect_Resp,
	 sv_cmd_preview_connect_resp,
	 0x80050101,
	 PACKET_HEADER_
	 Int32_(State)
	 Int32_(Brightness)
	 Int32_(Contrast)
	 Int32_(Saturation)
	 Int32_(Hue)
	 )

PDU_(JNetCmd_Preview_Frame,
	 sv_cmd_preview_frame,
	 0x80050102,
	 PACKET_HEADER_
	 Int32_(FrameType)
	 ByteArray_(FrameData, This->I_CmdLen - 24)
	 )

/*------------------------------------------------------------------------*\
 *	管理中心与集中存储服务器的交互
\*------------------------------------------------------------------------*/

/*添加集中存储服务器 */

#if (TW_MANAGER_TYPE == TW_NVMP)&&(TW_MANAGER_VER == TW_VER_0150)
PDU_(JNetCmd_AddCentralizeStorageServer,
	 sv_cmd_add_centralizestorage_server,
	 0x02010001,
	 PACKET_HEADER_
	 FixedString_(Token,128)		/*会话的Token */
	 UInt64_(csEquipId)				/*集中存储的设备Id */
	 OctetString_(wsUriPrefix,512)	/*管理中心webservice的前缀, eg:  http://128.8.153.99:8080/sc/ */
	 )
#else
PDU_(JNetCmd_AddCentralizeStorageServer,
	 sv_cmd_add_centralizestorage_server,
	 0x02010001,
	 PACKET_HEADER_
	 FixedString_(Token,128)		/*会话的Token */
	 UInt64_(csEquipId)				/*集中存储的设备Id */
	 OctetString_(wsUriPrefix,512)	/*管理中心webservice的前缀, eg:  http://128.8.153.99:8080/sc/ */
	 UInt32_(smsIp)					/*流媒体Ip */
	 UInt16_(smsPort)				/*流媒体Port */
	 UInt32_(smsNetId)				/*流媒体的网络Id */
	 )
#endif

 
PDU_(JNetCmd_AddCentralizeStorageServer_Resp,
	 sv_cmd_add_centralizestorage_server_resp,
	 0x82010001,
	 PACKET_HEADER_
	 UInt32_(state)		/*返回的状态, 0 表示成功 */
	 )

/*重启 */
PDU_(JNetCmd_RestartComputer,
	 sv_cmd_restartcomputer,
	 0x000D0001,
	 PACKET_HEADER_
	 FixedString_(Token,128)
	 )

PDU_(JNetCmd_RestartComputer_Resp,
	 sv_cmd_restartcomputer_resp,
	 0x800D0001,
	 PACKET_HEADER_
	 UInt32_(state)		/*返回的状态, 0 表示成功 */
	 )

/*
//------------------------------------------------------------------------
//	根据用户名和密码连接注销PCDVR
//------------------------------------------------------------------------

PDU_(JNetCmd_EachLogin_PcDvr,
	 sv_cmd_eachLogin_pcdvr,
	 0x00010001,
	 PACKET_HEADER_
	 FixedString_(Loginname,20)
	 FixedString_(Password ,40)
	 UInt16_(Version)
	 FixedString_(Reserved,24)
	 )

PDU_(JNetCmd_EachLogin_PcDvr_Resp,
	sv_cmd_eachLogin_pcdvr_resp,
	0x80010001,
	PACKET_HEADER_
	Int32_(State)
	FixedString_(Token,128)
	OctetString_(Username ,128)
)


PDU_(JNetCmd_EachLogout_PcDvr,
	 sv_cmd_eachlogout_pcdvr,
	 0x00010003 ,
	 PACKET_HEADER_
	 FixedString_(Token,128)
	 ) 

PDU_(JNetCmd_EachLogout_PcDvr_Resp,
	 sv_cmd_eachlogout_pcdvr_resp,
	 0x80010003 ,
	 PACKET_HEADER_
	 ) 
*/

/*------------------------------------------------------------------------*\
 *	接受命令式集中存储的命令请求和相应
\*------------------------------------------------------------------------*/

PDU_(JNetCmd_DownloadFile,
	 sv_cmd_downloadfile,
	 0x02010002 ,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 UInt64_(dvrEquipId)
	 UInt16_(channelNo)
	 FixedString_(startTime ,23)
	 FixedString_(endTime ,23)
	 UInt64_( userId )
	 OctetString_( desc ,4000)
	 ) 

PDU_(JNetCmd_DownloadFile_Resp,
	 sv_cmd_downloadfile_resp,
	 0x82010002 ,
	 PACKET_HEADER_
	 Int32_(state)
	 ) 

/*------------------------------------------------------------------------*\
 *	DVRS文件下载命令 
\*------------------------------------------------------------------------*/
PDU_(JNetCmd_Downdloadbytime_Range,
	 sv_cmd_downloadfile_by_time_range,
	 0x00060402,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 /*UInt64_(dvrEquipId) */
	 UInt16_(channelNo)
	 FixedString_(startTime ,23)
	 FixedString_(endTime ,23)
	 )

PDU_(JNetCmd_ReplayDownloadfile_Resp,
	 sv_cmd_replay_downloadfile_resp,
	 0x80060403,
	 PACKET_HEADER_
	 Int32_(state)
	 )

PDU_(JNetCmd_ReplayDownloadfile_Chunkdata,
	 sv_cmd_replay_downloadfile_chunkdata,
	 0x80060404,
	 PACKET_HEADER_
	 ByteArray_(ChunkData, This->I_CmdLen - 20)
	 )

PDU_(JNetCmd_ReplayDownloadfile_Finished,
	 sv_cmd_replay_downloadfile_finished,
	 0x00060405,
	 PACKET_HEADER_
	 )

PDU_(JNetCmd_ReplayDownloadfile_Finished_Resp,
	 sv_cmd_replay_downloadfile_finished_resp,
	 0x80060405,
	 PACKET_HEADER_
	 )

/*------------------------------------------------------------------------*\
 *	接受用户命令去前端下载录像的命令请求和相应
\*------------------------------------------------------------------------*/

PDU_(JNetCmd_HandleDownloadFile,
	 sv_cmd_handledownloadfile,
	 0x02010004 ,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 UInt64_(dvrEquipId)
	 UInt16_(channelNo)
	 FixedString_(startTime ,23)
	 FixedString_(endTime ,23)
	 /*OctetString_( lawcaseid ,255) */
	 OctetString_( fileid ,255)
	 OctetString_(filename ,255)
	 Int32_(warntype)
	 ) 

PDU_(JNetCmd_HandleDownloadFile_Resp,
	 sv_cmd_handledownloadfile_resp,
	 0x82010004 ,
	 PACKET_HEADER_
	 Int32_(state)
	 OctetString_(filename ,255)
	 ) 

PDU_(JNetCmd_Delete_HandleDownloadFile,
	 sv_cmd_delete_handledownloadfile,
	 0x02010005,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 Int32_(number)
	 SkipBlock_((This->I_CmdLen-152))
	 )

	 /*
PDU_(JNetCmd_Delete_HandleDownloadFile_,
	 sv_cmd_delete_handledownloadfile_,
	 0x02010005,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 Int32_(number)
	 )
	 */

PDU_(JNetCmd_Delete_HandleDownloadFile_Resp,
	 sv_cmd_delete_handledownloadfile_resp,
	 0x82010005,
	 PACKET_HEADER_
	 Int32_(state)
	 )

 /*------------------------------------------------------------------------*\
  * 状态巡检
 \*------------------------------------------------------------------------*/
PDU_(JNetCmd_AskState,
	 sv_cmd_ask_state,
	 0x00020003,
	 PACKET_HEADER_
	 UInt64_(EquipId)
	 Int32_(State)
	 )

PDU_(JNetCmd_AskState_Resp,
	 sv_cmd_ask_state_resp,
	 0x80020003,
	 PACKET_HEADER_
	 Int32_(State)
	 )


/*------------------------------------------------------------------------*\
* 查询集中存储录像文件列表
\*------------------------------------------------------------------------*/
PDU_(JNetCmd_Replay_Search_Record,
	 sv_cmd_replay_search_record,
	 0x00060006,
	 PACKET_HEADER_
	 FixedString_(token ,128)
	 UInt64_(dvrEquipId)
	 Int32_(channelNo)
	 Byte_(recordType)
	 FixedString_(startTime ,23)
	 FixedString_(endTime ,23)
	 UInt16_(startIndex)
	 UInt16_(count)
	 )

//PDU_(JNetCmd_Replay_Search_Record_Resp,
//	 sv_cmd_replay_search_record_resp,
//	 0x80060006,
//	 PACKET_HEADER_
//	 Int32_(state)
//	 Int32_(recordFileNumber)
//	 ByteArray_(csRecordFileGroups, This->I_CmdLen - 28)
//	 )

//------------------------------------------------------------------------
//	 回放过集中存储
//------------------------------------------------------------------------

PDU_(JNetCmd_Replay_Play_File,
	 sv_cmd_replay_play_file,
	 0x000C0301,
	 PACKET_HEADER_
	 FixedString_(token,128)
	 UInt16_(channelNo)
	 OctetString_(fileName,512)
	 )

PDU_(JNetCmd_Replay_Play_File_Resp,
	 sv_cmd_replay_play_file_resp,
	 0x800C0301,
	 PACKET_HEADER_
	 Int32_(state)
	 )

PDU_(JNetCmd_Replay_Send_Frame,
	 sv_cmd_replay_send_frame,
	 0x800C0302,
	 PACKET_HEADER_
	 Int32_(frameType)
	 Int32_(sendType)
	 Int32_(replayDataLen)
	 ByteArray_(frameData, This->I_CmdLen - 32)
	 )

PDU_(JNetCmd_Replay_Set_Speed,
	 sv_cmd_replay_set_speed,
	 0x000C0303,
	 PACKET_HEADER_
	 UInt32_(speed)
	 )

PDU_(JNetCmd_Replay_Set_Postion,
	 sv_cmd_replay_set_postion,
	 0x000C0304,
	 PACKET_HEADER_
	 Int32_(postion)
	 )

PDU_(JNetCmd_Replay_Pause,
	 sv_cmd_replay_pause,
	 0x000C0305,
	 PACKET_HEADER_
	 )

PDU_(JNetCmd_Replay_Continue,
	 sv_cmd_replay_continue,
	 0x000C0306,
	 PACKET_HEADER_
	 )

PDU_(JNetCmd_Replay_Shutdown,
	 sv_cmd_replay_shutdown,
	 0x000C0307,
	 PACKET_HEADER_
	 )

PDU_(JNetCmd_Replay_Common_Resp,
	 sv_cmd_replay_common_resp,
	 0x800C0308,
	 PACKET_HEADER_
	 Int32_(state)
	 )

 /*------------------------------------------------------------------------*\
  * 性能测试
 \*------------------------------------------------------------------------*/
PDU_(JNetCmd_Perform_Test,
	 sv_cmd_perform_test,
	 0x0e000001,
	 PACKET_HEADER_
	 UInt64_(ChannelId)
	 UInt64_(Timestamp) /* 秒 */
	 )

PDU_(JNetCmd_Perform_Test_Resp,
	 sv_cmd_perform_test_resp,
	 0x8e000001,
	 PACKET_HEADER_
	 Int32_(State)
	 )

/*======================================================================================*/
/* capture cmd */

PDU_(JUser_Login_Info,
cc_user_login_info,
0x0f000001,
PACKET_HEADER_
Int32_(UserId)
Int32_(RoomId)
)

PDU_(JUser_Logout_Info,
cc_user_logout_info,
0x0f010001,
PACKET_HEADER_
Int32_(UserId)
Int32_(RoomId)
)

PDU_(JStart_Send_Frame,
cc_start_send_frame,
0x0f000002,
PACKET_HEADER_
Int32_(UserId)
Int32_(RoomId)
)

/*------------------------------------------------------------------------*\
* 媒体数据包
\*------------------------------------------------------------------------*/
PDU_(JMedia_sample,
cc_media_sample,
0x0f000010,
PACKET_HEADER_
Int32_(UserId)
Int32_(FrameType)
ByteArray_(FrameData, This->I_CmdLen - 24)
)



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#undef SkipBlock_
#undef OctetString_
#undef FixedString_
#undef UInt64_
#undef UInt16_
#undef UInt32_
#undef Byte_
#undef Int32_
#undef ByteArray_
#undef PDU_

