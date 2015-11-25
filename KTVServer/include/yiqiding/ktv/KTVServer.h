/**
 * KTV Server Application
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang,Yuchun Zhang
 * @date 2014.03.10
 */

#pragma once

#include "yiqiding/Thread.h"
#include "yiqiding/net/TCPAD.h"
#include "yiqiding/ktv/Connection.h"
#include "yiqiding/ktv/PacketManager.h"
#include "yiqiding/ktv/Balancer.h"
#include "yiqiding/ktv/db/Database.h"
#include "yiqiding/net/TelNet.h"
#include "yiqiding/ktv/ktvLua.h"
#include "yiqiding/time/wheeltimer.h"
#include "yiqiding/ktv/KTVStorage.h"
#include "yiqiding/utility/memman.h"
#include "yiqiding/ktv/PacketHeart.h"
#include "yiqiding/utility/Config.h"
#include "yiqiding/ktv/KtvMicrophoneService.h"

namespace yiqiding { namespace ktv {
	/// Default values
	enum {
		DEFAULT_PCM_LISTEN_PORT				=	47738,
		DEFAULT_BOX_LISTEN_PORT				=	58849,
		DEFAULT_ERP_LISTEN_PORT				=	25377,
		DEFAULT_APP_LISTEN_PORT				=	3050,
		DEFAULT_INI_LISTEN_PORT				=	11235,
		DEFAULT_TEL_LISTEN_PORT				=	2500,
		DEFAULT_ERP_UPLOAD_LISTEN_PORT		=	45623,
		DEFAULT_NUM_IO_THREADS				=	10,
		DEFAULT_NUM_MIN_WORKER_THREADS		=	10,
		DEFAULT_NUM_MAX_WORKER_THREADS		=	20,
		DEFAULT_MAX_CACHE_NUM				=	20,
		DEFAULT_BACK_QUEUE_SIZE				=	5,
		DEFAULT_DB_PORT						=	3306,
		DEFAULT_ERP_DB_PORT					=	3306,
		DEFAULT_LOG_REQUEST					=	0,
		DEFAULT_CONSOLE_HIDE				=	1,
		DEFAULT_SERVER_VALIDATE				=	0,
		DEFAULT_IMAGE_CACHE					=	1,

		DEFAULT_CONTROL_ENABLE				=	0,
		DEFAULT_INFOSERVER					=	0,

		DEFAULT_SAFE_APP_PORT				=	3051,
		DEFAULT_SAFE_ERP_PORT				=	25378,
		DEFAULT_SAFE_BOX_PORT				=	58850,
		DEFAULT_SAFE_INI_PORT				=	11236,

		DEFAULT_DEBUG						=	0,
	};

	static const char DEFAULT_KTV_SID[]				=	"0";
	static const char DEFAULT_SERVER_LOG_FILE[]		=	"ktvserver.log";
	static const char DEFAULT_INFO_APK_PATH[]		=	"install/KTVBox.apk";
	
	static const char DEFAULT_CASH_APK_PATH[]		=	"";
	static const char DEFAULT_CASH_APK_VERSION[]	=	"1.0";
	static const char DEFAULT_CASH_APK_IP[]		=	"127.0.0.1";
	static const char DEFAULT_CASH_APK_PORT[]		=	"1234";

	static const char DEFAULT_INFO_ROMURL[]			=	"install/update.zip";
	static const char DEFAULT_INFO_VIDEO_URL[]		=	"http://192.168.1.242:8081";
	static const char DEFAULT_INFO_IP[]				=	"192.168.1.242";
	static const char DEFAULT_INFO_ROM_VERSION[]	=	"2.100.100";
	static const char DEFAULT_INFO_VERSION[]		=	"000";
	static const char DEFAULT_INFO_WINESERVICEURL[]	=	"http://192.168.1.242:8888";
	static const char DEFAULT_INFO_TVPLAYURL[]		=	"rtsp://192.168.1.254/h264";
	static const char DEFUALT_INFO_CARA[]			=	"0";
	static const char DEFAULT_INFO_CARAURL[]		=	"install/DriverFind.apk";
	static const char DEFAULT_INFO_PORT_APK_PATH[]	=	"install/KTVBoxPort.apk";
	static const char DEFAULT_INFO_PORT_VERISON[]	=	"000";

	static const char DEFAULT_INFO_DATA_SERVER[]	=	"http://115.159.85.203:8080/luoha";

	static const char DEFAULT_DB_HOSTNAME[]	=	"127.0.0.1";
	static const char DEFAULT_DB_USERNAME[]	=	"yiqiding";
	static const char DEFAULT_DB_PASSWORD[]	=	"ktv_dev";
	static const char DEFAULT_DB_SCHEMA[]	=	"yiqiding_ktv";

	static const char DEFAULT_DB_ERP_HOSTNAME[] = "127.0.0.1";
	static const char DEFAULT_DB_ERP_USERNAME[]	= "yqc";
	static const char DEFAULT_DB_ERP_PASSWORD[] = "yqc2014";
	static const char DEFAULT_DB_ERP_SCHEMA[]	= "yqcdb";

	static const char Version[]				=   "3.2.7_win64";

	static const char DEFAULT_LOCAL_JINGDU[]	=	"121.431996";
	static const char DEFAULT_LOCAL_WEIDU[]		=	"31.194322";
	static const char DEFAULT_LOCAL_ADDRESS[]	=	"XuHong Center Road No 21 , 3th";
	static const char DEFAULT_LOCAL_CITY[]		=	"shanghai";

	static const char DEFAULT_SERVER_SAFE[]	=	"NONE";	//BOTH¡¡//ONLY

	static const char DEFAULT_APP_SERVER[]	=	"ask for Wu Chaofan";

	
	/// KTV Server
	class Server :
		public	net::tcp::async::Server,
		private	net::tcp::async::EventListener,

		private	packet::Listener,
		private net::tcp::async::ConnectionAllocator {
	protected:
		yiqiding::net::tel::ServerSend				*_srv;
		ThreadPool			_thread_pool;
		ConnectionManager	_conn_man;
		packet::Manager		_pac_man;
		Balancer			_balancer;
		Database			_database;
		HeartManger			_heart_man;
		KtvMicrophoneService* _microphone_service;
		int					_port_erp;
		int					_port_box;
		int					_port_app;
		int					_port_ini;
		int					_port_audio;

		int					_safe_port_erp;
		int					_safe_port_box;
		int					_safe_port_app;
		int					_safe_port_ini;

		std::string			_erp_hostname;
		std::string			_erp_username;
		std::string			_erp_password;
		std::string			_erp_schema;
		int					_erp_port;

		std::string			_db_hostname;
		std::string			_db_username;
		std::string			_db_password;
		std::string			_db_schema;
		int					_db_port;

		std::string			_apk_path;
		std::string			_ip;
		std::string			_videourl;
		std::string			_version;

		// cash
		std::string			_cash_apk_ip;
		std::string			_cash_apk_port;
		std::string			_cash_apk_path;
		std::string			_cash_apk_version;

		bool				_debug;

		std::string			_romVersion;
		std::string			_romUrl;
		std::string			_tvPlayUrl;
		std::string			_wineServiceUrl;
		uint32_t			_startTime;
		std::string			_jingdu;		//longtitude
		std::string			_weidu;			//latititude
		std::string			_address;		//µØÖ·
		std::string			_city;
		std::string			_appServerUrl;
		bool				_validate;
		bool				_image_cache;
		std::string			_sid;
		bool				_control_enable;//ÖÐ¿Ø
		bool				_infoserver;
		std::string			_caraVersion;
		std::string			_caraurl;

		std::string			_portapkurl;
		std::string			_portapkversion;

		std::string			_dataserver;

		// store app info
		std::map<uint32_t, std::string> _appinfo;
		Mutex	_appinfo_mutex;


#ifdef LOG_REQUEST
		bool				_req_file;
#endif
		// Connection Allocator
		net::tcp::async::Connection* alloc(net::tcp::async::SocketListener* listener, net::tcp::async::ConnectionPool* pool, net::tcp::async::EventListener* event_listener);

		// Packet Listener
		virtual void onReceivePacket(KTVConnection* conn, Packet* pac);

		// Socket Listener
		virtual void onCompleteAccept(net::tcp::async::Connection* conn);
		virtual void onCompleteRead(net::tcp::async::Connection* conn, char* data, size_t size)	{
			
			if(conn->getListenPort() == _port_audio)
			{
				uint32_t self = inet_addr(conn->getAddress().c_str());
				
				_conn_man.sendToMusic(self , data , size);
						
			}
			else
				_pac_man.onCompleteRead(conn, data, size); 
		};
		virtual void onConnectionLost(net::tcp::async::Connection* conn);
	public:
		Server(	size_t num_io_threads			=	DEFAULT_NUM_IO_THREADS,
				size_t num_min_worker_threads	=	DEFAULT_NUM_MIN_WORKER_THREADS,
				size_t num_max_worker_threads	=	DEFAULT_NUM_MAX_WORKER_THREADS,
				size_t max_cache_num			=	DEFAULT_MAX_CACHE_NUM);
		~Server();

		// Server control
#ifdef USE_SECURE_SSL
		inline void listenERP(int port,	int back_queue_size = 5, bool secured = false)	{ if(secured) _safe_port_erp = port ;  else _port_erp = port; listen(port, back_queue_size , secured); };
		inline void listenBox(int port,	int back_queue_size = 5, bool secured = false)	{ if(secured) _safe_port_box = port ; else _port_box = port; listen(port, back_queue_size , secured); };
		inline void listenApp(int port,	int back_queue_size = 5, bool secured = false)	{ if(secured) _safe_port_app = port ; else _port_app = port; listen(port , back_queue_size , secured);};
		inline void listenIni(int port,	int back_queue_size = 5, bool secured = false)	{ if(secured) _safe_port_ini = port ; else _port_ini = port; listen(port , back_queue_size , secured);}
#else

		inline void listenERP(int port, int back_queue_size = 5)	{ _port_erp = port; listen(port, back_queue_size); };
		inline void listenBox(int port, int back_queue_size = 5)	{ _port_box = port; listen(port, back_queue_size); };
		inline void listenApp(int port, int back_queue_size = 5)	{ _port_app = port; listen(port , back_queue_size);};
		inline void listenIni(int port, int back_queue_size = 5)	{ _port_ini = port; listen(port , back_queue_size);}

#endif
		inline void listenAudio(int port , int back_queue_size = 5)	{ _port_audio = port ; listen(port , back_queue_size);}



		// Inherited functions
		//void listen(int port, int back_queue_size = 5);
		//void stop();

		// phone microphone
		void initMicrophoneService();
		KtvMicrophoneService* getMicrophoneService() const   {return _microphone_service;}

		// insert/del/search appinfo
		void insertAppInfo(uint32_t app_id, std::string data);
		void deleteAppInfo(uint32_t app_id);
		std::string getAllAppInfo();

		// Getter
		inline int					getERPListeningPort()	const	{ return _port_erp; };
		inline int					getBoxListeningPort()	const	{ return _port_box; };
		inline int					getAppListeningPort()	const	{ return _port_app; };
		inline int					getIniListeningPort()	const	{ return _port_ini;}

		inline int					getERPSafeListeningPort()	const	{ return _safe_port_erp; };
		inline int					getBoxSafeListeningPort()	const	{ return _safe_port_box; };
		inline int					getAppSafeListeningPort()	const	{ return _safe_port_app; };
		inline int					getIniSafeListeningPort()	const	{ return _safe_port_ini;}

		inline int					getAudioListeningPort()		const	{ return _port_audio;}

		inline const std::string&	getAPKPath()			const	{ return _apk_path; };
		inline const std::string&	getIPAddr()				const   { return _ip;}
		inline const std::string&	getVideoUrl()			const	{ return _videourl;}
		inline const std::string&	getVersion()			const	{ return _version;}
		inline const std::string&	getRomVersion()			const	{ return _romVersion;}
		inline const std::string&	getRomUrl()				const	{ return _romUrl;}
		inline const std::string&	getTvPlayUrl()			const	{ return _tvPlayUrl;}
		inline const std::string&	getWineServiceUrl()		const	{ return _wineServiceUrl;}
		inline  ThreadPool*	getThreadPool()							{ return &_thread_pool;}
		inline  uint32_t			getStartTime()			const	{ return _startTime;}	
		inline const std::string&  getJingDu()				const	{ return _jingdu;}
		inline const std::string &	getWeiDu()				const	{ return _weidu;}
		inline const std::string &  getAddress()			const	{ return _address;}
		inline const std::string& getAppServerUrl()			const   { return _appServerUrl;}
		inline bool getValidate()							const	{ return _validate;}
		inline bool getImageCache()							const	{ return _image_cache;}
		inline const std::string getSid()					const	{ return _sid;}
		inline bool getControlenabled()						const	{ return _control_enable;}
#ifdef LOG_REQUEST
		inline bool GetRequestInFile()						const	{ return _req_file;}
#endif		
		inline const std::string& getErpUsername()			const	{return _erp_username ;}
		inline const std::string& getErpHostname()			const	{return _erp_hostname ;}
		inline const std::string& getErpPassword()			const	{return _erp_password ;}
		inline const std::string& getErpSchema()			const	{return _erp_schema ;}
		inline int	getErpPort()							const	{return _erp_port;}	
		inline yiqiding::net::tel::ServerSend * getTelNet()									{return _srv;}
		inline bool getInfoServer()							const	{return _infoserver;}
		inline const std::string& getCity()					const   {return _city;}		
		inline const std::string& getcaraVersion()			const	{return _caraVersion;}
		inline const std::string& getcaraUrl()				const	{return _caraurl;}
		inline const std::string& getPortApkUrl()			const	{return _portapkurl;}
		inline const std::string& getPortApkVersion()		const	{return _portapkversion;}
		inline const std::string& getDataServer()			const	{return _dataserver;}

		inline const std::string& getCashApkIp()			const	{return _cash_apk_ip;}
		inline const std::string& getCashApkPort()			const	{return _cash_apk_port;}
		inline const std::string& getCashApkPath()			const	{return _cash_apk_path;}
		inline const std::string& getCashApkVersion()		const	{return _cash_apk_version;}

		inline bool getDebug()		const	{return _debug;}

		// Setter
		inline void setDatabaseLogin(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db, int port = 0)	{ _database.setLogin(host, user, pwd, db, port); };
		inline void setAPKPath(const std::string& apk_path)						{ _apk_path = apk_path; };
		inline void	setIPAddr(const std::string& ip)							{ _ip = ip ;}
		inline void setVideoUrl(const std::string &url)							{ _videourl = url;}
		inline void setVersion(const std::string &version)						{ _version = version;}
	
		inline void setCashApkIp(const std::string &ip)							{ _cash_apk_ip = ip;}
		inline void setCashApkPort(const std::string &port)						{ _cash_apk_port = port;}
		inline void setCashApkPath(const std::string &path)						{ _cash_apk_path = path;}
		inline void setCashApkVersion(const std::string &version)				{ _cash_apk_version = version;}

		inline void setDebug(bool val)											{ _debug = val;}

#ifdef LOG_REQUEST
		inline void setRequestInFile(bool yes)									{ _req_file = yes;}
#endif
		inline void setRomUrl(const std::string &romUrl)						{ _romUrl = romUrl;}
		inline void setRomVersion(const std::string &romVersion)				{ _romVersion = romVersion;}
		inline void setTvPlayUrl(const std::string &tvPlayUrl)					{ _tvPlayUrl = tvPlayUrl;}
		inline void setWineServiceUrl(const std::string &wineServiceUrl)		{ _wineServiceUrl = wineServiceUrl;}
		inline void setStartTime(uint32_t seconds)								{ _startTime = seconds;}
		inline void setJingDu(const std::string &jingdu)						{ _jingdu = jingdu;}
		inline void setWeiDu(const std::string &weidu)							{_weidu = weidu;}
		inline void setAddress(const std::string &address)						{ _address = address;}
		inline void setAppServer(const std::string &appServerUrl)				{_appServerUrl = appServerUrl;}
		inline void setValidate(bool validate)									{_validate = validate;}
		inline void setImageCache(bool val)										{_image_cache = val;}

		inline void setErpUsername(const std::string &username)					{_erp_username = username;}
		inline void setErpHostname(const std::string &hostname)					{_erp_hostname = hostname;}
		inline void setErpPassword(const std::string &password)					{_erp_password = password;}
		inline void setErpSchema(const std::string &schema)						{_erp_schema = schema;}
		inline void setErpPort(int port)										{_erp_port = port;}

		// set db info
		inline void setDBUsername(const std::string &username)					{_db_username = username;}
		inline void setDBHostname(const std::string &hostname)					{_db_hostname = hostname;}
		inline void setDBPassword(const std::string &password)					{_db_password = password;}
		inline void setDBSchema(const std::string &schema)						{_db_schema = schema;}
		inline void setDBPort(int port)											{_db_port = port;}
		inline std::string getDBUsername()					{return _db_username;}
		inline std::string getDBHostname()					{return _db_hostname;}
		inline std::string getDBPassword()					{return _db_password;}
		inline std::string getDBSchema()					{return _db_schema;}
		inline int getDBPort()								{return _db_port;}


		inline void setSid(const std::string &sid)												{_sid = sid;}
		inline void setTelNet(yiqiding::net::tel::ServerSend *srv)										{_srv = srv;}
		inline void setControlenabled( bool enabled)							{ _control_enable = enabled;}
		inline void setInfoServer(bool infoserver)								{_infoserver = infoserver;}
		inline void setCity(std::string city)									{_city = city;}
		inline void setCaraVersion(std::string version)							{_caraVersion = version;}
		inline void setCaraUrl(std::string url)									{_caraurl = url;}
		inline void setPortApkUrl(const std::string &url)						{ _portapkurl = url;}
		inline void setPortApkVersion(const std::string &version)				{ _portapkversion = version;}	

		inline void setDataServer(const std::string &dataServer)				{_dataserver = dataServer;}
		// Tools
		inline ConnectionManager*	getConnectionManager()	{ return &_conn_man; };
		inline PacketManager*		getPacketManager()		{ return &_pac_man; };
		inline Balancer*			getBalancer()			{ return &_balancer; };
		inline Database*			getDatabase()			{ return &_database; };

		// Show TelNet Info
		 int showAllConnection(yiqiding::net::tel::ServerSend *srv);

		 // Debug info
		 int showBoxConnection(yiqiding::net::tel::ServerSend * srv);
		 int showAppConnection(yiqiding::net::tel::ServerSend * srv);
		 int showMusicConnection(yiqiding::net::tel::ServerSend * srv);
		 int showERPConnection(yiqiding::net::tel::ServerSend * srv);
		 int showAppBoxMapping(yiqiding::net::tel::ServerSend * srv);
		 int showAccountInfo(yiqiding::net::tel::ServerSend * srv);
		 int uploadTestFile(yiqiding::net::tel::ServerSend * srv, std::string path, std::string name);
		 int showAll(yiqiding::net::tel::ServerSend * srv);

		 int showAllVirtualConnection(yiqiding::net::tel::ServerSend *srv);
		 int showServerInfo(yiqiding::net::tel::ServerSend *srv);
		 int showRequset(yiqiding::net::tel::ServerSend *srv , int type , int id , int req , int top , bool sort);
		 int showInitRequest(yiqiding::net::tel::ServerSend *srv , const char *ip);
		 int showAllKGame(yiqiding::net::tel::ServerSend *srv);
		 int showAllKGame2(yiqiding::net::tel::ServerSend *srv);
		 
		
		 static int sshowAllKGame2(Server *s , yiqiding::net::tel::ServerSend *srv) { return s->showAllKGame2(srv);}
		 static int sshowAllKGame(Server *s , yiqiding::net::tel::ServerSend *srv) { return s->showAllKGame(srv);}
		 static int sshowAllConnection(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showAllConnection(srv);}
		 static int sshowAppConnection(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showAppConnection(srv);}
		 static int sshowERPConnection(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showERPConnection(srv);}
		 static int sshowAccountInfo(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showAccountInfo(srv);}
		 static int sshowMusicConnection(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showMusicConnection(srv);}
		 static int sshowAppBoxMapping(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showAppBoxMapping(srv);}
		 static int suploadTestFile(Server *s ,yiqiding::net::tel::ServerSend *srv, std::string path, std::string name){ return s->uploadTestFile(srv, path, name);}
		 static int sshowAll(Server *s ,yiqiding::net::tel::ServerSend * srv){ return s->showAll(srv);}

		 static int sshowBoxConnection(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showBoxConnection(srv);}
		 static int sshowAllVirtualConnection(Server *s ,yiqiding::net::tel::ServerSend *srv) { return s->showAllVirtualConnection(srv);}
		 static int sshowServerInfo(Server *s ,yiqiding::net::tel::ServerSend *srv){ return s->showServerInfo(srv);}
		 static int sshowERPRequest(Server *s , yiqiding::net::tel::ServerSend *srv , int erp_id , int req , int top , bool sort){return s->showRequset(srv , 0 , erp_id , req , top , sort);}		
		 static int sshowBOXRequest(Server *s , yiqiding::net::tel::ServerSend *srv , int box_id , int req , int top , bool sort){return s->showRequset(srv , 1 , box_id , req , top , sort);}
		 static int sshowAPPRequest(Server *s , yiqiding::net::tel::ServerSend *srv , int app_id , int req , int top , bool sort){return s->showRequset(srv , 2 , app_id , req , top , sort);}
		 static int sshowInitRequest(Server *s , yiqiding::net::tel::ServerSend *srv , const char *ip){ return s->showInitRequest(srv , ip);}
		 static int ssetVersion(Server *s , yiqiding::net::tel::ServerSend *srv , const char *version){  
				if((int)version < 0x10000)
					return 0;
				s->setVersion(version); 
				yiqiding::utility::Config conf("ktvserver.conf");
				conf.write("info" , "version" , version);
				srv->teleSend(version); return 1;}
		 static int ssetRomVersion(Server *s , yiqiding::net::tel::ServerSend *srv , const char *romVersion){ 
				if((int)romVersion < 0x10000)
					return 0;
				s->setRomVersion(romVersion); 
				yiqiding::utility::Config conf("ktvserver.conf");
				conf.write("info" , "romVersion" , romVersion);
				srv->teleSend(romVersion);return 1;}
		 static int sshowVersion(Server *s , yiqiding::net::tel::ServerSend *srv) {
		 
			 
		#ifndef NO_DONGLE 
		const std::string dongle = "dongle:yes\r\n";
		#else
		const std::string dongle = "dongle:no\r\n";
		#endif

		#ifndef _DEBUG
		const std::string v_d = "Release version\r\n";
		#else
		const std::string v_d = "Debug version\r\n";
		#endif

		#ifdef LOG_REQUEST
			const std::string log_request = "log_request:yes\r\n";
		#else
			const std::string log_request = "log_request:no\r\n";
		#endif
			
			const std::string info = v_d + dongle + log_request  +"version:"+Version  +"\r\ncomplied:" + __DATE__ +" " + __TIME__ + "\r\n";


			 return srv->teleSend(info);
		 }

		static int dofile(Server *s , yiqiding::net::tel::ServerSend *srv , const char *name)
		{
			if((int)name <= 0x10000)
				return 0;
			std::string path = "./tel/";
			path += name;
			return KtvdoLua(path , s , srv);
		}

		static int testCrash(Server *S , yiqiding::net::tel::ServerSend *srv )
		{
		#ifdef _DEBUG	
			char *data = NULL;
			strcpy(data , "testCrash");
		#endif
			return 0;
	
		}

		 static int sshowShutdown(Server *s , yiqiding::net::tel::ServerSend *srv){
			return box::BoxInfoMan::getInstace()->showInitCall(srv);
		 }

		static int showMemUsed(Server *s , yiqiding::net::tel::ServerSend *srv)
		{
			yiqiding::net::DynamicData data;
			showUsed(data);
			data.write("\r\n" , 3);
			srv->teleSend(data.getData() ,(int)data.getLenth());

			return 1;
		}

		static int showMemAvailable(Server *s , yiqiding::net::tel::ServerSend *srv)
		{
			yiqiding::net::DynamicData data;
			showAvailable(data);
			data.write("\r\n" , 3);
			srv->teleSend(data.getData() ,(int)data.getLenth());
			return 1;
		}
	};
}}
