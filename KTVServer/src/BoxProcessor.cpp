/**
 * Box Processor Implementation
 * @author Shiwei Zhang
 * @date 2014.02.19
 */

#include <string>
#include <tinyxml2.h>
#include "yiqiding/ktv/BoxProcessor.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/Exception.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Logger.h"
#include "json/json.h"
#include "yiqiding/ktv/GameTimer.h"
#ifndef NO_DONGLE
#include "yiqiding/ktv/Dongle.h"
#endif
#include "yiqiding/ktv/KTVLua.h"
#include "yiqiding/ktv/MessageRule.h"
#include "yiqiding/ktv/AdUrl.h"
#include "yiqiding/ktv/FireWarn.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/KTVChallenge.h"
#include "yiqiding/utility/Transcode.h"
#include "yiqiding/ktv/Volume.h"
#include "KTVKGame2.h"
#include "yiqiding/ktv/db/GameTimer2.h"
#include "yiqiding/ktv/GameReward.h"
#include "yiqiding/ktv/GameReward2.h"
#include "yiqiding/ktv/KTVPCM.h"
#include "ws2tcpip.h"

using namespace yiqiding::ktv;
using namespace yiqiding::utility;
using yiqiding::ktv::BoxProcessor;
using yiqiding::Exception;
using yiqiding::utility::Logger;
using namespace yiqiding::ktv::game;
using namespace yiqiding::social;
#pragma region HELPER FUNCTION
//////////////////////////////////////////////////////////////////////////
/// HELPER FUNCTION
static inline const char* getText(tinyxml2::XMLElement* node) {
	const char* p = node->GetText();
	return p?p:"";
}
#pragma endregion

BoxProcessor::BoxProcessor(Server* server, BoxConnection* conn, Packet* pac , const std::string &ip , int port) : packet::Processor(server, conn, pac , ip , port) 
{
	
};

//////////////////////////////////////////////////////////////////////////
// Box Processor
//////////////////////////////////////////////////////////////////////////
void BoxProcessor::finishControlRoom() {
	std::auto_ptr<std::string> err_msg;
	bool confirm;

	// Parse XML
	try {
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID())
			throw Exception("Box Internal", "Invalid XML document", __FILE__, __LINE__);

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			throw Exception("Box Internal", "Missing root", __FILE__, __LINE__);
		tinyxml2::XMLElement* node = root->FirstChildElement("status");
		if (node == NULL)
			throw Exception("Box Internal", "Missing status", __FILE__, __LINE__);
		else if (toBool(getText(node))) {
			node = root->FirstChildElement("error");
			if (node == NULL)
				throw Exception("Box Internal", "Missing error", __FILE__, __LINE__);
			else
				throw Exception("Box", getText(node), __FILE__, __LINE__);
		}
		node = root->FirstChildElement("confirm");
		if (node == NULL)
			throw Exception("Box Internal", "Missing confirm", __FILE__, __LINE__);
		else
			confirm = toBool(getText(node));
	} catch (const Exception& err) {
		if (err.getObjectName() == "Box")
			err_msg.reset(new std::string(err.getReason()));
		else if (err.getObjectName() == "Box Internal")
			err_msg.reset(new std::string("Internal Box error"));
		else
			throw err;
	}

	// Find the corresponding sending packet
	std::auto_ptr<extended::ConnectionCache> cache(_server->getConnectionManager()->getConnectionBox(_pac->getDeviceID())->findCache(_pac->getIdentifier()));
	if (cache.get() == NULL)	return;	// No such a request

	// Prepare packet
	Packet* ref_pack = cache->getReferencePacket();
	if (ref_pack == NULL)	return;	// Error. Should not happen.

	Packet out_pack(ref_pack->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		if (err_msg.get() == NULL) {
			node->InsertEndChild(doc.NewText("0"));
			root->InsertEndChild(node = doc.NewElement("confirm"));
			node->InsertEndChild(doc.NewText(toCString(confirm)));
		} else {
			node->InsertEndChild(doc.NewText("1"));
			root->InsertEndChild(node = doc.NewElement("error"));
			node->InsertEndChild(doc.NewText(err_msg->c_str()));
		}

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Send result to ERP
	try {
		_server->getConnectionManager()->sendToERP(ERP_ROLE_MAIN, &out_pack);
		setAttPack(&out_pack , ERP_ROLE_MAIN);
	} catch (const extended::ConnectionLost&) {
		Logger::get("server")->log("Box id " + toString(ref_pack->getDeviceID()) + ": Fail to send to ERP", Logger::WARNING);
	}
}
//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processApkPath() {
	uint32_t box_id;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Missing boxid");
		else
			box_id = (uint32_t)strtoul(getText(node), NULL, 0);
	}

	// Process box id
	if (!isBoxIDValid(box_id))	return;
	/// @todo Further processing?

	// Feed back
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("path"));
		node->InsertEndChild(doc.NewText(_server->getAPKPath().c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	// Send to remote
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processOpenRoom() {
	uint32_t box_id;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Missing boxid");
		else
			box_id = (uint32_t)strtoul(getText(node), NULL, 0);
	}

	// Process box id
	if (!isBoxIDValid(box_id))	return;

	// Begin Request
	unsigned int timestamp;
	bool open = _server->getDatabase()->getInfoConnector()->getBoxStatus(box_id , timestamp);

	// open
	if (open && _server->getConnectionManager()->getBoxCode(box_id) == "")
	{
		char strcode[20];
		sprintf_s(strcode , sizeof(strcode) ,"%02d%s" , _pac->getDeviceID() , utility::generateTempCode().c_str());
		_server->getConnectionManager()->setBoxCode(_pac->getDeviceID() ,strcode);	
		Logger::get("system")->log("server restart , " + toString(box_id) + " must generate code !" , Logger::WARNING);
	}

	// Begin dummy feedback: Always return false
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("room"));
		node->InsertEndChild(doc.NewText(toCString(open)));
	//	root->InsertEndChild(node = doc.NewElement("timestamp"));
	//	node->InsertEndChild(doc.NewText(yiqiding::utility::toString(timestamp).c_str()));
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	// Send to remote
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);

	//
	if(open)
	{
		notifySongsToBox();
	}

}
//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processAllocServer() {
	uint32_t box_id;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Missing boxid");
		else
			box_id = (uint32_t)strtoul(getText(node), NULL, 0);
	}

	// Process box id
	if (!isBoxIDValid(box_id))	return;

	// Process
	const Balancer::Node* server_node;
	try {
		server_node = _server->getBalancer()->request(box_id);
		getConnection()->setAllocated();
	} catch (const Exception& err) {
		if (err.getObjectName() == "Balancer")
			return sendErrorMessage(err.getReason());
		else
			throw err;
	} 

	// Feed back
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("serverid"));
		text = toString(server_node->getID());
		node->InsertEndChild(doc.NewText(text.c_str()));
		root->InsertEndChild(node = doc.NewElement("server"));
		node->InsertEndChild(doc.NewText(server_node->getHostname().c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	// Send to remote
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processService() {
	uint32_t box_id;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Missing boxid");
		else
			box_id = (uint32_t)strtoul(getText(node), NULL, 0);
	}

	// Process box id
	if (!isBoxIDValid(box_id))	return;

	// Process
	Packet out_pack(packet::KTV_REQ_ERP_SERVICE_CALL);
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("boxid"));
		text = toString(box_id);
		node->InsertEndChild(doc.NewText(text.c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	ConnectionManager* conn_man = _server->getConnectionManager();
	// Register in Cache
	conn_man->getConnectionERP(ERP_ROLE_MAIN)->cache(&out_pack, _pac);
	// Send to ERP
	try {
		conn_man->sendToERP(ERP_ROLE_MAIN, &out_pack);
		setAttPack(&out_pack , ERP_ROLE_MAIN);
	} catch (const extended::ConnectionLost&) {
		delete conn_man->getConnectionERP(ERP_ROLE_MAIN)->findCache(out_pack.getIdentifier());
		sendErrorMessage("ERP system is down");
	}
}

//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processReallocServer() {
	uint32_t box_id;
	std::set<size_t> na_server;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Missing boxid");
		else
			box_id = (uint32_t)strtoul(getText(node), NULL, 0);
		if ((node = root->FirstChildElement("serverids")) == NULL)
			return sendErrorMessage("Missing serverids");
		for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
			na_server.insert(strtoul(i->GetText(), NULL, 0));
	}

	if(box_id != _pac->getDeviceID()) return;


	// Process box id
	if (!isBoxIDValid(box_id))	return;

	// Process
	const Balancer::Node* server_node;
	try {
		server_node = _server->getBalancer()->request(box_id, na_server);
		getConnection()->setAllocated();
	} catch (const Exception& err) {
		if (err.getObjectName() == "Balancer")
			return sendErrorMessage(err.getReason());
		else
			throw err;
	}

	// Feed back
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("serverid"));
		text = toString(server_node->getID());
		node->InsertEndChild(doc.NewText(text.c_str()));
		root->InsertEndChild(node = doc.NewElement("server"));
		node->InsertEndChild(doc.NewText(server_node->getHostname().c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	// Send to remote
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}

//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processUpdateSongCount() {
	uint32_t mid;

	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node;
		if (node = root->FirstChildElement("mid"))
			mid = toUInt(getText(node));
		else return sendErrorMessage("Missing mid");
	}

	// Process
	_server->getDatabase()->getMediaConnector()->addMediaRecord((unsigned int)getConnection()->getBoxID(), mid);

	// No feedback
}



//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processGetGPS()
{
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
	/*	
		//old code get latitude & longitude from sql

		std::auto_ptr<std::map<std::string , model::ConfigItem>> map = _server->getDatabase()->getConfigConnector()->getConfigInfo();

		if (!map->count(model::latitude) || !map->count(model::longitude))
		{
			return sendErrorMessage("Inter Server has no latitude or longitude");
		}
	*/	
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));

		root->InsertEndChild(node = doc.NewElement(model::longitude.c_str()));
	//	node->InsertEndChild(doc.NewText(((*map)[model::longitude]).getValue().c_str()));
		node->InsertEndChild(doc.NewText(_server->getJingDu().c_str()));

		root->InsertEndChild(node = doc.NewElement(model::latitude.c_str()));	
	//	node->InsertEndChild(doc.NewText(((*map)[model::latitude]).getValue().c_str()));
		node->InsertEndChild(doc.NewText(_server->getWeiDu().c_str()));

		root->InsertEndChild(node = doc.NewElement("address"));
		node->InsertEndChild(doc.NewText(yiqiding::utility::transcode(_server->getAddress() , yiqiding::utility::CodePage::GBK , yiqiding::utility::CodePage::UTF8).c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	// Send to remote
	out_pack.dispatch(_conn);


	setOutPack(&out_pack);
}

void BoxProcessor::processGetAddress()
{
	Packet out_pack(_pac->getHeader());
	Json::Value root;
	root["longitude"] = _server->getJingDu();
	root["latitude"] = _server->getWeiDu();
	root["address"] = yiqiding::utility::transcode(_server->getAddress() , yiqiding::utility::CodePage::GBK , yiqiding::utility::CodePage::UTF8);
	root["city"] = yiqiding::utility::transcode(_server->getCity() , yiqiding::utility::CodePage::GBK , yiqiding::utility::CodePage::UTF8);;

	std::string msg = root.toStyledString();
	out_pack.setPayload(msg.c_str() , msg.length());
	try {
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	} catch (const std::exception& err) {
		;
	}

}

void BoxProcessor::processLogCore()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	int type = 0;
	std::string content;
	if(!reader.parse(str , root) || !root.isObject())
	{
		
		sendErrorJsonMessage(packet::ERROR_FORMAT ,"invalid json string" );
		Logger::get("system")->log("processChangeBox Invalid json string" , Logger::WARNING);
		return;
	}
	if(!root.isMember("log"))
	{

		sendErrorJsonMessage(packet::ERROR_FORMAT ," Miss log" );
		Logger::get("system")->log("processLogCore Miss log" , Logger::WARNING);
		return;
	}
	if(root.isMember("type") && root["type"].isConvertibleTo(Json::intValue))
	{
		type = root["type"].asInt(); 
	}
	content = root["log"].asString();

	if(type != 0)
		_server->getTelNet()->Send(content);
	yiqiding::utility::Logger::get("core")->log(content , yiqiding::utility::Logger::NONE);

	Packet pack(_pac->getHeader());
	try{
		Json::Value newroot;
		newroot["status"]	=	packet::BACK_NO_ERROR;
		newroot["confirm"] = 1;
		std::string msg = newroot.toStyledString();
		pack.setPayload(msg.c_str() , msg.length());
		pack.dispatch(_conn);
		setOutPack(&pack);
	}
	catch(...){}

}

void BoxProcessor::processChangeBox()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	uint32_t toboxid;
	std::string songs;
	uint32_t fromboxid;
	Json::FastWriter writer;
	if(!reader.parse(str , root))
	{	
		Logger::get("system")->log("processChangeBox Invalid json string" , Logger::WARNING);
		return;
	}
	if(!root.isMember("songs"))
	{	
		Logger::get("system")->log("processChangeBox Miss songs" , Logger::WARNING);
		return;
	}
	songs = writer.write(root["songs"]);
	songs = "{\"songs\":" + songs.substr(0 , songs.length() - 1) + "}";
	std::auto_ptr<extended::ConnectionCache> cache(_server->getConnectionManager()->getConnectionBox(_pac->getDeviceID())->findCache(_pac->getIdentifier()));
	if (cache.get() == NULL)	
	{
		Logger::get("system")->log("processChangeBox Cache can not find boxid: " + toString(_pac->getDeviceID()) + "identifier: " + toString(_pac->getIdentifier()));
		return;	// No such a request
	}
	// Prepare packet
	Packet* ref_pack = cache->getReferencePacket();
	if (ref_pack == NULL)	
	{
		Logger::get("system")->log("processChangeBox Cache ref_pack not find boxid: " + toString(_pac->getDeviceID()) + "identifier: " + toString(_pac->getIdentifier()));
		return;	// Error. Should not happen.
	}
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(ref_pack->getPayload(), ref_pack->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
		{	
			Logger::get("system")->log("processChangeBox Cache ref_pack ErrorID");
			return ;
		}

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)// Error. Should not happen.
		{
			Logger::get("system")->log("processChangeBox Cache ref_pack root is null");
			return;
		}
		tinyxml2::XMLElement* node = root->FirstChildElement("toboxid");
		if(node == NULL || !yiqiding::utility::isULong(node->GetText())) // Error. Should not happen.
		{	
			Logger::get("system")->log("processChangeBox toboxid is Miss or not uint ");
			return;
		}
		toboxid = yiqiding::utility::toUInt(node->GetText());
		node = root->FirstChildElement("fromboxid");
		if(node == NULL || !yiqiding::utility::isULong(node->GetText()))
		{
			Logger::get("system")->log("processChangeBox fromboxid is Miss or not uint ");
			return;
		}
		fromboxid = yiqiding::utility::toUInt(node->GetText());
		if(fromboxid != _pac->getDeviceID())
		{
			Logger::get("system")->log("processChangeBox fromboxid is not equal DeviceID");
			return;
		}
	}


	Packet pack(packet::KTV_REQ_BOX_CHANGE_TO);
	pack.setPayload(songs.c_str() , songs.length());

	try {

		_server->getConnectionManager()->sendToBox(toboxid , &pack);
		setAttPack(&pack , toboxid);

	} catch (const yiqiding::ktv::extended::BoxConnectionLost& err) {
		_server->getConnectionManager()->setBoxSongs(toboxid , songs);
		Logger::get("system")->log("toboxid " + toString(toboxid) + "been down , stored");
	}


	
}

void BoxProcessor::processMsgRule()
{
	Json::Value root;
	root["datetime"] = (unsigned int)::time(NULL);
	root["file"] = MessageRule::getInstance()->getContent().c_str();
	root["status"] = 0;
	std::string msg = root.toStyledString();

	Packet pack(_pac->getHeader());
	try{
		pack.setPayload(msg.c_str() , msg.length());
		pack.dispatch(_conn);
		setOutPack(&pack);
	}
	catch(extended::BoxConnectionLost &e)
	{
		yiqiding::utility::Logger::get("server")->log("Process MsgRule Lost boxid " + yiqiding::utility::toString(_pac->getDeviceID()));
	}

}

void BoxProcessor::processAdUrl()
{
	Packet pack(_pac->getHeader());
	try
	{
		std::string urls = Singleton<AdUrl>::getInstance()->getUrls();
		if(urls == "")
			urls = "{\"urls\":[]}";
		pack.setPayload(urls.c_str() , urls.length());
		pack.dispatch(_conn);
		setOutPack(&pack);
	}
	catch(extended::BoxConnectionLost &e)
	{
		yiqiding::utility::Logger::get("server")->log("Process AdUrl Lost boxid " + yiqiding::utility::toString(_pac->getDeviceID()));
	}


}

void BoxProcessor::notifySongsToBox()
{
	std::string songs = _server->getConnectionManager()->getBoxSongs(_pac->getDeviceID());
	if(songs == "")
		return;

	Packet pack(packet::KTV_REQ_BOX_CHANGE_TO);
	{
		pack.setPayload(songs.c_str() , songs.length());
	}
	try{
		pack.dispatch(_conn);
		setAttPack(&pack , _pac->getDeviceID());
		//after notify , clear songs
		_server->getConnectionManager()->setBoxSongs(_pac->getDeviceID() , "");
	}
	catch(const yiqiding::ktv::extended::BoxConnectionLost &e)
	{
		Logger::get("system")->log("notifySongsToBox boxid:" + _pac->getDeviceID());
	}
}

void BoxProcessor::processTurnMsgToAll()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Invalid json string");
	if(!root.isMember("toboxids") || !root["toboxids"].isArray())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss toboxids or not array!");
	if(!root.isMember("type") || !root["type"].isInt())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss type or not int");
	if(!root.isMember("message"))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss message");

	Json::Value arrayRoot = root["toboxids"];

	bool isAll = (arrayRoot.size() == 0);



	for (int i = 0 ; i < (int)arrayRoot.size() ; ++i)
	{
		if(!arrayRoot[i].isObject() || !arrayRoot[i].isMember("boxid") || ! arrayRoot[i]["boxid"].isConvertibleTo(Json::uintValue))
		{
			return sendErrorJsonMessage(packet::ERROR_FORMAT , "toboxid not uint or Miss");
		}
	}

	Packet out_packet(packet::KTV_REQ_BOX_TURN_MSG);
	{
		Json::Value newroot;
		newroot["fromboxid"] = _pac->getDeviceID();
		newroot["type"] = root["type"];
		newroot["message"] = root["message"];
		std::string msg = newroot.toStyledString();
		out_packet.setPayload(msg.c_str() , msg.length());
	}

	Packet back_packet(_pac->getHeader());
	Json::Value newroot;
	Json::Value arryObj;
	{
		newroot["toboxids"] = root["toboxids"];
		newroot["type"] = root["type"];
		newroot["status"] = packet::BACK_NO_ERROR;
	}

	if(isAll)
	{
		for each (auto box_conn in _server->getConnectionManager()->getConnectionBox()) {
			if(box_conn.first == _pac->getDeviceID())
				continue;
			auto conn = box_conn.second->getConnection(_server);
			if (conn) {
				try {
					out_packet.dispatch(conn);
				} catch (...) {}
				conn->release();
			}
		}
		
		{
			std::string msg = newroot.toStyledString();
			back_packet.setPayload(msg.c_str() , msg.length());
		}


	}
	else{
		for(int i = 0 ; i < arrayRoot.size() ; ++i)
		{
			Json::Value item;
			try{
				_server->getConnectionManager()->sendToBox(arrayRoot[i]["boxid"].asUInt() , &out_packet);
			}
			catch(...){
				item["boxid"] = arrayRoot[i]["boxid"].asUInt();
				arryObj.append(item);
			}
		}
		
	}
	{
		newroot["failboxids"] = arryObj;
		std::string msg = newroot.toStyledString();
		back_packet.setPayload(msg.c_str() , msg.length());
	}
	setAttPack(&out_packet , -1);
	back_packet.dispatch(_conn);
	setOutPack(&back_packet);

}


void BoxProcessor::processTurnMsgToAllApp()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Invalid json string");
	if(!root.isMember("touids") || !root["touids"].isArray())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss touids or Not Array!");
	if(!root.isMember("type") || !root["type"].isInt())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss type or Not Int!");
	if(!root.isMember("message"))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss message!");
	
	Json::Value arrayRoot = root["touids"];
	//check
	for(int i = 0 ; i < (int)arrayRoot.size() ; ++i)
	{
		if (!arrayRoot[i].isMember("uid") || !arrayRoot[i]["uid"].isConvertibleTo(Json::uintValue))
		{
			return sendErrorJsonMessage(packet::ERROR_FORMAT , "uid not Uint!");
		}
	}

	Packet out_packet(packet::KTV_REQ_APP_TURN_MESSAGE);
	{
		Json::Value newroot;
		std::string code = _server->getConnectionManager()->getBoxCode(_pac->getDeviceID());
		if(code == "")
		{
			return sendErrorJsonMessage(packet::ERROR_CODE_NOT_GENERATE , "code has not generate");
		}
		newroot["fromuid"] = code.c_str();
		newroot["type"] = root["type"];
		newroot["message"] = root["message"];
		std::string msg = newroot.toStyledString();
		
		out_packet.setPayload(msg.c_str() , msg.length());

	}

	Packet back_packet(_pac->getHeader());
	Json::Value newroot;
	Json::Value arryObj;
	{
		newroot["uids"] = root["touids"];
		newroot["type"] = root["type"];
		newroot["status"] = packet::BACK_NO_ERROR;
	}

	for(int i = 0 ; i < arrayRoot.size() ; ++i)
	{
		Json::Value item;
		try{
			_server->getConnectionManager()->sendToApp(arrayRoot[i]["uid"].asUInt() , &out_packet);
		}
		catch(...){
			item["uid"] = arrayRoot[i]["uid"].asUInt();
			arryObj.append(item);
		}
	}
	{
		newroot["failuids"] = arryObj;
		std::string msg = newroot.toStyledString();
		back_packet.setPayload(msg.c_str() , msg.length());
	}
	setAttPack(&out_packet , -1);
	back_packet.dispatch(_conn);
	setOutPack(&back_packet);
}




//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processTurnMsgToApp()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Invalid json string");
	
	if ( !root.isMember("touid") || !root.isMember("message"))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Missing touid or Missing message");
	
	if(!root["touid"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "touid must be UInt");

	if (!root.isMember("type") || !root["type"].isInt())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Missing type or type is not int");

	int type = root["type"].asInt();

	uint32_t aid = root["touid"].asUInt();
	std::string message;  

	bool confirm = false;
	std::string code = _server->getConnectionManager()->getBoxCode(_pac->getDeviceID());
	if(code == "")
	{
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_GENERATE , "code has not generate");
	}
	

	Packet packet(packet::KTV_REQ_APP_TURN_MESSAGE);

	root.removeMember("touid");
	root["fromuid"] = code.c_str();
	message = root.toStyledString();
	packet.setPayload(message.c_str() , message.length());


	try{
		_server->getConnectionManager()->sendToApp(aid , &packet);
		setAttPack(&packet , aid);
		confirm = true;
	}
	catch (const extended::AppConnectionLost&) {
		confirm = false;	
	}
	
	
	try {
		Packet back(_pac->getHeader());
		Json::Value newroot;
		if (confirm)
		{
			newroot["status"] = packet::BACK_NO_ERROR;
		}
		else
		{
			newroot["status"] = packet::ERROR_APP_DOWN;
			newroot["error"] = "app has been down";
		}
		newroot["uid"] = aid;
		newroot["type"] = type;
		std::string msg = newroot.toStyledString();
		back.setPayload(msg.c_str() , msg.length());
		back.dispatch(_conn);
		setOutPack(&back);
	} catch (const extended::BoxConnectionLost& ) {
		Logger::get("server")->log("Server Fail to send to Box id" + toString(_pac->getDeviceID()), Logger::WARNING);
	}
	
}

//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processTempCode()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Invalid json string");
	if( !root.isMember("serialid"))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "Miss serialid");

	if(!root["serialid"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "serilid must be UInt");

	if(root["serialid"].asUInt() != _pac->getDeviceID())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "serialid Wrong");

	/////////////////////////////////////////////////////////////////////////
	//generator json
	Packet packet(_pac->getHeader());
	Json::Value newroot;
	Json::Value value;

	
	
	//don't get code
	std::string code = _server->getConnectionManager()->getBoxCode(_pac->getDeviceID());
	if (code == "")
	{
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_GENERATE , "code is not generate ");
	}

	value["code"] = code.c_str();
	value["port"] = _server->getAppListeningPort();
	newroot["result"] = value;
	newroot["status"] = packet::BACK_NO_ERROR;
	newroot["appServerUrl"] = _server->getAppServerUrl();

	try {
		std::string content = newroot.toStyledString();
		packet.setPayload(content.c_str() , content.length());
		packet.dispatch(_conn);
		setOutPack(&packet);

	} catch (const extended::ConnectionLost&) {
		Logger::get("server")->log("Box id " + toString(_pac->getDeviceID()) + ": Fail to Generate TempCode" , Logger::WARNING);
	}
	

	
	

}


//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processGameJoin()
{
	// Parse XML
	bool confirm = false;
	uint32_t kid = 0;
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("confirm");
		if (node == NULL)
			return sendErrorMessage("Missing confirm");
		
		confirm = yiqiding::utility::toBool(node->GetText());
		if (!confirm)
		{
			return;
		}

		node = root->FirstChildElement("kid");
		if (node == NULL)
			return sendErrorMessage("Missing kid");
		if (!yiqiding::utility::isULong(node->GetText()))
			return sendErrorMessage("kid must uint");
		kid = yiqiding::utility::toUInt(node->GetText());
		{
			MutextReader wr(SingerGame::getInstace()->getMutexWR());
			GameTimer *gt = SingerGame::getInstace()->get(kid);
			if (gt == NULL)
			{
				return Logger::get("system")->log("Game Kid:" + toString(kid) + " is not exist , boxid " + toString(_pac->getDeviceID()) , Logger::WARNING);
			}
			if (gt->canJoin())
			{
				return Logger::get("system")->log("Game Kid:" + toString(kid) + " is timeout , boxid " + toString(_pac->getDeviceID()) , Logger::WARNING);
			}
			gt->joinSingeGame(_pac->getDeviceID());
		}
	}
}



//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processGameReq()
{
	uint32_t kid;
	bool confirm = false;
	tinyxml2::XMLDocument doc;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	// Obtain values
	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if (root == NULL)
		return sendErrorMessage("Missing root");
	tinyxml2::XMLElement* node = root->FirstChildElement("kid");
	if (node == NULL || !isULong(node->GetText()))
	{
		return sendErrorMessage("Missing kid or kid must uint");
	}
	kid = toUInt(node->GetText());
	{
		MutextReader wr(SingerGame::getInstace()->getMutexWR());
		GameTimer *gt = SingerGame::getInstace()->get(kid);
		if(gt != NULL && gt->getState() >= GAME_START_TIMER )
			confirm = true;
	}

	packet::Packet out_pack(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);

}


//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processGameUpload()
{
	uint32_t kid = 0;
	uint32_t score = 0;
	bool confirm = false;
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload(), _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID()) 
			return sendErrorMessage("Invalid XML document");

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node = root->FirstChildElement("kid");
		if (node == NULL || !isULong(node->GetText()))
		{
			return sendErrorMessage("Missing kid or kid must uint");
		}
		kid = toUInt(node->GetText());
		
		node = root->FirstChildElement("score");
		if (node == NULL || !isULong(node->GetText()))
		{
			return sendErrorMessage("Misssing score or score must uint");
		}
		score = toUInt(node->GetText());
	
		{
			MutextReader wr(SingerGame::getInstace()->getMutexWR());
			GameTimer *gt = SingerGame::getInstace()->get(kid);
			if (gt != NULL && gt->canUpLoad())
			{
				gt->scoreSingeGame(_pac->getDeviceID() , score);
				confirm = true;
			}

		
			
			packet::Packet out_pack(_pac->getHeader());
			{
				tinyxml2::XMLDocument doc;
				tinyxml2::XMLElement* root;
				tinyxml2::XMLElement* node;
				std::string text;
				doc.InsertEndChild(root = doc.NewElement("root"));
				root->InsertEndChild(node = doc.NewElement("status"));
				node->InsertEndChild(doc.NewText("0"));
				root->InsertEndChild(node = doc.NewElement("confirm"));
				node->InsertEndChild(doc.NewText(toCString(confirm)));
				tinyxml2::XMLPrinter printer(0, true);
				doc.Print(&printer);
				out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
			}
			out_pack.dispatch(_conn);
			setOutPack(&out_pack);
			
		}
	}


}

//////////////////////////////////////////////////////////////////////////
void BoxProcessor::processGetKtvGame()
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	tinyxml2::XMLElement* cnode;
	tinyxml2::XMLElement* croot;
	tinyxml2::XMLElement* droot;
	tinyxml2::XMLElement* dnode;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("status"));
	node->InsertEndChild(doc.NewText("0"));
	root->InsertEndChild(node = doc.NewElement("ktinyinfo"));
	{
		yiqiding::MutextReader mr(SingerGame::getInstace()->getMutexWR()); 
		for each(auto bc in SingerGame::getInstace()->getTimers())
		{
			if(bc.second->getState() > GAME_JOIN_TIMER)
				continue;
			node->InsertEndChild(croot = doc.NewElement("item"));
			croot->InsertEndChild(cnode = doc.NewElement("kid"));
			cnode->InsertEndChild(doc.NewText(toString(bc.second->getKid()).c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("notifytitle"));
			cnode->InsertEndChild(doc.NewText(bc.second->getNotifyTitle().c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("notifymessage"));
			cnode->InsertEndChild(doc.NewText(bc.second->getNotifyMessage().c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("mids"));
			for each(auto mid in bc.second->getMids())
			{
				cnode->InsertEndChild(droot = doc.NewElement("item"));
				droot->InsertEndChild(dnode = doc.NewElement("mid"));
				dnode->InsertEndChild(doc.NewText(toString(mid._mid).c_str()));
				droot->InsertEndChild(dnode = doc.NewElement("name"));
				dnode->InsertEndChild(doc.NewText(mid._name.c_str()));
			}
			croot->InsertEndChild(cnode = doc.NewElement("starttime"));
			cnode->InsertEndChild(doc.NewText(toString(bc.second->getStartTime()).c_str()));
		}
	}
	Packet out_pack(_pac->getHeader());
	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);
	// Pack
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);

	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}

//the same as InitProcessor
void BoxProcessor::processFireInfo()
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

void BoxProcessor::processVolume()
{
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(Singleton<ktv::Volume>::getInstance()->getVolumeInfo() , root))
	{
		root["sound"] = 3;
		root["music"] = 50;
		root["mic"] = 50;
		root["power"] = 50;
	}

	root["status"] = packet::BACK_NO_ERROR;
	
	packet::Packet pack(_pac->getHeader());
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	try{
		pack.dispatch(_conn);
		setOutPack(&pack);
	}catch(...)
	{
		yiqiding::utility::Logger::get("system")->log("Box from " + _ip + ":" + yiqiding::utility::toString(_port)+ " get volume info error");
	}

}


//challenge
void BoxProcessor::processAddGame()
{
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(_pac->getPayload() , root))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("toboxid") || !root.isMember("roomno") || !root.isMember("roomname")
		|| ! root.isMember("mid") || !root.isMember("name") || !root.isMember("singer"))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss some node");
	}

	if(!root["toboxid"].isConvertibleTo(Json::uintValue) || ! root["roomno"].isString() 
		|| !root["roomname"].isString() || !root["mid"].isConvertibleTo(Json::uintValue) 
		|| !root["name"].isString() || !root["singer"].isString())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "node format error");
	}

	uint32_t mid = root["mid"].asUInt();
	uint32_t toboxid = root["toboxid"].asUInt();
	std::string roomno = root["roomno"].asString();
	std::string roomname = root["roomname"].asString();
	std::string name = root["name"].asString();
	std::string singer = root["singer"].asString();

	if(toboxid == _pac->getDeviceID())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "toboxid not myself");
	}


	bool confirm = false;
	
	if(!Singleton<ManKTVChallenge>::getInstance()->isExist(_pac->getDeviceID()) &&
		! Singleton<ManKTVChallenge>::getInstance()->isExist(toboxid))
	{
		Json::Value root;
		packet::Packet att(packet::KTV_REQ_BOX_GAME_CHALLENGE);
		root["fromboxid"] = _pac->getDeviceID();
		root["roomno"] = roomno.c_str();
		root["roomname"] = roomname.c_str();
		root["mid"] = mid;
		root["name"] = name.c_str();
		root["singer"] = singer.c_str();
		int win = 0;
		int lose = 0;
		int draw = 0;
		int score = 0;
		try{
			_server->getDatabase()->getInfoConnector()->getChallenge(_pac->getDeviceID() ,win , lose , draw , score);
		}catch(const std::exception &err){
			yiqiding::utility::Logger::get("system")->log(yiqiding::utility::toString("sql:") + err.what());
		}


		root["win"] = win;		
		root["lose"] = lose;
		root["draw"] = draw;

		std::string msg = root.toStyledString();
		att.setPayload(msg.c_str() , msg.length());
		try{
		
			_server->getConnectionManager()->sendToBox(toboxid , &att);
			setAttPack(&att , toboxid);
			confirm = true;
		}
		catch(...)
		{
			yiqiding::utility::Logger::get("system")->log(yiqiding::utility::toString(toboxid) + " down " , Logger::WARNING);
			confirm = false;
		}
	}
	else
	{
		yiqiding::utility::Logger::get("system")->log(yiqiding::utility::toString(_pac->getDeviceID()) + " or " + yiqiding::utility::toString(toboxid) + " is exist \n" , Logger::WARNING);
	}

	if(confirm)
		confirm = Singleton<ManKTVChallenge>::getInstance()->addChallenge(_server , mid , _pac->getDeviceID() , toboxid); 

	{
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = toCString(confirm);
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			;
		}
	}
	
	yiqiding::utility::Logger::get("system")->log("fromboxid " + yiqiding::utility::toString(_pac->getDeviceID()) + "add a game");
}


void BoxProcessor::processAnswerGame()
{
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(_pac->getPayload() , root))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("fromboxid") || !root.isMember("join") 
		)
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss some node");
	}

	if(!root["fromboxid"].isConvertibleTo(Json::uintValue) ||  !root["join"].isConvertibleTo(Json::intValue) 
	)
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "node format error");
	}

	uint32_t fromboxid = root["fromboxid"].asUInt();
	int		 join = root["join"].asInt();

	bool confirm  = false;
	//接受
	if(join == 1)
	{
		confirm = Singleton<ManKTVChallenge>::getInstance()->joinChanllenge(fromboxid , _pac->getDeviceID());
	}
	else
	{
		confirm = Singleton<ManKTVChallenge>::getInstance()->rejectChanllenge(fromboxid);
	}
	//att
	if(confirm)
	{	
		std::auto_ptr<packet::Packet> packet(ManKTVChallenge::generatorNotifyJoin(join));	
		try{
			_server->getConnectionManager()->sendToBox(fromboxid , packet.get());
			setAttPack(packet.get() , fromboxid);
		}
		catch(...)
		{
			confirm = false;
		}
	}
	//out
	{
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = toCString(confirm);
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			;
		}
	}
}

void BoxProcessor::processCancelGame()
{
	bool confirm = Singleton<ManKTVChallenge>::getInstance()->cancelChallenge(_pac->getDeviceID());
	{
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = toCString(confirm);
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			;
		}
	}
}


void BoxProcessor::processUploadScore()
{
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(_pac->getPayload() , root))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("normal") || !root.isMember("score") || !root.isMember("yinzhun")
		|| !root.isMember("wending") || !root.isMember("biaoxianli") || !root.isMember("jiezou")
		|| !root.isMember("jiepai") || !root.isMember("jiqiao")
		)
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss some node");
	}

	if(!root["normal"].isConvertibleTo(Json::intValue) 
		||  !root["score"].isConvertibleTo(Json::intValue) 
		||	!root["yinzhun"].isConvertibleTo(Json::intValue)
		||	!root["wending"].isConvertibleTo(Json::intValue)
		||	!root["biaoxianli"].isConvertibleTo(Json::intValue)
		||	!root["jiezou"].isConvertibleTo(Json::intValue)
		||	!root["jiepai"].isObject()
		|| !root["jiepai"].isMember("perfect")
		|| !root["jiepai"]["perfect"].isConvertibleTo(Json::intValue)
		|| !root["jiepai"].isMember("great")
		|| !root["jiepai"]["great"].isConvertibleTo(Json::intValue)
		|| !root["jiepai"].isMember("good")
		|| !root["jiepai"]["good"].isConvertibleTo(Json::intValue)
		|| !root["jiepai"].isMember("ok")
		|| !root["jiepai"]["ok"].isConvertibleTo(Json::intValue)
		|| !root["jiepai"].isMember("miss")
		|| !root["jiepai"]["miss"].isConvertibleTo(Json::intValue)
		||	!root["jiqiao"].isConvertibleTo(Json::intValue)
		)
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "node format error");
	}


	ChallengeScore	score = {
		root["normal"].asUInt(),
		root["score"].asInt(),
		root["yinzhun"].asInt(),
		root["wending"].asInt(),
		root["biaoxianli"].asInt(),
		root["jiezou"].asInt(),
		{
			root["jiepai"]["perfect"].asInt(),
			root["jiepai"]["great"].asInt(),
			root["jiepai"]["good"].asInt(),
			root["jiepai"]["ok"].asInt(),
			root["jiepai"]["miss"].asInt()
		},
		root["jiqiao"].asInt()
	};

	bool confirm = false;

	int boxid = Singleton<ManKTVChallenge>::getInstance()->getPeerBoxid(_pac->getDeviceID());
	if(boxid != -1)
	{
		confirm = Singleton<ManKTVChallenge>::getInstance()->uploadChanllenge(_server ,score , _pac->getDeviceID());
		
		//att

		if(confirm)
		{
			packet::Packet att(packet::KTV_REQ_BOX_GAME_SOCRE);
			att.setPayload(_pac->getPayload() , _pac->getLength());
			try {
				_server->getConnectionManager()->sendToBox(boxid , &att);
				setAttPack(&att , boxid);
			} catch (const std::exception& err) {
				
			}
		}


	}
	{

	
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = toCString(confirm);
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try{
		out.dispatch(_conn);
		setOutPack(&out);
		}catch(...){}
	}
}





/////////////////////////////////
void BoxProcessor::processUpdateBoxInfo()
{

	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}	
	boxIn in;
	if(root.isMember("songpath"))
		in.songpath = root["songpath"];
	if(root.isMember("songname"))
		in.songname = root["songname"];
	if(root.isMember("songsinger"))
		in.songsinger = root["songsinger"];
	if(root.isMember("songmid"))
		in.songmid = root["songmid"];
	if (root.isMember("roomno"))
		in.roomno = root["roomno"];
	if(root.isMember("roomname"))
		in.roomname = root["roomname"];
	if (root.isMember("optionreceive"))
		in.optionreceive = root["optionreceive"];
	if (root.isMember("optiongame"))
		in.optiongame = root["optiongame"];
	if (root.isMember("optionkgame"))
		in.optionkgame = root["optionkgame"];
	if(root.isMember("optiongift"))
		in.optiongift = root["optiongift"];
	if(root.isMember("users"))
		in.users = root["users"];
	if(root.isMember("optionmusic"))
		in.optionmusic = root["optionmusic"];
	if(root.isMember("message"))
		in.message = root["message"];

	Singleton<BoxSocial>::getInstance()->update(
		_pac->getDeviceID() , in);

	{
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["boxid"] = _pac->getDeviceID();
		root["result"] = true;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			
		}
	}

}

void BoxProcessor::processGetBoxList()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}	

	if(!root.isMember("number") || !root["number"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss number");


	if(!root.isMember("page") || !root["page"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss page");

	int total;
	Json::Value list = Singleton<BoxSocial>::getInstance()->getAll(root["number"].asInt() , root["total"].asInt() , total);
	{
		packet::Packet out(_pac->getHeader());
		Json::Value newroot;
		newroot["total"] = total;
		newroot["page"] = root["page"].asInt();
		newroot["status"] = packet::BACK_NO_ERROR;
		newroot["boxlist"] = list;

		std::string msg = newroot.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			
		}
	}

}

void BoxProcessor::processGetHisGame()
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	tinyxml2::XMLElement* cnode;
	tinyxml2::XMLElement* croot;
	tinyxml2::XMLElement* droot;
	tinyxml2::XMLElement* dnode;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("status"));
	node->InsertEndChild(doc.NewText("0"));
	root->InsertEndChild(node = doc.NewElement("ktinyinfo"));


	auto list = _server->getDatabase()->getMediaConnector()->getGameList(0 , 3);
	
	for each(auto bc in *list)
	{
		node->InsertEndChild(croot = doc.NewElement("item"));
		croot->InsertEndChild(cnode = doc.NewElement("kid"));
		cnode->InsertEndChild(doc.NewText(toString(bc->getKId()).c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("notifytitle"));
		cnode->InsertEndChild(doc.NewText(bc->getKTitle().c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("notifymessage"));
		cnode->InsertEndChild(doc.NewText(bc->getKMessage().c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("mids"));
		for each(auto mid in bc->getKMids())
		{
			cnode->InsertEndChild(droot = doc.NewElement("item"));
			droot->InsertEndChild(dnode = doc.NewElement("mid"));
			dnode->InsertEndChild(doc.NewText(toString(mid._mid).c_str()));
			droot->InsertEndChild(dnode = doc.NewElement("name"));
			dnode->InsertEndChild(doc.NewText(mid._name.c_str()));
		}
		croot->InsertEndChild(cnode = doc.NewElement("starttime"));
		cnode->InsertEndChild(doc.NewText(toString(bc->getKStartTime()).c_str()));
		
		root->InsertEndChild(cnode = doc.NewElement("scores"));


		std::set<int> uploadscores;
		for each(auto score in bc->getKScores())
			uploadscores.insert(score._boxid);

		std::auto_ptr<std::map<uint32_t , std::string> > RoomNames = yiqiding::ktv::box::BoxInfoMan::getInstace()->getRoomNames(uploadscores);
		
		
		for each(auto b in bc->getKScores() )
		{
			node->InsertEndChild(croot = doc.NewElement("item"));
			croot->InsertEndChild(cnode = doc.NewElement("boxid"));
			cnode->InsertEndChild(doc.NewText(toString(b._boxid).c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("roomname"));
			cnode->InsertEndChild(doc.NewText((*RoomNames)[b._boxid].c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("score"));
			cnode->InsertEndChild(doc.NewText(toString(b._score).c_str()));
		}
	}
	
	Packet out_pack(_pac->getHeader());
	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);
	// Pack
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);

	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}


void BoxProcessor::processGetOneBox()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("boxid") || !root["boxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss boxid");
	
	Json::Value v = Singleton<BoxSocial>::getInstance()->get(root["boxid"].asInt());
	v["status"] = packet::BACK_NO_ERROR;

	packet::Packet out(_pac->getHeader());
	std::string msg = v.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	try {
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (const std::exception& err) {
		
	}

}

void BoxProcessor::processGame2Start()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}
	
	if(!root.isMember("boxid") || !root["boxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss boxid");
	
	if(!root.isMember("type") || !root["type"].isConvertibleTo(Json::stringValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss type");

	int boxid = root["boxid"].asInt();
	root["boxid"] = _pac->getDeviceID();

	packet::Packet att(packet::KTV_REQ_BOX_GAME2_INV);
	std::string msg = root.toStyledString();
	att.setPayload(msg.c_str() , msg.length());
	try{
		_server->getConnectionManager()->sendToBox(boxid ,
			&att);
		setAttPack(&att , boxid);
		_server->getConnectionManager()->getConnectionBox(boxid)->cache(&att , _pac);
	}catch(...){
	}	
}

void BoxProcessor::processGame2Inv()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("accept") || !root["accept"].isConvertibleTo(Json::booleanValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss accept");

	std::auto_ptr<yiqiding::ktv::extended::ConnectionCache> cache(_server->getConnectionManager()->getConnectionBox(_pac->getDeviceID())->findCache(_pac->getIdentifier()));
	if(cache.get() == NULL)
		return;

	int boxid = cache->getReferencePacket()->getDeviceID();
	//get type
	bool one;
	{
		std::string str(cache->getReferencePacket()->getPayload() , cache->getReferencePacket()->getLength());
		Json::Reader reader;
		Json::Value root;
		if(!reader.parse(str , root) || !root.isObject())
			return;

		if(!root.isMember("type") || !root["type"].isConvertibleTo(Json::stringValue))
			return;

		one = (root["type"].asString() == "one");		
	}
	bool bAccept = root["accept"].asBool();
	if(bAccept)
	{
		Singleton<KTVKGame2>::getInstance()->add(boxid , _pac->getDeviceID() ,one);
	}

	packet::Packet att(cache->getReferencePacket()->getHeader());
	root["status"] = packet::BACK_NO_ERROR;
	std::string msg = root.toStyledString();
	att.setPayload(msg.c_str() , msg.length());
	try {
		_server->getConnectionManager()->sendToBox(boxid , &att);
		setAttPack(&att , boxid);
	} catch (const std::exception& err) {
		
	}
}

void BoxProcessor::processGame2Exit()
{
	packet::Packet att(packet::KTV_REQ_BOX_GAME2_EXIT);
	att.setPayload(_pac->getPayload() , _pac->getLength());
	GameInfo info;
	if(!Singleton<KTVKGame2>::getInstance()->get(_pac->getDeviceID() , info))
		return;
	int boxid;
	if(info.fromboxid == _pac->getDeviceID())
		boxid = info.toboxid;
	else
		boxid = info.fromboxid;

	try {
		Singleton<KTVKGame2>::getInstance()->del(_pac->getDeviceID());
		_server->getConnectionManager()->sendToBox(boxid , &att);
		setAttPack(&att , boxid);
	} catch (const std::exception& err) {
		
	}
}

void BoxProcessor::processGame2Score()
{

	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	Json::Value infojson(Json::objectValue);
	int result_1 = 0;
	int result_2 = 0;
	if(!reader.parse(str , root) || !root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	
	if(!root.isMember("result_1") || !root["result_1"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss reulst_1");


	result_1 = root["result_1"].asInt();

	if(root.isMember("result_2") && root["result_2"].isConvertibleTo(Json::uintValue))
		result_2 = root["reulst_2"].asInt();

	if(root.isMember("info") && root["info"].isConvertibleTo(Json::objectValue))
		infojson = root["info"];

	Singleton<KTVKGame2>::getInstance()->update(_pac->getDeviceID() , result_1 , result_2 , _pac->getIdentifier() , infojson);

	if(!Singleton<KTVKGame2>::getInstance()->check(_pac->getDeviceID()))
		return;

	GameInfo info;
	if(!Singleton<KTVKGame2>::getInstance()->get(_pac->getDeviceID() , info))
		return;

	//for delete 
	Singleton<KTVKGame2>::getInstance()->del(_pac->getDeviceID());


	bool fromwin =  false;
	if(info.from_result_1 + info.from_result_2 > info.to_result_1 + info.to_result_2)
		fromwin = true;
	
	//from
	{
		Json::Value root;
		packet::Packet att(_pac->getHeader());
		att.setIdentifier(info.from_identitfy);
		root["iswinning"] = fromwin;
		root["result_1"] = info.to_result_1;
		root["info"] = info.to_info;
		if(!info.one)
			root["result_2"] = info.to_result_2;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		att.setPayload(msg.c_str() , msg.length());
		try{
			if(info.fromboxid == _pac->getDeviceID())
			{
				att.dispatch(_conn);
				setOutPack(&att);
			}
			else
			{
				_server->getConnectionManager()->sendToBox(info.fromboxid , &att);
				setAttPack(&att , info.fromboxid);
			}
		}catch(...){}
	}

	//to
	{
		Json::Value root;
		packet::Packet att(_pac->getHeader());
		att.setIdentifier(info.to_identity);
		root["iswinning"] = !fromwin;
		root["result_1"] = info.from_result_1;
		if(!info.one)
			root["result_2"] = info.from_result_2;
		root["info"] = info.from_info;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		att.setPayload(msg.c_str() , msg.length());
		try{
			if(info.toboxid == _pac->getDeviceID())
			{
				att.dispatch(_conn);
				setOutPack(&att);
			}
			else
			{
				_server->getConnectionManager()->sendToBox(info.toboxid , &att);
				setAttPack(&att , info.toboxid);
			}
		}catch(...){}
	}
	
}

void BoxProcessor::processKGame2Accept()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(str , root) || !root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::uintValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "id error");
	}

	int id = root["id"].asInt();

	bool confirm = Singleton<yiqiding::ktv::KTVGameTimer2>::getInstance()->accept(id ,_pac->getDeviceID());

	{
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		Json::Reader reader;

		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;

		std::string msg = root.toStyledString();

		out.setPayload(msg.c_str()  , msg.length());

		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (...) {
			
		}
	}
}

void BoxProcessor::processKGame2Socre()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(str , root) || !root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");


	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::uintValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "id error");
	}

	if(!root.isMember("score") || !root["score"].isConvertibleTo(Json::realValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "score error");
	}

	bool confirm = Singleton<yiqiding::ktv::KTVGameTimer2>::getInstance()->scores(root["id"].asInt() , _pac->getDeviceID() , root["score"].asDouble());
	{
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		packet::Packet out(_pac->getHeader());
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try {
			out.dispatch(_conn);
			setOutPack(&out);
		} catch (const std::exception& err) {
			
		}
	}
}

void BoxProcessor::processKGame2Get()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(str , root) || !root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");

	if(!root.isMember("type") || !root["type"].isConvertibleTo(Json::intValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "type error");
	}

	int type = root["type"].asInt();
	{
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		Json::Value game;
		game.resize(0);
		std::auto_ptr< std::vector<model::Game2> >  list;
		
		if(type == 1)
		 list.reset(_server->getDatabase()->getMediaConnector()->getHisGame2().release());
		else
		 list.reset(Singleton<yiqiding::ktv::KTVGameTimer2>::getInstance()->getGames().release());
	
		for each(auto g in *list)
		{
			Json::Value item;
			item["songs"] = StrToJson(g.getSongs());
			item["time"] = yiqiding::utility::getDateTime(g.getTimeofStart()).c_str();
			item["awards"] = StrToJson(g.getAwards());
			item["title"] = g.getTitle().c_str();
			item["desc"]  = g.getDesc().c_str();
			item["id"] = g.getId();
			item["num"] = g.getNum();
			item["scores"] = StrToJson(g.getScores());
			game.append(item);
		}
		root["games"] = game;
		
		packet::Packet out(_pac->getHeader());
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		try{
			out.dispatch(_conn);
			setOutPack(&out);
		}catch(...){}
	}
}

void BoxProcessor::processAddReward()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}
	
	bool confirm = true;
		Singleton<yiqiding::ktv::GameReward>::getInstance()->addReward(_server , _pac->getDeviceID() ,
		 root["roomno"] , root["roomname"] , root["requirement"] ,
		root["serial_id"],root["name"],root["singer"] ,root["gift"] , root["user"]);
	
	packet::Packet out(_pac->getHeader());
	
	try{
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){}

}

void BoxProcessor::processAddRecord()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	int id;
	bool finish;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	if(!root.isMember("finish") || !root["finish"].isConvertibleTo(Json::booleanValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss finish");
	id = root["id"].asUInt();
	finish = root["finish"].asBool();

	bool confirm = Singleton<GameReward>::getInstance()->addRecord(id , _pac->getDeviceID() , root["record"] , finish);

	packet::Packet out(_pac->getHeader());
	try{
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){}

}

void BoxProcessor::processSingReward()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	int id;
	
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	
	id = root["id"].asUInt();


	bool confirm = Singleton<GameReward>::getInstance()->lock(id , _pac->getDeviceID());

	packet::Packet out(_pac->getHeader());
	try{
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){}
}

void BoxProcessor::processCancelReward()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	int id;

	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::uintValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");

	id = root["id"].asUInt();


	bool confirm = Singleton<GameReward>::getInstance()->cancelfromid(id , _pac->getDeviceID());

	packet::Packet out(_pac->getHeader());
	try{
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){}
}

void BoxProcessor::processGetReward()
{
	Json::Value root;

	root["rewards"] = Singleton<GameReward>::getInstance()->getValues();
	packet::Packet out(_pac->getHeader());
	try{
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){}
}

void BoxProcessor::processAddReward2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	int id = 0;
	bool confirm = false;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	//if fail id return 0 .
	id = Singleton<yiqiding::ktv::GameReward2>::getInstance()->add(_server ,_pac->getDeviceID() ,
		root["roomno"] , root["roomname"] , root["serial_id"] , root["name"] , root["singer"] , root["award"]);
	
	if(id)
		confirm = true;

	packet::Packet out(_pac->getHeader());
	try {
		Json::Value root;
		root["confirm"] = confirm;
		root["id"] = id;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}

}

void BoxProcessor::processSendGift(){
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root = yiqiding::utility::StrToJson(str);
	if(!root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	
	if(!root["toboxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "toboxid not be int error");
	int toboxid = root["toboxid"].asInt();
	root.removeMember("toboxid");
	root["fromboxid"] = _pac->getDeviceID();
	
	unsigned int time;
	bool confirm = _server->getDatabase()->getInfoConnector()->getBoxStatus(toboxid , time);
	Json::Value outroot;
	outroot["status"] = packet::BACK_NO_ERROR;
	packet::Packet back(_pac->getHeader());
	//机顶盒关包厢
	if(!confirm)
	{
		outroot["result"] = 2;
		std::string msg = outroot.toStyledString();
		back.setPayload(msg.c_str() , msg.length());
		back.dispatch(_conn);
		setOutPack(&back);
	}else{
		try{
			std::string msg = root.toStyledString();
			packet::Packet att(packet::KTV_REQ_BOX_RECV_GIFT);
			att.setPayload(msg.c_str() , msg.length());
			_server->getConnectionManager()->sendToBox(toboxid , &att);
			setAttPack(&att , toboxid);
			confirm = true; 
			_server->getConnectionManager()->getConnectionBox(toboxid)->cache(&att , _pac);
		}catch(...){
			confirm = false;
		}
		//机顶盒关闭
		if(!confirm){
			outroot["result"] = 1;
			std::string msg = outroot.toStyledString();
			back.setPayload(msg.c_str() , msg.length());
			back.dispatch(_conn);
			setOutPack(&back);
		}
	}




}

void BoxProcessor::processRecvGift(){
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root = yiqiding::utility::StrToJson(str);
	if(!root.isObject())
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");


	if(!root["confirm"].isConvertibleTo(Json::intValue)){
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "toboxid not be int error");
	}

	int confirm = root["confirm"].asInt();
	if(confirm)
	{
		std::auto_ptr<yiqiding::ktv::extended::ConnectionCache > cache(_server->getConnectionManager()->getConnectionBox(_pac->getDeviceID())->findCache(_pac->getIdentifier()));
		if(cache.get() == NULL)
			return;
		
		if(cache->getOutGoingPacket()->getRequest() != _pac->getRequest())
			return;

		packet::Packet att(cache->getReferencePacket()->getHeader());
		int toboxid = cache->getReferencePacket()->getDeviceID();
		
	
		Json::Value root;
		root["result"] = 0;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		att.setPayload(msg.c_str() , msg.length());
		try{
			_server->getConnectionManager()->sendToBox(toboxid , &att);
			setAttPack(&att , toboxid);
		}catch(...){

		}
	}
}

void BoxProcessor::processReadyReward2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	bool confirm = false;
	int id = 0;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}
	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	id = root["id"].asInt();

	confirm = Singleton<yiqiding::ktv::GameReward2>::getInstance()->ready(id , _pac->getDeviceID());

	packet::Packet out(_pac->getHeader());
	try {
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}

}

void BoxProcessor::processAddRecord2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	bool confirm = false;
	int id = 0;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}
	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	id = root["id"].asInt();

	confirm = Singleton<yiqiding::ktv::GameReward2>::getInstance()->sing(id , _pac->getDeviceID() ,
		root["roomno"] , root["roomname"] , root["score"] , root["url"]);

	packet::Packet out(_pac->getHeader());
	try {
		Json::Value root;
		root["confirm"] = confirm;
		root["status"] = packet::BACK_NO_ERROR;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}
}

void BoxProcessor::processGetMyReward2()
{
	packet::Packet out(_pac->getHeader());
	Json::Value root;
	root["status"] = packet::BACK_NO_ERROR;
	root["rewards"] = Singleton<yiqiding::ktv::GameReward2>::getInstance()->getMy(_pac->getDeviceID());
	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	try {
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}
}

void BoxProcessor::processGetAllReward2()
{
	packet::Packet out(_pac->getHeader());
	Json::Value root;
	root["status"] = packet::BACK_NO_ERROR;
	root["rewards"] = Singleton<yiqiding::ktv::GameReward2>::getInstance()->getAll();
	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	try {
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}
}

void BoxProcessor::processCancelReward2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	bool confirm = false;
	int id = 0;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	id = root["id"].asInt();

	confirm = Singleton<yiqiding::ktv::GameReward2>::getInstance()->cancel(id , _pac->getDeviceID());


	try {
		packet::Packet out(_pac->getHeader());
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = confirm;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}

}

void BoxProcessor::processSelectReward2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Value root;
	Json::Reader reader;
	bool confirm = false;
	int id = 0;
	int boxid = 0;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");
	id = root["id"].asInt();

	if(!root.isMember("boxid") || !root["boxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss boxid");
	boxid = root["boxid"].asInt();

	confirm =  Singleton<yiqiding::ktv::GameReward2>::getInstance()->select(id , boxid);
	packet::Packet out(_pac->getHeader());

	try {
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = confirm;
		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (...) {
	}

}



void BoxProcessor::processPCMStart(){
	
	Json::Value root = StrToJson(std::string(_pac->getPayload() , _pac->getLength()));
	if(!root.isObject() || !root.isMember("time") || !root["time"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error json or miss time or time not uint");
	
	uint32_t times = root["time"].asInt();

	uint32_t self = inet_addr(_conn->getAddress().c_str());
	bool confirm = Singleton<KTVPCM>::getInstance()->startPcm(self , _pac->getDeviceID() , times);
	
	Json::Value newroot;
	newroot["status"] = packet::BACK_NO_ERROR;
	newroot["confirm"] = confirm;
	std::string msg = newroot.toStyledString();
	packet::Packet out(_pac->getHeader());
	out.setPayload(msg.c_str() , msg.length());
	try{
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){
		;
	}
}


void BoxProcessor::processPCMStop(){
	uint32_t self = inet_addr(_conn->getAddress().c_str());
	bool confirm = Singleton<KTVPCM>::getInstance()->stopPcm(self , _pac->getDeviceID());

	Json::Value newroot;
	newroot["status"] = packet::BACK_NO_ERROR;
	newroot["confirm"] = confirm;
	std::string msg = newroot.toStyledString();
	packet::Packet out(_pac->getHeader());
	out.setPayload(msg.c_str() , msg.length());
	try{
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){
		;
	}
}

void BoxProcessor::processPCMListen(){

	Json::Value root = StrToJson(std::string(_pac->getPayload() , _pac->getLength()));
	if(!root.isObject() || !root.isMember("boxid") || !root["boxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error json or miss boxid or boxid not uint");

	uint32_t boxid = root["boxid"].asInt();

	packet::Packet att(packet::KTV_REQ_BOX_PCM_LISTEN_ALERT);
	root["boxid"] = _pac->getDeviceID();
	std::string msg = root.toStyledString();
	att.setPayload(msg.c_str() , msg.length());
	bool confirm = false;

	try {
		_server->getConnectionManager()->sendToBox(boxid ,&att);
		_server->getConnectionManager()->getConnectionBox(boxid)->cache(&att , _pac);
		setAttPack(&att , boxid);
		confirm = true;
	} catch (const std::exception& err) {
		confirm = false;
	}


//	uint32_t self = inet_addr(box::BoxInfoMan::getInstace()->getIP(boxid).c_str());
//	uint32_t other = inet_addr(_conn->getAddress().c_str());
//	bool confirm = Singleton<KTVPCM>::getInstance()->listenPcm(boxid , _pac->getDeviceID() ,self ,  other);



	if(confirm == false)
	{
		Json::Value newroot;
		newroot["status"] = packet::BACK_NO_ERROR;
		newroot["result"] = 1;
		newroot["time"]	=	0;
		std::string msg = newroot.toStyledString();
		packet::Packet out(_pac->getHeader());
		out.setPayload(msg.c_str() , msg.length());
		try{
			out.dispatch(_conn);
			setOutPack(&out);
		}catch(...){
			;
		}
	}
}

void BoxProcessor::processPCMRemove(){
	Json::Value root = StrToJson(std::string(_pac->getPayload() , _pac->getLength()));
	if(!root.isObject() || !root.isMember("boxid") || !root["boxid"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error json or miss time or time not uint");

	uint32_t boxid = root["boxid"].asInt();

	uint32_t self = inet_addr(box::BoxInfoMan::getInstace()->getIP(boxid).c_str());
	uint32_t other = inet_addr(_conn->getAddress().c_str());
	bool confirm = Singleton<KTVPCM>::getInstance()->removePcm( root["message"],boxid , _pac->getDeviceID() ,self ,  other );

	Json::Value newroot;
	newroot["status"] = packet::BACK_NO_ERROR;
	newroot["confirm"] = confirm;
	std::string msg = newroot.toStyledString();
	packet::Packet out(_pac->getHeader());
	out.setPayload(msg.c_str() , msg.length());
	try{
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){
		;
	}
}


void BoxProcessor::processPCMConfirmListen()
{
	Json::Value root = StrToJson(std::string(_pac->getPayload() , _pac->getLength()));
	if(!root.isObject() || !root.isMember("confirm") || !root["confirm"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error json or miss confirm");
	
	int confirm = root["confirm"].asInt();

	std::auto_ptr<extended::ConnectionCache> cache(_server->getConnectionManager()->getConnectionBox(_pac->getDeviceID())->findCache(_pac->getIdentifier()));
	if(!cache.get())
		return;

	if(cache->getOutGoingPacket()->getRequest() != packet::KTV_REQ_BOX_PCM_LISTEN_ALERT)
		return;

	int result;
	int identify = cache->getReferencePacket()->getIdentifier();
	int boxid = cache->getReferencePacket()->getDeviceID();
	uint32_t self = inet_addr(_conn->getAddress().c_str());
	uint32_t other = inet_addr(box::BoxInfoMan::getInstace()->getIP(boxid).c_str());
	uint32_t time = 0;

	if(confirm == 1)
	{
		
		if(!Singleton<KTVPCM>::getInstance()->listenPcm(_pac->getDeviceID() , boxid , self , other , time))
			result = 3;
		else
			result = 0;
	}
	else if(confirm == 0){
		result = 2;
	}
	else{
		result = 2 + confirm;
	}

	{
		Json::Value newroot;
		newroot["status"] = packet::BACK_NO_ERROR;
		newroot["result"] = result;
		newroot["time"]	=	time;
		std::string msg = newroot.toStyledString();
		packet::Packet out(cache->getReferencePacket()->getHeader());
		out.setPayload(msg.c_str() , msg.length());
		try{
			_server->getConnectionManager()->sendToBox(boxid , &out);
			setAttPack(&out , boxid);
		}catch(...){
			;
		}
	}


}


void BoxProcessor::processGetAccountsList()
{
	packet::Packet out(_pac->getHeader());
	std::string msg = _server->getAllAppInfo();
	Logger::get("server")->log("app info list" + msg, Logger::NORMAL);
	try {
		//_server->getConnectionManager()->sendDataToBox(_pac->getDeviceID(), msg.c_str(), msg.length());
		out.setPayload(msg.c_str() , msg.length());
		out.dispatch(_conn);
		Logger::get("server")->log("send accounts data to box; data : " + msg, Logger::NORMAL);
	} catch (...) {
		Logger::get("server")->log("send accounts to box failed!" + msg, Logger::WARNING);
	}
}

//////////////////////////////////////////////////////////////////////////
void BoxProcessor::onReceivePacket() {

	

	// Security check
	if (getConnection()->isRegistered()) {
		if (getConnection()->getBoxID() != _pac->getDeviceID())
			return sendErrorMessage("Authentication failure");
	} else {


		MutexGuard guard(getConnection()->getMutex());

		if (getConnection()->isRegistered()) {
			if (getConnection()->getBoxID() != _pac->getDeviceID())
				return sendErrorMessage("Authentication failure");
		}
		else{
			MutextReader mr(box::BoxInfoMan::getMutexWR());
			uint32_t boxid = _pac->getDeviceID();
			
			box::BoxInfoItem *item = box::BoxInfoMan::getInstace()->getItem(_ip);
			if(item == NULL || item->getBoxId() != boxid)
				return sendErrorMessage("Authentication failure");

			getConnection()->setBoxID(boxid);
			_server->getConnectionManager()->updateBox(_pac->getDeviceID(), _conn->getID() , _ip);
		}
	}
#ifdef _DEBUG

	_server->getConnectionManager()->getConnectionBox((uint32_t)getConnection()->getBoxID())->record_push(_rdpac);

#endif

	if (KtvdoLua(_pac->getRequest() , _server ,  this))
		return;

	// Process Packet
	switch (_pac->getRequest()) {
/*	case packet::KTV_REQ_BOX_CONTROL_ROOM:	finishControlRoom();		break;*/
/*	case packet::BOX_REQ_APK_PATH:			processApkPath();			break;*/
	case packet::BOX_REQ_OPEN_ROOM:			processOpenRoom();			break;
	case packet::BOX_REQ_FIRE_INFO:			processFireInfo();			break;
/*	case packet::BOX_REQ_ALLOC_SERVER:		processAllocServer();		break;*/
	case packet::KTV_REQ_BOX_CHANGE_FROM:	processChangeBox();			break;
	case packet::BOX_REQ_SERVICE:			processService();			break;
	case packet::BOX_REQ_REALLOC_SERVER:	processReallocServer();		break;
	case packet::BOX_REQ_UPDATE_SONG_COUNT:	processUpdateSongCount();	break;
	case packet::BOX_REQ_GPS_INFO:			processGetGPS();			break;
	case packet::GENERAL_REQ_KEEPALIVE:		proceessHeart();			break;
	case packet::BOX_REQ_TURN_MESSAGE:		processTurnMsgToApp();		break;
	case packet::BOX_REQ_TURN_MESSAGE_ALL:	processTurnMsgToAllApp();	break;
	case packet::BOX_REQ_TMEP_CODE:			processTempCode();			break;
	case packet::BOX_REQ_MSG_RULE:			processMsgRule();			break;
	case packet::BOX_REQ_AD_URL:			processAdUrl();				break;
	case packet::BOX_REQ_VOLUME_INFO:		processVolume();			break;
	case packet::BOX_REQ_TURN_MSG:			processTurnMsgToAll();		break;
	case packet::BOX_REQ_ACCOUNTS_LIST:		processGetAccountsList();   break;

	case packet::KTV_REQ_BOX_GAME_JOIN:		processGameJoin();			break;
	case packet::BOX_REQ_SCORE_UPLOAD:		processGameUpload();		break;
	case packet::BOX_REQ_KTV_GAME:			processGetKtvGame();		break;
	case packet::BOX_REQ_GAME_STATUS:		processGameReq();			break;

	case packet::BOX_REQ_LOG_INFO:			processLogCore();			break;

	case packet::BOX_REQ_GAME_CHALLENGE:	processAddGame();			break;
	case packet::BOX_REQ_GAME_ANSWER_CHALLENGE: processAnswerGame();	break;
	case packet::BOX_REQ_GAME_CANCEL:		processCancelGame();		break;		
	case packet::BOX_REQ_GAME_UPLOAD:		processUploadScore();		break;
	case packet::BOX_REQ_SUBMIT_SOC_INFO:	processUpdateBoxInfo();		break;
	case packet::BOX_REQ_LIST_SOC_INFO:		processGetBoxList();		break;
	case packet::BOX_REQ_ONE_SOC_INFO:		processGetOneBox();			break;
	case packet:: BOX_REQ_GAME_HIS:			processGetHisGame();		break;
	
	case packet::BOX_REQ_GAME2_START:		processGame2Start();		break;
	case packet::BOX_REQ_GAME2_SCORE:		processGame2Score();		break;
	case packet::BOX_REQ_GAME2_EXIT:		processGame2Exit();			break;
	case packet::KTV_REQ_BOX_GAME2_INV:		processGame2Inv();			break;

	case packet::BOX_REQ_ACCEPT_KGAME2:		processKGame2Accept();		break;
	case packet::BOX_REQ_UPLOAD_KGAME2:		processKGame2Socre();		break;
	case packet::BOX_REQ_SCORE_KGAME2:		processKGame2Get();			break;

	case packet::BOX_REQ_ADD_REWARD:		processAddReward();			break;
	case packet::BOX_REQ_ADD_RECORD:		processAddRecord();			break;
	case packet::BOX_REQ_REQ_SING:			processSingReward();		break;
	case packet::BOX_REQ_CANCEL_REWARD:		processCancelReward();		break;
	case packet::BOX_REQ_REWARD_LIST:		processGetReward();			break;	

	case packet::BOX_REQ_ADD_REWARD2:		processAddReward2();		break;	
	case packet::BOX_REQ_GET_MY_REWARD2:	processGetMyReward2();		break;
	case packet::BOX_REQ_PLAY_REWARD2:		processAddRecord2();		break;
	case packet::BOX_REQ_GET_LIST_REWARD2:	processGetAllReward2();		break;
	case packet::BOX_REQ_SELECT_REWARD2:	processSelectReward2();		break;
	case packet::BOX_REQ_CANCEL_REWARD2:	processCancelReward2();		break;
	case packet::BOX_REQ_READY_PLAY_REWARD2:processReadyReward2();		break;

	//gitf
	case packet::BOX_REQ_SEND_GIFT:			processSendGift();			break;
	case packet::KTV_REQ_BOX_RECV_GIFT:		processRecvGift();			break;

	//pcm
	case packet::BOX_REQ_PCM_START:			processPCMStart();			break;
	case packet::BOX_REQ_PCM_STOP:			processPCMStop();			break;
	case packet::BOX_REQ_PCM_LISTEN:		processPCMListen();			break;
	case packet::BOX_REQ_PCM_REMOVE:		processPCMRemove();			break;
	case packet::KTV_REQ_BOX_PCM_LISTEN_ALERT:processPCMConfirmListen();break;

	

	case packet::BOX_REQ_ADDRESS_INFO:		processGetAddress();		break;
	default:	sendErrorMessage("Request not supported");				break;
	}
}
