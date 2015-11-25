/**
 * Protocol & Packet
 * @author Yuchun Zhang
 * @date 2014.03.10
 */

#pragma once

#include <cstdint> 
#include "yiqiding/net/TCPAD.h"
#include "yiqiding/io/iostream.h"
#include "yiqiding/Exception.h"
#include "yiqiding/utility/Utility.h"

namespace yiqiding { namespace ktv { namespace packet {
	// Structure definition
	/// Packet header 4 * 6 = 24 bytes
	struct Header {
		uint32_t auth;
		uint32_t version;
		uint32_t request;
		uint32_t identifier;
		uint32_t length;
		uint32_t device_id;
	};

	// Constants definition 
	enum {
		HEADER_SIZE		=	sizeof(Header),
		PACKET_AUTH		=	17,
		PACKET_VERSION	=	100,
		PACKET_MAX_LENGTH = 1024 * 100,
	};

	enum {
		BACK_NO_ERROR				=	0,
		ERROR_FORMAT				=	30000,
		ERROR_SERVER_INTER			=	30001,/*服务器内部错误*/
		ERROR_CODE_NOT_GENERATE		=	31000,/*验证码没有生成*/
		ERROR_CODE_NOT_EXIST		=	31001,/*验证码不存在*/
		ERROR_BOX_DOWN				=	31002,/*KTV机顶盒已断开*/
		ERROR_APP_DOWN				=	31003,/*手机客户端已断开*/
		ERROR_DEVICE_ID_CHANGE		=	31004,/*设备id已发生变化*/
		ERROR_REQ_NOT_SUPPORT		=	31005,/*服务不支持*/
	};

	enum Request {
		// No request
		REQ_NONE					=	0,

		//TO FROM ALL
		SECURE_ALL_TURN				=	90004,
		// KTVBox --> KTVServer
		BOX_REQ_INI_OTHER_INFO		=	10000,
		BOX_REQ_INI_INFO			=	10001,
		BOX_REQ_OPEN_ROOM			=	10002,
		/*BOX_REQ_ALLOC_SERVER		=	10003,*/
		BOX_REQ_SERVICE				=	10004,
		BOX_REQ_REALLOC_SERVER		=	10005,
		BOX_REQ_UPDATE_SONG_COUNT	=	10006,
		BOX_REQ_TMEP_CODE			=	10007,
		BOX_REQ_GPS_INFO			=	10008,
		BOX_REQ_TURN_MESSAGE		=	90002,
		BOX_REQ_TURN_MESSAGE_ALL	=	90003,
		BOX_REQ_SCORE_UPLOAD		=	10009,
		BOX_REQ_KTV_GAME			=	10010,
		BOX_REQ_BOX_LIST			=	10011,
		BOX_REQ_SCROLL_INFO			=	10012,
		BOX_REQ_SET_OPTION			=	10013,
		BOX_REQ_MSG_RULE			=	10014,
		BOX_REQ_UNIX_TIME			=	10015,
		BOX_REQ_GAME_HIS			=	11000,
		BOX_REQ_FIRE_INFO			=	10031,
		BOX_REQ_AD_URL				=	10032,
		BOX_REQ_VALIADTE			=	10033,
		BOX_REQ_VOLUME_INFO			=	10034,
		BOX_REQ_GAME_STATUS			=	10035,
		BOX_REQ_GAME_LST			=	10036,
		BOX_REQ_LOG_INFO			=	10037,
		BOX_REQ_ADDRESS_INFO		=	10041,
		BOX_REQ_OTHER_STATUS		=	10043,
		BOX_REQ_SEND_GIFT			=	10044,
		BOX_REQ_PORT_INFO			=	10046,

		BOX_REQ_TURN_MSG			=	10100,
		BOX_REQ_ACCEPT_KGAME2		=	12005,
		BOX_REQ_UPLOAD_KGAME2		=	12007,
		BOX_REQ_SCORE_KGAME2		=	12008,
		BOX_REQ_ADD_REWARD			=	13000,
		BOX_REQ_ADD_RECORD			=	13001,
		BOX_REQ_REQ_SING			=	13002,
		BOX_REQ_CANCEL_REWARD		=	13003,
		BOX_REQ_REWARD_LIST			=	13004,
		
		BOX_REQ_ADD_REWARD2			=	14000,
		BOX_REQ_GET_MY_REWARD2		=	14002,
		BOX_REQ_PLAY_REWARD2		=	14004,
		BOX_REQ_GET_LIST_REWARD2	=	14006,
		BOX_REQ_SELECT_REWARD2		=	14008,
		BOX_REQ_CANCEL_REWARD2		=	14009,
		BOX_REQ_READY_PLAY_REWARD2	=	14010,
		BOX_REQ_GAME_CHALLENGE		=	20300,
		BOX_REQ_GAME_ANSWER_CHALLENGE=	20302,
		BOX_REQ_GAME_UPLOAD			=	20304,
		BOX_REQ_GAME_CANCEL			=	20306,

		BOX_REQ_SUBMIT_SOC_INFO		=	40001,
		BOX_REQ_LIST_SOC_INFO		=	40002,
		BOX_REQ_ONE_SOC_INFO		=	40100,

		BOX_REQ_GAME2_START			=	40004,
		BOX_REQ_GAME2_EXIT			=	40005,
		BOX_REQ_GAME2_SCORE			=	40006,

		BOX_REQ_PCM_START			=	17000,
		BOX_REQ_PCM_STOP			=	17001,
		BOX_REQ_PCM_LISTEN			=	17002,
		BOX_REQ_PCM_REMOVE			=	17004,
		
		// KTVServer --> KTVBox
		KTV_REQ_BOX_TURN_MSG		=	10101,
		KTV_REQ_BOX_CONTROL_ROOM	=	10018,
		KTV_REQ_BOX_SHOW_MESSAGE	=	10019,
		KTV_REQ_BOX_CHANGE_SERVER	=	10020,
		KTV_REQ_BOX_TURN_MESSAGE	=	90012,
		KTV_REQ_BOX_GAME_NOTIFY		=	10021,
		KTV_REQ_BOX_GAME_JOIN		=	10022,
		KTV_REQ_BOX_GAME_START		=	10023,
		KTV_REQ_BOX_GAME_CANCEL		=	10024,
		KTV_REQ_BOX_GAME_SORT		=	10025,
		KTV_REQ_BOX_SCROLL_INFO		=	10026,
		KTV_REQ_BOX_CHANGE_FROM		=	10027,
		KTV_REQ_BOX_CHANGE_TO		=	10028,
		KTV_REQ_BOX_SYN_TO_BOX		=	10029,
		KTV_REQ_BOX_FIRE_SWITCH		=	10030,
		KTV_REQ_BOX_OTHER_STATUS	=	10042,
		KTV_REQ_BOX_RECV_GIFT		=	10045,

		KTV_REQ_BOX_PCM_LISTEN_ALERT =	17003,
		KTV_REQ_BOX_PCM_STOP_ALERT	=	17005,
		KTV_REQ_BOX_PCM_STOP_NOTIFY	=	17006,

		KTV_REQ_BOX_NOTIFY_KGAME2   =   12003,
		KTV_REQ_BOX_INVITE_KGAME2	=	12004,
		KTV_REQ_BOX_START_KGAME2	=	12006,
		KTV_REQ_BOX_SCORE_KGAME2	=	12009,
		KTV_REQ_BOX_NOTIFY_CANCEL	=	13005,
		BOX_REQ_ACCOUNTS_LIST       =   13006,
		KTV_REQ_BOX_NOTIFY_ADD_REWARD2=	14001,/* 通知 有新的悬赏 */
		KTV_REQ_BOX_ALERT_ENDING_REWARD2=14003,/* 提醒发起人 */
		KTV_REQ_BOX_NOTIFY_ENDED_REWARD2=14005,/* 通知所有人结束 */
		KTV_REQ_BOX_ALERT_PLAY_REWARD2	=14007,/* 提醒发起人 有人演唱 */
		KTV_REQ_BOX_ALERT_CANCEL_REWARD2	=14011, /* 提醒取消 */
		KTV_REQ_BOX_ALERT_ME_CALCEL_REWARD2 =14012,


		KTV_REQ_BOX_GAME_CHALLENGE	=	20301,
		KTV_REQ_BOX_OK_CHALLENGE	=	20303,
		KTV_REQ_BOX_GAME_SOCRE		=	20305,

		KTV_REQ_BOX_GAME2_INV		=	40014,
		KTV_REQ_BOX_GAME2_EXIT		=	40015,

		// KTVServer ---> KTVApp
		KTV_REQ_APP_TURN_MESSAGE	=	90011,
        KTV_NOTIFY_KICK_MESSAGE		=	90013,
		
		// KTVApp	----> KTVServer
		APP_REQ_TURN_MESSAGE		=	90001,
		
		APP_REQ_VALIDATE_CODE		=	50001,
		APP_REQ_SCROLL_INFO			=	91000,
		APP_REQ_RES_URL_INFO		=	90003,
		// ERP --> KTVServer
		ERP_REQ_ADD_KAGME2			=	12000,
		ERP_REQ_DEL_KGAME2			=	12001,
		ERP_REQ_GET_KGAME2			=	12002,
		ERP_REQ_CONTROL_ROOM		=	20001,
		ERP_REQ_CONTROL_BLACKLIST	=	20002,
		ERP_REQ_BROADCAST_MESSAGE	=	20003,
		ERP_REQ_SINGLE_MESSAGE		=	20004,
		ERP_REQ_OPEN_SING_GAME		=	20005,
		ERP_REQ_CLOSE_SING_GAME		=	20006,
		ERP_REQ_MODIFY_SING_GAME	=	20007,
		ERP_REQ_ALL_SING_GAME		=	20008,
		ERP_REQ_DETAIL_DATA_SING_GAME=	20009,
		ERP_ERQ_ALL_DATA_SING_GAME  =   20010,

		ERP_REQ_ADD_MUSIC			=	20011,
		ERP_REQ_ADD_SINGER			=	20012,
		ERP_REQ_ADD_ACTIVITY		=	20013,
		ERP_REQ_ADD_PLAY_LIST		=	20014,
		ERP_REQ_GET_MUSIC_LIST		=	20021,
		ERP_REQ_GET_SINGER_LIST		=	20022,
		ERP_REQ_GET_ACTIVITY_LIST	=	20023,
		ERP_REQ_GET_PLAY_LIST		=	20024,
		ERP_REQ_GET_PLAY_LIST_MUSIC	=	20025,	
		ERP_REQ_EDIT_MUSIC			=	20031,
		ERP_REQ_EDIT_SINGER			=	20032,
		ERP_REQ_EDIT_ACTIVITY_LIST	=	20033,
		ERP_REQ_EDIT_PLAY_LIST		=	20034,
		ERP_REQ_EDIT_RANK_LIST		=	20035,
		ERP_REQ_QUERY_MUSIC			=	20041,
		ERP_REQ_QUERY_SINGER		=	20042,
		ERP_REQ_QUERY_PLAY_LIST		=	20044,

		ERP_ERQ_DELETE_ACTIVITY		=	20053,
		ERP_REQ_SORT_ACTIVITY		=	20063,
		ERP_REQ_CHANGE_BOX			=	20100,
		
		ERP_REQ_UPLOAD_KTVBOX_INFO  =	30001,
		ERP_ERQ_INI_INFO			=	30002,
		ERP_REQ_SET_MSG_RULE		=	30003,
		ERP_REQ_SET_FIRE_INFO		=	30004,
		ERP_REQ_SWITCH_FIRE			=	30005,
		ERP_REQ_SET_AD_URL			=	30006,
		ERP_REQ_ONLINE_BOX_LIST		=	30007,
		ERP_REQ_RESOURCE_URL		=	30008,
		ERP_REQ_SET_VOLUME			=	30009,
		ERP_REQ_GET_VOLUME			=	30010,
		// KTVServer --> ERP
		KTV_REQ_ERP_SERVICE_CALL	=	21001,
		KTV_REQ_ERP_SCORE_UPLOAD	=	21002,
		KTV_REQ_ERP_CLOUD_UPDATE	=	30011,
		KTV_REQ_ERP_CLOUD_FINISH	=	30012,
		// General
		GENERAL_REQ_KEEPALIVE		=	11,

		KTV_NOTIFY_BOX_LOST_APP		=   22000,
		BOX_NOTIFY_KTV_LOST_APP		=   33000,
		APP_REQ_TURN_MESSAGE2       =   90005,
		APP_REQ_BOX_ID				=   30300,
		
		REQ_BOX_CHECK_DATA			=	10016,
		BOX_NOTIFY_CHECK_DATA		=	10017,
		REQ_SELF_CHECK				=   10018,
	};

	/// Packet class
	class Packet {
	private:
		/// Header + Payload.
		/// @warning only apply C memory functions. e.g. malloc(), free();
		char*	_data;
		/// The size of data that the packet totally consumed
		/// a.k.a the true data size of _data
		size_t	_consumed;

		//using in feed..
		bool	_checked;
	
	public:
		void toNetwork();
		void toHost();	
		Packet();
		Packet(Request);
		Packet(const Header&);
		Packet(const Packet&);
		//dangerous api 
		Packet(Request , size_t size);		
		~Packet();

		Packet& operator=(const Packet&);

		// Operation functions
	
		/// Feed the packet to get enough data.
		/// @param	data	data to feed
		/// @param	size	size of data
		/// @return	the size of data that the packet consumed.
		size_t	feed(const char* data, size_t size);
		//size_t  feed_origin(const char* data, size_t size);

		/// Send the packet via a Connection
		void	dispatch(net::tcp::async::Connection* conn) const;
		void	dispatch(io::ostream& out) const;
		
		//Send Raw Data via a Connection
		static void	dispatch(net::tcp::async::Connection *conn , const char *data , int len);
		
		// Setter
		inline void setAuth(uint32_t auth)				{ getHeaderM().auth = auth; };
		inline void setVersion(uint32_t version)		{ getHeaderM().version = version; };
		inline void setRequest(Request request)			{ getHeaderM().request = request; };
		inline void setIdentifier(uint32_t identifier)	{ getHeaderM().identifier = identifier; };
		inline void setDeviceID(uint32_t device_id)		{ getHeaderM().device_id = device_id; };
		void setPayload(const char* payload, size_t length);

		//dangerous api
		void setData(const char *data , size_t length);

		
		// Getter
	private:
		inline Header&			getHeaderM()			{ return *(Header*)_data; };
	public:
		inline const Header&	getHeader() const		{ return *(Header*)_data; };
		inline uint32_t			getAuth() const			{ return getHeader().auth; };
		inline uint32_t			getVersion() const		{ return getHeader().version; };
		inline Request			getRequest() const		{ return (Request)getHeader().request; };
		inline uint32_t			getIdentifier() const	{ return getHeader().identifier; };
		inline uint32_t			getLength() const		{ return getHeader().length; };
		inline uint32_t			getDeviceID() const		{ return getHeader().device_id; };
		inline const char*		getPayload() const		{ return _data + HEADER_SIZE; };
		inline bool				isFull() const			{ return _consumed >= HEADER_SIZE && _consumed == HEADER_SIZE + getLength(); };
		std::string				toString() const ;
		std::string				toLogString() const ;
	};

	// Exception
	class BadPacket : public yiqiding::Exception {
	private:
		Header _header;
	public:
		explicit BadPacket(Packet* pack, const std::string& reason, const std::string& src_file, line_t line_no) : yiqiding::Exception("Packet", reason, src_file, line_no), _header(pack->getHeader()) {};
		virtual ~BadPacket() throw() {};

		const Header& getHeader() const throw() { return _header; };
	};
}}}

namespace yiqiding { namespace ktv {
	using packet::Packet;
}}
