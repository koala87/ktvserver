/**
 * KTV Database Configuration Connector
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.01.26
 */

#pragma once
#include "yiqiding/ktv/db/ConfigModel.h"
#include "yiqiding/ktv/db/Definition.h"

namespace yiqiding { namespace ktv { namespace db {
	class ConfigConnector {
	private:
		ConnectionPool*	_pool;
	public:
		ConfigConnector(ConnectionPool* pool) : _pool(pool) {};
		~ConfigConnector() {};

		void updateFilterBlack(bool enable);

		std::auto_ptr<std::map<std::string , model::ConfigItem> > getConfigInfo(); 
	};
}}}
