#include "yiqiding/ktv/InitProcessor.h"
#include "yiqiding/ktv/BoxInfoMan.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/FireWarn.h"
#include "yiqiding/net/SocketPool.h"
#include "json/json.h"
#ifndef NO_DONGLE
#include "yiqiding/ktv/Dongle.h"
#endif
#include "tinyxml2.h"
#include "yiqiding/ktv/KTVLua.h"
#include "ws2tcpip.h"
#include "yiqiding/ktv/MultiScreen.h"
using yiqiding::ktv::InitProcessor;
using namespace yiqiding::ktv;


void InitProcessor::onReceivePacket()
{
	if (_pac->getDeviceID() != 0xffffffff)
	{
		 sendErrorMessage("InitProcessor Device Id Error");
		 return _conn->shutdown();
	}
	
#ifdef _DEBUG
	unsigned int ip;
	inet_pton(AF_INET , _ip.c_str() , &ip);
	_rdpac->setReqDeviceId(ip);

	//_rdpac->setReqDeviceId(inet_addr(_ip.c_str()));
	_server->getConnectionManager()->getConnectionInit()->record_push(_rdpac);
#endif
	
	if(KtvdoLua(_pac->getRequest() ,_server, this))	
		return _conn->shutdown();

	switch(_pac->getRequest())
	{
	case packet::BOX_REQ_INI_OTHER_INFO:	processOtherInit();							break;
		case packet::BOX_REQ_INI_INFO:		processInit();								break;
		case packet::BOX_REQ_FIRE_INFO:		processFireInfo();							break;	
		default:							sendErrorMessage("Request not supported");	break;
	}
	
	return _conn->shutdown();


}
void InitProcessor::processFireInfo()
{
	Json::Value root;
	root["fireImageUrl"] = Singleton<fire::FireWarn>::getInstance()->getImageUrl().c_str();
	root["fireVideoUrl"] = Singleton<fire::FireWarn>::getInstance()->getVideoUrl().c_str();
	root["status"]	=	packet::BACK_NO_ERROR;

	std::string msg = root.toStyledString();
	packet::Packet pack(_pac->getHeader());
	pack.setPayload(msg.c_str() , msg.length());
	try{
		pack.dispatch(_conn);
		setOutPack(&pack);
	}catch(...)
	{
		yiqiding::utility::Logger::get("system")->log("Box from " + _ip + ":" + yiqiding::utility::toString(_port)+ " get fire info error");
	}
}

void InitProcessor::processInit()
{
	{
		MutextReader mr(box::BoxInfoMan::getMutexWR());
		int result = 0;

	{
		tinyxml2::XMLDocument doc;
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	
		if (doc.ErrorID())
			return sendErrorMessage("Invalid Json Message");
		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("serialid");
		if (node == NULL || !node->GetText())
			return sendErrorMessage("Missing serialid");
		std::string serialid = node->GetText();
#ifndef NO_DONGLE
		try{
			if(Dongle::getInstance()->checkUDID(::time(NULL) , serialid.c_str()))
				result = 0;
			else
				result = 1;
		}
		catch(const std::exception &)
		{
			result = 1;
		}
#endif
	}

	std::string serverurl;
	uint32_t	serverid;
	box::BoxInfoItem * item;
	uint32_t boxid;
	std::string shopname;
	std::string roomname;
	const std::string &version	= _server->getVersion();
	const std::string &video	= _server->getVideoUrl();
	const std::string &ip		= _server->getIPAddr();
	int			port    = _server->getBoxListeningPort();
	const std::string &apkurl  = _server->getAPKPath(); 
	const std::string &romUrl  = _server->getRomUrl();
	const std::string &romVersion = _server->getRomVersion();
	const std::string &tvPlayUrl = _server->getTvPlayUrl();
	const std::string &wineServiceUrl = _server->getWineServiceUrl();
	std::string roomno;

	yiqiding::utility::Logger::get("system")->log("Box from " + _ip + ":" + yiqiding::utility::toString(_port)+ " Get Init Parameters");


	if(!result)
	{
		
		item = box::BoxInfoMan::getInstace()->getItem(_ip);
		if (item != NULL)
		{		
			boxid = item->getBoxId();
			shopname = box::BoxInfoMan::getInstace()->getShopName();
			roomname = item->getRoomName();
			roomno = item->getRoomNo();

			const Balancer::Node* server_node;
			try {
				server_node = _server->getBalancer()->request(boxid);
				serverid = server_node->getID();
				serverurl = server_node->getHostname();
				result = 0;
			}
			catch(const Exception &)
			{
				result = 3;
			}
		}
		else
		{
			result = 2;
		}
	}
	
	packet::Packet out_packet(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("result"));
		node->InsertEndChild(doc.NewText(yiqiding::utility::toString(result).c_str()));
		if(!result)
		{
			root->InsertEndChild(node = doc.NewElement("boxid"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(boxid).c_str()));
			root->InsertEndChild(node = doc.NewElement("infoip"));
			node->InsertEndChild(doc.NewText(ip.c_str()));
			root->InsertEndChild(node = doc.NewElement("infoport"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(port).c_str()));
			root->InsertEndChild(node = doc.NewElement("serverurl"));
			node->InsertEndChild(doc.NewText(serverurl.c_str()));
			root->InsertEndChild(node = doc.NewElement("serverid"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(serverid).c_str()));
			root->InsertEndChild(node = doc.NewElement("video"));
			node->InsertEndChild(doc.NewText(video.c_str()));
			root->InsertEndChild(node = doc.NewElement("roomname"));
			node->InsertEndChild(doc.NewText(roomname.c_str()));
			root->InsertEndChild(node = doc.NewElement("roomno"));
			node->InsertEndChild(doc.NewText(roomno.c_str()));
			root->InsertEndChild(node = doc.NewElement("shopname"));
			node->InsertEndChild(doc.NewText(shopname.c_str()));
			root->InsertEndChild(node = doc.NewElement("version"));
			node->InsertEndChild(doc.NewText(version.c_str()));
			root->InsertEndChild(node = doc.NewElement("apkurl"));
			node->InsertEndChild(doc.NewText(apkurl.c_str()));
			root->InsertEndChild(node = doc.NewElement("romVersion"));
			node->InsertEndChild(doc.NewText(romVersion.c_str()));
			root->InsertEndChild(node = doc.NewElement("romUrl"));
			node->InsertEndChild(doc.NewText(romUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("tvPlayUrl"));
			node->InsertEndChild(doc.NewText(tvPlayUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("wineServiceUrl"));
			node->InsertEndChild(doc.NewText(wineServiceUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("servers"));
			
			for each(auto n in _server->getBalancer()->getNodes())
			{
				tinyxml2::XMLElement* cnode;
				node->InsertEndChild(cnode = doc.NewElement("item"));
				cnode->InsertEndChild(doc.NewText(n->getHostname().c_str()));
			}

			root->InsertEndChild(node = doc.NewElement("validate"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getValidate())));
			root->InsertEndChild(node = doc.NewElement("image_cache"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getImageCache())));
			root->InsertEndChild(node = doc.NewElement("sid"));
			node->InsertEndChild(doc.NewText(_server->getSid().c_str()));
			root->InsertEndChild(node = doc.NewElement("controlenabled"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getControlenabled())));
			root->InsertEndChild(node = doc.NewElement("carapkversion"));
			node->InsertEndChild(doc.NewText(_server->getcaraVersion().c_str()));
			root->InsertEndChild(node = doc.NewElement("carapkurl"));
			node->InsertEndChild(doc.NewText(_server->getcaraUrl().c_str()));
			root->InsertEndChild(node = doc.NewElement("portapkurl"));
			node->InsertEndChild(doc.NewText(_server->getPortApkUrl().c_str()));
			root->InsertEndChild(node = doc.NewElement("portapkversion"));
			node->InsertEndChild(doc.NewText(_server->getPortApkVersion().c_str()));
		
		}

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_packet.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	out_packet.dispatch(_conn);
	setOutPack(&out_packet);
	}
	box::BoxInfoMan::getInstace()->addInitCall(inet_addr(_ip.c_str()));
	


	


}

void InitProcessor::processOtherInit()
{
	MutextReader mr(box::BoxInfoMan::getMutexWR());
	int result = 0;

	{
		tinyxml2::XMLDocument doc;
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());

		if (doc.ErrorID())
			return sendErrorMessage("Invalid Json Message");
		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("serialid");
		if (node == NULL || !node->GetText())
			return sendErrorMessage("Missing serialid");
		std::string serialid = node->GetText();
#ifndef NO_DONGLE
		try{
			if(Dongle::getInstance()->checkUDID(::time(NULL) , serialid.c_str()))
				result = 0;
			else
				result = 1;
		}
		catch(const std::exception &)
		{
			result = 1;
		}
#endif
	}

	std::string serverurl;
	uint32_t	serverid;
	box::BoxInfoItem * item;
	uint32_t boxid;
	std::string shopname;
	std::string roomname;
	const std::string &version	= _server->getVersion();
	const std::string &video	= _server->getVideoUrl();
	const std::string &ip		= _server->getIPAddr();
	int			port    = _server->getAppListeningPort();

	const std::string &apkurl  = _server->getAPKPath(); 
	const std::string &romUrl  = _server->getRomUrl();
	const std::string &romVersion = _server->getRomVersion();
	const std::string &tvPlayUrl = _server->getTvPlayUrl();
	const std::string &wineServiceUrl = _server->getWineServiceUrl();
	std::string roomno;

	yiqiding::utility::Logger::get("system")->log("Box from " + _ip + ":" + yiqiding::utility::toString(_port)+ " Get Init Parameters");


	if(!result)
	{
		boxid = Singleton<MultiScreen>::getInstance()->getSlave(_ip);
		std::string ip = Singleton<MultiScreen>::getInstance()->getMasterIp(_ip);
		
		item = box::BoxInfoMan::getInstace()->getItem(ip);
		if (item != NULL)
		{		
			shopname = box::BoxInfoMan::getInstace()->getShopName();
			roomname = item->getRoomName();
			roomno = item->getRoomNo();
		}

		if (boxid != INVALID_BOXID)
		{		
			const Balancer::Node* server_node;
			try {
				server_node = _server->getBalancer()->request(boxid);
				serverid = server_node->getID();
				serverurl = server_node->getHostname();
				result = 0;
			}
			catch(const Exception &)
			{
				result = 3;
			}
		}
		else
		{
			result = 2;
		}
	}

	packet::Packet out_packet(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("result"));
		node->InsertEndChild(doc.NewText(yiqiding::utility::toString(result).c_str()));
		if(!result)
		{
			root->InsertEndChild(node = doc.NewElement("boxid"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(boxid).c_str()));
			root->InsertEndChild(node = doc.NewElement("infoip"));
			node->InsertEndChild(doc.NewText(ip.c_str()));
			root->InsertEndChild(node = doc.NewElement("infoport"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(port).c_str()));
			root->InsertEndChild(node = doc.NewElement("serverurl"));
			node->InsertEndChild(doc.NewText(serverurl.c_str()));
			root->InsertEndChild(node = doc.NewElement("serverid"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(serverid).c_str()));
			root->InsertEndChild(node = doc.NewElement("video"));
			node->InsertEndChild(doc.NewText(video.c_str()));
			root->InsertEndChild(node = doc.NewElement("roomname"));
			node->InsertEndChild(doc.NewText(roomname.c_str()));
			root->InsertEndChild(node = doc.NewElement("roomno"));
			node->InsertEndChild(doc.NewText(roomno.c_str()));
			root->InsertEndChild(node = doc.NewElement("shopname"));
			node->InsertEndChild(doc.NewText(shopname.c_str()));
			root->InsertEndChild(node = doc.NewElement("version"));
			node->InsertEndChild(doc.NewText(version.c_str()));
			root->InsertEndChild(node = doc.NewElement("apkurl"));
			node->InsertEndChild(doc.NewText(apkurl.c_str()));
			root->InsertEndChild(node = doc.NewElement("romVersion"));
			node->InsertEndChild(doc.NewText(romVersion.c_str()));
			root->InsertEndChild(node = doc.NewElement("romUrl"));
			node->InsertEndChild(doc.NewText(romUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("tvPlayUrl"));
			node->InsertEndChild(doc.NewText(tvPlayUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("wineServiceUrl"));
			node->InsertEndChild(doc.NewText(wineServiceUrl.c_str()));
			root->InsertEndChild(node = doc.NewElement("servers"));
			

			for each(auto n in _server->getBalancer()->getNodes())
			{
				tinyxml2::XMLElement* cnode;
				node->InsertEndChild(cnode = doc.NewElement("item"));
				cnode->InsertEndChild(doc.NewText(n->getHostname().c_str()));
			}

			root->InsertEndChild(node = doc.NewElement("validate"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getValidate())));
			root->InsertEndChild(node = doc.NewElement("image_cache"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getImageCache())));
			root->InsertEndChild(node = doc.NewElement("sid"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toString(_server->getSid()).c_str()));
			root->InsertEndChild(node = doc.NewElement("controlenabled"));
			node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(_server->getControlenabled())));
			root->InsertEndChild(node = doc.NewElement("carapkversion"));
			node->InsertEndChild(doc.NewText(_server->getcaraVersion().c_str()));
			root->InsertEndChild(node = doc.NewElement("carapkurl"));
			node->InsertEndChild(doc.NewText(_server->getcaraUrl().c_str()));
			root->InsertEndChild(node = doc.NewElement("portapkurl"));
			node->InsertEndChild(doc.NewText(_server->getPortApkUrl().c_str()));
			root->InsertEndChild(node = doc.NewElement("portapkversion"));
			node->InsertEndChild(doc.NewText(_server->getPortApkVersion().c_str()));

		}

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_packet.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	out_packet.dispatch(_conn);
	setOutPack(&out_packet);
}
