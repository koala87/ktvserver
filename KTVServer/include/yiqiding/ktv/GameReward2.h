#pragma once
/*
K歌悬赏2的逻辑开发
*/
#include <string>
#include "json/json.h"
#include <map>
#include <set>
#include <vector>
#include "yiqiding/Thread.h"
#include "yiqiding/time/Timer.h"
#include "KTVServer.h"

namespace yiqiding { namespace ktv{

class Record2{
	unsigned int _boxid;
	Json::Value _roomno;
	Json::Value _roomname;
	Json::Value	_score;
	Json::Value	_url;
	bool		_status;
public:
	Record2(){}
	Record2(unsigned int boxid , const Json::Value &roomno ,
		const Json::Value &roomname , const Json::Value &score , const Json::Value &url)
		:_boxid(boxid),_roomno(roomno),_roomname(roomname),_score(score),_url(url),_status(true)
	{}
	void updateStatus(bool available){ _status = available;}
	unsigned int getBoxid() const	{return _boxid;}
	Json::Value get();
};


class Reward2:public yiqiding::time::Timer{
public:
		enum State{
		State_DOING = 0,	
		State_ENDING = 1,
		State_ENDED = 2,
		State_CANCELED = 3,
		State_RESUMED = 4,
	};
private:
	yiqiding::ktv::Server *_s;
	static	int	   _sid;
	yiqiding::Mutex	_mutex;
	int				_id;
	unsigned int	_boxid;
	Json::Value		_roomno;
	Json::Value		_roomname;
	Json::Value		_award;
	Json::Value		_serial_id;
	Json::Value		_name;
	Json::Value		_singer;
	unsigned int	_time;
	unsigned int	_remaintime;
	std::map<unsigned int,Record2> _map;
	std::set<unsigned int>	_sets;
	State			_status;	//
	unsigned int	_selectedboxid;
private:
	void notifyAddReward2();
	void alertFinishReward2();/* 提醒发起人 */
	void notifyEndReward2();
	void alertSingReward2(int id , 
		unsigned int boxid ,const Json::Value &roomno , 
		const Json::Value &roomname ,const Json::Value &score ,
		const Json::Value &url); /* 提醒发起人 */
	void alertEndReward2(int type);	/* 提醒正在参加者 */
	void alertMyEndedReward2();/* 超时结束 */
	void process();

public:
	bool add(yiqiding::ktv::Server *s , unsigned int boxid ,
		const Json::Value &roomno , const Json::Value &roomname,
		const Json::Value &serial_id ,const Json::Value &name,
		const Json::Value &singer, const Json::Value &award);
	bool sing(unsigned int boxid ,const Json::Value &roomno ,const Json::Value &roomname,
		const Json::Value &score , const Json::Value &url);
	bool ready(unsigned int boxid);
	bool cancel(unsigned int boxid);
	bool select(int boxid);
	Json::Value get();

	//getter
	int getId() const						{ return _id;}
	Reward2::State getRewardState() const	{ return _status;}
	unsigned int getBoxid() const			{ return _boxid;}

	//setter
	void updateStatus();
	void updateStatus(int boxid);					
};

class Game2Pt{
	int _id;
public:
	Game2Pt(int id):_id(id){}
	bool operator()(Reward2 *reward2)
	{
		return reward2->getId() == _id;
	}
};


class GameReward2{
	yiqiding::Mutex _mutex;
	std::vector<Reward2*> _list;

public:
	int add(yiqiding::ktv::Server *s ,unsigned int boxid , const Json::Value &roomno , const Json::Value &roomname,
		const Json::Value &serial_id , const Json::Value &name, const Json::Value &singer , const Json::Value &award);
	bool sing(int id , unsigned int boxid , const Json::Value &roomno , const Json::Value &roomname ,
		const Json::Value &score , const Json::Value &url);
	bool select(int id , unsigned int boxid);
	bool ready(int id , unsigned int boxid);
	bool cancel(int id ,unsigned int boxid);
	Json::Value getAll();
	Json::Value getMy(int boxid);
	void updateStatus(int boxid);
};
}}