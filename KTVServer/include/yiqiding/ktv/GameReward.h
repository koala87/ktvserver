#pragma once
/*
K歌悬赏的逻辑开发
*/

#include "json/json.h"
#include <string>
#include "time.h"
#include "KTVServer.h"

namespace yiqiding { namespace ktv { 

class Reward  
{

	enum Status
	{
		UNFINISHED = 0,
		FINISHED = 1,
		LOCKED = 2,
		CANCELED = 3
	};

	static int _gid;
	
	int _id;
	int _boxid;


	Json::Value _roomno;
	Json::Value _roomname;
	Json::Value	_requirement;
	Json::Value _serial_id;
	Json::Value _name;
	Json::Value _singer;
	Json::Value	_gift;
	Json::Value	_user;
	Json::Value _record;
	Json::Value _failrecords;		//
	Json::Value _time;

	int			_lockboxid;			//锁定
	unsigned int _lockticks;				//
	Status		_status;
	yiqiding::ktv::Server *_server;
	
	void notifybox();
	void checklock();
public:
	Reward(yiqiding::ktv::Server *server , int boxid , Json::Value roomno ,
		Json::Value roomname , Json::Value requirement , 
		Json::Value serial_id , Json::Value name , Json::Value singer , 
		Json::Value gift , Json::Value user):_server(server),_id(++_gid),_boxid(boxid),_roomno(roomno),
		_roomname(roomname),_requirement(requirement),_serial_id(serial_id),_name(name),
		_singer(singer),_gift(gift),_user(user),_time((unsigned int)::time(NULL)),_lockboxid(0),_status(UNFINISHED),_record(Json::objectValue)
	{
		_failrecords.resize(0);
	}

	bool addRecord(int boxid , Json::Value record , bool finish);
	bool lock(int boxid);
	void cancel();
	Json::Value getValue();

	//getter
	int getId() const {return _id;}
	int getBoxId() const {return _boxid;}

};

class RewardPtr{
	int _id;
public:
	RewardPtr(int id):_id(id){}
	bool operator()(Reward *reward)
	{
		return _id == reward->getId();
	}
};


class GameReward{
	std::vector<Reward*> _list;
	yiqiding::Mutex		 _mutex;
	public:
	void addReward(yiqiding::ktv::Server *server , int boxid , Json::Value roomno ,
		Json::Value roomname , Json::Value requirement , 
		Json::Value serial_id , Json::Value name , Json::Value singer , 
		Json::Value gift , Json::Value user);
	bool addRecord(int id , int boxid , Json::Value record , bool finish);
	bool lock(int id , int boxid);
	bool cancelfromid(int id , int boxid);
	void cancelfromboxid(int boxid);
	Json::Value getValues();
};

 }}