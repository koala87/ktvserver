/**
 * KTV Database Connector
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.13
 */

#pragma once

#include <string>
#include "yiqiding/ktv/db/Definition.h"
#include "yiqiding/ktv/db/ConfigConnector.h"
#include "yiqiding/ktv/db/MediaConnector.h"
#include "yiqiding/ktv/db/InfoConnector.h"
#include "yiqiding/ktv/db/CloudConnector.h"

namespace yiqiding { namespace ktv { namespace db {
	/// Although it was called a connector factory, it doest not new connectors.
	/// This is because all connections are created by ConnectionPool.
	class Database {
	private:
		sql::mysql::ConnectionPool	_pool;
		ConfigConnector				_config_connector;
		MediaConnector				_media_connector;
		InfoConnector				_info_connector;
		CloudConnector				_cloud_connector;
	public:
		Database() : _pool("utf8"), _config_connector(&_pool), _media_connector(&_pool), _info_connector(&_pool),_cloud_connector(&_pool) {};
		~Database() {};
		
		// Getter
		inline ConfigConnector*	getConfigConnector()	{ return &_config_connector; };
		inline MediaConnector*	getMediaConnector()		{ return &_media_connector; };
		inline InfoConnector*	getInfoConnector()		{ return &_info_connector; };
		inline CloudConnector* getCloudConnector()		{return &_cloud_connector;}
		inline const LoginInfo &		getLoginInfo()			{ return _pool.getLoginInfo();}
		// Setter
		inline void setLogin(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db, int port = 0)	{ _pool.setLoginInfo(sql::mysql::LoginInfo(host, user, pwd, db, port)); };
	
};
}}}

namespace yiqiding { namespace ktv {
	using db::Database;
}}
