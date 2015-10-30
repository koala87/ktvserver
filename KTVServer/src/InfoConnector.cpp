/**
 * KTV Database Information Connector Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.13
 */

#include <sstream>
#include "yiqiding/utility/Utility.h"
#include "yiqiding/ktv/db/InfoConnector.h"
#include "yiqiding/utility/Logger.h"
using namespace yiqiding::ktv::db;

//////////////////////////////////////////////////////////////////////////

void InfoConnector::updateBoxStatus(unsigned int boxid, bool open) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("INSERT INTO `yiqiding_info`.`box_status`(`boxid`, `status`,`time`) VALUES(?, ? , ?) ON DUPLICATE KEY UPDATE `boxid` = VALUES(`boxid`), `status` = VALUES(`status`) , `time` = VALUES(`time`)");
	stmt->setUInt		(1, boxid);
	stmt->setBoolean		(2, open);
	stmt->setString(3 , yiqiding::utility::getDateTime(::time(NULL)));
	stmt->execute();

	_pool->returnConnection(conn);
}

void InfoConnector::insertChallenge(uint32_t fromboxid , uint32_t toboxid ,int fromwin , int towin , int fromscore , int toscore , int mid)
{
	Connection conn = _pool->getConnection();
	PreparedStatement stmt = conn->prepareStatement("insert into yiqiding_info.ktv_single_game(fromboxid ,toboxid,fromwin , towin , fromscore , toscore , mid) values(?,?,?,?,?,?,?)");
	stmt->setUInt(1,fromboxid);
	stmt->setUInt(2,toboxid);
	stmt->setInt(3,fromwin);
	stmt->setInt(4,towin);
	stmt->setInt(5,fromscore);
	stmt->setInt(6,toscore);
	stmt->setInt(7,mid);
	stmt->execute();

	_pool->returnConnection(conn);
}

bool InfoConnector::getChallenge(unsigned int boxid , int &win , int &lose, int &draw , int &score)
{
	Connection conn = _pool->getConnection();
	PreparedStatement stmt = conn->prepareStatement("select \
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.fromboxid = ? and fromwin = 1 ) +	\
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.toboxid = ? and towin = 1) as `win`,\
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.fromboxid = ? and fromwin = 0) + \
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.toboxid = ? and towin = 0) as draw,\
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.fromboxid = ? and fromwin = -1) + \
	(select count(*) from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.toboxid = ? and towin = -1) as lose, \
	(select max(fromscore) as maxcol from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.fromboxid = ?) as `fromscore`,\
	(select max(toscore) as maxcol from (select * from yiqiding_info.ktv_single_game where time >= DATE_add(curdate(),INTERVAL 6 HOUR) and time <= DATE_add(curdate() , INTERVAL 30 HOUR)) as t3 where t3.toboxid = ?) as `toscore`");
	stmt->setUInt(1 , boxid);
	stmt->setUInt(2 , boxid);
	stmt->setUInt(3 , boxid);
	stmt->setUInt(4 , boxid);
	stmt->setUInt(5 , boxid);
	stmt->setUInt(6 , boxid);
	stmt->setUInt(7 , boxid);
	stmt->setUInt(8 , boxid);

	ResultSet result = stmt->executeQuery();
	if(result->next())
	{
		int fromscore = 0;
		int toscore = 0;
		win = result->getInt("win");
		lose = result->getInt("lose");
		draw = result->getInt("draw");
		if(!result->isNull("fromscore"))
			fromscore = result->getInt("fromscore");
		if(!result->isNull("toscore"))
			toscore = result->getInt("toscore");
		score = fromscore > toscore? fromscore:toscore;

	}
	_pool->returnConnection(conn);

	return true;
}

bool InfoConnector::getBoxStatus(unsigned int boxid) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `status`,`time` FROM `yiqiding_info`.`box_status` WHERE `boxid` = ?");
	stmt->setUInt(1, boxid);
	ResultSet result = stmt->executeQuery();
	bool open = false;
	if (result->next() && result->getBoolean(1))
		open = true;
	
	_pool->returnConnection(conn);
	return open;
}

bool InfoConnector::getBoxStatus(unsigned int boxid , unsigned int& time)
{
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `status`,`time` FROM `yiqiding_info`.`box_status` WHERE `boxid` = ?");
	stmt->setUInt(1, boxid);
	ResultSet result = stmt->executeQuery();
	bool open = false;
	time = 0;
	if (result->next() )
	{
		open = result->getBoolean(1);
		time = yiqiding::utility::getDateTime(result->getString(2).c_str());
	}
	_pool->returnConnection(conn);
	return open;
}

void InfoConnector::UpdateBoxInfo(const std::map<std::string , yiqiding::ktv::box::BoxInfoItem *> &mapboxs)
{
	Connection conn = _pool->getConnection();
	{
		PreparedStatement stmt = conn->prepareStatement("delete from `yiqiding_info`.box_info");
		stmt->execute();
	}

	for each( auto item in mapboxs)
	{
		PreparedStatement stmt = conn->prepareStatement("insert into `yiqiding_info`.`box_info` values(? , ? , ? , ?)");
		stmt->setUInt(1 , item.second->getBoxId());
		stmt->setString(2 , item.second->getIp());
		stmt->setString(3 , item.second->getRoomNo());
		stmt->setString(4 , item.second->getRoomName());
		stmt->execute();
	}

	{
		PreparedStatement stmt = conn->prepareStatement("delete from publicsong_ip");
		stmt->execute();
	}

	for each(auto item in mapboxs)
	{
		PreparedStatement stmt = conn->prepareStatement("insert into publicsong_ip(type , ip) values(? , ?)");
		stmt->setString(1 , item.second->getType());
		stmt->setString(2 , item.second->getIp());
		stmt->execute();
	}

	_pool->returnConnection(conn);
	
}