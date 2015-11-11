/**
 * KTV Server Main Entrance
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang , Yuchun Zhang
 * @date 2014.03.10
 */

#include <string>
#include <cstdlib>
#include <memory>
#include <atlstr.h>
#include "yiqiding/net/KTVOFtp.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/utility/Config.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/net/TelNet.h"
#include "yiqiding/ktv/GameTimer.h"
#include "yiqiding/ktv/BoxInfoMan.h"
#include "yiqiding/ktv/ServerCrash.h"
#include "yiqiding/ktv/MessageRule.h"
#include "yiqiding/io/File.h"
#include "yiqiding/ktv/FireWarn.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/LogDelayUpLoad.h"
#include "yiqiding/ktv/AdUrl.h"
#include "yiqiding/ktv/Volume.h"
#include "yiqiding/ktv/BoxData.h"
#include "yiqiding/io/KtvDoCmd.h"
#include "yiqiding/ktv/KTVTimerPlan.h"
#include "yiqiding/ktv/db/GameTimer2.h"
#include "yiqiding/ktv/MultiScreen.h"
#include "yiqiding/ktv/KTVCloud.h"
#include "yiqiding/ktv/KTVPCM.h"

#ifndef NO_DONGLE
#include "yiqiding/ktv/Dongle.h"
#endif
using namespace yiqiding;
using yiqiding::utility::Logger;
using yiqiding::utility::Config;
using yiqiding::utility::ConfigSector;
using yiqiding::net::tel::TelNetServer;

// Global Variable (system events)
static HANDLE ___stop_event = INVALID_HANDLE_VALUE;
static int __update_hour = 10;
static int __update_min = 19;
static int __update_second = 19;

// Prototypes
BOOL WINAPI consoleHandler(DWORD ctrl);

extern int g_debug = 0; 

// Program entrance
int main(int argc, const char* argv[]) {
	try {
		// Prepare 
		___stop_event = CreateEvent(0, FALSE, FALSE, 0);
		SetConsoleCtrlHandler(consoleHandler, TRUE);

		// Configure KTV Server
		std::auto_ptr<ktv::Server> ktv_server;
		std::auto_ptr<yiqiding::net::tel::TelNetServer> telnet_server;
		std::auto_ptr<ktv::KTVStorage>	ktv_storage;
		std::auto_ptr<yiqiding::net::KTVFtp> ftp;
		std::string lastmsg = "";
		//srand
		srand((unsigned int)::time(NULL));
		{
			// Load config file
			Config ktv_conf;
			try {
				if (argc > 1)
					ktv_conf.load(argv[1]);
				else
					ktv_conf.load("KTVServer.conf");
			} catch (const yiqiding::Exception& ) {
				lastmsg = "You are loading default setting.";
			}

			// Prepare
			std::string value;
			ConfigSector sector;

			// Load console configuration
			if(ktv_conf["console"].getInt("hide" , yiqiding::ktv::DEFAULT_CONSOLE_HIDE ) == 1)
				FreeConsole();


			// Load log configuration
			value = ktv_conf["server"].getValue("log_file" , yiqiding::ktv::DEFAULT_SERVER_LOG_FILE);
			// Set KTV Server log file
			Logger::get("server", value);
			// Merge Logger (internal system)
			Logger::alias("server", "system");
			Logger::alias("server", "node");
			Logger::alias("server" , "storage");

			

			if(!lastmsg.empty())
				Logger::get("server")->log(lastmsg , Logger::WARNING);

			// Prepare server
			size_t	num_io_threads			=	ktv_conf["server"].getInt("num_io_threads" , yiqiding::ktv::DEFAULT_NUM_IO_THREADS);
			size_t	num_min_worker_threads	=	ktv_conf["server"].getInt("num_min_worker_threads" ,yiqiding::ktv::DEFAULT_NUM_MIN_WORKER_THREADS);
			size_t	num_max_worker_threads	=	ktv_conf["server"].getInt("num_max_worker_threads" ,yiqiding::ktv::DEFAULT_NUM_MAX_WORKER_THREADS);
			size_t	max_cache_num			=	ktv_conf["server"].getInt("max_cache_num" ,yiqiding::ktv::DEFAULT_MAX_CACHE_NUM);
			int		back_queue_size			=	ktv_conf["server"].getInt("back_queue_size" ,yiqiding::ktv::DEFAULT_BACK_QUEUE_SIZE);
			int		pcm_listen_port			=	ktv_conf["server"].getInt("pcm_listen_port" , yiqiding::ktv::DEFAULT_PCM_LISTEN_PORT);
			int		box_listen_port			=	ktv_conf["server"].getInt("box_listen_port" ,yiqiding::ktv::DEFAULT_BOX_LISTEN_PORT);
			int		erp_listen_port			=	ktv_conf["server"].getInt("erp_listen_port" ,yiqiding::ktv::DEFAULT_ERP_LISTEN_PORT);
			int		app_listen_port			=	ktv_conf["server"].getInt("app_listen_port" ,yiqiding::ktv::DEFAULT_APP_LISTEN_PORT);
			int		ini_listen_port			=	ktv_conf["server"].getInt("ini_listen_port" ,yiqiding::ktv::DEFAULT_INI_LISTEN_PORT);
			int		tel_listen_port			=	ktv_conf["server"].getInt("tel_listen_port" ,yiqiding::ktv::DEFAULT_TEL_LISTEN_PORT);
			int		upload_listen_port		=	ktv_conf["server"].getInt("upload_listen_port" , yiqiding::ktv::DEFAULT_ERP_UPLOAD_LISTEN_PORT);
			int		log_request				=   ktv_conf["server"].getInt("log_request" , yiqiding::ktv::DEFAULT_LOG_REQUEST);
			
			std::string		use_safe_app	=	ktv_conf["server"].getValue("use_safe_app" , yiqiding::ktv::DEFAULT_SERVER_SAFE);
			std::string		use_safe_erp	=	ktv_conf["server"].getValue("use_safe_erp" , yiqiding::ktv::DEFAULT_SERVER_SAFE);
			std::string		use_safe_box	=	ktv_conf["server"].getValue("use_safe_box" , yiqiding::ktv::DEFAULT_SERVER_SAFE);
			std::string		use_safe_ini	=	ktv_conf["server"].getValue("use_safe_ini" , yiqiding::ktv::DEFAULT_SERVER_SAFE);

			int		app_safe_port			=	ktv_conf["server"].getInt("app_safe_port" , yiqiding::ktv::DEFAULT_SAFE_APP_PORT);
			int		box_safe_port			=	ktv_conf["server"].getInt("box_safe_port" , yiqiding::ktv::DEFAULT_SAFE_BOX_PORT);
			int		erp_safe_port			=	ktv_conf["server"].getInt("erp_safe_port" , yiqiding::ktv::DEFAULT_SAFE_ERP_PORT);
			int		ini_safe_port			=	ktv_conf["server"].getInt("ini_safe_port" , yiqiding::ktv::DEFAULT_SAFE_INI_PORT);

			ktv_server.reset(new ktv::Server(num_io_threads, num_min_worker_threads, num_max_worker_threads, max_cache_num));
#ifdef LOG_REQUEST
			ktv_server->setRequestInFile(log_request == 1);
#endif


#ifndef NO_DONGLE
			yiqiding::ktv::Dongle::getInstance()->setUserPassword(ktv_conf["dongle"].getValue("password" , yiqiding::ktv::DEFAULT_DONGLE_USER_PASSWORD));
			yiqiding::ktv::Dongle::getInstance()->checkPassword();
#endif


			ktv_storage.reset(new ktv::KTVStorage(ktv_server->getThreadPool()));
			
			time::CTimerManager::getInstance(ktv_server->getThreadPool());

			//start telnet server
			telnet_server.reset(new TelNetServer());
			telnet_server->Start(tel_listen_port);
			telnet_server->addFunc("showAllConnection" , (net::tel::ExeFunc)ktv::Server::sshowAllConnection, "show all connection to server\r\n" ,  ktv_server.get());
			// add debug functions
			telnet_server->addFunc("showBoxConnection" , (net::tel::ExeFunc)ktv::Server::sshowBoxConnection, "show all box connections to server\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showAppConnection" , (net::tel::ExeFunc)ktv::Server::sshowAppConnection, "show all app connections to server\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showERPConnection" , (net::tel::ExeFunc)ktv::Server::sshowERPConnection, "show all erp connections to server\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showMusicConnection" , (net::tel::ExeFunc)ktv::Server::sshowMusicConnection, "show app/box mapping\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showAccountInfo" , (net::tel::ExeFunc)ktv::Server::sshowAccountInfo, "show app acounts info\r\n" ,  ktv_server.get());
			telnet_server->addFunc("uploadTestFile" , (net::tel::ExeFunc)ktv::Server::suploadTestFile, "upload test file\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showAppBoxMapping" , (net::tel::ExeFunc)ktv::Server::sshowAppBoxMapping, "show app/box mapping\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showAll" , (net::tel::ExeFunc)ktv::Server::sshowAll, "show all information\r\n" ,  ktv_server.get());

			telnet_server->addFunc("showAllVirtualConnection" , (net::tel::ExeFunc)ktv::Server::sshowAllVirtualConnection , "show all virtual connection to server\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showServerInfo" , (net::tel::ExeFunc)ktv::Server::sshowServerInfo, "show Server info\r\n" ,  ktv_server.get());
			telnet_server->addFunc("showVersion" , (net::tel::ExeFunc)ktv::Server::sshowVersion, "show current version\r\n");
			telnet_server->addFunc("showERPRequest" , (net::tel::ExeFunc)ktv::Server::sshowERPRequest,
				"show erp request , args(int erp_id , int req , int top , int sort) : req = -1 , top = -1 meaning all , sort = 0 meaning increasing  \r\n" , ktv_server.get());
			
			telnet_server->addFunc("showBOXRequest" , (net::tel::ExeFunc)ktv::Server::sshowBOXRequest, 
				"show box request , args(int box_id , int req , int top , int sort) : req = -1 , top = -1 meaning all , sort = 0 meaning increasing  \r\n" , ktv_server.get());
			telnet_server->addFunc("showAPPRequest" , (net::tel::ExeFunc)ktv::Server::sshowAPPRequest,
				"show app request , args(int app_id , int req , int top , int sort) : req = -1 , top = -1 meaning all , sort = 0 meaning increasing  \r\n" , ktv_server.get());
			telnet_server->addFunc("showINITRequest" , (net::tel::ExeFunc)ktv::Server::sshowInitRequest , "show init request , args(ip) , \r\n" , ktv_server.get());
			telnet_server->addFunc("setVersion" , (net::tel::ExeFunc)ktv::Server::ssetVersion , "setVersion , args(version) \r\n" , ktv_server.get());
			telnet_server->addFunc("setRomVersion" , (net::tel::ExeFunc)ktv::Server::ssetRomVersion , "setromversion , args(romversion) \r\n" , ktv_server.get());

			telnet_server->addFunc("dofile" ,  (net::tel::ExeFunc)ktv::Server::dofile , "lua dofile , args( string name  )\r\n" , ktv_server.get());

			telnet_server->addFunc("testCrash" , (net::tel::ExeFunc)ktv::Server::testCrash , "test Crash\r\n" , ktv_server.get());

			telnet_server->addFunc("showMemUsed" , (net::tel::ExeFunc)ktv::Server::showMemUsed , "show mem pool used\r\n" , ktv_server.get());

			telnet_server->addFunc("showMemAvailable" , (net::tel::ExeFunc)ktv::Server::showMemAvailable , "show mem pool available\r\n" , ktv_server.get());

			telnet_server->addFunc("showAllKGame" , (net::tel::ExeFunc)ktv::Server::sshowAllKGame , "show ktv challenge \r\n" , ktv_server.get());
			telnet_server->addFunc("showAllKGame2" , (net::tel::ExeFunc)ktv::Server::sshowAllKGame2 , "show ktv game2 \r\n" , ktv_server.get());
			telnet_server->addFunc("showshutdown" , (net::tel::ExeFunc)ktv::Server::sshowShutdown , "show shut down time \r\n" , ktv_server.get());
			// Load MySQL Settings
			std::string mysql_hostname				=	ktv_conf["database"].getValue("hostname", yiqiding::ktv::DEFAULT_DB_HOSTNAME);
			int			mysql_port					=	ktv_conf["database"].getInt("port" , yiqiding::ktv::DEFAULT_DB_PORT);	
			std::string mysql_username				=	ktv_conf["database"].getValue("username" , yiqiding::ktv::DEFAULT_DB_USERNAME);
			std::string mysql_password				=	ktv_conf["database"].getValue("password" , yiqiding::ktv::DEFAULT_DB_PASSWORD);
			std::string mysql_database				=	ktv_conf["database"].getValue("database" , yiqiding::ktv::DEFAULT_DB_SCHEMA);


			ktv_server->setErpHostname(ktv_conf["database"].getValue("erp_hostname" , yiqiding::ktv::DEFAULT_DB_ERP_HOSTNAME));
			ktv_server->setErpPort(ktv_conf["database"].getInt("erp_port" , yiqiding::ktv::DEFAULT_ERP_DB_PORT));
			ktv_server->setErpUsername(ktv_conf["database"].getValue("erp_username" , yiqiding::ktv::DEFAULT_DB_ERP_USERNAME));
			ktv_server->setErpPassword(ktv_conf["database"].getValue("erp_password" , yiqiding::ktv::DEFAULT_DB_ERP_PASSWORD));
			ktv_server->setErpSchema(ktv_conf["database"].getValue("erp_database" , yiqiding::ktv::DEFAULT_DB_ERP_SCHEMA));

			
			ktv_server->setDatabaseLogin(mysql_hostname, mysql_username, mysql_password, mysql_database, mysql_port);
			ktv_server->setStartTime((uint32_t)::time(NULL));

			// Load Info
			ktv_server->setCaraVersion(ktv_conf["info"].getValue("carapkversion" , yiqiding::ktv::DEFUALT_INFO_CARA));
			ktv_server->setCaraUrl(ktv_conf["info"].getValue("carapkurl" , yiqiding::ktv::DEFAULT_INFO_CARAURL));
			ktv_server->setPortApkUrl(ktv_conf["info"].getValue("portapkurl" , yiqiding::ktv::DEFAULT_INFO_PORT_APK_PATH));
			ktv_server->setPortApkVersion(ktv_conf["info"].getValue("portapkversion" , yiqiding::ktv::DEFAULT_INFO_PORT_VERISON));
			ktv_server->setAPKPath(ktv_conf["info"].getValue("apk_path" , yiqiding::ktv::DEFAULT_INFO_APK_PATH));
			ktv_server->setVersion(ktv_conf["info"].getValue("version" , yiqiding::ktv::DEFAULT_INFO_VERSION));

			ktv_server->setCashApkIp(ktv_conf["info"].getValue("cash_apk_ip" , yiqiding::ktv::DEFAULT_CASH_APK_IP));
			ktv_server->setCashApkPort(ktv_conf["info"].getValue("cash_apk_port" , yiqiding::ktv::DEFAULT_CASH_APK_PORT));
			ktv_server->setCashApkPath(ktv_conf["info"].getValue("cash_apk_path" , yiqiding::ktv::DEFAULT_CASH_APK_PATH));
			ktv_server->setCashApkVersion(ktv_conf["info"].getValue("cash_apk_version" , yiqiding::ktv::DEFAULT_CASH_APK_VERSION));

			ktv_server->setDebug(ktv_conf["info"].getInt("debug" , yiqiding::ktv::DEFAULT_DEBUG));

			g_debug = ktv_server->getDebug();

			ktv_server->setRomUrl(ktv_conf["info"].getValue("romUrl" , yiqiding::ktv::DEFAULT_INFO_ROMURL));
			ktv_server->setRomVersion(ktv_conf["info"].getValue("romVersion" , yiqiding::ktv::DEFAULT_INFO_ROM_VERSION));
			ktv_server->setVideoUrl(ktv_conf["info"].getValue("video_url" , yiqiding::ktv::DEFAULT_INFO_VIDEO_URL));
			ktv_server->setIPAddr(ktv_conf["info"].getValue("ip" , yiqiding::ktv::DEFAULT_INFO_IP));		
			ktv_server->setTvPlayUrl(ktv_conf["info"].getValue("tvPlayUrl" , yiqiding::ktv::DEFAULT_INFO_TVPLAYURL));
			ktv_server->setWineServiceUrl(ktv_conf["info"].getValue("wineServiceUrl" , yiqiding::ktv::DEFAULT_INFO_WINESERVICEURL));
			ktv_server->setValidate(ktv_conf["info"].getInt("validate" , yiqiding::ktv::DEFAULT_SERVER_VALIDATE));
			ktv_server->setImageCache(ktv_conf["info"].getInt("image_cache" , yiqiding::ktv::DEFAULT_IMAGE_CACHE));
			ktv_server->setDataServer(ktv_conf["info"].getValue("dataserver" , yiqiding::ktv::DEFAULT_INFO_DATA_SERVER));
			ktv_server->setJingDu(ktv_conf["local"].getValue("jingdu" , yiqiding::ktv::DEFAULT_LOCAL_JINGDU));
			ktv_server->setWeiDu(ktv_conf["local"].getValue("weidu" , yiqiding::ktv::DEFAULT_LOCAL_WEIDU));
			ktv_server->setAddress(ktv_conf["local"].getValue("address" , yiqiding::ktv::DEFAULT_LOCAL_ADDRESS));
			ktv_server->setCity(ktv_conf["local"].getValue("city" , yiqiding::ktv::DEFAULT_LOCAL_CITY));
			ktv_server->setControlenabled(ktv_conf["local"].getInt("controlenabled" , yiqiding::ktv::DEFAULT_CONTROL_ENABLE));
			ktv_server->initMicrophoneService();

			std::string sid = ktv_conf["local"].getValue("sid" , yiqiding::ktv::DEFAULT_KTV_SID);
			ktv_server->setSid(sid);
			ktv_server->setTelNet(telnet_server.get());

			bool infoserver = ktv_conf["info"].getInt("infoserver" , yiqiding::ktv::DEFAULT_INFOSERVER);
			ktv_server->setInfoServer(infoserver);
			// Load Balance Servers
			ktv::Balancer* balancer = ktv_server->getBalancer();
			sector = ktv_conf["node_list"];
			for each (auto node in sector)
			{	
				balancer->addNode(atoi(node.first.c_str()), node.second);
				yiqiding::net::DistributeContent::getInstance()->addHost(new yiqiding::net::DistributeCurl(node.second + "/" + "upload.php"));
			}
			{

				std::string update_time = ktv_conf["info"].getValue("updatetime" , "10:19:19");
				auto ts = yiqiding::utility::split(update_time , ':');
				if(ts.size() >= 3)
				{
					__update_hour = yiqiding::utility::toInt(ts[0]);
					__update_min = yiqiding::utility::toInt(ts[1]);
					__update_second = yiqiding::utility::toInt(ts[2]);
				}
			}

			yiqiding::CrashDump::setCrashProcess(new yiqiding::ktv::ServerCrash());
			yiqiding::ktv::game::SingerGame::getInstace()->load(ktv_server.get() , "game.xml");
			yiqiding::ktv::MessageRule::getInstance()->load(yiqiding::ktv::MSG_PATH);

			yiqiding::ktv::box::BoxInfoMan::getInstace(ktv_server.get());

			if(yiqiding::io::File::isExist("box.xml") && !yiqiding::ktv::box::BoxInfoMan::getInstace(ktv_server.get())->load("box.xml"))
			{
				return Logger::get("server")->log(yiqiding::utility::toString("update box.xml error , check yiqiding_ktv database") , Logger::WARNING);
			}

			Singleton<yiqiding::ktv::LogDelayUpload>::getInstance()->setServer(ktv_server.get());
			Singleton<yiqiding::ktv::LogDelayUpload>::getInstance()->load("cache");
			Singleton<yiqiding::ktv::LogDelayUpload>::getInstance()->load("zip");

			if(yiqiding::io::File::isExist(ktv::fire::path) && !Singleton<yiqiding::ktv::fire::FireWarn>::getInstance()->load(ktv::fire::path))
			{
				return Logger::get("server")->log("load " + ktv::fire::path , Logger::WARNING);
			}

			Singleton<yiqiding::ktv::AdUrl>::getInstance()->load(yiqiding::ktv::AdPath);

			Singleton<yiqiding::ktv::Volume>::getInstance()->load(yiqiding::ktv::VlPath);
			Singleton<yiqiding::ktv::KTVGameTimer2>::getInstance()->load(ktv_server.get());
			{
				yiqiding::ktv::ServerInfo info = {sid , yiqiding::ktv::box::BoxInfoMan::getInstace()->getShopName() ,"000" , yiqiding::ktv::Version , "000" , "000"};
				Singleton<yiqiding::ktv::KTVBoxData>::getInstance()->load("data" , info , 10);
			}
			Singleton<yiqiding::ktv::MultiScreen>::getInstance()->load();

			// Server start
#ifdef USE_SECURE_SSL
			if(use_safe_ini != "ONLY")
			{
				ktv_server->listenIni(ini_listen_port, back_queue_size);
				Logger::get("server")->log("listening Ini at " + utility::toString(ini_listen_port));
			}
			
			if(use_safe_ini == "BOTH" || use_safe_ini == "ONLY")
			{
				ktv_server->listenIni(ini_safe_port , back_queue_size , true);
				Logger::get("server")->log("listening Ini Safe at " + utility::toString(ini_safe_port));
			}

			if(use_safe_box != "ONLY")
			{
				ktv_server->listenBox(box_listen_port, back_queue_size);
				Logger::get("server")->log("listening Box at " + utility::toString(box_listen_port), Logger::NORMAL);
			}

			if(use_safe_box == "BOTH" || use_safe_box == "ONLY")
			{
				ktv_server->listenBox(box_safe_port, back_queue_size , true);
				Logger::get("server")->log("listening Box Safe at " + utility::toString(box_safe_port), Logger::NORMAL);
			}

			if(use_safe_erp != "ONLY")
			{
				ktv_server->listenERP(erp_listen_port, back_queue_size);
				Logger::get("server")->log("listening ERP at " + utility::toString(erp_listen_port), Logger::NORMAL);
			}

			if(use_safe_erp == "BOTH" || use_safe_erp == "ONLY")
			{
				ktv_server->listenERP(erp_safe_port, back_queue_size , true);
				Logger::get("server")->log("listening ERP Safe at " + utility::toString(erp_safe_port), Logger::NORMAL);
			}

			if(use_safe_app != "ONLY")
			{
				ktv_server->listenApp(app_listen_port , back_queue_size);
				Logger::get("server")->log("listening App at " + utility::toString(app_listen_port) , Logger::NORMAL);
			}

			if(use_safe_app  == "BOTH" || use_safe_app == "ONLY")
			{
				ktv_server->listenApp(app_safe_port , back_queue_size , true);
				Logger::get("server")->log("listening App Safe at " + utility::toString(app_safe_port) , Logger::NORMAL);
			}


#else
			ktv_server->listenIni(ini_listen_port, back_queue_size);
			Logger::get("server")->log("listening Ini at " + utility::toString(ini_listen_port));

			ktv_server->listenBox(box_listen_port, back_queue_size);
			Logger::get("server")->log("listening Box at " + utility::toString(box_listen_port), Logger::NORMAL);

			ktv_server->listenERP(erp_listen_port, back_queue_size);
			Logger::get("server")->log("listening ERP at " + utility::toString(erp_listen_port), Logger::NORMAL);

			ktv_server->listenApp(app_listen_port , back_queue_size);
			Logger::get("server")->log("listening App at " + utility::toString(app_listen_port) , Logger::NORMAL);

#endif
			ktv_server->listenAudio(pcm_listen_port , back_queue_size);
			Logger::get("server")->log("listening pcm at " + utility::toString(pcm_listen_port) , Logger::NORMAL);
			
			Singleton<yiqiding::ktv::KTVPCM>::getInstance()->setServer(ktv_server.get());

			ktv_storage->listen(upload_listen_port);
			ktv_storage->start();
			
			{
				char path[256];
				GetCurrentDirectoryA(255 , path);
				ftp.reset(new yiqiding::net::KTVFtp());
				ftp->start();
				ftp->adduser(FTP_C | FTP_R | FTP_D | FTP_X | FTP_W , "root" , "spring" , path , "/");
				ftp->adduser(FTP_R , "user" , "ktv" , path , path);
				
			}

			Logger::get("server")->log(yiqiding::utility::toString("Server has started successfully , V:") + yiqiding::ktv::Version);
		}	// Release config memory
		// Wait stop event

		bool confirm = Singleton<yiqiding::ktv::cloud::MusicCloud>::getInstance()->load(ktv_server.get());
		yiqiding::io::TimerPlan<yiqiding::ktv::cloud::MusicCloud> tp1(yiqiding::io::TimerofRound::makeTimebyday(__update_hour,__update_min,__update_second));
		if(confirm){
			tp1.setPt(Singleton<yiqiding::ktv::cloud::MusicCloud>::getInstance());
			tp1.start();
		}
		yiqiding::io::TimerPlan<yiqiding::ktv::UpLoadCoreLog> tp(yiqiding::io::TimerofRound::makeTimebyday(5,21,4));
		tp.start();

		WaitForSingleObject(___stop_event, INFINITE);

		yiqiding::ktv::game::SingerGame::getInstace()->save("game.xml");

		yiqiding::ktv::game::SingerGame::unLoad();

		time::CTimerManager::unLoad();

		// Stop then telnet
		telnet_server->Stop();

		ktv_storage->stop();

		// Stop the server
		ktv_server->stop();
		

		ftp->stop();

		Logger::get("server")->log("server stopped", Logger::NORMAL);


		Singleton<yiqiding::ktv::LogDelayUpload>::unLoad();
		yiqiding::ktv::box::BoxInfoMan::unLoad();

		// Release server memory (automatic)
		// ktv_server.reset();
	} 
#ifndef NO_DONGLE 
	catch (const yiqiding::ktv::DongleException& e) {
		//MessageBox(NULL, TEXT("Çë²åÈëÖÇÄÜËø"), TEXT("KTVServer Fatal Error"), MB_OK | MB_ICONERROR);
		std::string msg = e.what();
		Logger::get("server")->log(msg + " Dongle Error" , Logger::FATAL);
		return EXIT_FAILURE;
	} 
#endif	
	catch (const std::exception& e) {
		//ATL::CString err_msg(e.what());
		//MessageBox(NULL, err_msg, TEXT("KTVServer Fatal Error"), MB_OK | MB_ICONERROR);

		std::string msg = e.what();
		Logger::get("server")->log(msg + " KTVServer Fatal Error" , Logger::FATAL);
		return EXIT_FAILURE;
	}
	yiqiding::net::Socket::Driver::unload();
	Logger::unload();

	// Release system memory
	SetConsoleCtrlHandler(NULL, FALSE);
	CloseHandle(___stop_event);

	return EXIT_SUCCESS;
}





BOOL WINAPI consoleHandler(DWORD ctrl) {
	switch (ctrl) {
//	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		SetEvent(___stop_event);
		return FALSE;
	default:
		return TRUE;
	}
}
