/**
 * Server Debug , Telnet Cmd
 * @author YuChun Zhang
 * @date 2014.01.22
 */

#include "yiqiding/ktv/ServerDebug.h"
#include "yiqiding/ktv/KTVServer.h"

using namespace yiqiding::net::tel;
extern yiqiding::ktv::Server *__server;
namespace yiqiding {namespace ktv{

	int showAllConnection(yiqiding::net::tel::ServerSend *srv)
	{
		return __server->showAllConnection(srv);
	}

	int showAllVirtualConnection(yiqiding::net::tel::ServerSend *srv)
	{
		return __server->showAllVirtualConnection(srv);
	}

	int showServerInfo(yiqiding::net::tel::ServerSend *srv)
	{
		return __server->showServerInfo(srv);
	}

}}





