/**
 * Packet Processor Implementation
 * @author Shiwei Zhang
 * @date 2014.01.21
 */

#include <tinyxml2.h>
#include "yiqiding/ktv/PacketProcessor.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/KTVServer.h"
#include "json/json.h"
using namespace yiqiding::ktv::packet;
using yiqiding::utility::Logger;

#pragma comment(lib, "tinyxml2")

//////////////////////////////////////////////////////////////////////////
// Processor
//////////////////////////////////////////////////////////////////////////

void Processor::run() {
	try {
		try {
			onReceivePacket();
		} catch (const std::exception& e) {
			sendErrorMessage("Internal Server Error");
			Logger::get("server")->log(_conn->getAddressPort() + ": " + e.what(), Logger::WARNING);
		}
	} catch (const std::exception& e) {
		Logger::get("server")->log(_conn->getAddressPort() + ": " + e.what(), Logger::WARNING);
	}
}

void Processor::sendErrorMessage(const std::string& err_msg) {
	// Generate XML
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("status"));
	node->InsertEndChild(doc.NewText("1"));
	root->InsertEndChild(node = doc.NewElement("error"));
	node->InsertEndChild(doc.NewText(err_msg.c_str()));

	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);

	// Pack
	Packet out_pack(_pac->getHeader());
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);

	// Send to remote
	out_pack.dispatch(_conn);

	setOutPack(&out_pack);
}

void Processor::sendErrorJsonMessage(int code , const std::string &err_msg)
{
	Json::Value root;
	Json::Value value; 
	root["result"] = value;
	root["status"] = code;
	root["error"] = err_msg.c_str();

	std::string json = root.toStyledString();
	Packet out_pack(_pac->getHeader());
	out_pack.setPayload(json.c_str() , json.length());

	out_pack.dispatch(_conn);

	setOutPack(&out_pack);
}

void Processor::sendJsonMessage(const Json::Value &val)
{
	Json::Value root;
	Json::Value value; 
	root["result"] = val;
	root["status"] = 0;
	root["error"] = "";

	std::string json = root.toStyledString();
	Packet out_pack(_pac->getHeader());
	out_pack.setPayload(json.c_str() , json.length());

	out_pack.dispatch(_conn);

	setOutPack(&out_pack);
}
