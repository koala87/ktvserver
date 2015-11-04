/**
 * ERP Processor Implementation
 * @author Shiwei Zhang
 * @date 2014.02.17
 */

#include <memory>
#include <cctype>
#include <string>
#include <tinyxml2.h>
#include "yiqiding/ktv/ERPProcessor.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/Exception.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Romaji.h"
#include "yiqiding/utility/Pinyin.h"
#include "yiqiding/crypto/MD5.h"
#include "yiqiding/ktv/GameTimer.h"
#include "yiqiding/ktv/Dongle.h"
#include "yiqiding/ThreadW1Rn.h"
#include "yiqiding/ktv/BoxInfoMan.h"
#include "yiqiding/ktv/KTVLua.h"
#include "yiqiding/ktv/MessageRule.h"
#include "json/json.h"
#include "yiqiding/ktv/FireWarn.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/utility/PinyinAll.h"
#include "yiqiding/ktv/AdUrl.h"
#include "yiqiding/ktv/Volume.h"
#include "yiqiding/ktv/db/GameTimer2.h"
#include "yiqiding/ktv/GameReward.h"
#include "yiqiding/ktv/GameReward2.h"
#include "yiqiding/ktv/MultiScreen.h"
#include "yiqiding/ktv/KTVCloud.h"

using namespace yiqiding::ktv;
using namespace yiqiding::utility;
using namespace yiqiding::container;
using yiqiding::ktv::ERPProcessor;
using yiqiding::utility::Logger;
using yiqiding::Exception;
using namespace yiqiding::ktv::game;

#pragma region HELPER FUNCTION
//////////////////////////////////////////////////////////////////////////
/// HELPER FUNCTION
static inline const char* getText(tinyxml2::XMLElement* node) {
	const char* p = node->GetText();
	return p?p:"";
}
#pragma endregion

ERPProcessor::ERPProcessor(Server* server, ERPConnection* conn, Packet* pac , const std::string &ip , int port): packet::Processor(server, conn, pac , ip , port) 
{
	
}

#pragma region ERP_ROLE_RECEPTION
//////////////////////////////////////////////////////////////////////////
// ERP_ROLE_RECEPTION
//////////////////////////////////////////////////////////////////////////





void ERPProcessor::finishServiceCall() {
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
			throw Exception("ERP Internal", "Invalid XML document", __FILE__, __LINE__);

		// Obtain values
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			throw Exception("ERP Internal", "Missing root", __FILE__, __LINE__);
		tinyxml2::XMLElement* node = root->FirstChildElement("status");
		if (node == NULL)
			throw Exception("ERP Internal", "Missing status", __FILE__, __LINE__);
		else if (toBool(getText(node))) {
			node = root->FirstChildElement("error");
			if (node == NULL)
				throw Exception("ERP Internal", "Missing error", __FILE__, __LINE__);
			else
				throw Exception("ERP", getText(node), __FILE__, __LINE__);
		}
		node = root->FirstChildElement("confirm");
		if (node == NULL)
			throw Exception("ERP Internal", "Missing confirm", __FILE__, __LINE__);
		else
			confirm = toBool(getText(node));
	} catch (const Exception& err) {
		if (err.getObjectName() == "ERP")
			err_msg.reset(new std::string(err.getReason()));
		else if (err.getObjectName() == "ERP Internal")
			err_msg.reset(new std::string("Internal ERP system error"));
		else
			throw err;
	}

	// Find the corresponding sending packet
	std::auto_ptr<extended::ConnectionCache> cache(_server->getConnectionManager()->getConnectionERP(ERP_ROLE_MAIN)->findCache(_pac->getIdentifier()));
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

	// Send result to Box
	try {
		_server->getConnectionManager()->sendToBox(ref_pack->getDeviceID(), &out_pack);
		setAttPack(&out_pack , ref_pack->getDeviceID());
	} catch (const extended::ConnectionLost&) {
		Logger::get("server")->log("ERP: Fail to send to box id " + toString(ref_pack->getDeviceID()), Logger::WARNING);
	}
}

//////////////////////////////////////////////////////////////////////////

void ERPProcessor::processControlRoom() {
	uint32_t box_id;
	bool room;
	bool confirm = false;
	bool status ;
	bool keep = false;

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
		if ((node = root->FirstChildElement("room")) == NULL)
			return sendErrorMessage("Missing room");
		else
			room = toBool(getText(node));
		node = root->FirstChildElement("keep");
		if(node != NULL)
			keep = toBool(getText(node));
	}

	// Process
	Packet out_pack(packet::KTV_REQ_BOX_CONTROL_ROOM);
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("room_status"));
		node->InsertEndChild(doc.NewText(toCString(room)));
		root->InsertEndChild(node = doc.NewElement("keep"));
		node->InsertEndChild(doc.NewText(toCString(keep)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	ConnectionManager* conn_man = _server->getConnectionManager();


	// Register status in database
	try {
			
		 status = _server->getDatabase()->getInfoConnector()->getBoxStatus(box_id);
		_server->getDatabase()->getInfoConnector()->updateBoxStatus(box_id, room);
		confirm = true;
	}
	catch(...){
		confirm = false;
	}

	//when close suc, clear songs
	if(confirm && !room )
		Singleton<GameReward>::getInstance()->cancelfromboxid(box_id);
	// ERP close Box, may send multiple times
	if(confirm && status && !room){
		Singleton<GameReward2>::getInstance()->updateStatus(box_id);
		//Logger::get("server")->log("[reward debug] close box" , Logger::WARNING);
	}
	_server->getConnectionManager()->setBoxSongs(box_id , "");

	try{
		Packet back_pack(_pac->getHeader());
		{
			// Generate XML
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

			// Pack
			back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		}
		back_pack.dispatch(_conn);
		setOutPack(&back_pack);
	}
	catch(...){}
	try{
		//open`
		if(room && !status)
		{
			char code[20];
			sprintf_s(code , sizeof(code) ,"%02d%s" , box_id , utility::generateTempCode().c_str());
			//yiqiding::crypto::MD5 md5(code);
			_server->getConnectionManager()->setBoxCode(box_id ,/*md5.hex()*/code);
		}
		// Send to Box
		conn_man->sendToBox(box_id, &out_pack);
		setAttPack(&out_pack , box_id);

	

		
	}
	catch(const extended::BoxConnectionLost &){
		Logger::get("system")->log("boxid : " + yiqiding::utility::toString(box_id) + " been down" , Logger::WARNING);
	}

	try {
		// Send To Other Box
		Json::Value root;
		packet::Packet other(packet::KTV_REQ_BOX_OTHER_STATUS);
		{
			root["room_status"] = confirm;
			root["keep"]	=	keep;

			const std::string &ip = box::BoxInfoMan::getInstace()->getIP(box_id);
			int boxids[MAX_SLAVE] = {};
			Singleton<MultiScreen>::getInstance()->getSlave(ip , boxids);
			std::string msg = root.toStyledString();
			other.setPayload(msg.c_str() , msg.length());
			for each(auto boxid in boxids)
			{
				if(boxid == 0)
					break;
				_server->getConnectionManager()->sendToApp(boxid , &other);
			}
		}
	} catch (const std::exception& err) {
		
	}
	
	
}

//////////////////////////////////////////////////////////////////////////
void ERPProcessor::processSingleMessage(){
	std::string words;
	std::string boxid;
	// Parse XML
	{
		tinyxml2::XMLDocument doc;
		{
			std::string xml(_pac->getPayload() , _pac->getLength());
			doc.Parse(xml.c_str());
		}
		if (doc.ErrorID())
			return sendErrorMessage("Invalid XML document");

		//obtain values
		tinyxml2::XMLElement *root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Miss root");
		tinyxml2::XMLElement * node;
		if (node = root->FirstChildElement("words"))
			words = getText(node);
		else 
			return sendErrorMessage("Miss words");
		node = root->FirstChildElement("boxid");
		if (node == NULL)
			return sendErrorMessage("Miss boxid");
		 boxid = getText(node);
		 if (!utility::isLong(boxid))
			return sendErrorMessage("boxid is a not int");

		 int box_id = utility::toInt(boxid);

		 bool confirm = false;
			
		 try {
				Packet out_pack(packet::KTV_REQ_BOX_SHOW_MESSAGE);
			 
				// Generate XML
				tinyxml2::XMLDocument doc;
				tinyxml2::XMLElement* root;
				tinyxml2::XMLElement* node;
				std::string text;
				doc.InsertEndChild(root = doc.NewElement("root"));
				root->InsertEndChild(node = doc.NewElement("words"));
				node->InsertEndChild(doc.NewText(words.c_str()));

				tinyxml2::XMLPrinter printer(0, true);
				doc.Print(&printer);

				// Pack
				out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
			

					_server->getConnectionManager()->sendToBox(box_id , &out_pack);
					confirm = true;
					setAttPack(&out_pack , box_id);
		 }
		 catch(...)
		 {
			 confirm =false;
		 }

		 Packet out_pack(_pac->getHeader());
		 {
			 // Generate XML
			 tinyxml2::XMLDocument doc;
			 tinyxml2::XMLElement* root;
			 tinyxml2::XMLElement* node;
			 doc.InsertEndChild(root = doc.NewElement("root"));
			 root->InsertEndChild(node = doc.NewElement("status"));
			 node->InsertEndChild(doc.NewText("0"));
			 root->InsertEndChild(node = doc.NewElement("confirm"));
			 node->InsertEndChild(doc.NewText(toCString(confirm)));

			 tinyxml2::XMLPrinter printer(0, true);
			 doc.Print(&printer);

			 // Pack
			 out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		 }

		 // Sending
		 out_pack.dispatch(_conn);
		 setOutPack(&out_pack);
			

	}
}


//////////////////////////////////////////////////////////////////////////

void ERPProcessor::processBroadcastMessage() {
	std::string words;

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
		if (node = root->FirstChildElement("words"))
			words = getText(node);
		else return sendErrorMessage("Missing words");
	}

	// Process
	bool confirm = false;
	try {
		Packet out_pack(packet::KTV_REQ_BOX_SHOW_MESSAGE);
		{
			// Generate XML
			tinyxml2::XMLDocument doc;
			tinyxml2::XMLElement* root;
			tinyxml2::XMLElement* node;
			std::string text;
			doc.InsertEndChild(root = doc.NewElement("root"));
			root->InsertEndChild(node = doc.NewElement("words"));
			node->InsertEndChild(doc.NewText(words.c_str()));

			tinyxml2::XMLPrinter printer(0, true);
			doc.Print(&printer);

			// Pack
			out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		}

		auto box_connections = _server->getConnectionManager()->getConnectionBox();
		setAttPack(&out_pack , -1);
		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			if (conn) {
				try {
					out_pack.dispatch(conn);
					confirm = true;
				} catch (...) {}
				conn->release();
			}
		}

	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}


void ERPProcessor::processUploadBoxInfo()
{
	
	/**/
	
	bool confirm;
	tinyxml2::XMLDocument doc;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");
	

	confirm = yiqiding::ktv::box::BoxInfoMan::getInstace()->save("box.xml" , _pac->getPayload() , _pac->getLength());

	//notify to all box to refresh info
	if(confirm)
	{
		MutextReader mr(box::BoxInfoMan::getMutexWR());
		std::string &shopname = box::BoxInfoMan::getInstace()->getShopName();
		auto box_connections = _server->getConnectionManager()->getConnectionBox();
		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			uint32_t box_id = -1;
			if (conn) {
				try {
						Packet pack(packet::KTV_REQ_BOX_SYN_TO_BOX);
						Json::Value root;
						yiqiding::ktv::box::BoxInfoItem *item = yiqiding::ktv::box::BoxInfoMan::getInstace()->getItem(box_conn.second->getIP());
						if(item)
						{
							root["valid"] = 1;
							root["boxid"] = item->getBoxId();
							root["roomname"] = item->getRoomName().c_str();
							root["roomno"] = item->getRoomNo().c_str();
							root["shopname"] = shopname.c_str();
							root["type"]	=	item->getType().c_str();
							box_id = item->getBoxId();
						}
						else
						{
							root["valid"] = 0;
						}

						std::string msg = root.toStyledString();
						pack.setPayload(msg.c_str() , msg.length());
						pack.dispatch(conn);
						setOutPack(&pack);
				} catch (...) {}
				conn->release();
				
				//change box_id , must shutdown.
				if( box_conn.first != box_id)
				{
					conn->shutdown();
				}
			}
			

		}
		Packet pack(packet::KTV_REQ_BOX_SYN_TO_BOX);
		setAttPack(&pack , -1);	
	}

	packet::Packet out_pack(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	out_pack.dispatch(_conn);
	setOutPack(&out_pack);

}

void ERPProcessor::processChangeBox()
{
	uint32_t fromboxid;
	uint32_t toboxid;
	tinyxml2::XMLDocument doc;
	bool confirm = false;
	bool sqlconfirm = false;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if (root == NULL)
		return sendErrorMessage("Missing root");
	tinyxml2::XMLElement* node;
	node = root->FirstChildElement("fromboxid");
	if(node == NULL || !yiqiding::utility::isULong(node->GetText()))
		return sendErrorMessage("Miss fromboxid or fromboxid is not uint");
	fromboxid = yiqiding::utility::toUInt(node->GetText());
	node = root->FirstChildElement("toboxid");
	if (node == NULL || !yiqiding::utility::isULong(node->GetText()))
		return sendErrorMessage("Miss toboxid or toboxid is not uint");
	toboxid = yiqiding::utility::toUInt(node->GetText());

	
	packet::Packet pack(packet::KTV_REQ_BOX_CHANGE_FROM);
	 
	try{	
		_server->getDatabase()->getInfoConnector()->updateBoxStatus(fromboxid , false);
		bool old = _server->getDatabase()->getInfoConnector()->getBoxStatus(toboxid);
		_server->getDatabase()->getInfoConnector()->updateBoxStatus(toboxid , true);
		if(old == false)
		{
			char code[20];
			sprintf_s(code , sizeof(code) ,"%02d%s" , toboxid , utility::generateTempCode().c_str());
			//yiqiding::crypto::MD5 md5(code);
			_server->getConnectionManager()->setBoxCode(toboxid ,/*md5.hex()*/code);
		}
		sqlconfirm = true;
	}
	catch(const std::exception &e)
	{
		Logger::get("system")->log(toString(e.what()) + " Mysql Error:");
	}

	if(sqlconfirm)
	{
		try
		{
			_server->getConnectionManager()->getConnectionBox(fromboxid)->cache(&pack , _pac);
			_server->getConnectionManager()->sendToBox(fromboxid , &pack);
			setAttPack(&pack , fromboxid);
			confirm = true;
		}
		catch(const yiqiding::ktv::extended::BoxConnectionLost &e)
		{
			Logger::get("system")->log(toString(e.what()) + " change box , fromboxid:" + toString(fromboxid) + "been down");
		}
		catch(...){}

		//directly send to toboxid , blank
		if(!confirm)
		{
			Packet pack(packet::KTV_REQ_BOX_CHANGE_TO);
			std::string songs = "{\"songs\":[]}";
			pack.setPayload(songs.c_str() , songs.length());

			try {

				_server->getConnectionManager()->sendToBox(toboxid , &pack);
				setAttPack(&pack , toboxid);

			} catch (const yiqiding::ktv::extended::BoxConnectionLost& ) {
				//_server->getConnectionManager()->setBoxSongs(toboxid , songs);
				Logger::get("system")->log("toboxid " + toString(toboxid) + "been down , blank");
			}
		}
	}


	packet::Packet out_pack(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(sqlconfirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);

}




#pragma endregion

#pragma region ERP_ROLE_MEDIA


void ERPProcessor::processResource()
{

	const Balancer::Node * balancer_node = _server->getBalancer()->random();

	packet::Packet out_pack(_pac->getHeader());
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("server"));
		node->InsertEndChild(doc.NewText(balancer_node->getHostname().c_str()));

		root->InsertEndChild(node = doc.NewElement("id"));
		node->InsertEndChild(doc.NewText(toString(balancer_node->getID()).c_str()));
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);

}


//////////////////////////////////////////////////////////////////////////
// ERP_ROLE_MEDIA
//////////////////////////////////////////////////////////////////////////
#pragma region processControlBlacklist
void ERPProcessor::processControlBlacklist() {
	bool black;

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
		tinyxml2::XMLElement* node = root->FirstChildElement("black");
		if (node == NULL)
			return sendErrorMessage("Missing black");
		else
			black = toBool(getText(node));
	}

	// Process
	bool confirm = true;
	try {
		_server->getDatabase()->getConfigConnector()->updateFilterBlack(black);
	} catch (const std::exception&) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processAddMusic
void ERPProcessor::processAddMusic() {
	model::Media music;
	std::string language;
	std::string type;

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
		if (node = root->FirstChildElement("name"))
			music.setName(getText(node));
		else return sendErrorMessage("Missing name");
		if ((node = root->FirstChildElement("sids")) && node->GetText()) {
			auto sids = split(getText(node), ',');
			for (int i = 0; i < 2 && i < sids.size(); i++) {
				switch (i) {
				case 0:
					music.setArtistSid1(toInt(trim(sids[i])));
					break;
				case 1:
					music.setArtistSid2(toInt(trim(sids[i])));
					break;
				}
			}
		}
		if (node = root->FirstChildElement("language"))
			language = getText(node);
		else return sendErrorMessage("Missing language");
		if ((node = root->FirstChildElement("lyric")) && node->GetText())
			music.setLyric(getText(node));
		if (node = root->FirstChildElement("path"))
			music.setPath(getText(node));
		else return sendErrorMessage("Missing path");
		if (node = root->FirstChildElement("originaltrack"))
			music.setOriginalTrack(toInt(getText(node)));
		else return sendErrorMessage("Missing originaltrack");
		if (node = root->FirstChildElement("soundtrack"))
			music.setSoundTrack(toInt(getText(node)));
		else return sendErrorMessage("Missing soundtrack");
		if (node = root->FirstChildElement("type"))
			type = getText(node);
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("stars"))
			music.setStars(toFloat(getText(node)));
		else return sendErrorMessage("Missing stars");
		if (node = root->FirstChildElement("black"))
			music.setBlack(toBool(getText(node)));
		else return sendErrorMessage("Missing black");
		if (node = root->FirstChildElement("count"))
			music.setCount(toInt(getText(node)));
		else return sendErrorMessage("Missing count");
	}

	// Process
	bool confirm = true;
	try {
		auto mc = _server->getDatabase()->getMediaConnector();
		music.setLanguage(mc->getMediaLanguageId(language));
		music.setType(mc->getMediaTypeId(type));
		std::string singer;
		if (!music.isArtistSid1Null()) {
			auto actor = mc->getActor(music.getArtistSid1());
			singer = actor->getName();
		}
		if (!music.isArtistSid2Null()) {
			auto actor = mc->getActor(music.getArtistSid2());
			if (!singer.empty())
				singer += ' ';
			singer += actor->getName();
		}
		music.setSinger(singer);
		if (music.getLanguage() == mc->getMediaLanguageId("japan"))
			music.setPinyin(PinyinAll::getInstance()->toPinyin( toWideChar(music.getName()) , PinyinAll::JAP ));
		else if(music.getLanguage() == mc->getMediaLanguageId("korean"))
			music.setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(music.getName()) , PinyinAll::KOR ));
		else
			music.setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(music.getName()) , PinyinAll::CHN ));
		music.setHeader(getPinyinHead(music.getPinyin()));
		music.setPinyin(removeSplit(music.getPinyin()));


		music.setWords((int)music.getHeader().size());
		if (!music.getHeader().empty())
			music.setHead(music.getHeader().front());
		mc->add(music);
#ifdef _DEBUG
		} catch (const std::exception& err) {
			Logger::get("server")->log(err.what(), Logger::WARNING);
#else
		} catch (...) {
#endif // _DEBUG
			confirm = false;
		}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);

	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processAddSinger
void ERPProcessor::processAddSinger() {
	model::Actor actor;
	std::string type;

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
		if (node = root->FirstChildElement("name"))
			actor.setName(getText(node));
		else return sendErrorMessage("Missing name");
		if (node = root->FirstChildElement("stars"))
			actor.setStars(toFloat(getText(node)));
		if (node = root->FirstChildElement("type"))
			type = getText(node);
		else return sendErrorMessage("Missing type");
#ifndef REMOVE_SQL_IMAGE
		if (node = root->FirstChildElement("nation"))
			actor.setNation(getText(node));
		else return sendErrorMessage("Missing nation");
		if (node = root->FirstChildElement("image"))
			actor.setImage(getText(node));
#endif
		if (node = root->FirstChildElement("count"))
			actor.setCount(toInt(getText(node)));
		else return sendErrorMessage("Missing count");
	}

	// Process
	bool confirm = true;
	try {
		auto mc = _server->getDatabase()->getMediaConnector();
		actor.setType(mc->getActorTypeId(type));
		actor.setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(actor.getName())));
		actor.setHeader(getPinyinHead(actor.getPinyin()));
		actor.setPinyin(removeSplit(actor.getPinyin()));
		if (!actor.getHeader().empty())
			actor.setHead(actor.getHeader().front());
		mc->add(actor);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processAddActivity
void ERPProcessor::processAddActivity() {
	model::Activity activity;

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
		if (node = root->FirstChildElement("title"))
			activity.setTitle(getText(node));
		else return sendErrorMessage("Missing title");
		if (node = root->FirstChildElement("thumb"))
			activity.setThumb(getText(node));
		else return sendErrorMessage("Missing thumb");
		if (node = root->FirstChildElement("type"))
			activity.setType(getText(node));
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("memberCount"))
			activity.setMemberCount(toInt(getText(node)));
		else return sendErrorMessage("Missing memberCount");
		if (node = root->FirstChildElement("startdate"))
			activity.setStartDate(getText(node));
		else return sendErrorMessage("Missing startdate");
		if (node = root->FirstChildElement("enddate"))
			activity.setEndDate(getText(node));
		else return sendErrorMessage("Missing enddate");
		if (node = root->FirstChildElement("starttime"))
			activity.setStartTime(getText(node));
		else return sendErrorMessage("Missing starttime");
		if (node = root->FirstChildElement("endtime"))
			activity.setEndTime(getText(node));
		else return sendErrorMessage("Missing endtime");
		if (node = root->FirstChildElement("status"))
			activity.setStatus(getText(node));
		else return sendErrorMessage("Missing status");
		if (node = root->FirstChildElement("address"))
			activity.setAddress(getText(node));
		else return sendErrorMessage("Missing address");
		if (node = root->FirstChildElement("fee"))
			activity.setFee(toDouble(getText(node)));
		else return sendErrorMessage("Missing fee");
		if (node = root->FirstChildElement("sponsor"))
			activity.setSponsor(getText(node));
		if (node = root->FirstChildElement("photos"))
			activity.setPhotos(getText(node));
		if (node = root->FirstChildElement("description"))
			activity.setDescription(getText(node));
	}

	// Process
	bool confirm = true;
	try {
		_server->getDatabase()->getMediaConnector()->add(activity);
	} catch (const std::exception &e) {
		yiqiding::utility::Logger::get("system")->log(e.what() , Logger::WARNING);
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processAddPlayList
void ERPProcessor::processAddPlayList() {
	model::SongList list;

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
		if (node = root->FirstChildElement("title"))
			list.setTitle(getText(node));
		else return sendErrorMessage("Missing title");
		if (node = root->FirstChildElement("image"))
			list.setImage(getText(node));
		else return sendErrorMessage("Missing image");
		if (node = root->FirstChildElement("type"))
			list.setType(getText(node));
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("special"))
			list.setSpecial(toBool(getText(node)));
		else return sendErrorMessage("Missing special");
		if (node = root->FirstChildElement("songs")) {
			auto songstrs = split(getText(node),',');
			int count = 0;
			std::vector<unsigned int> songs;
			for each (auto songstr in songstrs) {
				std::string song = trim(songstr);
				if (!song.empty()) {
					songs.push_back(toUInt(song));
					++count;
				}
			}
			list.setSongs(songs);
			list.setCount(count);
		}
	}

	// Process
	bool confirm = true;
	try {
		_server->getDatabase()->getMediaConnector()->add(list);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processGetMusicList
void ERPProcessor::processGetMusicList() {
	std::string raw_type;
	std::string raw_language;
	std::string raw_board;
	size_t number;
	size_t page;

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
		if (node = root->FirstChildElement("type"))
			raw_type = getText(node);
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("language"))
			raw_language = getText(node);
		else return sendErrorMessage("Missing language");
		if (node = root->FirstChildElement("board"))
			raw_board = getText(node);
		else return sendErrorMessage("Missing board");
		if (node = root->FirstChildElement("number")) {
			number = toUInt(getText(node));
			if (number == 0)
				return sendErrorMessage("Invalid number: number is zero");
		} else return sendErrorMessage("Missing number");
		if (node = root->FirstChildElement("page"))
			page = toUInt(getText(node));
		else return sendErrorMessage("Missing page");
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	auto type = mc->getMediaTypeId(raw_type);
	auto language = mc->getMediaLanguageId(raw_language);
	auto media_type = mc->getMediaType();
	auto media_language = mc->getMediaLanguage();
	std::auto_ptr<container::SmartVector<model::Media> > list;
	size_t total;
	if (raw_board == "All") {
		list = mc->getMedia(type, language, page * number, number);
		total = mc->countMedia(type, language);
	} else {
		list = mc->getMediaListMedia(raw_board, type, language, page * number, number);
		total = mc->countMedia(raw_board, type, language);
	}
	total = ((total == 0)? 0 : (total - 1) / number + 1);

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* songs;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("page"));
		node->InsertEndChild(doc.NewText(((value = toString(page)), value.c_str())));
		root->InsertEndChild(node = doc.NewElement("total"));
		node->InsertEndChild(doc.NewText(((value = toString(total)), value.c_str())));
		root->InsertEndChild(songs = doc.NewElement("songs"));
		for each (auto media in *list) {
			songs->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("mid"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getMid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("name"));
			node->InsertEndChild(doc.NewText(media->getName().c_str()));
			root->InsertEndChild(node = doc.NewElement("singer"));
			node->InsertEndChild(doc.NewText(media->getSinger().c_str()));
#ifndef	REMOVE_SQL_IMAGE
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(media->isImageNull()?"":media->getImage().c_str()));
#endif
			root->InsertEndChild(node = doc.NewElement("language"));
			node->InsertEndChild(doc.NewText(media_language[media->getLanguage()].c_str()));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(media_type[media->getType()].c_str()));
			root->InsertEndChild(node = doc.NewElement("stars"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getStars())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("path"));
			node->InsertEndChild(doc.NewText(media->getPath().c_str()));
			root->InsertEndChild(node = doc.NewElement("sids"));
			{
				value.clear();
				if (!media->isArtistSid1Null())
					value = toString(media->getArtistSid1());
				if (!media->isArtistSid2Null()) {
					if (!value.empty())
						value.push_back(',');
					value.append(toString(media->getArtistSid2()));
				}
			}
			node->InsertEndChild(doc.NewText(value.c_str()));
			root->InsertEndChild(node = doc.NewElement("pinyin"));
			node->InsertEndChild(doc.NewText(media->getPinyin().c_str()));
			root->InsertEndChild(node = doc.NewElement("lyric"));
			node->InsertEndChild(doc.NewText(media->getLyric().c_str()));
			root->InsertEndChild(node = doc.NewElement("header"));
			node->InsertEndChild(doc.NewText(media->getHeader().c_str()));
			root->InsertEndChild(node = doc.NewElement("originaltrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getOriginalTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("soundtrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getSoundTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("words"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getWords())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("black"));
			node->InsertEndChild(doc.NewText(toCString(media->getBlack())));
			root->InsertEndChild(node = doc.NewElement("hot"));
			node->InsertEndChild(doc.NewText(toCString(media->getHot())));
			root->InsertEndChild(node = doc.NewElement("head"));
			node->InsertEndChild(doc.NewText(media->isHeadNull()?"":((value = media->getHead()), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("enabled"));
			node->InsertEndChild(doc.NewText(""));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processGetSingerList
void ERPProcessor::processGetSingerList() {
	std::string raw_type;
	size_t number;
	size_t page;

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
		if (node = root->FirstChildElement("type"))
			raw_type = getText(node);
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("number")) {
			number = toUInt(getText(node));
			if (number == 0)
				return sendErrorMessage("Invalid number: number is zero");
		} else return sendErrorMessage("Missing number");
		if (node = root->FirstChildElement("page"))
			page = toUInt(getText(node));
		else return sendErrorMessage("Missing page");
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	auto type = mc->getActorTypeId(raw_type);
	auto actor_type = mc->getActorType();
	auto list = mc->getActor(type, page * number, number);
	size_t total = mc->countActor(type);
	total = ((total == 0)? 0 : (total - 1) / number + 1);

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* artists;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("page"));
		node->InsertEndChild(doc.NewText(((value = toString(page)), value.c_str())));
		root->InsertEndChild(node = doc.NewElement("total"));
		node->InsertEndChild(doc.NewText(((value = toString(total)), value.c_str())));
		root->InsertEndChild(artists = doc.NewElement("artists"));
		for each (auto actor in *list) {
			artists->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("sid"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getSid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("name"));
			node->InsertEndChild(doc.NewText(actor->getName().c_str()));
			root->InsertEndChild(node = doc.NewElement("stars"));
			node->InsertEndChild(doc.NewText(actor->isStarsNull()?"":((value = toString(actor->getStars()), value.c_str()))));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(actor_type[actor->getType()].c_str()));
#ifndef	REMOVE_SQL_IMAGE			
			root->InsertEndChild(node = doc.NewElement("nation"));
			node->InsertEndChild(doc.NewText(actor->getNation().c_str()));
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(actor->isImageNull()?"":actor->getImage().c_str()));
#endif		
			root->InsertEndChild(node = doc.NewElement("pinyin"));
			node->InsertEndChild(doc.NewText(actor->getPinyin().c_str()));
			root->InsertEndChild(node = doc.NewElement("songcount"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getSongCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("header"));
			node->InsertEndChild(doc.NewText(actor->getHeader().c_str()));
			root->InsertEndChild(node = doc.NewElement("head"));
			node->InsertEndChild(doc.NewText(actor->isHeadNull()?"":((value = actor->getHead()), value.c_str())));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processGetActivityList
void ERPProcessor::processGetActivityList() {
	// Process
	auto list = _server->getDatabase()->getMediaConnector()->getActivity();

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* activities;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(activities = doc.NewElement("activities"));
		for each (auto activity in *list) {
			activities->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("said"));
			node->InsertEndChild(doc.NewText(((value = toString(activity->getAid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("title"));
			node->InsertEndChild(doc.NewText(activity->getTitle().c_str()));
			root->InsertEndChild(node = doc.NewElement("thumb"));
			node->InsertEndChild(doc.NewText(activity->getThumb().c_str()));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(activity->getType().c_str()));
			root->InsertEndChild(node = doc.NewElement("memberCount"));
			node->InsertEndChild(doc.NewText(((value = toString(activity->getMemberCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("startdate"));
			node->InsertEndChild(doc.NewText(activity->getStartDate().c_str()));
			root->InsertEndChild(node = doc.NewElement("enddate"));
			node->InsertEndChild(doc.NewText(activity->getEndDate().c_str()));
			root->InsertEndChild(node = doc.NewElement("starttime"));
			node->InsertEndChild(doc.NewText(activity->getStartTime().c_str()));
			root->InsertEndChild(node = doc.NewElement("endtime"));
			node->InsertEndChild(doc.NewText(activity->getEndTime().c_str()));
			root->InsertEndChild(node = doc.NewElement("status"));
			node->InsertEndChild(doc.NewText(activity->getStatus().c_str()));
			root->InsertEndChild(node = doc.NewElement("address"));
			node->InsertEndChild(doc.NewText(activity->getAddress().c_str()));
			root->InsertEndChild(node = doc.NewElement("fee"));
			node->InsertEndChild(doc.NewText(((value = toString(activity->getFee())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("sponsor"));
			node->InsertEndChild(doc.NewText(activity->isSponsorNull()?"":activity->getSponsor().c_str()));
			root->InsertEndChild(node = doc.NewElement("photos"));
			node->InsertEndChild(doc.NewText(activity->isPhotosNull()?"":activity->getPhotos().c_str()));
			root->InsertEndChild(node = doc.NewElement("description"));
			node->InsertEndChild(doc.NewText(activity->isDescriptionNull()?"":activity->getDescription().c_str()));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processGetPlayList
void ERPProcessor::processGetPlayList() {
	std::string type;
	size_t number;
	size_t page;

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
		if (node = root->FirstChildElement("type"))
			type = getText(node);
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("number")) {
			number = toUInt(getText(node));
			if (number == 0)
				return sendErrorMessage("Invalid number: number is zero");
		} else return sendErrorMessage("Missing number");
		if (node = root->FirstChildElement("page"))
			page = toUInt(getText(node));
		else return sendErrorMessage("Missing page");
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	auto list = mc->getSongList(type, page * number, number);
	size_t total = mc->countSongList(type);
	total = ((total == 0)? 0 : (total - 1) / number + 1);

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* songlists;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("page"));
		node->InsertEndChild(doc.NewText(((value = toString(page)), value.c_str())));
		root->InsertEndChild(node = doc.NewElement("total"));
		node->InsertEndChild(doc.NewText(((value = toString(total)), value.c_str())));
		root->InsertEndChild(songlists = doc.NewElement("songlists"));
		for each (auto songlist in *list) {
			songlists->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("lid"));
			node->InsertEndChild(doc.NewText(((value = toString(songlist->getLid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("title"));
			node->InsertEndChild(doc.NewText(songlist->getTitle().c_str()));
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(songlist->getImage().c_str()));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(songlist->getType().c_str()));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(songlist->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("special"));
			node->InsertEndChild(doc.NewText(toCString(songlist->getSpecial())));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processGetPlayListMusic
void ERPProcessor::processGetPlayListMusic() {
	unsigned int lid;

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
		if (node = root->FirstChildElement("lid"))
			lid = toUInt(getText(node));
		else return sendErrorMessage("Missing lid");
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	auto media_type = mc->getMediaType();
	auto media_language = mc->getMediaLanguage();
	auto list = mc->getSongListMedia(lid);

	// Feedback
	// This part is copied from processGetMusicList()
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* songs;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(songs = doc.NewElement("songs"));
		for each (auto media in *list) {
			songs->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("mid"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getMid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("name"));
			node->InsertEndChild(doc.NewText(media->getName().c_str()));
			root->InsertEndChild(node = doc.NewElement("singer"));
			node->InsertEndChild(doc.NewText(media->getSinger().c_str()));
#ifndef REMOVE_SQL_IMAGE
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(media->isImageNull()?"":media->getImage().c_str()));
#endif
			root->InsertEndChild(node = doc.NewElement("language"));
			node->InsertEndChild(doc.NewText(media_language[media->getLanguage()].c_str()));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(media_type[media->getType()].c_str()));
			root->InsertEndChild(node = doc.NewElement("stars"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getStars())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("path"));
			node->InsertEndChild(doc.NewText(media->getPath().c_str()));
			root->InsertEndChild(node = doc.NewElement("sids"));
			{
				value.clear();
				if (!media->isArtistSid1Null())
					value = toString(media->getArtistSid1());
				if (!media->isArtistSid2Null()) {
					if (!value.empty())
						value.push_back(',');
					value.append(toString(media->getArtistSid2()));
				}
			}
			node->InsertEndChild(doc.NewText(value.c_str()));
			root->InsertEndChild(node = doc.NewElement("pinyin"));
			node->InsertEndChild(doc.NewText(media->getPinyin().c_str()));
			root->InsertEndChild(node = doc.NewElement("lyric"));
			node->InsertEndChild(doc.NewText(media->getLyric().c_str()));
			root->InsertEndChild(node = doc.NewElement("header"));
			node->InsertEndChild(doc.NewText(media->getHeader().c_str()));
			root->InsertEndChild(node = doc.NewElement("originaltrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getOriginalTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("soundtrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getSoundTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("words"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getWords())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("black"));
			node->InsertEndChild(doc.NewText(toCString(media->getBlack())));
			root->InsertEndChild(node = doc.NewElement("hot"));
			node->InsertEndChild(doc.NewText(toCString(media->getHot())));
			root->InsertEndChild(node = doc.NewElement("head"));
			node->InsertEndChild(doc.NewText(media->isHeadNull()?"":((value = media->getHead()), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("enabled"));
			node->InsertEndChild(doc.NewText(""));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processEditMusic
void ERPProcessor::processEditMusic() {
	tinyxml2::XMLDocument doc;
	unsigned int mid;
	
	// Parse XML
	{
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
	
	// Get base content
	auto mc = _server->getDatabase()->getMediaConnector();
	auto music = mc->getMedia(mid);

	// Update info
	{
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node;
		if (node = root->FirstChildElement("name")) {
			music->setName(getText(node));
			if (music->getLanguage() == mc->getMediaLanguageId("japan"))
				music->setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(music->getName()) , PinyinAll::JAP));
			else if(music->getLanguage() == mc->getMediaLanguageId("korean"))
				music->setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(music->getName()) , PinyinAll::KOR));
			else
				music->setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(music->getName()) , PinyinAll::CHN));
			music->setHeader(getPinyinHead(music->getPinyin()));
			music->setPinyin(removeSplit(music->getPinyin()));
			music->setWords((int)music->getHeader().size());
			if (!music->getHeader().empty())
				music->setHead(music->getHeader().front());
		}
		if ((node = root->FirstChildElement("sids")) && node->GetText()) {
			music->setArtistSid1();
			music->setArtistSid2();
			auto sids = split(getText(node), ',');
			for (int i = 0; i < 2 && i < sids.size(); i++) {
				switch (i) {
				case 0:
					music->setArtistSid1(toInt(trim(sids[i])));
					break;
				case 1:
					music->setArtistSid2(toInt(trim(sids[i])));
					break;
				}
			}
			std::string singer;
			if (!music->isArtistSid1Null()) {
				auto actor = mc->getActor(music->getArtistSid1());
				singer = actor->getName();
			}
			if (!music->isArtistSid2Null()) {
				auto actor = mc->getActor(music->getArtistSid2());
				if (!singer.empty())
					singer += ' ';
				singer += actor->getName();
			}
			music->setSinger(singer);
		}
		if (node = root->FirstChildElement("language"))
			music->setLanguage(mc->getMediaLanguageId(getText(node)));
		if ((node = root->FirstChildElement("lyric")))
			music->setLyric(getText(node));
		if (node = root->FirstChildElement("path"))
			music->setPath(getText(node));
		if (node = root->FirstChildElement("originaltrack"))
			music->setOriginalTrack(toInt(getText(node)));
		if (node = root->FirstChildElement("soundtrack"))
			music->setSoundTrack(toInt(getText(node)));
		if (node = root->FirstChildElement("type"))
			music->setType(mc->getMediaTypeId(getText(node)));
		if (node = root->FirstChildElement("stars"))
			music->setStars(toFloat(getText(node)));
		if (node = root->FirstChildElement("black"))
			music->setBlack(toBool(getText(node)));
		if (node = root->FirstChildElement("count"))
			music->setCount(toInt(getText(node)));
		if (node = root->FirstChildElement("hot"))
			music->setHot(toBool(getText(node)));
		// "enable" ignored
	}

	// Process
	bool confirm = true;
	try {
		mc->update(*music);
#ifdef _DEBUG
	} catch (const std::exception& err) {
		Logger::get("server")->log(err.what(), Logger::WARNING);
#else
	} catch (...) {
#endif // _DEBUG
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processEditSinger
void ERPProcessor::processEditSinger() {
	unsigned int sid;
	tinyxml2::XMLDocument doc;

	// Parse XML
	{
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
		if (node = root->FirstChildElement("sid"))
			sid = toUInt(getText(node));
		else return sendErrorMessage("Missing sid");
	}

	// Get base content
	auto mc = _server->getDatabase()->getMediaConnector();
	auto actor = mc->getActor(sid);

	// Update info
	{
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node;
		if (node = root->FirstChildElement("name")) {
			actor->setName(getText(node));
			actor->setPinyin(PinyinAll::getInstance()->toPinyin(toWideChar(actor->getName())));
			actor->setHeader(getPinyinHead(actor->getPinyin()));
			actor->setPinyin(removeSplit(actor->getPinyin()));
			if (!actor->getHeader().empty())
				actor->setHead(actor->getHeader().front());
		}
		if (node = root->FirstChildElement("stars"))
			actor->setStars(toFloat(getText(node)));
		if (node = root->FirstChildElement("type"))
			actor->setType(mc->getActorTypeId(getText(node)));
#ifndef REMOVE_SQL_IMAGE
		if (node = root->FirstChildElement("nation"))
			actor->setNation(getText(node));
		if (node = root->FirstChildElement("image"))
			actor->setImage(getText(node));
#endif
		if (node = root->FirstChildElement("count"))
			actor->setCount(toInt(getText(node)));
	}

	// Process
	bool confirm = true;
	try {
		mc->update(*actor);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processEditActivityList
void ERPProcessor::processEditActivityList() {
	tinyxml2::XMLDocument doc;
	unsigned int aid;

	// Parse XML
	{
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
		if (node = root->FirstChildElement("said"))
			aid = toUInt(getText(node));
		else return sendErrorMessage("Missing said");
	}

	// Get base content
	auto mc = _server->getDatabase()->getMediaConnector();
	auto activity = mc->getActivity(aid);

	// Update info
	{
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node;
		if (node = root->FirstChildElement("title"))
			activity->setTitle(getText(node));
		if (node = root->FirstChildElement("thumb"))
			activity->setThumb(getText(node));
		if (node = root->FirstChildElement("type"))
			activity->setType(getText(node));
		if (node = root->FirstChildElement("memberCount"))
			activity->setMemberCount(toInt(getText(node)));
		if (node = root->FirstChildElement("startdate"))
			activity->setStartDate(getText(node));
		if (node = root->FirstChildElement("enddate"))
			activity->setEndDate(getText(node));
		if (node = root->FirstChildElement("starttime"))
			activity->setStartTime(getText(node));
		if (node = root->FirstChildElement("endtime"))
			activity->setEndTime(getText(node));
		if (node = root->FirstChildElement("status"))
			activity->setStatus(getText(node));
		if (node = root->FirstChildElement("address"))
			activity->setAddress(getText(node));
		if (node = root->FirstChildElement("fee"))
			activity->setFee(toDouble(getText(node)));
		if (node = root->FirstChildElement("sponsor"))
			activity->setSponsor(getText(node));
		if (node = root->FirstChildElement("photos"))
			activity->setPhotos(getText(node));
		if (node = root->FirstChildElement("description"))
			activity->setDescription(getText(node));
	}

	// Process
	bool confirm = true;
	try {
		mc->update(*activity);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processEditPlayList
void ERPProcessor::processEditPlayList() {
	tinyxml2::XMLDocument doc;
	unsigned int lid;

	// Parse XML
	{
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
		if (node = root->FirstChildElement("lid"))
			lid = toUInt(getText(node));
		else return sendErrorMessage("Missing lid");
	}

	// Get base content
	auto mc = _server->getDatabase()->getMediaConnector();
	auto list = mc->getSongList(lid);

	// Update info
	{
		tinyxml2::XMLElement* root = doc.FirstChildElement("root");
		if (root == NULL)
			return sendErrorMessage("Missing root");
		tinyxml2::XMLElement* node;
		if (node = root->FirstChildElement("title"))
			list->setTitle(getText(node));
		if (node = root->FirstChildElement("image"))
			list->setImage(getText(node));
		if (node = root->FirstChildElement("type"))
			list->setType(getText(node));
		if (node = root->FirstChildElement("special"))
			list->setSpecial(toBool(getText(node)));
		if (node = root->FirstChildElement("songs")) {
			auto songstrs = split(getText(node),',');
			int count = 0;
			std::vector<unsigned int> songs;
			for each (auto songstr in songstrs) {
				std::string song = trim(songstr);
				if (!song.empty()) {
					songs.push_back(toUInt(song));
					++count;
				}
			}
			list->setSongs(songs);
			list->setCount(count);
		}
	}

	// Process
	bool confirm = true;
	try {
		mc->update(*list);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processEditRankList
void ERPProcessor::processEditRankList() {
	std::string list_name;
	std::vector<unsigned int> sids;

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
		if (node = root->FirstChildElement("type"))
			list_name = getText(node);
		else return sendErrorMessage("Missing type");
		if (node = root->FirstChildElement("songs")) {
			auto songs = split(getText(node),',');
			for each (auto songstr in songs) {
				std::string song = trim(songstr);
				if (!song.empty())
					sids.push_back(toUInt(song));
			}
		}
	}

	// Process
	bool confirm = true;
	try {
		_server->getDatabase()->getMediaConnector()->updateMediaList(list_name, sids);
	} catch (...) {
		confirm = false;
	}

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processQueryMusic
void ERPProcessor::processQueryMusic() {
	std::auto_ptr<unsigned int> mid;
	std::auto_ptr<std::string> name;
	int match = false;
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
			mid.reset(new unsigned int(toUInt(getText(node))));
		if (node = root->FirstChildElement("name"))
			name.reset(new std::string(getText(node)));
		if (node = root->FirstChildElement("match"))
			match = yiqiding::utility::toBool(getText(node));
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	std::auto_ptr<container::SmartVector<model::Media> > list;
	if (mid.get()) {
		list.reset(new container::SmartVector<model::Media>);
		list->push_back(mc->getMedia(*mid).release());
	} else if (name.get()) {
		list = mc->getMediaByName(*name , match != 0);
	} else
		return sendErrorMessage("Missing mid or name");
	auto media_type = mc->getMediaType();
	auto media_language = mc->getMediaLanguage();

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* songs;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(songs = doc.NewElement("songs"));
		for each (auto media in *list) {
			songs->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("mid"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getMid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("name"));
			node->InsertEndChild(doc.NewText(media->getName().c_str()));
			root->InsertEndChild(node = doc.NewElement("singer"));
			node->InsertEndChild(doc.NewText(media->getSinger().c_str()));
#ifndef REMOVE_SQL_IMAGE
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(media->isImageNull()?"":media->getImage().c_str()));
#endif
			root->InsertEndChild(node = doc.NewElement("language"));
			node->InsertEndChild(doc.NewText(media_language[media->getLanguage()].c_str()));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(media_type[media->getType()].c_str()));
			root->InsertEndChild(node = doc.NewElement("stars"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getStars())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("path"));
			node->InsertEndChild(doc.NewText(media->getPath().c_str()));
			root->InsertEndChild(node = doc.NewElement("sids"));
			{
				value.clear();
				if (!media->isArtistSid1Null())
					value = toString(media->getArtistSid1());
				if (!media->isArtistSid2Null()) {
					if (!value.empty())
						value.push_back(',');
					value.append(toString(media->getArtistSid2()));
				}
			}
			node->InsertEndChild(doc.NewText(value.c_str()));
			root->InsertEndChild(node = doc.NewElement("pinyin"));
			node->InsertEndChild(doc.NewText(media->getPinyin().c_str()));
			root->InsertEndChild(node = doc.NewElement("lyric"));
			node->InsertEndChild(doc.NewText(media->getLyric().c_str()));
			root->InsertEndChild(node = doc.NewElement("header"));
			node->InsertEndChild(doc.NewText(media->getHeader().c_str()));
			root->InsertEndChild(node = doc.NewElement("originaltrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getOriginalTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("soundtrack"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getSoundTrack())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("words"));
			node->InsertEndChild(doc.NewText(((value = toString(media->getWords())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("black"));
			node->InsertEndChild(doc.NewText(toCString(media->getBlack())));
			root->InsertEndChild(node = doc.NewElement("hot"));
			node->InsertEndChild(doc.NewText(toCString(media->getHot())));
			root->InsertEndChild(node = doc.NewElement("head"));
			node->InsertEndChild(doc.NewText(media->isHeadNull()?"":((value = media->getHead()), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("enabled"));
			node->InsertEndChild(doc.NewText(""));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processQuerySinger
void ERPProcessor::processQuerySinger() {
	std::auto_ptr<unsigned int> sid;
	std::auto_ptr<std::string> name;

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
		if (node = root->FirstChildElement("sid"))
			sid.reset(new unsigned int(toUInt(getText(node))));
		if (node = root->FirstChildElement("name"))
			name.reset(new std::string(getText(node)));
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	std::auto_ptr<container::SmartVector<model::Actor> > list;
	if (sid.get()) {
		list.reset(new container::SmartVector<model::Actor>);
		list->push_back(mc->getActor(*sid).release());
	} else if (name.get()) {
		list = mc->getActorByName(*name);
	} else
		return sendErrorMessage("Missing sid or name");
	auto actor_type = mc->getActorType();

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* artists;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(artists = doc.NewElement("artists"));
		for each (auto actor in *list) {
			artists->InsertEndChild(root = doc.NewElement("item"));
			root->InsertEndChild(node = doc.NewElement("sid"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getSid())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("name"));
			node->InsertEndChild(doc.NewText(actor->getName().c_str()));
			root->InsertEndChild(node = doc.NewElement("stars"));
			node->InsertEndChild(doc.NewText(actor->isStarsNull()?"":((value = toString(actor->getStars()), value.c_str()))));
			root->InsertEndChild(node = doc.NewElement("type"));
			node->InsertEndChild(doc.NewText(actor_type[actor->getType()].c_str()));
#ifndef REMOVE_SQL_IMAGE
			root->InsertEndChild(node = doc.NewElement("nation"));
			node->InsertEndChild(doc.NewText(actor->getNation().c_str()));
			root->InsertEndChild(node = doc.NewElement("image"));
			node->InsertEndChild(doc.NewText(actor->isImageNull()?"":actor->getImage().c_str()));
#endif
			root->InsertEndChild(node = doc.NewElement("pinyin"));
			node->InsertEndChild(doc.NewText(actor->getPinyin().c_str()));
			root->InsertEndChild(node = doc.NewElement("songcount"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getSongCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("count"));
			node->InsertEndChild(doc.NewText(((value = toString(actor->getCount())), value.c_str())));
			root->InsertEndChild(node = doc.NewElement("header"));
			node->InsertEndChild(doc.NewText(actor->getHeader().c_str()));
			root->InsertEndChild(node = doc.NewElement("head"));
			node->InsertEndChild(doc.NewText(actor->isHeadNull()?"":((value = actor->getHead()), value.c_str())));
		}
		list.reset();

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
//////////////////////////////////////////////////////////////////////////
#pragma region processQueryPlayList
void ERPProcessor::processQueryPlayList() {
	std::auto_ptr<unsigned int> lid;
	std::auto_ptr<std::string> title;

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
		if (node = root->FirstChildElement("lid"))
			lid.reset(new unsigned int(toUInt(getText(node))));
		if (node = root->FirstChildElement("title"))
			title.reset(new std::string(getText(node)));
	}

	// Process
	auto mc = _server->getDatabase()->getMediaConnector();
	std::auto_ptr<model::SongList> songlist;
	if (lid.get()) {
		songlist = mc->getSongList(*lid);
	} else if (title.get()) {
		songlist = mc->getSongListByTitle(*title);
	} else
		return sendErrorMessage("Missing lid or title");

	// Feedback
	Packet out_pack(_pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string value;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("list"));
		root = node;
		root->InsertEndChild(node = doc.NewElement("lid"));
		node->InsertEndChild(doc.NewText(((value = toString(songlist->getLid())), value.c_str())));
		root->InsertEndChild(node = doc.NewElement("title"));
		node->InsertEndChild(doc.NewText(songlist->getTitle().c_str()));
		root->InsertEndChild(node = doc.NewElement("image"));
		node->InsertEndChild(doc.NewText(songlist->getImage().c_str()));
		root->InsertEndChild(node = doc.NewElement("type"));
		node->InsertEndChild(doc.NewText(songlist->getType().c_str()));
		root->InsertEndChild(node = doc.NewElement("count"));
		node->InsertEndChild(doc.NewText(((value = toString(songlist->getCount())), value.c_str())));
		root->InsertEndChild(node = doc.NewElement("special"));
		node->InsertEndChild(doc.NewText(toCString(songlist->getSpecial())));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}

	// Sending
	out_pack.dispatch(_conn);
	setOutPack(&out_pack);
}
#pragma endregion
#pragma endregion


void ERPProcessor::processOpenSingGame()
{
	// parse XML
	std::vector<MidName> mids;

	bool confirm			 = true;
	
	bool			 typetime = false;
	std::string		 notfiymessage;
	std::string		 notifytitle;
	std::set<uint32_t> notifytimes;
	uint32_t		 kid		= 0;
	uint32_t		 now		= (uint32_t)::time(NULL);
	uint32_t		 starttime = now + TIME_BEFORE_START;
	std::auto_ptr<GameTimer> gt;
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
		
		if ((node = root->FirstChildElement("mids")) == NULL)
			return sendErrorMessage("Missing mids");
		for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
		{
			tinyxml2::XMLElement *cmid = i->FirstChildElement("mid");
			tinyxml2::XMLElement *cname = i->FirstChildElement("name");

			if(cmid == NULL || cname == NULL || !yiqiding::utility::isULong(cmid->GetText()))
				return sendErrorMessage("Missing mid 、name or mid is not uint");
			
			mids.push_back(MidName(yiqiding::utility::toUInt(cmid->GetText()) , cname->GetText() ));
		}
		if (mids.size() == 0)
		{
			return sendErrorMessage("Missing mids item");
		}

		if ((node = root->FirstChildElement("typetime")) == NULL)
		{
			return sendErrorMessage("Missing typetime");
		}
		typetime = yiqiding::utility::toBool(node->GetText());

		if ((node = root->FirstChildElement("notifymessage")) == NULL || !node->GetText())
		{
			return sendErrorMessage("Missing notifymessage");
		}
		notfiymessage = node->GetText();

		if ((node = root->FirstChildElement("notifytitle")) == NULL || !node->GetText())
		{
			return sendErrorMessage("Missing notifytitle");
		}
		notifytitle = node->GetText();

		if (!typetime)	//false 需要检测notifytimes 及 starttime
		{

			if ((node = root->FirstChildElement("starttime")) == NULL)
			{
				return sendErrorMessage("Missing starttime");
			}
			if (!yiqiding::utility::isULong(node->GetText()))
			{
				return sendErrorMessage("starttime is not uint");
			}
			starttime = yiqiding::utility::toUInt(node->GetText());

			//小于5分钟
			if ( starttime <= now +  TIME_START_AFTER_NOW)
			{
				Logger::get("system")->log(toString(starttime) + ":" +toString(now) + " starttime less than now 5 minuters" , Logger::WARNING);
				return GameTimer::sendOpenSingGame(this , _pac , _conn , 0 , 0);
			}

			if ((node = root->FirstChildElement("notifytimes")) == NULL)
			{
				return sendErrorMessage("Missing notifytimes");
			}
			for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
			{
				if(!yiqiding::utility::isULong(i->GetText()))
					return sendErrorMessage("notifytimes item is not uint");
				uint32_t time = yiqiding::utility::toUInt(i->GetText());
				if (time > starttime - TIME_BEFORE_START - TIME_END_AFTER_GAME || time < now )
				{
					Logger::get("system")->log(toString(time) + ":" + toString(starttime) + ":" +toString(now) + " time > starttime || time < now " , Logger::WARNING);
					return GameTimer::sendOpenSingGame(this , _pac , _conn , 0 , 0);
				}
				notifytimes.insert(time);
				
			}		
		}

		if(!SingerGame::getInstace()->checkTime(starttime , (int)mids.size()))
		{
			Logger::get("system")->log(toString(starttime) + ":" +toString(now) + " check time" , Logger::WARNING);
			return  GameTimer::sendOpenSingGame(this , _pac , _conn , 0 , 0);
		}
	}
	//send to box
	try
	{

		kid = SingerGame::getNewKid();
		gt.reset(new GameTimer(_server , kid));
		gt->beginSingeGame(typetime , mids ,notifytimes , notfiymessage , notifytitle , starttime);
		if (typetime)	//
		{
			confirm = gt->sendJoin();
		}
		else
		{
			gt->sendNotify(false);
		}
	}
	catch(const yiqiding::ktv::db::DBException &e)
	{
		Logger::get("system")->log(e.what() , Logger::WARNING);
	}

	//back 
	{
		
		GameTimer::sendOpenSingGame(this , _pac , _conn , kid , 1);
		if (!confirm)
		{
			gt->sendScoreNoBodyOnline();
		}
		SingerGame::getInstace()->add(gt.release());
		SingerGame::getInstace()->save("game.xml");
	}
	
}


void ERPProcessor::processCloseSingGame()
{
	uint32_t kid = 0; 
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
		if (node == NULL)
			return sendErrorMessage("Missing kid");
		if (!utility::isULong(node->GetText()))
			return sendErrorMessage("kid must be uint");
			kid = utility::toUInt(node->GetText());
		{
			yiqiding::MutextReader mr(SingerGame::getInstace()->getMutexWR()); 
			GameTimer *gt = SingerGame::getInstace()->get(kid);
			if (gt != NULL && gt->canDel())
			{
				confirm = true;
			}
		
		}
		if(confirm)
		{
			confirm = SingerGame::getInstace()->del(kid);
			SingerGame::getInstace()->save("game.xml");

			packet::Packet out_pack(packet::ERP_REQ_CLOSE_SING_GAME);
			{
				// Generate XML
				tinyxml2::XMLDocument doc;
				tinyxml2::XMLElement* root;
				tinyxml2::XMLElement* node;
				std::string text;
				doc.InsertEndChild(root = doc.NewElement("root"));
				root->InsertEndChild(node = doc.NewElement("kid"));
				node->InsertEndChild(doc.NewText(toString(kid).c_str()));

				tinyxml2::XMLPrinter printer(0, true);
				doc.Print(&printer);
				// Pack
				out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
			}
			auto box_connections = _server->getConnectionManager()->getConnectionBox();
			setAttPack(&out_pack , -1);
			for each (auto box_conn in box_connections) {
				auto conn = box_conn.second->getConnection(_server);
				if (conn) {
					try {
						out_pack.dispatch(conn);
					} catch (...) {}
					conn->release();
				}
			}
			

		}

		{
			packet::Packet back_pack(_pac->getHeader());
			{
				// Generate XML
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
				// Pack
				back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
			}
			back_pack.dispatch(_conn);
			setOutPack(&back_pack);
		}
		
		
	}
	
}

void ERPProcessor::processModifySingGame()
{
	// parse XML
	std::vector<MidName> mids;
	uint32_t		 starttime = 0;
	bool			 typetime = false;
	std::string		 notfiymessage;
	std::string		 notifytitle;
	std::set<uint32_t> notifytimes;
	uint32_t		 kid		= 0;
	uint32_t		 now		= (uint32_t)::time(NULL);
	bool			 confirm	= false;
	GameTimer *gt = NULL;
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

		node = root->FirstChildElement("kid");
		if (node == NULL)
			return sendErrorMessage("Missing Kid");
		if (!yiqiding::utility::isULong(node->GetText()))
			return sendErrorMessage("kid must uint");
		kid = yiqiding::utility::toUInt(node->GetText());
		{
			MutextReader wr(SingerGame::getInstace()->getMutexWR());
		
			gt = SingerGame::getInstace()->get(kid);
			if (!gt || !gt->canModify() || gt->getType())
			{	
				return GameTimer::sendModifySingGame(this , _pac , _conn , false);
			}
		
			if ((node = root->FirstChildElement("mids")) == NULL)
				return sendErrorMessage("Missing mids");
			for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
			{
				tinyxml2::XMLElement *cmid = i->FirstChildElement("mid");
				tinyxml2::XMLElement *cname = i->FirstChildElement("name");

				if(cmid == NULL || cname == NULL || !yiqiding::utility::isULong(cmid->GetText()))
					return sendErrorMessage("Missing mid 、name or mid is not uint");

				mids.push_back(MidName(yiqiding::utility::toUInt(cmid->GetText()) , cname->GetText() ));
			}
			if (mids.size() == 0)
			{
				return sendErrorMessage("Missing mids item");
			}

			if ((node = root->FirstChildElement("typetime")) == NULL)
			{
				return sendErrorMessage("Missing typetime");
			}
			typetime = gt->getType();

			if ((node = root->FirstChildElement("notifymessage")) == NULL || !node->GetText())
			{
				return sendErrorMessage("Missing notifymessage");
			}
			notfiymessage = node->GetText();

			if ((node = root->FirstChildElement("notifytitle")) == NULL || !node->GetText())
			{
				return sendErrorMessage("Missing notifytitle");
			}
			notifytitle = node->GetText();

			if ((node = root->FirstChildElement("starttime")) == NULL)
			{
				return sendErrorMessage("Missing starttime");
			}
			if (!yiqiding::utility::isULong(node->GetText()))
			{
				return sendErrorMessage("starttime is not uint");
			}
			starttime = yiqiding::utility::toUInt(node->GetText());

			//小于5分钟
			if ( starttime <= now +  TIME_START_AFTER_NOW)
			{
				return GameTimer::sendModifySingGame(this , _pac , _conn , false);
			}

			if ((node = root->FirstChildElement("notifytimes")) == NULL)
			{
				return sendErrorMessage("Missing notifytimes");
			}
			for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
			{
				if(!yiqiding::utility::isULong(i->GetText()))
					return sendErrorMessage("notifytimes item is not uint");
				uint32_t time = yiqiding::utility::toUInt(i->GetText());
				if (time > starttime - TIME_BEFORE_START - TIME_END_AFTER_GAME || time < now )
				{
					return GameTimer::sendModifySingGame(this , _pac , _conn , false);
				}
				
				notifytimes.insert(time);

			}	
	

			if(!SingerGame::getInstace()->checkTimeExcept(starttime , (int)mids.size() , kid))
				return  GameTimer::sendModifySingGame(this , _pac , _conn , false);

			confirm = gt->modifySingeGame(mids , notifytimes , notfiymessage , notifytitle , starttime);
			if (confirm)
			{
				if (typetime)	//
				{
					confirm = gt->sendJoin();
				}
				else
				{
					gt->sendNotify(false);
				}
			}
			GameTimer::sendModifySingGame(this , _pac , _conn , confirm);	
		}
		SingerGame::getInstace()->save("game.xml");

	}
}

void ERPProcessor::processAllSingGame()
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
		root->InsertEndChild(node = doc.NewElement("kinfo"));
		{
			yiqiding::MutextReader mr(SingerGame::getInstace()->getMutexWR()); 
			for each(auto bc in SingerGame::getInstace()->getTimers())
			{
				if(bc.second->getState() == GAME_END)
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
				croot->InsertEndChild(cnode = doc.NewElement("typetime"));
				cnode->InsertEndChild(doc.NewText(toCString(bc.second->getType())));
				croot->InsertEndChild(cnode = doc.NewElement("state"));
				cnode->InsertEndChild(doc.NewText(toCString(bc.second->getState() != GAME_NOTIFY_TIMER)));
				croot->InsertEndChild(cnode = doc.NewElement("notifytimes"));
				for each(auto t in bc.second->getNotifyTimes())
				{
					cnode->InsertEndChild(droot = doc.NewElement("item"));
					droot->InsertEndChild(doc.NewText(toString(t).c_str()));
				}
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

void ERPProcessor::processDetailSingGame()
{
	uint32_t kid = 0; 
	bool confirm = false;
	GameTimer *gt = NULL;
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
		if (node == NULL)
			return sendErrorMessage("Missing kid");
		if (!utility::isULong(node->GetText()))
			return sendErrorMessage("kid must be uint");
		kid = utility::toUInt(node->GetText());

		////////////////////////////////////////////////////////////
		packet::Packet back_pack(_pac->getHeader());
		{
			
			
			std::auto_ptr<model::Game> gt;
			try{
				gt.reset(_server->getDatabase()->getMediaConnector()->getGame(kid).release());
				confirm = true;
			}
			catch(const yiqiding::ktv::db::DBException &)
			{
				confirm = false;
			}
			{
					// Generate XML
					tinyxml2::XMLDocument doc;
					tinyxml2::XMLElement* root;
					tinyxml2::XMLElement* node;
					std::string text;
					doc.InsertEndChild(root = doc.NewElement("root"));
					root->InsertEndChild(node = doc.NewElement("status"));
					node->InsertEndChild(doc.NewText("0"));
					root->InsertEndChild(node = doc.NewElement("confirm"));
					node->InsertEndChild(doc.NewText(toCString(confirm)));
					root->InsertEndChild(node = doc.NewElement("kid"));
					node->InsertEndChild(doc.NewText(toString(kid).c_str()));
					
					if (confirm)
					{
						tinyxml2::XMLElement* croot;
						tinyxml2::XMLElement* cnode;
						root->InsertEndChild(node = doc.NewElement("mids"));

						for each(auto midname in gt->getKMids())
						{
							node->InsertEndChild(croot = doc.NewElement("item"));
							croot->InsertEndChild(cnode = doc.NewElement("mid"));
							cnode->InsertEndChild(doc.NewText(toString(midname._mid).c_str()));
							croot->InsertEndChild(cnode = doc.NewElement("name"));
							cnode->InsertEndChild(doc.NewText(midname._name.c_str()));
						}

						root->InsertEndChild(node = doc.NewElement("starttime"));
						node->InsertEndChild(doc.NewText(toString(gt->getKStartTime()).c_str()));
						root->InsertEndChild(node = doc.NewElement("typetime"));
						node->InsertEndChild(doc.NewText(toString(gt->getKType()).c_str()));
						root->InsertEndChild(node = doc.NewElement("notifytitle"));
						node->InsertEndChild(doc.NewText(gt->getKTitle().c_str()));
						root->InsertEndChild(node = doc.NewElement("notifymessage"));
						node->InsertEndChild(doc.NewText(gt->getKMessage().c_str()));
						root->InsertEndChild(node = doc.NewElement("state"));
						node->InsertEndChild(doc.NewText(toString(gt->getKState()).c_str()));
						root->InsertEndChild(croot = doc.NewElement("notifytimes"));
						for each (auto time in gt->getKSendTime())
						{
							croot->InsertEndChild(cnode = doc.NewElement("item"));
							cnode->InsertEndChild(doc.NewText(toString(time).c_str()));
						}
						root->InsertEndChild(node = doc.NewElement("scores"));


						std::set<int> uploadscores;
						for each(auto score in gt->getKScores())
							uploadscores.insert(score._boxid);

						std::auto_ptr<std::map<uint32_t , std::string> > RoomNames = yiqiding::ktv::box::BoxInfoMan::getInstace()->getRoomNames(uploadscores);

						for each(auto bc in gt->getKScores())
						{
							node->InsertEndChild(croot = doc.NewElement("item"));
							croot->InsertEndChild(cnode = doc.NewElement("boxid"));
							cnode->InsertEndChild(doc.NewText(toString(bc._boxid).c_str()));
							croot->InsertEndChild(cnode = doc.NewElement("roomname"));
							cnode->InsertEndChild(doc.NewText((*RoomNames)[bc._boxid].c_str()));
							croot->InsertEndChild(cnode = doc.NewElement("score"));
							cnode->InsertEndChild(doc.NewText(toString(bc._score).c_str()));
						}

					}
					tinyxml2::XMLPrinter printer(0, true);
					doc.Print(&printer);
					// Pack
					back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
			}
			
			
		}
		back_pack.dispatch(_conn);
		setOutPack(&back_pack);
	}

}

void ERPProcessor::processAllDataSingGame()
{
	uint32_t pos = 0;
	uint32_t n = 0;
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
		
		
		tinyxml2::XMLElement* node = root->FirstChildElement("number");
		if (node == NULL)
			return sendErrorMessage("Missing number");
		if (!utility::isULong(node->GetText()))
			return sendErrorMessage("number must be uint");
		n = utility::toUInt(node->GetText());
		if (n == 0)
			return sendErrorMessage("number can't be zero");

		 node = root->FirstChildElement("page");
		if (node == NULL)
			return sendErrorMessage("Missing page");
		if(!utility::isULong(node->GetText()))
			return sendErrorMessage("page must be uint");
		pos = n * utility::toUInt(node->GetText());
	}


	packet::Packet back_pack(_pac->getHeader());
	{	
		auto list = _server->getDatabase()->getMediaConnector()->getGameList(pos , n);
		uint32_t total = _server->getDatabase()->getMediaConnector()->countGameList();
		total = ((total == 0)? 0 : (total - 1)/ n + 1);
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* croot;
		tinyxml2::XMLElement* cnode;
		tinyxml2::XMLElement* droot;
		tinyxml2::XMLElement* dnode;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("page"));
		node->InsertEndChild(doc.NewText(toString(pos/n).c_str()));
		root->InsertEndChild(node = doc.NewElement("total"));
		node->InsertEndChild(doc.NewText(toString(total).c_str()));

		root->InsertEndChild(node = doc.NewElement("kinfo"));
		{
			
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
				croot->InsertEndChild(cnode = doc.NewElement("typetime"));
				cnode->InsertEndChild(doc.NewText(toString(bc->getKType()).c_str()));
				croot->InsertEndChild(cnode = doc.NewElement("state"));
				cnode->InsertEndChild(doc.NewText(toCString(bc->getKState() != GAME_NOTIFY_TIMER)));
				croot->InsertEndChild(cnode = doc.NewElement("notifytimes"));
				for each(auto t in bc->getKSendTime())
				{
					cnode->InsertEndChild(droot = doc.NewElement("item"));
					droot->InsertEndChild(doc.NewText(toString(t).c_str()));
				}
			}
			tinyxml2::XMLPrinter printer(0, true);
		
		}
		
		// Pack
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);	
	
	}
	back_pack.dispatch(_conn);
	setOutPack(&back_pack);
}


void ERPProcessor::processAddKGame2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	std::string songs;
	std::string title;
	std::string awards;
	unsigned int time = 0;
	std::string desc;
	int num;
	std::string boxids;
	int type;
	bool confirm = false;
	if(!root.isMember("songs") || !root["songs"].isArray())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error songs");
	}
	songs = root["songs"].toStyledString();

	if(!root.isMember("awards") || !root["awards"].isArray())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error awards");
	}
	awards = root["awards"].toStyledString();

	if(!root.isMember("title") || !root["title"].isConvertibleTo(Json::stringValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error title");
	}
	title = root["title"].asString();

	if(!root.isMember("desc") || !root["desc"].isConvertibleTo(Json::stringValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "error desc");
	}
	desc = root["desc"].asString();

	if(!root.isMember("num") || !root["num"].isConvertibleTo(Json::intValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "num error");
	}
	num = root["num"].asInt();

	if(root.isMember("boxids") && root["boxids"].isConvertibleTo(Json::arrayValue))
	{
		/*for (int i = 0 ; i < root["boxids"].size() ; ++i)
		{
			if(root["boxids"][i].isConvertibleTo(Json::intValue))
				boxids.insert(root["boxids"][i].asInt());
		}*/
		boxids = root["boxids"].toStyledString();
	}

	if(!root.isMember("type") || !root["type"].isConvertibleTo(Json::intValue))
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "type error");
	}
	type = root["type"].asInt();

	if(type && root.isMember("time") && root["time"].isConvertibleTo(Json::stringValue))
	{
		time = yiqiding::utility::getDateTime(root["time"].asString().c_str());
	}
	
	if(!time  || time > ::time(NULL))
	{
		confirm = true;
		Singleton<yiqiding::ktv::KTVGameTimer2>::getInstance()->add(_server , title , desc , awards , songs  , time , num , boxids);
	}


	
	

	{
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = confirm;
		std::string msg = root.toStyledString();
		packet::Packet out(_pac->getHeader());
		out.setPayload(msg.c_str() , msg.length());
		try{
			out.dispatch(_conn);
			setOutPack(&out);
		}
		catch(...){}
	}
	

}


void ERPProcessor::processDelKGame2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	int id;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("id") || !root["id"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss id");

	id = root["id"].asInt();

	bool confirm = Singleton<KTVGameTimer2>::getInstance()->del(_server , id);

	{
		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["confirm"] = confirm;
		std::string msg = root.toStyledString();
		packet::Packet out(_pac->getHeader());
		out.setPayload(msg.c_str() , msg.length());
		try{
			out.dispatch(_conn);
			setOutPack(&out);
		}
		catch(...){}
	}
}

void ERPProcessor::processGetGame2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	int page;
	int num;
	if(!reader.parse(str , root) || !root.isObject())
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "json error");
	}

	if(!root.isMember("page") || !root["page"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss page");

	page = root["page"].asInt();

	if(!root.isMember("num") || !root["num"].isConvertibleTo(Json::intValue))
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "miss num");

	num = root["num"].asInt();

	auto list = _server->getDatabase()->getMediaConnector()->getAllGame2(page , num);
	{


		Json::Value root;
		root["status"] = packet::BACK_NO_ERROR;
		root["total"] = _server->getDatabase()->getMediaConnector()->getGame2Count();
		Json::Value game(Json::arrayValue);
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
			item["status"] = g.getIsComplete();
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
#pragma region ERP Misc
//////////////////////////////////////////////////////////////////////////
// ERP Misc
//////////////////////////////////////////////////////////////////////////
std::string ERPProcessor::getPinyinHead(const std::string& pinyin) {
	std::string	result;
	auto words = split(pinyin,'_');
	for each (auto word in words) {
		if (!word.empty())
			result.push_back(toupper(word.front()));
	}
	return result;
}

std::string ERPProcessor::removeSplit(const std::string &pinyin , char split)
{
	std::vector<char> result;
	for(int i = 0 ; i < pinyin.length(); ++i)
		if(pinyin[i] != split)
			result.push_back(pinyin[i]);

	return std::string(result.begin() , result.end());
}

#pragma endregion

#pragma region ERP Processor
//////////////////////////////////////////////////////////////////////////
// ERP Processor
//////////////////////////////////////////////////////////////////////////


void ERPProcessor::onReceivePacket() {
	ERPRole role = (ERPRole)_pac->getDeviceID();

	// Security check
	if (getConnection()->isRegistered()) {
		if (getConnection()->getRole() != role)
			return sendErrorMessage("Authentication failure");
		} else {


			MutexGuard guard(getConnection()->getMutex());
			if (getConnection()->isRegistered()) {
				if (getConnection()->getRole() != role)
					return sendErrorMessage("Authentication failure");
			}
			
			else{

				getConnection()->setRole(role);
				_server->getConnectionManager()->updateERP(role, _conn->getID());
			}


	}
#ifdef _DEBUG
	_server->getConnectionManager()->getConnectionERP(getConnection()->getRole())->record_push(_rdpac);
#endif
	// Process Packet
	packet::Request request = _pac->getRequest();

	// Keep Alive
	if (request == packet::GENERAL_REQ_KEEPALIVE)
		return proceessHeart();

	if(KtvdoLua(request ,_server ,  this))
		return;

	// Process by Role
	switch (role) {
	case ERP_ROLE_MAIN:
		switch (request) {
		case packet::KTV_REQ_ERP_SERVICE_CALL:		finishServiceCall();		break;
		case packet::ERP_REQ_CONTROL_ROOM:			processControlRoom();		break;
		case packet::ERP_REQ_BROADCAST_MESSAGE:		processBroadcastMessage();	break;
		case packet::ERP_REQ_SINGLE_MESSAGE:		processSingleMessage();		break;
		case packet::GENERAL_REQ_KEEPALIVE:			proceessHeart();			break;
		case packet::ERP_REQ_UPLOAD_KTVBOX_INFO:	processUploadBoxInfo();		break;
		case packet::ERP_ERQ_INI_INFO:				processDongle();			break;
		case packet::ERP_REQ_CHANGE_BOX:			processChangeBox();			break;
		case packet::ERP_REQ_SET_MSG_RULE:			processSetMsgRule();		break;
		case packet::ERP_REQ_SET_FIRE_INFO:			processSetFireInfo();		break;
		case packet::ERP_REQ_SET_AD_URL:			processSetAdUrls();			break;
		case packet::ERP_REQ_ONLINE_BOX_LIST:		processGetOnlineBox();		break;

		case packet::KTV_REQ_ERP_CLOUD_UPDATE:		processCloudConfirm();		break;

		default:	sendErrorMessage("Request not supported");					break;
		}	break;

	case ERP_ROLE_MEDIA:
		switch (request) {
		case packet::ERP_REQ_CONTROL_BLACKLIST:		processControlBlacklist();	break;
		case packet::ERP_REQ_ADD_MUSIC:				processAddMusic();			break;
		case packet::ERP_REQ_ADD_SINGER:			processAddSinger();			break;
		case packet::ERP_REQ_ADD_ACTIVITY:			processAddActivity();		break;
		case packet::ERP_REQ_ADD_PLAY_LIST:			processAddPlayList();		break;
		case packet::ERP_REQ_GET_MUSIC_LIST:		processGetMusicList();		break;
		case packet::ERP_REQ_GET_SINGER_LIST:		processGetSingerList();		break;
		case packet::ERP_REQ_GET_ACTIVITY_LIST:		processGetActivityList();	break;
		case packet::ERP_REQ_GET_PLAY_LIST:			processGetPlayList();		break;
		case packet::ERP_REQ_GET_PLAY_LIST_MUSIC:	processGetPlayListMusic();	break;
		case packet::ERP_REQ_EDIT_MUSIC:			processEditMusic();			break;
		case packet::ERP_REQ_EDIT_SINGER:			processEditSinger();		break;
		case packet::ERP_REQ_EDIT_ACTIVITY_LIST:	processEditActivityList();	break;
		case packet::ERP_REQ_EDIT_PLAY_LIST:		processEditPlayList();		break;
		case packet::ERP_REQ_EDIT_RANK_LIST:		processEditRankList();		break;
		case packet::ERP_REQ_QUERY_MUSIC:			processQueryMusic();		break;
		case packet::ERP_REQ_QUERY_SINGER:			processQuerySinger();		break;
		case packet::ERP_REQ_QUERY_PLAY_LIST:		processQueryPlayList();		break;
		case packet::ERP_REQ_RESOURCE_URL:			processResource();			break;

		//sing game
		case packet::ERP_REQ_OPEN_SING_GAME:		processOpenSingGame();		break;
		case packet::ERP_REQ_CLOSE_SING_GAME:		processCloseSingGame();		break;
		case packet::ERP_REQ_MODIFY_SING_GAME:		processModifySingGame();	break;
		case packet::ERP_REQ_ALL_SING_GAME:			processAllSingGame();		break;
		case packet::ERP_REQ_DETAIL_DATA_SING_GAME:		processDetailSingGame();	break;
		case packet::ERP_ERQ_ALL_DATA_SING_GAME:		processAllDataSingGame();	break;
		case packet::GENERAL_REQ_KEEPALIVE:			proceessHeart();			break;
		case packet::ERP_ERQ_INI_INFO:				processDongle();			break;
		case packet::ERP_REQ_SET_VOLUME:			processSetVolume();			break;
		case packet::ERP_REQ_GET_VOLUME:			processGetVolume();			break;

		//kgame2
		case packet::ERP_REQ_ADD_KAGME2:			processAddKGame2();			break;
		case packet::ERP_REQ_DEL_KGAME2:			processDelKGame2();			break;
		case packet::ERP_REQ_GET_KGAME2:			processGetGame2();			break;



		default:	sendErrorMessage("Request not supported");					break;
	
		}	break;
	case ERP_ROLE_MARKET:
		switch (request){
		case packet::ERP_ERQ_INI_INFO:				processDongle();			break;
		case packet::GENERAL_REQ_KEEPALIVE:			proceessHeart();			break;

		default:	sendErrorMessage("Request not supported");					break;
		}

		break;
	case ERP_ROLE_KITCHEN:
		switch (request){
		case packet::ERP_ERQ_INI_INFO:				processDongle();			break;
		case packet::GENERAL_REQ_KEEPALIVE:			proceessHeart();			break;

		default:	sendErrorMessage("Request not supported");					break;
		}

		break;
	default:
		sendErrorMessage("Unknown role detected");
		break;
	}
}


void ERPProcessor::processSetMsgRule()
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
	if(root == NULL)
		return sendErrorMessage("Miss root");
	tinyxml2::XMLElement* node = root->FirstChildElement("file");
	if(node == NULL || node->GetText() == NULL)
		return sendErrorMessage("Miss file or file node error");
	bool confirm = MessageRule::getInstance()->save(MSG_PATH , node->GetText());

	packet::Packet out_pack(_pac->getHeader());
	try
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}
	catch(extended::ERPConnectionLost &)
	{
		Logger::get("server")->log("processSetMsgRule ERP LOST " + _ip);
	}
}





void ERPProcessor::processDongle()
{
	bool confirm = true;


#ifndef NO_DONGLE
	try{
		if(Dongle::getInstance()->checkUDID((uint32_t)::time(NULL) , NULL))
			confirm = true;
		else
			confirm = false;
	}catch(const std::exception &)
	{
		confirm = false;
	}
#endif

	packet::Packet out_pack(_pac->getHeader());
	try
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}
	catch(extended::ERPConnectionLost &)
	{
		Logger::get("server")->log("dongle ERP LOST " + _ip);
	}


}

void ERPProcessor::processSetFireInfo()
{
	tinyxml2::XMLDocument doc;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if(root == NULL)
		return sendErrorMessage("Miss root");
	tinyxml2::XMLElement* node = root->FirstChildElement("fireImageUrl");
	if(node == NULL)
		return sendErrorMessage("Miss fireImageUrl");
	std::string fireImageUrl = getText(node);
	node = root->FirstChildElement("fireVideoUrl");
	if(node == NULL)
		return sendErrorMessage("Miss fireVideoUrl");
	std::string fireVideoUrl = getText(node);

	packet::Packet out_pack(_pac->getHeader());
	{
		
		bool confirm = false;
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		confirm = Singleton<fire::FireWarn>::getInstance()->save(fire::path , fireImageUrl , fireVideoUrl);
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		out_pack.setPayload(printer.CStr() , printer.CStrSize() - 1);
		
		try{
			out_pack.dispatch(_conn);
			setOutPack(&out_pack);
		}catch(...)
		{
			Logger::get("system")->log("erp " + yiqiding::utility::toString(_pac->getDeviceID())  + "Lost Connection");
		}
	}
}

void ERPProcessor::processSetAdUrls()
{
	tinyxml2::XMLDocument doc;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if(root == NULL)
		return sendErrorMessage("Miss root");
	tinyxml2::XMLElement* node = root->FirstChildElement("urls");
	if(node == NULL)
		return sendErrorMessage("Miss urls");
	Json::Value arrayObj;
	for (auto i = node->FirstChildElement("item"); i != NULL; i = i->NextSiblingElement("item"))
	{
		Json::Value item;
		item["url"] = getText(i);
		arrayObj.append(item);
	}

	Json::Value back;
	back["urls"] = arrayObj;


	packet::Packet out_pack(_pac->getHeader());
	{

		bool confirm = false;
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		root->InsertEndChild(node = doc.NewElement("confirm"));
		confirm = Singleton<AdUrl>::getInstance()->save(AdPath , back.toStyledString());
		node->InsertEndChild(doc.NewText(toCString(confirm)));
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		out_pack.setPayload(printer.CStr() , printer.CStrSize() - 1);
	}
	try{
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}catch(...)
	{
		Logger::get("system")->log("erp " + yiqiding::utility::toString(_pac->getDeviceID())  + "Lost Connection");
	}

}


void ERPProcessor::processGetVolume()
{
	Json::Value root;
	Json::Reader reader;
	int sound = 3;
	int music = 50;
	int mic	  = 50;
	int power = 50;

	if (reader.parse(Singleton<Volume>::getInstance()->getVolumeInfo() , root))
	{
		if (root.isMember("sound") && root["sound"].isConvertibleTo(Json::intValue))
		{
			sound = root["sound"].asInt();
		}

		if (root.isMember("music") && root["music"].isConvertibleTo(Json::intValue))
		{
			music = root["music"].asInt();
		}

		if (root.isMember("mic") && root["mic"].isConvertibleTo(Json::intValue))
		{
			mic = root["mic"].asInt();
		}

		if (root.isMember("power") && root["power"].isConvertibleTo(Json::intValue))
		{
			power = root["power"].asInt();
		}
	}


	packet::Packet out_pack(_pac->getHeader());
	{

	
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		root->InsertEndChild(node = doc.NewElement("sound"));
		node->InsertEndChild(doc.NewText(toString(sound).c_str()));
		root->InsertEndChild(node = doc.NewElement("music"));
		node->InsertEndChild(doc.NewText(toString(music).c_str()));
		root->InsertEndChild(node = doc.NewElement("mic"));
		node->InsertEndChild(doc.NewText(toString(mic).c_str()));
		root->InsertEndChild(node = doc.NewElement("power"));
		node->InsertEndChild(doc.NewText(toString(power).c_str()));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		out_pack.setPayload(printer.CStr() , printer.CStrSize() - 1);
	}
	try{
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}catch(...)
	{
		Logger::get("system")->log("erp get volume " + yiqiding::utility::toString(_pac->getDeviceID())  + "Lost Connection");
	}


}

void ERPProcessor::processCloudConfirm()
{
	tinyxml2::XMLDocument doc;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if(root == NULL)
		return sendErrorMessage("Miss root");
	tinyxml2::XMLElement* node = root->FirstChildElement("confirm");
	std::string confirm =  getText(node);
	bool result = Singleton<yiqiding::ktv::cloud::MusicCloud>::getInstance()->checkidentify(_pac->getIdentifier());
	if(confirm == "false")
	{
		yiqiding::utility::Logger::get("system")->log("cloudmusic confirm false");
		return;
	}
	if(result)
		Singleton<yiqiding::ktv::cloud::MusicCloud>::getInstance()->notify(true);
	else
		yiqiding::utility::Logger::get("system")->log("cloudmusic confirm identify error" + yiqiding::utility::toString(_pac->getIdentifier()));

	


}


void ERPProcessor::processSetVolume()
{
	tinyxml2::XMLDocument doc;
	Json::Value back;
	{
		std::string xml(_pac->getPayload(), _pac->getLength());
		doc.Parse(xml.c_str());
	}
	if (doc.ErrorID()) 
		return sendErrorMessage("Invalid XML document");

	tinyxml2::XMLElement* root = doc.FirstChildElement("root");
	if(root == NULL)
		return sendErrorMessage("Miss root");
	
	tinyxml2::XMLElement* node = root->FirstChildElement("sound");
	if(node == NULL || !yiqiding::utility::isLong(node->GetText()) )
		return sendErrorMessage("Miss sound");
	back["sound"] = yiqiding::utility::toInt(node->GetText());

	node = root->FirstChildElement("music");
	if(node == NULL || !yiqiding::utility::isLong(node->GetText()) )
		return sendErrorMessage("Miss music");
	back["music"] = yiqiding::utility::toInt(node->GetText());

	node = root->FirstChildElement("mic");
	if(node == NULL || !yiqiding::utility::isLong(node->GetText()) )
		return sendErrorMessage("Miss mic");
	back["mic"] = yiqiding::utility::toInt(node->GetText());

	node = root->FirstChildElement("power");
	if(node == NULL || !yiqiding::utility::isLong(node->GetText()) )
		return sendErrorMessage("Miss power");
	back["power"] = yiqiding::utility::toInt(node->GetText());

	packet::Packet out_pack(_pac->getHeader());
	{

		bool confirm = false;
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement *root;
		tinyxml2::XMLElement *node;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));	
		root->InsertEndChild(node = doc.NewElement("confirm"));
		confirm = Singleton<Volume>::getInstance()->save(VlPath, back.toStyledString());
		node->InsertEndChild(doc.NewText(toCString(confirm)));
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		out_pack.setPayload(printer.CStr() , printer.CStrSize() - 1);
	}
	try{
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}catch(...)
	{
		Logger::get("system")->log("erp " + yiqiding::utility::toString(_pac->getDeviceID())  + "Lost Connection");
	}

}

void ERPProcessor::processGetOnlineBox()
{
	auto box_connections = _server->getConnectionManager()->getConnectionBox();
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement *root;
	tinyxml2::XMLElement *node;
	tinyxml2::XMLElement *cnode;
	tinyxml2::XMLElement *dnode;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("status"));
	node->InsertEndChild(doc.NewText("0"));	
	root->InsertEndChild(node = doc.NewElement("ktvinfos"));
	{
		MutextReader mr(box::BoxInfoMan::getMutexWR());
		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			if (conn) {
				box::BoxInfoItem *item = box::BoxInfoMan::getInstace()->getItem(box_conn.second->getIP());
				node->InsertEndChild(cnode = doc.NewElement("item"));
				
				cnode->InsertEndChild(dnode = doc.NewElement("boxid"));
				dnode->InsertEndChild(doc.NewText(toString(item->getBoxId()).c_str()));
				
				cnode->InsertEndChild(dnode = doc.NewElement("ip"));
				dnode->InsertEndChild(doc.NewText(box_conn.second->getIP().c_str()));
				
				cnode->InsertEndChild(dnode = doc.NewElement("roomname"));
				dnode->InsertEndChild(doc.NewText(item->getRoomName().c_str()));
				
				cnode->InsertEndChild(dnode = doc.NewElement("roomno"));
				dnode->InsertEndChild(doc.NewText(item->getRoomNo().c_str()));
			}
		}
	}


	packet::Packet out_pack(_pac->getHeader());
	{
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);

		out_pack.setPayload(printer.CStr() , printer.CStrSize() - 1);
	}
	try{
		out_pack.dispatch(_conn);
		setOutPack(&out_pack);
	}catch(...)
	{
		Logger::get("system")->log("erp " + yiqiding::utility::toString(_pac->getDeviceID())  + "Lost Connection");
	}


}
#pragma endregion
