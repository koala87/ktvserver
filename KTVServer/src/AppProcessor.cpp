/**
 * App Processor Implementation
 * @author Yuchun Zhang
 * @date 2014.03.10
 */
#include "yiqiding/ktv/AppProcessor.h"
#include "yiqiding/ktv/KTVServer.h"
#include "json/json.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/KTVLua.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/MultiScreen.h"
using namespace yiqiding::ktv;
using namespace yiqiding::utility;


AppProcessor::AppProcessor(Server* server, AppConnection* conn, Packet* pac,const std::string &ip , int port) : packet::Processor(server, conn, pac , ip , port) 
{
	
}

void AppProcessor::onReceivePacket() {
	// Security check


	if (getConnection()->isRegistered()) {
		if (getConnection()->getAppID() != _pac->getDeviceID())
			 return sendErrorJsonMessage(packet::ERROR_DEVICE_ID_CHANGE ,"Authentication failure");
	} else{
	
			MutexGuard guard(getConnection()->getMutex());
			if (getConnection()->isRegistered()) {
				if (getConnection()->getAppID() != _pac->getDeviceID())
					return sendErrorJsonMessage(packet::ERROR_DEVICE_ID_CHANGE , "Authentication failure");
			}
			else
			{
				uint32_t appid = _pac->getDeviceID();
				bool flag = false;
				try {

					Packet packet(packet::KTV_NOTIFY_KICK_MESSAGE);
					Json::Value root;
					root["status"] = packet::BACK_NO_ERROR;
					std::string message = root.toStyledString();
					packet.setPayload(message.c_str() , message.length());
					_server->getConnectionManager()->sendToApp(appid , &packet);
					flag = true;

				} catch (const extended::AppConnectionLost& err) {
					flag = false;
				}

				if(flag)
				{
					Logger::get("server")->log("App id " + toString(appid) + " has been kicked", Logger::WARNING);
				}
				
				getConnection()->setAppID(appid);
				

				_server->getConnectionManager()->updateApp(_pac->getDeviceID(), _conn->getID());
			}		
	}

#ifdef _DEBUG
	_server->getConnectionManager()->getConnectionApp((uint32_t)getConnection()->getAppID())->record_push(_rdpac);
#endif
	 
	// Process Packet

	if (KtvdoLua(_pac->getRequest() ,_server , this))
		return;
	

	switch(_pac->getRequest())
	{
	case packet::APP_REQ_TURN_MESSAGE:			processTurnMsgToBox();	break;
	case packet::APP_REQ_TURN_MESSAGE2:			processTurnMsgToBox2();	break;

	case packet::BOX_REQ_PORT_INFO:				processPortInfo();			break;
	/*case packet::APP_REQ_VALIDATE_CODE:		processCheckTempCode();	break;*/
	case packet::GENERAL_REQ_KEEPALIVE:			proceessHeart();		break;
	case packet::APP_REQ_RES_URL_INFO:			processReqUrl();		break;
	case packet::BOX_REQ_OTHER_STATUS:			processOtherStatus();	break;
	/*case packet::APP_REQ_SCROLL_INFO:			processScrollInfo();	break; using lua script*/
	case packet::APP_REQ_BOX_ID:				processReqBoxCode();	break;
	default:
		sendErrorJsonMessage(packet::ERROR_REQ_NOT_SUPPORT , "Request not supported");
	}
}

void AppProcessor::processScrollInfo()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT ,  "Invalid json string");

	if ( !root.isMember("touid") || !root.isMember("words") )
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing touid or Missing words");

	if( !root["touid"].isString())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "touid must be string");

	if (!root["words"].isString())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "words must be string");

	std::string touid = root["touid"].asString();

	bool confirm = false;

	int boxid = _server->getConnectionManager()->getBoxFromCode(touid);
	if (boxid  == -1)
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_EXIST , "Code may be wrong!");

	std::string message;
	root.removeMember("touid");
	message = root.toStyledString();

	Packet packet(packet::KTV_REQ_BOX_SCROLL_INFO);
	packet.setPayload(message.c_str() , message.length());


	try{
		_server->getConnectionManager()->sendToBox(boxid , &packet);
		setAttPack(&packet , boxid);	
		confirm = true;
	}
	catch (const extended::BoxConnectionLost &) {
		confirm = false;
	}

	try {
		Packet back(_pac->getHeader());
		Json::Value newroot;
		if(confirm)
		{
			newroot["status"] = packet::BACK_NO_ERROR;
		}
		else
		{
			newroot["status"] = packet::ERROR_BOX_DOWN;
			newroot["error"] = "box has been down";
		}
		std::string msg = newroot.toStyledString();
		back.setPayload(msg.c_str() , msg.length());
		back.dispatch(_conn);

		setOutPack(&back);

	} catch (const extended::AppConnectionLost & err) {
		Logger::get("server")->log("Server Fail to send to App id" + toString(_pac->getDeviceID()), Logger::WARNING);
	}


}

// record app account info, others are the same as turnMsgToBox
void AppProcessor::processTurnMsgToBox2()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;

	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT ,  "Invalid json string");

	if ( !root.isMember("touid") || !root.isMember("message") )
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing touid or Missing message");

	if( !root["touid"].isString())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "touid must be string");

	if (!root.isMember("type") || !root["type"].isInt())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing type or type is not int");

	
	// record app user info
	Json::Value msg = root["message"];
	Logger::get("server")->log("app info:" + msg.toStyledString());

	if(msg.isObject() && msg.isMember("content")){
		Json::Value content = msg["content"];
		if(content.isMember("user")){
			std::string str = content["user"].toStyledString();
			_server->insertAppInfo(_pac->getDeviceID(), str);
			//Logger::get("server")->log("insert appinfo appId : " + toString(_pac->getDeviceID()) + "; data : " + str, Logger::NORMAL);
		}
	}

	int type = root["type"].asInt();

	std::string touid = root["touid"].asString();

	bool confirm = false;

	int boxid =	_server->getConnectionManager()->getBoxFromCode(touid);

	//Logger::get("server")->log("turn2 message boxid : " + toString(boxid) + ", virCode : " + touid, Logger::NORMAL);

	if (boxid == -1)
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_EXIST , "Code may be wrong!");

	std::string message;

	root.removeMember("touid");
	root["fromuid"] = _pac->getDeviceID();

	message = root.toStyledString();

	Packet packet(packet::KTV_REQ_BOX_TURN_MESSAGE);
	packet.setPayload(message.c_str() , message.length());


	try{
		_server->getConnectionManager()->sendToBox(boxid , &packet);
		setAttPack(&packet , boxid);	
		confirm = true;

		// update appId and boxid mapping
		_server->getConnectionManager()->updateAppBoxMapping(getConnection()->getAppID(), boxid);
		//Logger::get("server")->log("update app/box mapping: appId 90005: " + toString(getConnection()->getAppID()) + " : boxId : " + toString(boxid), 
		//	Logger::NORMAL);
	}
	catch (const extended::BoxConnectionLost &) {
		confirm = false;
	}

	try {
		Packet back(_pac->getHeader());
		Json::Value newroot;
		if(confirm)
		{
			newroot["status"] = packet::BACK_NO_ERROR;
		}
		else
		{
			newroot["status"] = packet::ERROR_BOX_DOWN;
			newroot["error"] = "box has been down";
		}
		newroot["uid"] = touid;
		newroot["type"] = type;
		std::string msg = newroot.toStyledString();
		back.setPayload(msg.c_str() , msg.length());
		back.dispatch(_conn);

		setOutPack(&back);

	} catch (const extended::AppConnectionLost & err) {
		Logger::get("server")->log("Server Fail to send to App id" + toString(_pac->getDeviceID()), Logger::WARNING);
	}
}

void AppProcessor::processTurnMsgToBox()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT ,  "Invalid json string");

	if ( !root.isMember("touid") || !root.isMember("message") )
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing touid or Missing message");

	if( !root["touid"].isString())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "touid must be string");

	if (!root.isMember("type") || !root["type"].isInt())
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing type or type is not int");
	
	int type = root["type"].asInt();

	std::string touid = root["touid"].asString();

	bool confirm = false;

	int boxid =	_server->getConnectionManager()->getBoxFromCode(touid);
	if (boxid == -1)
	return sendErrorJsonMessage(packet::ERROR_CODE_NOT_EXIST , "Code may be wrong!");
	
	

	std::string message;
	
	root.removeMember("touid");
	root["fromuid"] = _pac->getDeviceID();
		
	message = root.toStyledString();

	Packet packet(packet::KTV_REQ_BOX_TURN_MESSAGE);
	packet.setPayload(message.c_str() , message.length());


	try{
		_server->getConnectionManager()->sendToBox(boxid , &packet);
		setAttPack(&packet , boxid);	
		confirm = true;

		// update appId and boxid mapping
		_server->getConnectionManager()->updateAppBoxMapping(getConnection()->getAppID(), boxid);
		Logger::get("server")->log("update app/box mapping 90001: appId " + toString(getConnection()->getAppID()) + " to boxId " + toString(boxid), 
			Logger::NORMAL);
	}
	catch (const extended::BoxConnectionLost &) {
		confirm = false;
	}



	try {
		Packet back(_pac->getHeader());
		Json::Value newroot;
		if(confirm)
		{
			newroot["status"] = packet::BACK_NO_ERROR;
		}
		else
		{
			newroot["status"] = packet::ERROR_BOX_DOWN;
			newroot["error"] = "box has been down";
		}
		newroot["uid"] = touid;
		newroot["type"] = type;
		std::string msg = newroot.toStyledString();
		back.setPayload(msg.c_str() , msg.length());
		back.dispatch(_conn);

		setOutPack(&back);

	} catch (const extended::AppConnectionLost & err) {
		Logger::get("server")->log("Server Fail to send to App id" + toString(_pac->getDeviceID()), Logger::WARNING);
	}
}

/*
void AppProcessor::processCheckTempCode()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet"Invalid json string");

	if ( !root.isMember("code") )
		return sendErrorJsonMessage("Missing code message");

	if( !root["code"].isString())
		return sendErrorJsonMessage("Code must be String");


	std::string code = root["code"].asString();
	if(code == "")
		return sendErrorJsonMessage("code message is empty");

	int box_id = _server->getConnectionManager()->getBoxFromCode(code);
	
	Json::Value newroot;
	Json::Value value;
	
	

	if (box_id == -1)
	{
		value["id"] = -1;
		newroot["confirm"] = 0;
	}
	else
	{
		value["id"] = box_id;
		newroot["confirm"] = 1;
	}
	newroot["status"] = true;
	newroot["error"] = "";
	newroot["message"] = value;

	std::string content = newroot.toStyledString();

	Packet packet(_pac->getHeader());
	
	packet.setPayload(content.c_str() , content.length());


	try{
		packet.dispatch(_conn);

		setOutPack(&packet);

	}
	catch (const extended::ConnectionLost&) {
		Logger::get("server")->log("App id " + toString(_pac->getDeviceID()) + ": Fail to Validate Code" , Logger::WARNING);
	}

}
*/

void AppProcessor::processOtherStatus()
{
	std::string ip = Singleton<MultiScreen>::getInstance()->getMasterIp(_ip);
	if(ip == "")
	{
		return sendErrorJsonMessage(packet::ERROR_FORMAT , "no invalid ip");
	}

	unsigned int boxid = yiqiding::ktv::box::BoxInfoMan::getInstace()->getItem(ip)->getBoxId();

	yiqiding::utility::Logger::get("system")->log(ip + " : " + toString(boxid));

	Json::Value root;
	unsigned int now;
	bool confirm = _server->getDatabase()->getInfoConnector()->getBoxStatus(boxid , now);
	root["room"] = confirm;
	
	if(confirm)
		root["code"] = _server->getConnectionManager()->getBoxCode(boxid);
	else
		root["code"] = "";

	packet::Packet out(packet::BOX_REQ_OTHER_STATUS);
	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	try {
		out.dispatch(_conn);
		setOutPack(&out);
	} catch (const std::exception& err) {
		
	}


}


void AppProcessor::processPortInfo(){

	auto p = _server->getDatabase()->getConfigConnector()->getConfigInfo();
	if(p.get() == NULL)
		return sendErrorJsonMessage(packet::ERROR_SERVER_INTER , "database config_resource error");

	std::string portinfo = p->operator[]("portinfo").getDetail();
	Json::Value root;
	if(portinfo == "")
		return sendErrorJsonMessage(packet::ERROR_SERVER_INTER , "tool not set portinfo");

	if(portinfo != "")
	{
		root = StrToJson(portinfo);
		if(!root.isObject())
			return sendErrorJsonMessage(packet::ERROR_SERVER_INTER , "tool set error , not a json");
	}

	root["status"] = packet::BACK_NO_ERROR;

	std::string msg = root.toStyledString();
	packet::Packet out(_pac->getHeader());
	out.setPayload(msg.c_str() , msg.length());
	try{
		out.dispatch(_conn);
		setOutPack(&out);
	}catch(...){
		;
	}

}


void AppProcessor::processReqUrl()
{
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT ,  "Invalid json string");

	if ( !root.isMember("touid") || !root["touid"].isString() )
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing touid");

	std::string code = root["touid"].asString();


	uint32_t boxid = _server->getConnectionManager()->getBoxFromCode(code);
	if(boxid == -1)
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_EXIST , "Code may be wrong!");
	
	const Balancer::Node * node = _server->getBalancer()->get(boxid);
	if(node == NULL)
	{
		utility::Logger::get("server")->log("box_id" + toString(boxid) + "has no node" , Logger::WARNING);
		node = _server->getBalancer()->random();
	}
	

	try
	{
		Packet back(_pac->getHeader());
		Json::Value newroot;
		newroot["server"] = node->getHostname().c_str();
		newroot["id"]	  = (int)node->getID();
		if(!_server->getInfoServer())
			newroot["infoserver"] = node->getHostname().c_str();
		else
			newroot["infoserver"] = ("http://" + _server->getIPAddr()).c_str();
		newroot["status"] = packet::BACK_NO_ERROR;
		std::string msg = newroot.toStyledString();

		back.setPayload(msg.c_str() , msg.length());

		back.dispatch(_conn);
		setOutPack(&back);
	}
	catch(...)
	{
		utility::Logger::get("server")->log("processReqUrl app:" + toString(_pac->getDeviceID()) + "lost connection" , Logger::WARNING);
	}
}

void AppProcessor::processReqBoxCode(){
	std::string str(_pac->getPayload() , _pac->getLength());
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(str , root))
		return sendErrorJsonMessage(packet::ERROR_FORMAT ,  "Invalid json string");

	if ( !root.isMember("ip") || !root["ip"].isString() )
		return sendErrorJsonMessage(packet::ERROR_FORMAT  , "Missing ip");

	std::string ip = root["ip"].asString();

	std::string strCode = _server->getConnectionManager()->getBoxCodeFromIp(ip);
	if(strCode == "")
		return sendErrorJsonMessage(packet::ERROR_CODE_NOT_EXIST , "IP may be wrong!");	
	try
	{
		Packet back(_pac->getHeader());
		Json::Value root;
		root["id"] = strCode;
		return sendJsonMessage(root);
	}
	catch(...)
	{
		utility::Logger::get("server")->log("processReqBoxCode:" + toString(_pac->getDeviceID()) + "lost connection" , Logger::WARNING);
	}
}