/**
 * KTV 与 Lua 交互的函数
 * Lohas Network Technology Co., Ltd
 * @author Yuchun Zhang
 * @date 2014.07.08
 */
#pragma once


extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <cstdint>
#include <string>


/*



*/
namespace yiqiding {
	
	namespace net { namespace tel{

	class ServerSend;
	}}
	
	
	namespace ktv{
	
	class Server;
	namespace packet{
	class Processor;
} 

}}





extern bool KtvdoLua(uint32_t funcNo , yiqiding::ktv::Server * server , yiqiding::ktv::packet::Processor *process );
extern bool KtvdoLua(const std::string &path , yiqiding::ktv::Server *server , yiqiding::net::tel::ServerSend *srv);
