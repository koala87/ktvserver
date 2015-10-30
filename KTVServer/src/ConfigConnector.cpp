/**
 * KTV Database Configuration Connector Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.01.26
 */

#include <sstream>
#include "yiqiding/ktv/db/ConfigConnector.h"
using namespace yiqiding::ktv::db;

//////////////////////////////////////////////////////////////////////////

void ConfigConnector::updateFilterBlack(bool enable) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("UPDATE `config_resource` SET `value` = ? WHERE `cid` = '1'");
	stmt->setBoolean(1, enable);
	stmt->execute();

	_pool->returnConnection(conn);
}


std::auto_ptr<std::map<std::string , model::ConfigItem>> ConfigConnector::getConfigInfo()
{
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("select * from config_resource");
	 

	 ResultSet result = stmt->executeQuery();
	 std::auto_ptr<std::map<std::string ,model::ConfigItem >> mapConfig(new std::map<std::string , model::ConfigItem>);
	 model::ConfigItem item;
	 while (result->next()) {
		 
		 item.setCid(result->getInt(1));
		 item.setName(result->getString(2));
		 item.setValue(result->getString(3));
		 item.setDetail(result->getString(4));

		(*mapConfig)[item.getName()] = item;
	 }

	_pool->returnConnection(conn);

	return mapConfig;
}