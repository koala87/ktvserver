/**
 * Packet Processor
 * @author Shiwei Zhang
 * @date 2014.01.23
 */

#pragma once

#include <string>
#include "yiqiding/ktv/Connection.h"
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/ktv/KTVConnection.h"
#include "yiqiding/Thread.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/FireWarn.h"

namespace yiqiding{ 

namespace ktv {class Server;}
}

namespace yiqiding{namespace net {namespace tel{	
class TelNet;
}}}

struct lua_State;

namespace yiqiding { namespace ktv { namespace packet {
	class Processor : public Runnable {
	
	protected:
		ktv::Server*					_server;
		KTVConnection*					_conn;
		Packet*							_pac;
		std::string						_ip;
		int								_port;
	
#ifdef _DEBUG
		ktv::extended::RecordPacket*	_rdpac;	
#endif

#ifdef LOG_REQUEST
		uint32_t						_identify;

		std::string getName()
		{
			switch(_conn->getType())
			{
					case CONNECTION_INI:	return "ini";
				    case CONNECTION_BOX:	return "box";
					case CONNECTION_ERP:	return "erp";
					case CONNECTION_APP:	return "app";
					default:				return "unknown";
			}
		}	   


#endif
		
	public:
		void sendErrorMessage(const std::string& err_msg);
		void sendErrorJsonMessage(int code ,  const std::string &err_msg);
		inline void	send(const Packet* pack)	{ pack->dispatch(_conn); };
		Processor(Server* server, KTVConnection* conn, Packet* pac , const std::string &ip , int port) : _server(server), _conn(conn), _pac(pac) , _ip(ip) , _port(port){

			_conn->referenceLock();
			_conn->capture();
			_conn->referenceUnlock();
#ifdef _DEBUG 
			_rdpac =new ktv::extended::RecordPacket(::time(NULL) ,pac);
#endif		

#ifdef LOG_REQUEST
			if(_server->GetRequestInFile())
			{
				_identify = timeGetTime();
				yiqiding::utility::Logger::get("request")->log(getName() + "[" + yiqiding::utility::toString(_identify) + "] from <"+ _ip + ":" + yiqiding::utility::toString(port)+"> <-" + yiqiding::utility::toString(pac->getDeviceID()) + ":" + pac->toLogString());
			}
#endif
		}


		virtual ~Processor()	{ delete _pac; _conn->release(); };
		void run();
		
		virtual void onReceivePacket() = 0;
		virtual std::string  getDir() = 0;
	public:

		void setAttPack(packet::Packet *pac , int deviceid)
		{
#ifdef _DEBUG
			_rdpac->setAttPack(pac ,deviceid);
#endif

#ifdef LOG_REQUEST
			if(_server->GetRequestInFile())
			yiqiding::utility::Logger::get("request")->log(getName() +"[" +  yiqiding::utility::toString(_identify) + "]^"+ yiqiding::utility::toString(deviceid) +":"+ pac->toLogString());
#endif
		}
		void setOutPack(packet::Packet *pac)
		{
#ifdef _DEBUG
			_rdpac->setOutPack(pac);
#endif

#ifdef LOG_REQUEST
			if(_server->GetRequestInFile())
			yiqiding::utility::Logger::get("request")->log(getName() +"["+ yiqiding::utility::toString(_identify) +"]->" + yiqiding::utility::toString(pac->getDeviceID()) +":"+ pac->toLogString());
#endif
		}

		void proceessHeart()
		{
			Packet out(_pac->getHeader()); 
			out.dispatch(_conn);

			setOutPack(&out);

			if(_conn->getType() == CONNECTION_BOX && Singleton<yiqiding::ktv::fire::FireWarn>::getInstance()->getStatus())
			{
				Packet fire(KTV_REQ_BOX_FIRE_SWITCH);
				std::string msg = "{\"fire\":1}";
				fire.setPayload(msg.c_str() , msg.length());
				fire.dispatch(_conn);
				setAttPack(&fire , _pac->getDeviceID());
			}
		}

			
		friend static void KTVSetLuaContext(lua_State *L ,Server *server , Processor *process ,yiqiding::net::tel::ServerSend *srv);

	};
}}}
