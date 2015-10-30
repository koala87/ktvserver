/**
 * KTV Database Server Information Connector
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.13
 */

#pragma once

#include "yiqiding/ktv/db/Definition.h"
#include "yiqiding/ktv/BoxInfoMan.h"


namespace yiqiding { namespace ktv { namespace db {
	class InfoConnector {
	private:
		ConnectionPool*	_pool;
	public:
		InfoConnector(ConnectionPool* pool) : _pool(pool) {};
		~InfoConnector() {};

		void updateBoxStatus(unsigned int boxid, bool open);

		bool getBoxStatus(unsigned int boxid);

		bool getBoxStatus(unsigned int boxid , unsigned int& time);

		//插入比赛结果
		void insertChallenge(uint32_t fromboxid , uint32_t toboxid ,int fromwin , int towin , int fromscore , int toscore , int mid); 

		//得到胜负平及最高分
		bool getChallenge(unsigned int boxid , int &win , int &lose, int &draw , int &score);

		void UpdateBoxInfo(const std::map<std::string , yiqiding::ktv::box::BoxInfoItem *>	 &mapboxs);
	};
}}}
