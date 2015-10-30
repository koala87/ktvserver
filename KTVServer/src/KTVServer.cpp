/**
 * KTV Server Application Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang,YuChun Zhang
 * @date 2014.03.10
 */

#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/AppProcessor.h"
#include "yiqiding/ktv/BoxProcessor.h"
#include "yiqiding/ktv/ERPProcessor.h"
#include "yiqiding/ktv/InitProcessor.h"
#include "yiqiding/utility/Transcode.h"
#include "yiqiding/Exception.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/KTVChallenge.h"
#include "KTVKGame2.h"
using namespace yiqiding::ktv;
using yiqiding::utility::Logger;

#pragma warning(disable:4355)	// 'this' : used in base member initializer list

//////////////////////////////////////////////////////////////////////////
// Serverd
//////////////////////////////////////////////////////////////////////////
yiqiding::net::tcp::async::Connection* Server::alloc(yiqiding::net::tcp::async::SocketListener* listener, yiqiding::net::tcp::async::ConnectionPool* pool, yiqiding::net::tcp::async::EventListener* event_listener) {
#ifdef USE_SECURE_SSL
	if (listener->getPort() == _port_box || listener->getPort() == _safe_port_box)
		return new BoxConnection(listener, pool, event_listener);
	else if (listener->getPort() == _port_erp || listener->getPort() == _safe_port_erp)
		return new ERPConnection(listener, pool, event_listener);
	else if (listener->getPort() == _port_app || listener->getPort() == _safe_port_app)
		return new AppConnection(listener , pool , event_listener);
	else if(listener->getPort() == _port_ini || listener->getPort() == _safe_port_ini)
		return new IniConnection(listener , pool , event_listener);
#else
	if (listener->getPort() == _port_box)
		return new BoxConnection(listener, pool, event_listener);
	else if (listener->getPort() == _port_erp)
		return new ERPConnection(listener, pool, event_listener);
	else if (listener->getPort() == _port_app)
		return new AppConnection(listener , pool , event_listener);
	else if(listener->getPort() == _port_ini)
		return new IniConnection(listener , pool , event_listener);
#endif
	else if(listener->getPort() == _port_audio)
		return new KTVConnection(listener , pool , event_listener);
	else
		throw yiqiding::Exception("Server", "Connection from unknown port", __FILE__, __LINE__);
}

void Server::onReceivePacket(KTVConnection* conn, Packet* pac) {
	std::auto_ptr<Packet> pack(pac);
	std::auto_ptr<packet::Processor> processor;
	switch (conn->getType()) {
	case CONNECTION_BOX:
		processor.reset(new BoxProcessor(this, (BoxConnection*)conn, pack.release() , conn->getAddress() , conn->getPort()));
		break;
	case CONNECTION_ERP:
		processor.reset(new ERPProcessor(this, (ERPConnection*)conn, pack.release() , conn->getAddress() , conn->getPort()));
		break;
	case CONNECTION_APP:
		processor.reset(new AppProcessor(this , (AppConnection *)conn , pack.release() , conn->getAddress() , conn->getPort()));
		break;
	case CONNECTION_INI:
		processor.reset(new InitProcessor(this , (IniConnection *)conn, pack.release() , conn->getAddress() , conn->getPort()));
		break;
	default:
		throw Exception("Server", "Packet from unknown connection type", __FILE__, __LINE__);
	}

#ifdef USE_PACKET_HEART	
	_heart_man.set(conn->getID());
#endif
	_thread_pool.add(new SelfDestruct(processor.release()));

}

void Server::onCompleteAccept(yiqiding::net::tcp::async::Connection* conn) {

	Logger::get("server")->log("Connection " + utility::toString(conn->getID()) + " port: " + utility::toString(conn->getListenPort()) + " connected from " + conn->getAddressPort(), Logger::NORMAL);

	if(conn->getListenPort() == getAudioListeningPort())
		this->getConnectionManager()->updateMusic(inet_addr(conn->getAddress().c_str()) , conn->getID());
	

#ifdef USE_PACKET_HEART
	_heart_man.add(conn->getID());
#endif
#ifdef USE_SECURE_SSL
	if(conn->isSecured())
	_thread_pool.add(new SelfDestruct(new yiqiding::net::tcp::async::SSLAccept(conn)));
#endif
}

void Server::onConnectionLost(yiqiding::net::tcp::async::Connection* conn) {
	Logger::get("server")->log("Connection " + utility::toString(conn->getID()) + " disconnected from " + conn->getAddressPort(), Logger::NORMAL);
	// Clean up
	// KTVConnection guaranteed
	switch (((KTVConnection*)conn)->getType()) {
	case CONNECTION_BOX:
		//_balancer.release(((BoxConnection*)conn)->getBoxID());
		int boxId;
		boxId = ((BoxConnection*)conn)->getBoxID();
		_conn_man.cleanAppBoxMappingByBoxId(boxId);
		break;
	case CONNECTION_ERP:
		if (((ERPConnection*)conn)->isRoleValid())
			_conn_man.revokeERP(conn->getID());
		break;
	case CONNECTION_APP:
		int appId;
		appId = ((AppConnection*)conn)->getAppID();
		_microphone_service->noticeConnectLostApp(appId);
		_conn_man.notifyBoxAppLost(appId);
		_conn_man.cleanAppBoxMappingByAppId(appId);
		deleteAppInfo(appId);
		break;
	default:
		if (conn->getListenPort() == _port_audio){
			int ip = inet_addr(conn->getAddress().c_str());
			getConnectionManager()->removeMusicConn(ip);
		}
		break;
	}		
}

int Server::showAllConnection(yiqiding::net::tel::ServerSend *srv)
{
	return net::tcp::async::Server::showAllConnection(srv);
}

int Server::showAllKGame(yiqiding::net::tel::ServerSend *srv)
{
	return Singleton<yiqiding::ktv::ManKTVChallenge>::getInstance()->showAllKGame(srv);
}

int Server::showAllKGame2(yiqiding::net::tel::ServerSend *srv)
{
	return Singleton<yiqiding::social::KTVKGame2>::getInstance()->show(srv);
}

int Server::showAllVirtualConnection(yiqiding::net::tel::ServerSend *srv)
{
	return _conn_man.showAllVirtualConnection(srv);	
}
int Server::showServerInfo(yiqiding::net::tel::ServerSend *srv)
{
	std::ostringstream out;



	out <<"TIME:" << yiqiding::utility::getDateTime(_startTime) << "\r\n"
		<< "INI:" << getIniListeningPort()<<"\r\n"
		<< "BOX:" << getBoxListeningPort()<<"\r\n"
		<< "APP:" << getAppListeningPort()<<"\r\n"
		<< "ERP:" << getERPListeningPort()<<"\r\n"
#ifdef LOG_REQUEST
		<< "LOG_REQUEST:" << GetRequestInFile()<<"\r\n"
#endif 
		<<"hostname:"<<getDatabase()->getLoginInfo().getHostname() <<"\r\n"
		<<"port:"<<getDatabase()->getLoginInfo().getPort()<<"\r\n"
		<<"username:"<<getDatabase()->getLoginInfo().getUsername()<<"\r\n"
		<<"password:"<<getDatabase()->getLoginInfo().getPassword()<<"\r\n"
		<<"database:"<<getDatabase()->getLoginInfo().getDatabaseName()<<"\r\n"

		<<"erp_hostname:"<<getErpHostname() <<"\r\n"
		<<"erp_port:"<<getErpPort()<<"\r\n"
		<<"erp_username:"<<getErpUsername()<<"\r\n"
		<<"erp_password:"<<getErpPassword()<<"\r\n"
		<<"erp_database:"<<getErpSchema()<<"\r\n"



		<< "APK_PATH:"<<getAPKPath()<<"\r\n"
		<< "VERSION:"<<getVersion()<<"\r\n"
		<< "VIDEO_URL:" << getVideoUrl() <<"\r\n"
		<< "IP:" << getIPAddr() << "\r\n"
		<< "ROM_PATH:"<<getRomUrl()<<"\r\n"
		<< "ROM_VERSION:"<<getRomVersion() <<"\r\n"
		<< "TVPLAYURL:"<<getTvPlayUrl()<<"\r\n"
		<< "WINESERVICEURL:"<<getWineServiceUrl()<<"\r\n"
		<< "SHOPNAME:" << yiqiding::utility::transcode(yiqiding::ktv::box::BoxInfoMan::getInstace()->getShopName() ,
		yiqiding::utility::CodePage::UTF8 , yiqiding::utility::CodePage::GBK) <<"\r\n"

		<< "SAFE_INI:"<< getIniSafeListeningPort()<< "\r\n"
		<< "SAFE_BOX:"<<getBoxSafeListeningPort() <<"\r\n"
		<< "SAFE_APP:"<<getAppSafeListeningPort()<<"\r\n"
		<< "SAFE_ERP:"<<getERPSafeListeningPort()<<"\r\n"

		<<"JingDu:" << getJingDu() << "\r\n"
		<<"WeiDu:" << getWeiDu() << "\r\n"
		<<"Address:" << getAddress() << "\r\n"
		<<"AppServer:" << getAppServerUrl() <<"\r\n"
		<<"validate:" << getValidate()<<"\r\n";

		for each(auto n in _balancer.getNodes())
		{
			out << "Id:"<<n->getID()<<" Load:"<<n->getLoad()<<" Name:"<<n->getHostname()<<"\r\n";
		}


		out << "total : " << _balancer.getMapNodes().size() << " box have Resources \r\n";

		for each(auto n in _balancer.getMapNodes())
		{
			out << "boxid:"<< n.first << " Id: " << n.second->getID() << "\r\n";
		}


	

	srv->teleSend(out.str());
	return 0;
}

int Server::showRequset( yiqiding::net::tel::ServerSend *srv , int type , int id , int req , int top , bool sort)
{
	return _conn_man.showRequest(srv , type , id ,req , top , sort);
}

int Server::showInitRequest(yiqiding::net::tel::ServerSend *srv , const char *ip)
{
	return _conn_man.showInitRequest(srv , ip);
}

Server::Server(size_t num_io_threads, size_t num_min_worker_threads, size_t num_max_worker_threads, size_t max_cache_num) :
	yiqiding::net::tcp::async::Server(num_io_threads, this, this),
	_thread_pool(num_min_worker_threads, num_max_worker_threads),
	_conn_man(this, max_cache_num),
	_pac_man(this),
	_database(),
	_heart_man(this),
	_port_erp(0), _port_box(0),_port_app(0),_port_ini(0)
#ifdef USE_SECURE_SSL
	,_safe_port_erp(0),_safe_port_box(0),_safe_port_app(0),_safe_port_ini(0)
#endif

#ifdef LOG_REQUEST
	,_req_file(false)
#endif

	{}


Server::~Server() {}

void Server::initMicrophoneService() {
	Logger::get("server")->log("start microphone service ...", Logger::NORMAL);
	_microphone_service = new KtvMicrophoneService(&_conn_man); 
}

// insert/del/search appinfo
void Server::insertAppInfo(uint32_t app_id, std::string data){
	MutexGuard lock(_appinfo_mutex);

	_appinfo[app_id] = data;
}
void Server::deleteAppInfo(uint32_t app_id){
	MutexGuard lock(_appinfo_mutex);

	if(_appinfo.find(app_id)!=_appinfo.end()){
		_appinfo.erase(app_id);
	}
	Logger::get("server")->log("delete app info app_id " + utility::toString(app_id), Logger::NORMAL);
}
std::string Server::getAllAppInfo(){
	MutexGuard lock(_appinfo_mutex);

	static std::string buffer;
	buffer.clear();
	Json::Value root, users;
	for(auto it=_appinfo.begin();it!=_appinfo.end();it++){
		Json::Reader reader;
		Json::Value user;
		if(!reader.parse(it->second , user))
			continue;
		users.append(user);
	}
	root["users"] = users;
	buffer = root.toStyledString();
	return buffer;
} 
