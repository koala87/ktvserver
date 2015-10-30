#include "yiqiding/ktv/GameReward.h"

using  namespace yiqiding::ktv;

int Reward::_gid  = ::time(NULL);

bool Reward::addRecord(int boxid , Json::Value record , bool finish)
{
	checklock();

	if(_status != LOCKED || boxid != _lockboxid)
		return false;
	if(finish)
	{
		_record = record;
		_status = FINISHED;
	}
	else
	{
		_failrecords.append(record);
		_status = UNFINISHED;
	}

	return true;
}

void Reward::notifybox()
{
	packet::Packet pack(packet::KTV_REQ_BOX_NOTIFY_CANCEL);
	Json::Value root;
	root["id"] = _id;
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	try{
		_server->getConnectionManager()->sendToBox(_lockboxid , &pack);
	}catch(...){}
}

bool Reward::lock(int boxid)
{
	//time out
	checklock();

	if(_status != UNFINISHED)
		return false;
	_status = LOCKED;
	_lockboxid = boxid;
	_lockticks = GetTickCount();
	return true;
}


void Reward::checklock()
{
	unsigned int now = GetTickCount();
	if(_status == LOCKED && (now - _lockticks) / (60*1000) >= 10)
		_status = UNFINISHED;
}

void Reward::cancel()
{
	checklock();
	if(_status == LOCKED)
		notifybox();
	if(_status != FINISHED)
	_status = CANCELED;
}


Json::Value Reward::getValue()
{
	checklock();

	Json::Value item;
	item["id"] = _id;
	item["boxid"] = _boxid;
	item["roomno"] = _roomno;
	item["roomname"] = _roomname;
	item["requirement"] = _requirement;
	item["serial_id"] = _serial_id;
	item["name"] = _name;
	item["singer"] = _singer;
	item["failrecords"] = _failrecords;
	item["status"] = (int)_status;
	item["record"] = _record;
	item["user"] = _user;
	item["gift"] = _gift;
	item["time"] = _time;

	return item;
}

void GameReward::addReward(yiqiding::ktv::Server *server ,
	int boxid , Json::Value roomno , Json::Value roomname , Json::Value requirement , 
	Json::Value serial_id , Json::Value name , Json::Value singer , Json::Value gift , Json::Value user)
{
	yiqiding::MutexGuard guard(_mutex);
	Reward *reward = new Reward(server , boxid , roomno , roomname , requirement , serial_id  , name ,singer , gift , user);
	_list.push_back(reward);
}

bool GameReward::addRecord(int id , int boxid , Json::Value record , bool finish)
{
	yiqiding::MutexGuard guard(_mutex);
	auto iter = std::find_if(_list.begin() , _list.end() , RewardPtr(id));
	if(iter == _list.end())
		return false;
	return (*iter)->addRecord(boxid , record , finish);
}

bool GameReward::lock(int id , int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	auto iter = std::find_if(_list.begin() , _list.end() , RewardPtr(id));
	if(iter == _list.end())
		return false;
	return (*iter)->lock(boxid);
}

bool GameReward::cancelfromid(int id , int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	auto iter = std::find_if(_list.begin() , _list.end() , RewardPtr(id));
	if(iter == _list.end() || (*iter)->getBoxId() != boxid)
		return false;
	(*iter)->cancel();
	return true;
}

void GameReward::cancelfromboxid(int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	for each(auto k in _list)
	{
		if(k->getBoxId() == boxid)
		{
			k->cancel();
		}
	}
}

Json::Value GameReward::getValues()
{
	Json::Value root;
	yiqiding::MutexGuard guard(_mutex);
	for each(auto k in _list)
	{
		root.append(k->getValue());
	}
	return root;
}
