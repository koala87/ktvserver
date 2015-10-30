/**
 * KTV 与 Lua 交互的函数
 * Lohas Network Technology Co., Ltd
 * @author Yuchun Zhang
 * @date 2014.07.08
 */

#include "yiqiding/ktv/KTVLua.h"
#include "yiqiding/io/File.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/PacketProcessor.h"
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/sql/MySQL.h"
#include "yiqiding/ktv/Connection.h"
#include "yiqiding/net/TelNet.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/FireWarn.h"

using namespace yiqiding::utility;
using namespace yiqiding::ktv::packet;
using namespace yiqiding::ktv;





inline bool lua_ktv_string(lua_State *l , std::string &value , int &index)
{
	if(!lua_isstring(l , index))
		return false;
	value = lua_tostring(l , index);
	++index;
	return true;
}

inline bool lua_ktv_uint32(lua_State *l , uint32_t &value , int &index) 
{
	if(!lua_isnumber(l , index))
		return false;
	value = lua_tounsigned(l , index);
	++index;
	return true;
}

inline bool lua_ktv_lightuserdata(lua_State *l , void *& value , int &index)
{
	if(!lua_islightuserdata(l , index))
		return false;
	value = lua_touserdata(l , index);
	++index;
	return true;
}

inline bool lua_ktv_bool(lua_State *l , bool &value , int &index)
{
	if(!lua_isboolean(l , index))
		return false;
	int k = lua_toboolean(l , index);
	value = (k != 0);
	++index;
	return true;
}

//设置为火警 状态.
static int SetFireStatus(lua_State *L)
{
	int n = lua_gettop(L);
	if (n < 1)
		return 0;

	bool status = (bool)lua_toboolean(L , 1);
	Singleton<yiqiding::ktv::fire::FireWarn>::getInstance()->setStatus(status);

	lua_pushboolean(L , true);
	return 1;
};

//返回lightuserdata
static int AllocPacket(lua_State *L /*,uint32_t funNo , uint32_t deceivdId , uint32_t identify , const char *content */) 
{
	Packet *pac = new Packet();
	int n = lua_gettop(L);
	if(n < 4)
		return 0;
	if(!lua_isnumber(L ,1))
		return 0;
	uint32_t funNo = lua_tounsigned(L , 1);
	pac->setRequest(static_cast<Request>(funNo));
	if(!lua_isnumber(L ,2 ))
		return 0;
	uint32_t deceivdId = lua_tounsigned(L ,2);
	pac->setDeviceID(deceivdId);
	if(!lua_isnumber(L , 3))
		return 0;
	uint32_t identify = lua_tounsigned(L , 3);
	pac->setIdentifier(identify);
	if(!lua_isstring(L , 4))
	{
		lua_pushlightuserdata(L , pac);
		return 1;
	}
	const char *content = lua_tostring(L , 4);
	pac->setPayload(content , strlen(content));

	lua_pushlightuserdata(L , pac);
	return 1;
}

//返回bool
static int FreePacket(lua_State *L )/* Packet *pac*/
{
	int n = lua_gettop(L);
	if(n < 1)
		return 0;
	if (!lua_islightuserdata(L ,1))
		return 0;
	void *value = lua_touserdata(L , 1); 
	Packet *pac = (Packet *)value;
	delete pac;
	lua_pushboolean(L , true);
	return 1;
}

//返回bool
static int SendBack(lua_State *L )/* Processor *process , Packet *pac )*/
{
	int n = lua_gettop(L);
	if (n < 2)
		return 0;
	if(!lua_islightuserdata(L ,1))
		return 0;
	void *value = lua_touserdata(L , 1);
	Processor *process = (Processor *)value;

	if (!lua_islightuserdata(L ,2))
		return 0;
	value = lua_touserdata(L , 2);
	Packet * pac = (Packet *) value;
	process->send(pac);
	process->setOutPack(pac);
	lua_pushboolean(L , true);
	return 1;
}

//返回是否成功
static int SendToBox(lua_State *L) /* uint32_t deceiveId , Server *server ,  Processor *process ,Packet *pac )*/
{
	int n = lua_gettop(L);
	if (n < 4)
	{
		return 0;
	}
	int index = 1;
	uint32_t deceiveId;
	void *value;
	if(!lua_ktv_uint32(L , deceiveId , index))
		return 0;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Packet *pac = (Packet *)value;
	try{
		server->getConnectionManager()->sendToBox(deceiveId , pac);
		process->setAttPack(pac , deceiveId);

		lua_pushboolean(L , true);
		return 1;
	}
	catch(const yiqiding::ktv::extended::BoxConnectionLost &)
	{
		;
	}
	return 0;
}

//返回是否成功
static int SendToAllBox(lua_State *L) /* Server *server ,  Processor *process ,Packet *pac)*/
{
	int n = lua_gettop(L);
	if (n < 3)
	{
		return 0;
	}

	int index = 1;
	void *value;
	
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;
	
	if(!lua_ktv_lightuserdata(L ,value , index))
		return 0;
	Processor *process = (Processor *)value;

	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Packet *pac = (Packet *)value;
	
	

	bool confirm = true;

	try {
		auto box_connections = server->getConnectionManager()->getConnectionBox();
		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(server);
			if (conn) {
				try {
					pac->dispatch(conn);
					confirm = true;
				} catch (...) {}
				conn->release();
			}
		}
	} catch (...) {
		confirm = false;
	} 

	if(!confirm)
	{	
		return 0;
	}

	process->setAttPack(pac , -1);
	lua_pushboolean(L , true);
	return 1;
}

//返回是否成功
static int SendToApp(lua_State *L)/* uint32 appid Server *server ,  Processor *process ,Packet *pac*/
{
	int n = lua_gettop(L);
	if (n < 4)
	{
		return 0;
	}
	int index = 1;
	uint32_t appid;
	void *value;
	if(!lua_ktv_uint32(L , appid , index))
		return 0;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;
	if (!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Packet *pac = (Packet *)value;

	try{
		server->getConnectionManager()->sendToApp(appid , pac);
		process->setAttPack(pac , appid);
		lua_pushboolean(L , true);
		return 1;
	}
	catch(const yiqiding::ktv::extended::AppConnectionLost &)
	{
		;
	}
	return 0;
}

//返回是否成功
static int SendToAllApp(lua_State *L)/* Server *server ,  Processor *process ,Packet *pac*/
{
	int n = lua_gettop(L);
	if (n < 3)
	{
		return 0;
	}
	int index = 1;
	void *value;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;
	if (!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Packet *pac = (Packet *)value;

	bool confirm;

	try {
		auto app_connections = server->getConnectionManager()->getConnectionApp();
		for each (auto app_conn in app_connections) {
			auto conn = app_conn.second->getConnection(server);
			if (conn) {
				try {
					pac->dispatch(conn);
					confirm = true;
				} catch (...) {}
				conn->release();
			}
		}
	} catch (...) {
		confirm = false;
	} 

	if(!confirm)
	{	
		return 0;	
	}

	process->setAttPack(pac , -1);
	lua_pushboolean(L , true);
	return 0;
}

//返回是否成功
static int SendToErp(lua_State *L)/* uint_32 role ,  Server *server ,  Processor *process ,Packet *pac*/
{
	int n = lua_gettop(L);
	if (n < 4)
	{
		return 0;
	}
	int index = 1;
	uint32_t role;
	void *value;

	if (!lua_ktv_uint32(L , role , index))
		return 0;
	if (!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;

	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;

	if (!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Packet *pac = (Packet *)value;

	try {
		server->getConnectionManager()->sendToERP((yiqiding::ktv::ERPRole)role , pac);
		process->setAttPack(pac , (yiqiding::ktv::ERPRole)role);
		lua_pushboolean(L , true);
		return 1;
	} catch(const yiqiding::ktv::ERPConnection &) {
		;
	}
	
	return 0;
}

static int SendToAllErp(lua_State *L)
{
	//printf("SendToAllErp");
	return 1;
}

//返回机顶盒编号
static int GetBoxIdFromCode(lua_State *L)/* std::string  , Server *server , */
{
	int n = lua_gettop(L);
	if (n < 2)
	{
		return 0;
	}
	int index = 1;
	std::string code;
	void *value;
	if(!lua_ktv_string(L , code , index))
		return 0;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Server *server = (Server *)value;

	if(code == "")
		return 0;

	int boxid = server->getConnectionManager()->getBoxFromCode(code);
	if(boxid == -1)
		return 0;

	lua_pushunsigned(L , boxid);
	return 1;

}

//返回成功
static int KtvLog(lua_State *L)/*const string &name , const string &content , uint_32 type*/
{
	int n = lua_gettop(L);
	if (n < 3)
	{
		return 0;
	}

	int index = 1;
	std::string name;
	std::string content;
	uint32_t type;

	if (!lua_ktv_string(L , name , index))
		return 0;

	if(!lua_ktv_string(L , content , index))
		return 0;

	if (!lua_ktv_uint32(L , type , index))
		return 0;

	Logger::get(name)->log(content , (yiqiding::utility::Logger::Level)type);
	lua_pushboolean(L , true);
	return 1;
}

//返回成功
static int SendErrorMessage(lua_State *L)/*Processor *process , string content */
{
	int n = lua_gettop(L);
	if (n < 2)
	{
		return 0;
	}

	int index = 1;
	void *value;
	std::string content;
	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;
	if (!lua_ktv_string(L ,content , index ))
		return 0;
	process->sendErrorMessage(content);
	lua_pushboolean(L , true);
	return 1;
}

//返回成功
static int SendErrorJsonMessage(lua_State *L)/* Processor process , uint32_t code , strint content*/
{
	int n = lua_gettop(L);
	if (n < 3)
	{
		return 0;
	}

	int index = 1;
	void *value;
	uint32_t code;
	std::string content;


	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	Processor *process = (Processor *)value;
	if(!lua_ktv_uint32(L , code , index))
		return 0;
	if (!lua_ktv_string(L ,content , index ))
		return 0;
	
	process->sendErrorJsonMessage(code , content);
	lua_pushboolean(L , true);
	return 1;
}

//返回成功
static int TelSend(lua_State *L) /* TelNet *srv , const  char *content  */
{
	int n = lua_gettop(L);
	if (n < 2)
	{
		return 0;
	}

	int index = 1;
	std::string content;
	void *value;

	if(!lua_ktv_lightuserdata(L , value , index))
		return 0;
	yiqiding::net::tel::TelNet *srv = (yiqiding::net::tel::TelNet *)value;
	if (!lua_ktv_string(L , content , index))
		return 0;
	srv->teleSend(content);
	lua_pushboolean(L , true);
	return 1;
}
/*
KTV Lua 上下文
*/
 static void packet::KTVSetLuaContext(lua_State *L , Server *server ,Processor *process , yiqiding::net::tel::ServerSend *srv)
{
	//上层table
	lua_newtable(L);

	lua_pushliteral(L , "SetFireStatus");
	lua_pushcfunction(L , SetFireStatus);
	lua_settable(L , -3);
	

	//
	lua_pushliteral(L , "nativeHandle");
	lua_pushlightuserdata(L , server);
	lua_settable(L , -3);


	//1 数据库tab
	lua_pushliteral(L , "MYSQL");
	lua_newtable(L);
	//
	{
		const yiqiding::sql::mysql::LoginInfo &info = server->getDatabase()->getLoginInfo();

		lua_pushliteral(L , "DataBase");
		lua_pushstring(L , info.getDatabaseName().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "Host");
		lua_pushstring(L , info.getHostname().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "Port");
		lua_pushnumber(L , info.getPort());
		lua_settable(L,-3);

		lua_pushliteral(L , "User");
		lua_pushstring(L , info.getUsername().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "PassWord");
		lua_pushstring(L , info.getPassword().c_str());
		lua_settable(L , -3);


		lua_pushliteral(L , "ErpDataBase");
		lua_pushstring(L , server->getErpSchema().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "ErpHost");
		lua_pushstring(L , server->getErpHostname().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "ErpPort");
		lua_pushnumber(L , server->getErpPort());
		lua_settable(L,-3);

		lua_pushliteral(L , "ErpUser");
		lua_pushstring(L , server->getErpUsername().c_str());
		lua_settable(L , -3);

		lua_pushliteral(L , "ErpPassWord");
		lua_pushstring(L , server->getErpPassword().c_str());
		lua_settable(L , -3);
	}
	lua_settable(L , -3);

	
	//2 请求req

	if(process != NULL)
	{
		lua_pushliteral(L , "REQ");
		lua_newtable(L);
		{
			lua_pushliteral(L , "nativeHandle");
			lua_pushlightuserdata(L , process);
			lua_settable(L , -3);

			lua_pushliteral(L , "functionNum");
			lua_pushunsigned(L , (uint32_t)process->_pac->getRequest());
			lua_settable(L , -3);

			lua_pushliteral(L , "identify");
			lua_pushunsigned(L , process->_pac->getIdentifier());
			lua_settable(L , -3);

			lua_pushliteral(L , "deviceId");
			lua_pushunsigned(L , process->_pac->getDeviceID());
			lua_settable(L , -3);

			lua_pushliteral(L , "body");
			lua_pushstring(L , std::string(process->_pac->getPayload() , process->_pac->getLength()).c_str());
			lua_settable(L , -3);

		}
		lua_settable(L , -3);
	}
	//3 packet   //4 回复消息
	lua_pushliteral(L , "PACK");
	lua_newtable(L);
	{
		lua_pushliteral(L , "AllocPacket");
		lua_pushcfunction(L , AllocPacket);
		lua_settable(L , -3);

		lua_pushliteral(L , "FreePacket");
		lua_pushcfunction(L , FreePacket);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendBack");
		lua_pushcfunction(L , SendBack);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToBox");
		lua_pushcfunction(L , SendToBox);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToAllBox");
		lua_pushcfunction(L , SendToAllBox);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToApp");
		lua_pushcfunction(L , SendToApp);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToAllApp");
		lua_pushcfunction(L , SendToAllApp);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToErp");
		lua_pushcfunction(L , SendToErp);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendToAllErp");
		lua_pushcfunction(L , SendToAllErp);
		lua_settable(L , -3);

		lua_pushliteral(L , "GetBoxIdFromCode");
		lua_pushcfunction(L , GetBoxIdFromCode);
		lua_settable(L , -3);

		lua_pushliteral(L , "KtvLog");
		lua_pushcfunction(L , KtvLog);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendErrorMessage");
		lua_pushcfunction(L , SendErrorMessage);
		lua_settable(L , -3);

		lua_pushliteral(L , "SendErrorJsonMessage");
		lua_pushcfunction(L , SendErrorJsonMessage);
		lua_settable(L , -3);

		
	}
	lua_settable(L , -3);

	//5 telnet
	if(srv != NULL)
	{
		lua_pushliteral(L , "TEL");
		lua_newtable(L);
		{
			lua_pushliteral(L , "nativeHandle");
			lua_pushlightuserdata(L , srv);
			lua_settable(L , -3);

			lua_pushliteral(L , "TelSend");
			lua_pushcfunction(L , TelSend);
			lua_settable(L , -3);

		}
		lua_settable(L , -3);
	}


	//服务器消息
	lua_setglobal(L ,"server");

	
}

static bool KtvdoLua(const char *path , Server *server , yiqiding::ktv::packet::Processor *process , yiqiding::net::tel::ServerSend *srv )
{
	lua_State* L;
	int r;
	if(!yiqiding::io::File::isExist(path))
		return false;
	L =  luaL_newstate();
	luaL_openlibs(L);
	KTVSetLuaContext(L ,server , process , srv);
	if( (r = luaL_loadfile(L, path)) != LUA_OK)
	{
		const char* error = lua_tostring(L, -1);
		Logger::get("system")->log(path + toString(" luaL_loadfile ") + error, Logger::WARNING);
		lua_pop(L, 1); 
		lua_close(L);
		return false;
	}
	if( (r = lua_pcall(L ,0 , 0 , 0 )) != LUA_OK)
	{
		const char* error = lua_tostring(L, -1);
		Logger::get("system")->log(path + toString(" lua_pcall ") + error , Logger::WARNING);
		lua_pop(L, 1); 
		lua_close(L);
		return false;
	}
	lua_close(L);
	return true;
}

bool KtvdoLua(uint32_t funNo ,Server *server ,  yiqiding::ktv::packet::Processor *process)
{
	std::ostringstream out;
	out << process->getDir() << funNo << ".lua";
	return KtvdoLua(out.str().c_str() , server , process , NULL);
}

bool KtvdoLua(const std::string &path , Server *server , yiqiding::net::tel::ServerSend *srv)
{
	return KtvdoLua(path.c_str() , server , NULL , srv);
}

