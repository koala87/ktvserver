/**
 * Virtual Connection (Middle Layer)
 * @author Shiwei Zhang , Yuchun Zhang
 * @date 2014.03.10
 */

#pragma once

#include <map>
#include <queue>
#include <vector>
#include <list>
#include <set>
#include <WinSock2.h>
 #include <windows.h> 
#include <mmsystem.h>
#include "yiqiding/ThreadW1Rn.h"
#include "yiqiding/net/TelNet.h"
#include "yiqiding/ktv/ERPRole.h"
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/ThreadW1Rn.h"


namespace yiqiding { namespace ktv { namespace extended {
	/// Virtual Connection Cache (Thread-Safe)
	class ConnectionCache {
	private:
		Packet*	_out_pac;
		Packet* _refer_pac;
	public:
		ConnectionCache(Packet* out_pac, Packet* refer_pac) : _out_pac(out_pac), _refer_pac(refer_pac) {}
		~ConnectionCache() { delete _out_pac; delete _refer_pac; }

		inline Packet*			getOutGoingPacket()				{ return _out_pac; };
		inline Packet*			getReferencePacket()			{ return _refer_pac; };
		inline const Packet*	getOutGoingPacket()		const	{ return _out_pac; };
		inline const Packet*	getReferencePacket()	const	{ return _refer_pac; };
	};


	class RecordPacket
	{
	private:
		time_t _time;
		
		uint32_t _reqTicks;
		uint32_t _outTicks;
		uint32_t _attTicks;
		Packet *_reqpac;
		Packet *_outpac;
		Packet *_attpac;
		uint32_t _deviceId;
	public:
		RecordPacket(const time_t &t , const Packet *reqpac):_reqpac(NULL),_outpac(NULL),_attpac(NULL)
		{
			_time = time_t(t);
			if(reqpac != NULL)
			_reqpac = new Packet(*reqpac);
			_reqTicks = timeGetTime();
		}
		~RecordPacket() { 
			if (_reqpac)
			{
				delete _reqpac;
			}
			if (_outpac)
			{
				delete _outpac;
			}
			if (_attpac)
			{
				delete _attpac;
			}
	

		 }
		void setOutPack(const Packet *outpac){
			if (outpac != NULL)
			{
				_outpac = new Packet(*outpac);
				_outTicks = timeGetTime();
			}
		}
		void setAttPack(const Packet *attpac , int deviceid){
			if (attpac != NULL)
			{
				_attpac = new Packet(*attpac);
				_deviceId = deviceid;
				_attTicks = timeGetTime();
			}
					
		}
		std::string toString(){
			std::ostringstream out;
			struct tm timeinfo;
			char time_str[20];
			localtime_s(&timeinfo, &_time);
			strftime(time_str,20,"%Y-%m-%d %H:%M:%S", &timeinfo);
			
		
			out <<time_str << "------------------------------\r\n\r\n"; 
			out <<"<-------"  <<" t:"<<_reqTicks << "\r\n";
			out << _reqpac->toString() << "\r\n";
			if (_outpac)
			{
				out << "------>" <<" t:" << _outTicks << "\r\n" ;
				out <<  _outpac->toString() << "\r\n";
			}
			if (_attpac)
			{
				out << "^------" << " t:" <<_attTicks << " deviceid: " << _deviceId << "\r\n";
				out << _attpac->toString() << "\r\n";	
			}
			return out.str();
		}
		const Packet * getOutPack() const{ return _outpac;}
		const Packet * getReqPack() const{ return _reqpac;}
		const time_t * getTime() const {return &_time;}
		uint32_t getDeviceId()		const {return _deviceId;}
		void setReqDeviceId(uint32_t deviceId) { _reqpac->setDeviceID(deviceId);}
		
	};


	const int RECORD_INIT_NUM	= 256;
	const int RECORD_INC_NUM	= 64;

	/// Virtual Connection Cache Manager (Thread-Safe)
	class ConnectionCacheManager {
	private:
		typedef ConnectionCache	Cache;
	
		size_t						_max_cache_num;
		std::map<size_t, Cache*>	_cache;
		Mutex						_cache_mutex;

		//debug record all coming message
		yiqiding::MutexWR				_record_mutex;
		std::list<RecordPacket *>		_record;

	


	public:
		ConnectionCacheManager(size_t max_cache_num) : _max_cache_num(max_cache_num) {};
		virtual ~ConnectionCacheManager();

		// Cache Operation

		/// Cache the out going packet and reference packet. Memory is copied by Connection.
		void	cache(const Packet* out_pac, const Packet* refer_pac = NULL);
		/// Find the cache by the identifier of the out going packet.
		/// Memory is taken control by the caller. Deletion required.
		/// If nothing found, NULL returned.
		Cache*	findCache(size_t identifier);

		//debug 
		void	record_push(RecordPacket *rdpack);
		void	showReq(yiqiding::net::tel::ServerSend *srv , int req , int top , bool sort );
		void	showAllReq(yiqiding::net::tel::ServerSend *srv , int top , bool sort);

		void	showReq(yiqiding::net::tel::ServerSend *srv , uint32_t deviceId);
	};
	
	/// Virtual Connection (Thread-Safe)
	/// Currently, there is no common Connection part. Hence, typedef is used.
	typedef ConnectionCacheManager	Connection;

	/// Virtual Box Connection (Thread-Safe)
	class BoxConnection : public Connection {
	private:
		size_t					_conn_id;
		std::string				_IpAddress;
	public:
		BoxConnection(size_t max_cache_num ,const std::string &ip) : Connection(max_cache_num), _conn_id(0),_IpAddress(ip) {};
		~BoxConnection() {};

		// Getter
		inline net::tcp::async::Connection* getConnection(net::tcp::async::ConnectionPool* pool) const { return pool->get(_conn_id); };
		inline size_t getConnectionID() const	{ return _conn_id; };
		inline const std::string &	getIP() const{ return _IpAddress;}
		// Setter
		inline void setConnectionID(size_t id)	{ _conn_id = id; };
		

	};


	/// Virtual App Connection (Thread-Safe)
	class AppConnection : public Connection{
	private:
		size_t _conn_id;
	public:
		AppConnection(size_t max_cache_num) : Connection(max_cache_num) {};
		~AppConnection() {};

		// Getter
		inline net::tcp::async::Connection* getConnection(net::tcp::async::ConnectionPool* pool) const { return pool->get(_conn_id); };
		inline size_t getConnectionID() const {	return _conn_id;};
		
		// Setter
		inline void setConnectionID(size_t id) { _conn_id = id;};
};

	
	/// Virtual Music Connection (Thread-Safe)
	class MusicConnection: public Connection{
		size_t	_conn_id;
		std::string _IpAddress;
	public:
		MusicConnection() : Connection(0), _conn_id(0){};
		~MusicConnection() {};

		// Getter
		inline net::tcp::async::Connection* getConnection(net::tcp::async::ConnectionPool* pool) const { return pool->get(_conn_id); };
		inline size_t getConnectionID() const	{ return _conn_id; };
		// Setter
		inline void setConnectionID(size_t id)	{ _conn_id = id; };
		inline const std::string &	getIP() const{ return _IpAddress;}
	};


	/// Virtual ERP Connection (Thread-Safe)
	class ERPConnection : public Connection {
	private:
		std::set<size_t>	_conn_id;
		mutable Mutex		_conn_id_mutex;
	public:
		ERPConnection(size_t max_cache_num) : Connection(max_cache_num) {};
		~ERPConnection() {};

		// Operation
		inline void updateConnectionID(size_t conn_id)	{ MutexGuard lock(_conn_id_mutex); _conn_id.insert(conn_id); };
		inline void removeConnectionID(size_t conn_id)	{ MutexGuard lock(_conn_id_mutex); _conn_id.erase(conn_id); };

		// Getter use _conn_id must using getMutex to protect
		inline std::set<size_t>	getConnectionID() const	{  return _conn_id; };	
		inline Mutex & getMutex() {return _conn_id_mutex;}
	
	};

	/// Virtual Connection Manager (Thread-Safe)
	/// Current architecture only supports sending. Receiving is not supported.
	/// General Processing is required.
	class ConnectionManager {
	private:
		net::tcp::async::ConnectionPool*	_pool;
		/// This member grows and never delete untill ~ConnectionManager();
		/// Hence, it is safe to let BoxConnection* go outside.
		std::map<uint32_t, BoxConnection*>	_box_conn;
		mutable Mutex						_box_conn_mutex;
		/// This member grows and never delete untill ~ConnectionManager();
		/// Hence, it is safe to let BoxConnection* go outside.
		std::map<uint32_t,AppConnection*>	_app_conn;
		mutable Mutex						_app_conn_mutex;
		///  
		///ip ~ connection
		std::map<uint32_t , MusicConnection *>		_music_conn;
		std::map<uint32_t , std::set<uint32_t> >	_music_virutal_conn;
		mutable Mutex								_music_conn_mutex;

		/// This member grows and never delete untill ~ConnectionManager();
		/// Hence, it is safe to let ERPConnection* go outside.
		std::map<ERPRole, ERPConnection*>	_erp_conn;
		std::map<size_t, ERPConnection*>	_erp_conn_index;
		mutable Mutex						_erp_conn_mutex;
		size_t								_max_cache_num;
		///
		/// Genertate TempCode
		std::map<uint32_t ,std::string>		_codes;
		mutable MutexWR						_codes_mutex;
		
		std::map<uint32_t ,std::string>		_songs;
		mutable Mutex						_songs_mutex;

		//init only one
		Connection							*_init_conn;
	
		/// Multiple apps can connect to the same box
		std::map<uint32_t, uint32_t> _app_box_map;
		mutable Mutex				 _app_box_map_mutex;

		std::map<uint32_t, std::set<uint32_t> > _box_apps_map;
		mutable Mutex							_box_apps_map_mutex;  

	public:
		ConnectionManager(net::tcp::async::ConnectionPool* conn_pool, size_t max_cache_num) :_init_conn(new Connection(max_cache_num)), _pool(conn_pool), _max_cache_num(max_cache_num) {}
		~ConnectionManager();

		//App
		void			updateApp(uint32_t app_id, size_t conn_id);
		void			sendToApp(uint32_t app_id, const Packet *pac) const;

		AppConnection*	getConnectionApp(uint32_t app_id);
		std::map<uint32_t , AppConnection*> getConnectionApp();

		// Box
		void			updateBox(uint32_t box_id, size_t conn_id , const std::string & ip);
		void			sendToBox(uint32_t box_id, const Packet* pac) const;
		/// @throw BoxConnectionLost if box_id is not found,  
		BoxConnection*	getConnectionBox(uint32_t box_id);
		std::map<uint32_t, BoxConnection*>	getConnectionBox();
			//code
		void			setBoxCode(uint32_t box_id , const std::string &code);
		int				getBoxFromCode(const std::string &code);
		std::string     getBoxCode(uint32_t box_id);
		std::string		getBoxCodeFromIp(std::string ip);
		// ERP
		void			updateERP(ERPRole role, size_t conn_id);
		void			revokeERP(size_t conn_id);
		void			sendToERP(ERPRole role, const Packet* pac) const;
		ERPConnection*	getConnectionERP(ERPRole role);
		//songs
		void			setBoxSongs(uint32_t box_id , const std::string songs);
		std::string		getBoxSongs(uint32_t);
		//init
		Connection*		getConnectionInit()	{ return _init_conn;}

		//music
		void			updateMusic(uint32_t self , size_t conn_id);
		
		void			listenMusic(uint32_t self , uint32_t other);
		void			removeMusic(uint32_t self , uint32_t other);
		void			clearMusic(uint32_t self);

		void			sendToMusic(uint32_t self , const char *data , int len);
		Connection*		getConnectionMusic(uint32_t);

		//telnet debug
		int showAllVirtualConnection(yiqiding::net::tel::ServerSend *srv);
		int showRequest(yiqiding::net::tel::ServerSend *srv , int type , int id , int requset , int top , bool sort);
		int showInitRequest(yiqiding::net::tel::ServerSend *srv , const char *ip);

		// App <-> Box
		void			updateAppBoxMapping(uint32_t appId, uint32_t boxId);
		uint32_t		getBoxIdFromAppId(uint32_t appId);
		std::set<uint32_t>*	getAppIdsFromBoxId(uint32_t boxId);
		void			cleanAppBoxMappingByAppId(uint32_t appId);
		void			cleanAppBoxMappingByBoxId(uint32_t boxId);

		void			notifyBoxAppLost(uint32_t appId);

		void            sendDataToBox(uint32_t box_id, const char* data, uint32_t len) const;
		void			removeMusicConn(uint32_t ip);

		int showBoxConnection(yiqiding::net::tel::ServerSend * srv);

		int showAppConnection(yiqiding::net::tel::ServerSend * srv);
	
		int showERPConnection(yiqiding::net::tel::ServerSend * srv);

		int showAppBoxMapping(yiqiding::net::tel::ServerSend * srv);
		
		int showMusicConnection(yiqiding::net::tel::ServerSend * srv);
	};

	// Exception
	class ConnectionLost : public Exception {
	public:
		explicit ConnectionLost(const std::string& who, const std::string& src_file, line_t line_no) : yiqiding::Exception(who, "Connection lost", src_file, line_no) {};
		virtual ~ConnectionLost() throw() {};
	};

	class AppConnectionLost : public ConnectionLost{
	public:
		explicit AppConnectionLost(uint32_t app_id, const std::string& src_file, line_t line_no) : ConnectionLost("App " + utility::toString(app_id), src_file, line_no) {};
		virtual ~AppConnectionLost() throw() {};
	};

	class BoxConnectionLost : public ConnectionLost {
	public:
		explicit BoxConnectionLost(uint32_t box_id, const std::string& src_file, line_t line_no) : ConnectionLost("Box " + utility::toString(box_id), src_file, line_no) {};
		virtual ~BoxConnectionLost() throw() {};
	};

	class ERPConnectionLost : public ConnectionLost {
	public:
		explicit ERPConnectionLost(ERPRole role, const std::string& src_file, line_t line_no) : ConnectionLost(std::string("ERP [") + ERPRoleString(role) + "]", src_file, line_no) {};
		virtual ~ERPConnectionLost() throw() {}; 
	};

	class MusicConnectionLost : public ConnectionLost {
	public:
		explicit MusicConnectionLost(uint32_t ip, const std::string& src_file, line_t line_no) : ConnectionLost(std::string("Music [") +  utility::toString(ip) + "]", src_file, line_no) {};
		virtual ~MusicConnectionLost() throw() {}; 
	};
}}}

namespace yiqiding { namespace ktv {
	using yiqiding::ktv::extended::ConnectionManager;
}}	
