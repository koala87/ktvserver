#include "yiqiding/ktv/GameReward2.h"

using namespace yiqiding::ktv;

Json::Value Record2::get()
{
	Json::Value root;
	root["boxid"] = _boxid;
	root["roomno"] = _roomno;
	root["roomname"] = _roomname;
	root["score"] = _score;
	root["url"] = _url;
	root["status"] = _status;
	return root;
}


bool Reward2::add(yiqiding::ktv::Server *s ,unsigned int boxid ,
	const Json::Value &roomno ,
	const Json::Value &roomname, 
	const Json::Value &serial_id ,
	const Json::Value &name, 
	const Json::Value &singer,
	const Json::Value &award)
{
	_id = ++_sid;
	_boxid = boxid;
	_roomno = roomno;
	_roomname = roomname;
	_serial_id = serial_id;
	_name = name;
	_singer = singer;
	_award = award;
	_s = s;

	_time = ::time(NULL);
	
	_remaintime = 2 * 60 * 60;
	_status = State_DOING;
	_selectedboxid = 0;

	start(_remaintime * 1000 ,yiqiding::time::WHEEL_ONESHOT);
	notifyAddReward2();
	return true;
}

bool Reward2::ready(unsigned int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	if(_sets.count(boxid))
		return false;
	_sets.insert(boxid);
	return true;
}

bool Reward2::cancel(unsigned int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	if(_boxid != boxid)
		return false;

	if(!_map.empty())
		return false;

	_status = State_CANCELED;
	stop();
	
	alertEndReward2(1);

	return true;
}

bool Reward2::sing(unsigned int boxid ,
	const Json::Value &roomno ,
	const Json::Value &roomname, 
	const Json::Value &score ,
	const Json::Value &url)
{
	yiqiding::MutexGuard guard(_mutex);

	if(!_sets.count(boxid))
		return false;
	_sets.erase(boxid);
	_map[boxid] = Record2(boxid ,roomno , roomname , score , url);
	alertSingReward2(_id , boxid , roomno , roomname , score , url);
	return true;
}

bool Reward2::select(int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	if(!_map.count(boxid))
		return false;
	
	if(_selectedboxid != 0)
		return false;

	_status = State_ENDED;
	stop();
	
	
	_selectedboxid = boxid;
	notifyEndReward2();
	return true;
}

Json::Value Reward2::get()
{
	yiqiding::MutexGuard guard(_mutex);
	Json::Value root;
	Json::Value record(Json::arrayValue);
	root["id"] = _id;
	root["boxid"] = _boxid;
	root["roomno"] = _roomno;
	root["roomname"] = _roomname;
	root["award"] = _award;
	root["serial_id"] = _serial_id;
	root["name"] = _name;
	root["singer"] = _singer;
	root["time"] = _time;
	long remaintime = _remaintime - (::time(NULL) - _time);
	if(remaintime < 0)
		root["remaintime"] = 0 ;
	else
		root["remaintime"] = (unsigned int )remaintime;
	for each(auto k in _map)
	{
		record.append(k.second.get());
	}
	root["records"] = record;
	root["status"] = _status >=2 ? _status - 1:_status;
	root["selectedboxid"] = _selectedboxid;
	
	return root;
}

void  Reward2::alertMyEndedReward2()
{
	yiqiding::MutexGuard guard(_mutex);
	Json::Value root;
	root["id"] = _id;
	packet::Packet pack(packet::KTV_REQ_BOX_ALERT_ME_CALCEL_REWARD2);
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	try {
		_s->getConnectionManager()->sendToBox(_boxid , &pack);
	} catch (const std::exception& err) {
		;
	}

}

void Reward2::process()
{
	yiqiding::MutexGuard guard(_mutex);
	switch(_status)
	{
	case State_DOING:
		if(_map.empty())
		{
			_status = State_CANCELED;
		}
		else
		{
			_status = State_ENDING;
			start(10 * 60 * 1000 , yiqiding::time::WHEEL_ONESHOT);
		}
		alertFinishReward2();
		break;
	case State_ENDING:
		{
			
			int k = rand() %_map.size();
			int index = 0;
			for each(auto m in _map)
			{
				if(index++ == k)
				{
					_selectedboxid = m.first;
					_status = State_ENDED;
					notifyEndReward2();
					break;
				}
			}
			
		}
		break;
	}
}



void Reward2::notifyAddReward2()
{
	packet::Packet out(packet::KTV_REQ_BOX_NOTIFY_ADD_REWARD2);
	Json::Value root;

	root["boxid"] = _boxid;
	root["roomno"] = _roomno;
	root["roomname"] = _roomname;
	root["serial_id"] = _serial_id;
	root["name"] = _name;
	root["singer"] = _singer;
	root["award"] = _award;
	root["id"]  = _id;
	root["remaintime"] = (unsigned int)(_remaintime - (::time(NULL) - _time));

	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());

	auto box_connections = _s->getConnectionManager()->getConnectionBox();
	for each (auto box_conn in box_connections) {
		auto conn = box_conn.second->getConnection(_s);
		if (conn) {
			try {
				out.dispatch(conn);
			} catch (...) {}
			conn->release();
		}
	}
}

void Reward2::notifyEndReward2()
{
	packet::Packet out(packet::KTV_REQ_BOX_NOTIFY_ENDED_REWARD2);
	packet::Packet sel(packet::KTV_REQ_BOX_NOTIFY_ENDED_REWARD2);
	Json::Value root;
	Json::Value record(Json::arrayValue);
	root["id"]  = _id;
	root["boxid"] = _boxid;
	root["roomno"] = _roomno;
	root["roomname"] = _roomname;
	root["serial_id"] = _serial_id;
	root["name"] = _name;
	root["singer"] = _singer;
	root["award"] = _award;
	for each(auto k in _map)
	{
		record.append(k.second.get());
	}
	root["selectedboxid"] = _selectedboxid;
	root["records"] = record;
	root["selected"] = false;
	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	
	root["selected"] = true;
	msg = root.toStyledString();
	sel.setPayload(msg.c_str() , msg.length());

	for each (auto k in _map) {
		try{
			if(_selectedboxid != k.first)
				_s->getConnectionManager()->sendToBox(k.first , &out);
			else
				_s->getConnectionManager()->sendToBox(_selectedboxid , &sel);
		}catch(...){
			;
		}
	}
	

}

void Reward2::alertSingReward2(int id , 
	unsigned int boxid ,const Json::Value &roomno , 
	const Json::Value &roomname ,const Json::Value &score ,
	const Json::Value &url)
{
	packet::Packet out(packet::KTV_REQ_BOX_ALERT_PLAY_REWARD2);
	Json::Value root;
	root["id"]  = id;
	root["boxid"] = boxid;
	root["roomno"] = roomno;
	root["roomname"] = roomname;
	root["score"] = score;
	root["url"] = url;

	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());
	try {
		_s->getConnectionManager()->sendToBox(_boxid , &out);
	} catch (...) {
	}
}

void Reward2::alertFinishReward2()
{
	packet::Packet out(packet::KTV_REQ_BOX_ALERT_ENDING_REWARD2);
	Json::Value root;
	Json::Value record(Json::arrayValue);
	root["id"]  = _id;
	root["serial_id"] = _serial_id;
	root["name"] = _name;
	root["singer"] = _singer;
	root["award"] = _award;
	for each(auto k in _map)
	{
		record.append(k.second.get());
	}
	root["records"] = record;

	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());

	try {
		_s->getConnectionManager()->sendToBox(_boxid , &out);
	} catch (...) {
		;
	}
}

void Reward2::alertEndReward2(int type)
{
	packet::Packet out(packet::KTV_REQ_BOX_ALERT_CANCEL_REWARD2);
	Json::Value root;
	Json::Value record(Json::arrayValue);
	root["id"]  = _id;
	root["type"] = type;
	std::string msg = root.toStyledString();
	out.setPayload(msg.c_str() , msg.length());

	for each(auto k in _sets)
	{
			try {
			_s->getConnectionManager()->sendToBox(k , &out);
		} catch (...) {
			;
		}
	}
}

void Reward2::updateStatus(int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	if(!_map.count(boxid))
		return;
	_map[boxid].updateStatus(false);
}

void Reward2::updateStatus()
{
	yiqiding::MutexGuard guard(_mutex);
	stop();
	_status = State_RESUMED;
	alertEndReward2(0);
}



int GameReward2::add(yiqiding::ktv::Server *s, unsigned int boxid , 
	const Json::Value &roomno ,
	const Json::Value &roomname, 
	const Json::Value &serial_id ,
	const Json::Value &name, 
	const Json::Value &singer ,
	const Json::Value &award)
{
	yiqiding::MutexGuard guard(_mutex);
	int id = 0;
	Reward2 *reward2 = new Reward2();
	reward2->add(s , boxid , roomno , roomname , serial_id , name , singer , award);
	id = reward2->getId();
	_list.push_back(reward2);
	return id;
}

 bool GameReward2::select(int id , unsigned int boxid)
 {
	 yiqiding::MutexGuard guard(_mutex);
	 auto k = std::find_if(_list.begin() , _list.end() , Game2Pt(id));
	 if(k == _list.end())
		 return false;
	return (*k)->select(boxid);
 }

bool GameReward2::cancel(int id ,unsigned int boxid)
{
	yiqiding::MutexGuard guard(_mutex);
	auto k = std::find_if(_list.begin() , _list.end() , Game2Pt(id));
	if(k == _list.end())
		return false;
	return (*k)->cancel(boxid);
}

 bool GameReward2::ready(int id , unsigned int boxid)
 {
	 yiqiding::MutexGuard guard(_mutex);
	 auto k = std::find_if(_list.begin() , _list.end() , Game2Pt(id));
	 if(k == _list.end())
		 return false;
	 return (*k)->ready(boxid);
 }

 int Reward2::_sid = ::time(NULL);

 bool GameReward2::sing(int id , unsigned int boxid , 
	 const Json::Value &roomno , const Json::Value &roomname ,
	 const Json::Value &score , const Json::Value &url)
 {
	 yiqiding::MutexGuard guard(_mutex);
	 auto k = std::find_if(_list.begin() , _list.end() , Game2Pt(id));
	 if(k == _list.end())
		 return false;
	 return (*k)->sing(boxid , roomno , roomname , score , url);
 }

 Json::Value GameReward2::getAll()
 {
	 Json::Value root(Json::arrayValue);
	 yiqiding::MutexGuard guard(_mutex);
	 for each(auto k in _list)
	 {
		if( k->getRewardState() != Reward2::State_CANCELED 
			 && k->getRewardState() != Reward2::State_RESUMED)
		{
			root.append(k->get());
		}
	 }
	return root;
 }

 Json::Value GameReward2::getMy(int boxid)
 {
	 Json::Value root(Json::arrayValue);
	 yiqiding::MutexGuard guard(_mutex);
	 for each(auto k in _list)
	 {
		if(k->getBoxid() == boxid && k->getRewardState() != Reward2::State_RESUMED)
			root.append(k->get());
	 }
	 return root;
 }

 void GameReward2::updateStatus(int boxid)
 {
	  yiqiding::MutexGuard guard(_mutex);
	  for each(auto k in _list)
	  {
		  if(k->getBoxid() == boxid)
			  k->updateStatus();
		  else
			  k->updateStatus(boxid);
	  }
 }

