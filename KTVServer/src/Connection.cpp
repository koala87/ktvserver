/**
 * Virtual Connection (Middle Layer) Implementation
 * @author Shiwei Zhang,Yuchun Zhang
 * @date 2014.03.10
 */

#include <memory>
#include <ctime>
#include "yiqiding/ThreadW1Rn.h"
#include "yiqiding/ktv/Connection.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/utility/Utility.h"
#include "ws2tcpip.h"
using namespace yiqiding::ktv::extended;
using namespace yiqiding::net;
using namespace yiqiding::utility;
using yiqiding::ktv::Packet;
using yiqiding::ktv::ERPRole;
using namespace yiqiding::ktv::packet;

//////////////////////////////////////////////////////////////////////////
// ConnectionCacheManager
//////////////////////////////////////////////////////////////////////////

ConnectionCacheManager::~ConnectionCacheManager() {
	{
		MutexGuard lock(_cache_mutex);

		for (auto i = _cache.begin(); i != _cache.end(); ++i)
		delete i->second;
		_cache.clear();
	}
	{
		MutexWriter wr(_record_mutex);

		for (auto i = _record.begin() ; i != _record.end(); ++i)
			delete *i;
		_record.clear();
	}
}


void ConnectionCacheManager::cache(const Packet* out_pac, const Packet* refer_pac) {
	Cache* cache = new Cache(new Packet(*out_pac), new Packet(*refer_pac));
	{
		MutexGuard lock(_cache_mutex);
		auto cache_itr = _cache.find(out_pac->getIdentifier());
		if (cache_itr != _cache.end()) {
			delete cache_itr->second;
			_cache.erase(cache_itr);
		}
		if (_cache.size() < _max_cache_num)
			_cache[out_pac->getIdentifier()] = cache;
		else
			delete cache;
	}
}

ConnectionCache* ConnectionCacheManager::findCache(size_t identifier) {
	Cache* cache = NULL;
	{
		MutexGuard lock(_cache_mutex);
		auto cache_itr = _cache.find(identifier);
		if (cache_itr != _cache.end()) {
			cache = cache_itr->second;
			_cache.erase(cache_itr);
		}
	}
	return cache;
}


void ConnectionCacheManager::record_push( RecordPacket *inpack)
{
	yiqiding::MutexWriter guard(_record_mutex);
	if(_record.size() == RECORD_INIT_NUM)
	{
		std::list<RecordPacket *>::iterator itor = _record.begin();
		int index = 0;
		while(index < RECORD_INC_NUM)
		{
			delete *itor;
			itor = _record.erase(itor);
			++index;
		}	
	}
	_record.push_back(inpack);
}

void ConnectionCacheManager::showReq(yiqiding::net::tel::ServerSend *srv , uint32_t deviceId)
{
	std::ostringstream out;
	{
		yiqiding::MutextReader guard(_record_mutex);
		int index = 0;
		for each(auto p in _record)
		{
			if(p->getReqPack()->getDeviceID() == deviceId)
			{
				out << index++ <<" " << p->toString() << "\r\n";
				++index;		
			}
		}

		srv->teleSend(out.str());
	}
}


void ConnectionCacheManager::showReq(yiqiding::net::tel::ServerSend *srv , int req , int top , bool sort)
{
	std::ostringstream out;
	{
		yiqiding::MutextReader guard(_record_mutex);

		if(!sort)
		{
			std::list<RecordPacket*>::iterator it =  _record.begin();
			int index = 0;
			while(it != _record.end())
			{
				if (top != -1 && index >= top)
				{
					break;
				}
				if((*it)->getReqPack()->getRequest() == req)
				{
					out << index++ <<" " << (*it)->toString() << "\r\n";
				}

				++it;
			}
		}
		else
		{
			std::list<RecordPacket*>::reverse_iterator it = _record.rbegin();
			int index = 0;
			while(it != _record.rend())
			{
				if (top != -1 && index >= top)
				{
					break;
				}
				if((*it)->getReqPack()->getRequest() == req)
				{
					out << index++ <<" " << (*it)->toString() << "\r\n";
				}
		
				++it;
			}
		}

	}

	srv->teleSend(out.str());
}


void ConnectionCacheManager::showAllReq(yiqiding::net::tel::ServerSend *srv , int top , bool sort)
{
	std::ostringstream out;
	{
		yiqiding::MutextReader guard(_record_mutex);
		int count = (int)_record.size();
		int index = 0;
		out << "\r\nall total:" << count << "\r\n\r\n";
		
		if(!sort)
		{	
			std::list<RecordPacket *>::iterator it = _record.begin();
			while(it != _record.end())
			{			
				if (top != -1 && index >= top)
				{
					break;
				}
				out <<"id:"<< index++ <<" " << (*it)->toString() << "\r\n";
				++it;
	
			}

		}
		else
		{
			std::list<RecordPacket *>::reverse_iterator it = _record.rbegin();
			while(it != _record.rend())
			{
				if (top != -1 && index >= top)
				{
					break;
				}
				out <<"id:"<< index++ <<" " << (*it)->toString() << "\r\n";
				++it;
		
			}
		}
	}

	srv->teleSend(out.str());
}





//////////////////////////////////////////////////////////////////////////
// Connection
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// ConnectionManager
//////////////////////////////////////////////////////////////////////////

ConnectionManager::~ConnectionManager() {
	{
		MutexGuard lock(_box_conn_mutex);
		for (auto i = _box_conn.begin(); i != _box_conn.end(); ++i)
			delete i->second;
		_box_conn.clear();
	}
	{
		MutexGuard lock(_erp_conn_mutex);
		for (auto i = _erp_conn.begin(); i != _erp_conn.end(); ++i)
			delete i->second;
		_erp_conn.clear();
	}
	{
		MutexGuard lock(_app_conn_mutex);
		for ( auto i = _app_conn.begin(); i != _app_conn.end(); ++i)
			delete i->second;
		_app_conn.clear();
	}
	{
		delete _init_conn;
	}
}

//////////////////////////////////////////////////////////////////////////

void ConnectionManager::updateBox(uint32_t box_id, size_t conn_id , const std::string &ip) {
	{
		MutexGuard lock(_box_conn_mutex);

		BoxConnection* conn;
		auto conn_itr = _box_conn.find(box_id);
		if (conn_itr == _box_conn.end()) {
			conn = new BoxConnection(_max_cache_num , ip);
			_box_conn[box_id] = conn;
			conn->setConnectionID(conn_id);
		} else {
			conn = conn_itr->second;
			size_t old_id = conn->getConnectionID();
			conn->setConnectionID(conn_id);
			// Check whether old connection is still alive or not.
			tcp::async::Connection* old_conn = _pool->get(old_id);
			if (old_conn != NULL) {	// Yes, kick it.
				old_conn->shutdown();
				old_conn->release();
			}
		}
	}

	Logger::get("server")->log("Connection " + toString(conn_id) + " is marked as Box client " + toString(box_id), Logger::NORMAL);
}

void ConnectionManager::setBoxCode(uint32_t box_id , const std::string &code)
{
	MutexWriter lock(_codes_mutex);
	_codes[box_id] = code; 	
}

int ConnectionManager::getBoxFromCode(const std::string &code)
{
	MutextReader lock(_codes_mutex);
	int value = -1;
	std::map<uint32_t , std::string>::const_iterator it = _codes.begin();
	while( it != _codes.end())
	{
		if (it->second == code)
		{
			value = (int)it->first;
			break;
		}
		it++;
	}
	return value;
}
std::string ConnectionManager::getBoxCode(uint32_t box_id)
{
	MutextReader lock(_codes_mutex);
		return _codes[box_id];	
}

void ConnectionManager::setBoxSongs(uint32_t box_id , const std::string songs)
{
	MutexGuard guard(_songs_mutex);
	_songs[box_id] = songs;
}

std::string ConnectionManager::getBoxSongs(uint32_t box_id) 
{
	MutexGuard gurad(_songs_mutex);
	return _songs[box_id];

}
void ConnectionManager::updateApp(uint32_t app_id, size_t conn_id){

	{
		MutexGuard lock(_app_conn_mutex);

		AppConnection* conn;
		auto conn_itr = _app_conn.find(app_id);
		if (conn_itr == _app_conn.end()) {
			conn = new AppConnection(_max_cache_num);
			_app_conn[app_id] = conn;
			conn->setConnectionID(conn_id);
		} else {
			conn = conn_itr->second;
			size_t old_id = conn->getConnectionID();
			conn->setConnectionID(conn_id);
			// Check whether old connection is still alive or not.
			tcp::async::Connection* old_conn = _pool->get(old_id);
			if (old_conn != NULL) {	// Yes, kick it.
				old_conn->shutdown();
				old_conn->release();
			}
		}
	}

	Logger::get("server")->log("Connection " + toString(conn_id) + " is marked as App client " + toString(app_id), Logger::NORMAL);
}

void ConnectionManager::sendToBox(uint32_t box_id, const Packet* pac) const {
	// Get connection handler
	BoxConnection* box_conn;
	{
		MutexGuard lock(_box_conn_mutex);
		auto conn_itr = _box_conn.find(box_id);
		if (conn_itr == _box_conn.end())
			throw BoxConnectionLost(box_id, __FILE__, __LINE__);
		box_conn = conn_itr->second;
	}
	tcp::async::Connection* conn = box_conn->getConnection(_pool);
	if (conn == NULL)
		throw BoxConnectionLost(box_id, __FILE__, __LINE__);

	// Dispatch
	try {
		pac->dispatch(conn);
	} catch (const std::exception& e) {
		conn->release();
		throw e;
	}
	conn->release();
}

void ConnectionManager::sendToApp(uint32_t app_id, const Packet *pac) const {
	// Get connection handler
	AppConnection* app_conn;
	{
		MutexGuard lock(_app_conn_mutex);
		auto conn_itr = _app_conn.find(app_id);
		if (conn_itr == _app_conn.end())
			throw AppConnectionLost(app_id, __FILE__, __LINE__);
		app_conn = conn_itr->second;
	}
	tcp::async::Connection* conn = app_conn->getConnection(_pool);
	if (conn == NULL)
		throw AppConnectionLost(app_id, __FILE__, __LINE__);

	// Dispatch
	try {
		pac->dispatch(conn);
	} catch (const std::exception& e) {
		conn->release();
		throw e;
	}
	conn->release();
}

AppConnection * ConnectionManager::getConnectionApp(uint32_t app_id){	
	MutexGuard lock(_app_conn_mutex);

	auto conn_itr = _app_conn.find(app_id);
	if (conn_itr == _app_conn.end())
		throw AppConnectionLost(app_id , __FILE__ , __LINE__); 
	else
		return conn_itr->second;

}


BoxConnection* ConnectionManager::getConnectionBox(uint32_t box_id) {
	MutexGuard lock(_box_conn_mutex);

	auto conn_itr = _box_conn.find(box_id);
	if (conn_itr == _box_conn.end())
		throw BoxConnectionLost(box_id,__FILE__, __LINE__);
	else
		return conn_itr->second;
}

std::map<uint32_t, BoxConnection*> ConnectionManager::getConnectionBox() {
	MutexGuard lock(_box_conn_mutex);
	
	return _box_conn;
}

std::map<uint32_t , AppConnection *> ConnectionManager::getConnectionApp(){
	MutexGuard lock(_app_conn_mutex);

	return _app_conn;
}

//////////////////////////////////////////////////////////////////////////

void ConnectionManager::updateERP(ERPRole role, size_t conn_id) {
	ERPConnection* conn;
	{
		MutexGuard lock(_erp_conn_mutex);
		auto conn_itr = _erp_conn.find(role);
		if (conn_itr == _erp_conn.end()) {
			conn = new ERPConnection(_max_cache_num);
			_erp_conn[role] = conn;
		} else {
			conn = conn_itr->second;
		}
		conn->updateConnectionID(conn_id);
		_erp_conn_index[conn_id] = conn;
	}

	Logger::get("server")->log("Connection " + toString(conn_id) + " is marked as ERP system [" + ERPRoleString(role) + "]", Logger::NORMAL);
}

void ConnectionManager::revokeERP(size_t conn_id) {
	MutexGuard lock(_box_conn_mutex);
	auto conn_idx = _erp_conn_index.find(conn_id);
	if (conn_idx != _erp_conn_index.end()) {
		conn_idx->second->removeConnectionID(conn_id);
		_erp_conn_index.erase(conn_idx);
	}
}

void ConnectionManager::sendToERP(ERPRole role, const Packet* pac) const {
	// Get connection handler
	ERPConnection* erp_conn;
	{
		MutexGuard lock(_erp_conn_mutex);
		auto conn_itr = _erp_conn.find(role);
		if (conn_itr == _erp_conn.end())
			throw ERPConnectionLost(role, __FILE__, __LINE__);
		erp_conn = conn_itr->second;
	}

	bool success = false;
	{
		MutexGuard lock(erp_conn->getMutex());
		auto conn_ids = erp_conn->getConnectionID();
		tcp::async::Connection* conn;
		for each (auto conn_id in conn_ids) {
			conn = _pool->get(conn_id);
			if (conn == NULL)
				continue;
			// Dispatch
			try {
				pac->dispatch(conn);
				success = true;
			} catch (const std::exception&) {
				conn->release();
				continue;
			}
			conn->release();
		}
	}

	if (!success)
		throw ERPConnectionLost(role, __FILE__, __LINE__);
}

ERPConnection* ConnectionManager::getConnectionERP(ERPRole role) {
	MutexGuard lock(_erp_conn_mutex);

	auto conn_itr = _erp_conn.find(role);
	if (conn_itr == _erp_conn.end())
		throw ERPConnectionLost(role, __FILE__, __LINE__);
	else
		return conn_itr->second;
}

int ConnectionManager::showAllVirtualConnection(yiqiding::net::tel::ServerSend *srv)
{
	
	std::ostringstream out;	
	{
		MutexGuard lock(_box_conn_mutex);
		out << "BoxConnection: "<<_box_conn.size() <<"\r\n";
		std::map<uint32_t , BoxConnection *>::iterator box_it = _box_conn.begin();
		while(box_it != _box_conn.end())
		{		
			out <<"CON_ID:"<<box_it->second->getConnectionID()<<"\tBOX_ID:"<<box_it->first<<"\r\n";
			++box_it;	
		}
	}
	{
		MutexGuard lock(_app_conn_mutex);
		out << "AppConnection:" <<_app_conn.size()<<"\r\n";
		std::map<uint32_t , AppConnection *>::iterator app_it = _app_conn.begin();
		while(app_it != _app_conn.end())
		{
			out <<"CON_ID:"<<app_it->second->getConnectionID()<<"\tAPP_ID:"<<app_it->first<<"\r\n";
			++app_it;
		}
	}

	{
		out << "ERPConnection:\r\n";
		MutexGuard lock(_erp_conn_mutex);
		std::map<ERPRole , ERPConnection*>::iterator erp_it = _erp_conn.begin();
		while(erp_it != _erp_conn.end())
		{
			MutexGuard lock(erp_it->second->getMutex());
			auto conn_ids = erp_it->second->getConnectionID();
			out << ERPRoleString(erp_it->first)<<":<"<<conn_ids.size()<<">\r\n" << "CON_IDS:";
			for each(auto conn_id in conn_ids){
				out << conn_id <<" ";
			}
			out<<"\r\n";
			++erp_it;
		}
		
	}

	srv->teleSend(out.str());

	return 0;
}

int ConnectionManager::showInitRequest(yiqiding::net::tel::ServerSend *srv , const char *ip)
{
	uint32_t dwIP;
	inet_pton(AF_INET , ip , &dwIP);

	//uint32_t dwIP = inet_addr(ip);
	_init_conn->showReq(srv , dwIP);
	return 0;
}


int ConnectionManager::showRequest(yiqiding::net::tel::ServerSend *srv , int type , int id , int request , int top , bool sort)
{
	Connection *conn = NULL;
	
	try{
		switch(type)
		{
		case 0: 
			conn = getConnectionERP((ktv::ERPRole)id);
			break;
		case 1:
			conn = getConnectionBox(id);
			break;
		case 2:
			conn = getConnectionApp(id);
			break;
		default:
			return -1;
		}
	}
	catch(const extended::ConnectionLost )
	{
		srv->teleSend("id may be wrong !\r\n");
		return -1;
	}

	if (request == -1)
	{
		 conn->showAllReq(srv, top , sort);
		 return 0;
	}
	else 
	{
		conn->showReq(srv , request , top , sort);
		return 0;
	}
}


void ConnectionManager::updateMusic(uint32_t self , size_t conn_id){
	MutexGuard guard(_music_conn_mutex);

	MusicConnection* conn;
	auto conn_itr = _music_conn.find(self);
	if (conn_itr == _music_conn.end()) {
		conn = new MusicConnection();
		_music_conn[self] = conn;
		conn->setConnectionID(conn_id);
		Logger::get("server")->log("[microphone debug]create a new music client " + toString(self), Logger::NORMAL);
	} else {
		conn = conn_itr->second;
		size_t old_id = conn->getConnectionID();
		conn->setConnectionID(conn_id);
		// Check whether old connection is still alive or not.
		tcp::async::Connection* old_conn = _pool->get(old_id);
		if (old_conn != NULL) {	// Yes, kick it.
			old_conn->shutdown();
			old_conn->release();
		}
		Logger::get("server")->log("[microphone debug]replace a music client" + toString(self), Logger::NORMAL);
	}
	Logger::get("server")->log("Connection " + toString(conn_id) + " is marked as Music client " + toString(self), Logger::NORMAL);
}

	


void ConnectionManager::listenMusic(uint32_t self , uint32_t other){
		MutexGuard guard(_music_conn_mutex);

		//delete first
		for each(auto k in _music_virutal_conn)
		{	
			for each(auto j in k.second )
			{
				if(j == other)
				{	
					_music_virutal_conn[k.first].erase(j);
					break;
				}
			}
		}
		
		_music_virutal_conn[self].insert(other);
}

void ConnectionManager::removeMusic(uint32_t self , uint32_t other){
	MutexGuard guard(_music_conn_mutex);
	_music_virutal_conn[self].erase(other);
}

void ConnectionManager::clearMusic(uint32_t self){
	MutexGuard guard(_music_conn_mutex);
	_music_virutal_conn[self].clear();

}

Connection* ConnectionManager::getConnectionMusic(uint32_t self){
	MutexGuard lock(_music_conn_mutex);

	auto conn_itr = _music_conn.find(self);
	if (conn_itr == _music_conn.end())
		throw MusicConnectionLost(self , __FILE__ , __LINE__); 
	else
		return conn_itr->second;
}

void ConnectionManager::sendToMusic(uint32_t self , const char *data , int len)
{
	// Get connection handler
	
	MutexGuard lock(_music_conn_mutex);
	MusicConnection* music_conn;
	std::set<uint32_t>	conns;
	{
	
		auto conn_itr = _music_virutal_conn.find(self);
		if (conn_itr == _music_virutal_conn.end())
			return;//throw MusicConnectionLost(self, __FILE__, __LINE__);
		conns = conn_itr->second;
	}

	for each( auto ips in conns)
	{
		auto conn_itr = _music_conn.find(ips);
		if (conn_itr == _music_conn.end())
				return;//throw MusicConnectionLost(self, __FILE__, __LINE__);
		music_conn = conn_itr->second;

		tcp::async::Connection* conn = music_conn->getConnection(_pool);
		if (conn == NULL)
			return;//throw MusicConnectionLost(self, __FILE__, __LINE__);

		// Dispatch
		try {
			Packet::dispatch(conn , data , len);
		} catch (const std::exception& e) {
			conn->release();
			return;//throw e;
		}
		conn->release();
	}
	
}
void ConnectionManager::removeMusicConn(uint32_t ip){
	MutexGuard guard(_music_conn_mutex);
	MusicConnection* music_conn = NULL;
	{
		auto it = _music_conn.find(ip);
		if(it != _music_conn.end()){
			Logger::get("server")->log("[microphone debug] delete music conn item");
			_music_conn.erase(it);
		}
	}
}

void ConnectionManager::sendDataToBox(uint32_t box_id, const char* data, uint32_t len) const{
	//Get connection handler
	//Logger::get("server")->log("[microphone] bug00 " + toString(box_id));
	MutexGuard guard(_music_conn_mutex);
	MutexGuard lock(_box_conn_mutex);

	Logger::get("server")->log("[microphone] into sendDataToBox");
	int ip = 0;
	BoxConnection* box_conn;
	{
		auto conn_itr = _box_conn.find(box_id);
		//Logger::get("server")->log("[microphone] bug03");
		if (conn_itr == _box_conn.end()){
			//Logger::get("server")->log("[microphone] bug04");
			return;
			//throw BoxConnectionLost(box_id, __FILE__, __LINE__); //<-- it's strange, it make server down
		}
		box_conn = conn_itr->second;
		ip = inet_addr(box_conn->getConnection(_pool)->getAddress().c_str());
		//Logger::get("server")->log("[microphone1] get ip address :" + toString(box_conn->getIP()) + " by boxId " + toString(box_id) +
		//	" ip:" + toString(ip), Logger::NORMAL);
		//Logger::get("server")->log("[microphone] debug 06");
	}
	
	MusicConnection* music_conn = NULL;
	{
		
		auto it = _music_conn.find(ip);
		if(it == _music_conn.end()){
			Logger::get("server")->log("[microphone] boxId :" + toString(box_id) + " does not connect pcm port : 47738 ", Logger::WARNING);
			return;
		}
		music_conn = it->second;
		tcp::async::Connection* conn  = music_conn->getConnection(_pool);

		Logger::get("server")->log("[microphone] debug 08");

		try {
			//Packet::dispatch(conn , data , len);
			
			//Dispatch
			Packet packet(packet::KTV_REQ_BOX_TURN_MESSAGE);

			packet.setPayload(data , len);
			//packet.dispatch(conn);
			Logger::get("server")->log("[microphone] send microphone data to boxId " + toString(box_id) + 
				" len : " + toString(len), Logger::NORMAL);

		} catch (const std::exception& e) {
			Logger::get("server")->log("[microphone] server fail to send microphone data to boxId" + toString(box_id), Logger::WARNING);
			conn->release();
			throw e;
		}
		conn->release();
	}
	//Logger::get("server")->log("[microphone] debug 09");
}

void ConnectionManager::updateAppBoxMapping(uint32_t app_id, uint32_t box_id){
	// verify app_id and box_id
	MutexGuard lock(_app_conn_mutex);
	MutexGuard lock1(_box_conn_mutex);

	if( _app_conn.find(app_id) == _app_conn.end() || _box_conn.find(box_id) == _box_conn.end()){
		// error out later
		return;
	}

	MutexGuard lock2(_app_box_map_mutex);
	MutexGuard lock3(_box_apps_map_mutex);
	auto app_exist_itr = _app_box_map.find(app_id); // app_id exists in _app_box_map
	auto box_exist_itr = _box_apps_map.find(box_id); // box_id exists in _box_app_map

	// add new item into _app_box_map
	if (app_exist_itr == _app_box_map.end()) {
		_app_box_map[app_id] = box_id;
	}

	// add new item into _box_apps_map
	if (box_exist_itr == _box_apps_map.end()){
		std::set<uint32_t> app_set;
		app_set.insert(app_id);
		_box_apps_map[box_id] = app_set;
	} else {
		box_exist_itr->second.insert(app_id);
	}
}

uint32_t ConnectionManager::getBoxIdFromAppId(uint32_t app_id){
	Logger::get("server")->log("[microphone] into getBoxIdFromAppId");
	MutexGuard lock(_app_box_map_mutex);

	auto ptr = _app_box_map.find(app_id);
	if (ptr != _app_box_map.end()){
		//Logger::get("server")->log("get boxId : " + toString(ptr->second) + " by " + toString(app_id), Logger::NORMAL);
		return ptr->second;
	} else {
		//Logger::get("server")->log("get boxId failed! : appId:" + toString(app_id), Logger::NORMAL);
		return -1;
	}
}

std::set<uint32_t>* ConnectionManager::getAppIdsFromBoxId(uint32_t box_id){
	MutexGuard lock(_box_apps_map_mutex);

	auto ptr = _box_apps_map.find(box_id);
	if (ptr != _box_apps_map.end()){
		return &(ptr->second);
	} else {
		return NULL;
	}
}

void ConnectionManager::notifyBoxAppLost(uint32_t appId){
	int boxId = getBoxIdFromAppId(appId);

	Packet pack(KTV_NOTIFY_BOX_LOST_APP);
	Json::Value root;
	root["appId"] = appId;

	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str(), msg.length());
	sendToBox(boxId, &pack);
}

void ConnectionManager::cleanAppBoxMappingByAppId(uint32_t appId){
	// notify box before delete
	MutexGuard lock(_app_box_map_mutex);
	MutexGuard lock1(_box_apps_map_mutex);

	auto ptr = _app_box_map.find(appId);

	if(ptr != _app_box_map.end()){
		uint32_t box_id = ptr->second;
		auto ptr1 = _box_apps_map.find(box_id);//find box item
		if(ptr1 != _box_apps_map.end() && ptr1->second.find(appId) != ptr1->second.end()){
			ptr1->second.erase(appId);
			if(ptr1->second.empty()){
				_box_apps_map.erase(box_id);
			}
		}
		_app_box_map.erase(appId);
		//Logger::get("server")->log("clean app list by app id : " + toString(appId) , Logger::NORMAL);
	}
}

void ConnectionManager::cleanAppBoxMappingByBoxId(uint32_t box_id){
	MutexGuard lock(_app_box_map_mutex);
	MutexGuard lock1(_box_apps_map_mutex);

	auto ptr = _box_apps_map.find(box_id);

	if(ptr!=_box_apps_map.end()){
		for(auto it=ptr->second.begin();it!=ptr->second.end();it++){
			if(_app_box_map.find(*it) != _app_box_map.end()){
				_app_box_map.erase(*it);
			}
		}
		_box_apps_map.erase(box_id);
		//Logger::get("server")->log("clean app list by box id : " + toString(box_id) , Logger::NORMAL);
	}
}

//ip format: xxx.xxx.xxx.xxx
std::string	ConnectionManager::getBoxCodeFromIp(std::string ip){
	MutexGuard lock1(_box_conn_mutex);
	MutextReader lock(_codes_mutex);

	uint32_t box_id= -1;
	std::string strCode = "";
	for(auto it=_box_conn.begin();it!=_box_conn.end();it++){
		if(it->second->getIP() == ip){
			box_id = it->first;
			break;
		}
	}
	Logger::get("server")->log("[the third app] get box id : " + toString(box_id) + " from ip : " + toString(ip) , Logger::NORMAL);
	if(box_id != -1){
		auto it1 = _codes.find(box_id);
		if(it1 != _codes.end()){
			strCode = it1->second;
		}
	}
	Logger::get("server")->log("[the third app] get box strCode : " + strCode + " from box id : " + toString(box_id) , Logger::NORMAL);
	return strCode;
}