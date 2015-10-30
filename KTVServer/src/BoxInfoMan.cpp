
#include "yiqiding/ktv/BoxInfoMan.h"
#include "tinyxml2.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/io/File.h"
#include "yiqiding/ktv/KTVServer.h"
#include "ws2tcpip.h"

#pragma once

using namespace yiqiding::ktv::box;

static inline const char* getText(tinyxml2::XMLElement* node) {
	const char* p = node->GetText();
	return p?p:"";
}

bool BoxInfoMan::parse(const char *data , uint32_t len)
{
	tinyxml2::XMLDocument doc;
	doc.Parse(data , len );
	if (doc.ErrorID())
		return false;

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if (root == NULL)
		return false;

	std::string shopname;
	std::map<std::string , BoxInfoItem *> mapBoxInfo;

	tinyxml2::XMLElement* node = root->FirstChildElement("shopname");
	if (node == NULL )
		return false;
	shopname = getText(node);
	node = root->FirstChildElement("ktvinfos");
	if (node == NULL)
		return false;
	for (tinyxml2::XMLElement *i = node->FirstChildElement("item");  i != NULL; i = i->NextSiblingElement())
	{
		tinyxml2::XMLElement *croot = i->FirstChildElement("boxid");
		if (croot == NULL || !yiqiding::utility::isULong(croot->GetText()))
			continue;
		uint32_t boxid = yiqiding::utility::toUInt(croot->GetText());
		croot = i->FirstChildElement("ip");
		if (croot == NULL || !croot->GetText())
			continue;
		std::string ip = croot->GetText();
		croot = i->FirstChildElement("roomname");
		if (croot == NULL || !croot->GetText())
			continue;
		std::string roomname = croot->GetText();
		croot = i->FirstChildElement("roomno");
		if (croot == NULL || !croot->GetText())
			continue;
		std::string roomno = croot->GetText();
		croot = i->FirstChildElement("type");
		std::string type;
		if(croot != NULL && croot->GetText() != NULL)
			type = croot->GetText();

		mapBoxInfo[ip] = new BoxInfoItem(ip ,boxid , roomname , roomno ,type);
	}

	clear();
	_shopname = shopname;
	_mapboxs = mapBoxInfo;

	return true;
}

void BoxInfoMan::clear()
{
	for each(auto i in _mapboxs)
	{
		delete i.second;
	}
	_mapboxs.clear();
}

 std::string  BoxInfoMan::getIP(unsigned int boxid)
{
	for each(auto k in _mapboxs)
	{
		if(k.second->getBoxId() == boxid)
			return k.first;
	}

	return "";
}

std::auto_ptr< std::map<uint32_t , std::string> > BoxInfoMan::getRoomNames(const std::set<int> & boxids)
{
	MutextReader mr(_wr);
	std::auto_ptr< std::map<uint32_t , std::string> > mapNames(new std::map<uint32_t , std::string>);

	for each(auto m in _mapboxs)
	{
		if(boxids.count(m.second->getBoxId()))
			(*mapNames)[m.second->getBoxId()] = m.second->getRoomName();
	}
	
	return mapNames;
}

bool BoxInfoMan::load(const std::string &path)
{
	MutextReader mr(_wr);

	yiqiding::io::File fp(path);
	try{
		fp.open(yiqiding::io::File::READ);
		DWORD lowlen , highlen;
		lowlen = GetFileSize(fp.native() , &highlen);
		char *data = new char[lowlen];
		fp.read(data , lowlen);

		if (!parse(data , lowlen))
			return false;
		_server->getDatabase()->getInfoConnector()->UpdateBoxInfo(_mapboxs);
		return true;
	
	}
	catch(...)
	{
		return false;
	}

}

BoxInfoItem * BoxInfoMan::getItem(const std::string &ip)
{
	if (!_mapboxs.count(ip))
		return NULL;
	return _mapboxs[ip];
}

bool BoxInfoMan::save(const std::string &path , const char *data , uint32_t len)
{

	MutexWriter mr(_wr);

	if(!parse(data , len))
		return false;

	yiqiding::io::File fp(path);
	try{
		
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		fp.write(data , len);
		fp.close();
		_server->getDatabase()->getInfoConnector()->UpdateBoxInfo(_mapboxs);
		return true;
	}
	catch(...)
	{
		return false;
	}
	
}

BoxInfoMan * BoxInfoMan::getInstace(yiqiding::ktv::Server *server)
{
	if (_instance == NULL)
	{
		MutexWriter guard(_wr);
		if (_instance == NULL)
		{
			_instance = new BoxInfoMan(server);
		}

	}

	return _instance;
}

void BoxInfoMan::unLoad()
{
	MutexWriter guard(_wr);
	if (_instance != NULL)
	{
		delete _instance;
		_instance = NULL;
	}

}

int BoxInfoMan::showInitCall(yiqiding::net::tel::ServerSend *srv){
	
	std::ostringstream out;
	SOCKADDR_IN in;
	{
		MutextReader mr(_wr);
		for each(auto k in _initcalls){
			memset(&in , sizeof(in) , 0);
			in.sin_addr.s_addr = k.first;//htonl(k.first);
		
			char ip[16];
			inet_ntop(AF_INET , &in.sin_addr.s_addr , ip , 16);

			out << ip << " : \r\n";
			for each(auto j in k.second)
				out<<"	" << yiqiding::utility::getDateTime(j) << "\r\n";
		}
	}

	return srv->teleSend(out.str());
	
}


void	BoxInfoMan::addInitCall(uint32_t ip){
	uint32_t tk = (uint32_t)::time(NULL);
	{
		MutexWriter guard(_wr);
		_initcalls[ip].insert(tk);
	}
	
}

BoxInfoMan * BoxInfoMan:: _instance = NULL;
yiqiding::MutexWR BoxInfoMan::_wr;
