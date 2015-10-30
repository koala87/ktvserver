
#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <set>
#include "yiqiding/ThreadW1Rn.h"
#include "yiqiding/net/TelNet.h"

namespace yiqiding { namespace ktv{
	class Server;
}}

	 

 namespace yiqiding{  namespace ktv{ namespace box{

	class BoxInfoItem
	{
		std::string _ip;
		uint32_t	_boxid;
		std::string _roomname;
		std::string _roomno;
		std::string _type;
	public:
		BoxInfoItem(const std::string &ip , uint32_t boxid , const std::string &roomname , const std::string &roomno , const std::string &type):_ip(ip) , _boxid(boxid),_roomname(roomname),_roomno(roomno),_type(type) {}
		uint32_t getBoxId(){ return _boxid;}
		const std::string & getRoomName(){return _roomname;}
		const std::string & getRoomNo(){ return _roomno;}
		const std::string & getIp(){ return _ip;}
		const std::string & getType() {return _type;}
	};


	class BoxInfoMan
	{
	private:
		// single
		static BoxInfoMan*				_instance;	
		static yiqiding::MutexWR		_wr;

		yiqiding::ktv::Server*			_server;
		std::string									_shopname;
		std::map<std::string , BoxInfoItem*>			_mapboxs;
		std::map<uint32_t , std::set<uint32_t> >		_initcalls;
		
		bool parse(const char *data , uint32_t len);
		void clear();
		BoxInfoMan(yiqiding::ktv::Server *server):_server(server){}
		~BoxInfoMan(){clear();}
	public:	
		
		bool load(const std::string &path );
		bool save(const std::string &path , const char *data , uint32_t len);
		//bool insertMysql()

		std::auto_ptr< std::map<uint32_t , std::string> > getRoomNames(const std::set<int> & boxids);
		
		int showInitCall(yiqiding::net::tel::ServerSend *srv);
		void	addInitCall(uint32_t ip);

		BoxInfoItem * getItem(const std::string &ip);
		std::string & getShopName() { return _shopname;}
		 std::string  getIP(unsigned int boxid);
		//@Thread Safe
		static yiqiding::MutexWR & getMutexWR () {return _wr;}
		static BoxInfoMan* getInstace(yiqiding::ktv::Server *server = NULL);
		static void unLoad();


	};

 }}}


