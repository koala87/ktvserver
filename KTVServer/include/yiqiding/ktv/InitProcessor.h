#pragma once

#include "yiqiding/ktv/PacketProcessor.h"

namespace yiqiding{ namespace ktv{ 

class InitProcessor:public packet::Processor
{
public:
	InitProcessor(Server* server, IniConnection* conn, Packet* pac , const std::string &ip , int port):packet::Processor(server , conn , pac , ip , port){}
	virtual ~InitProcessor()	{};

	void processInit();
	void processOtherInit();
	void processFireInfo();
	virtual void onReceivePacket();
	virtual std::string getDir() { return "./init/" ;}

};


}}
