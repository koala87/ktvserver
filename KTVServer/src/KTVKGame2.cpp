#include "KTVKGame2.h"
#include <sstream>
using namespace yiqiding::social;


void BoxSocial::update(int boxid , const boxIn &in)
{

	MutexGuard guard(_mutex);

	boxIn &m = _mapBox[boxid];

	if(in.songpath.type() != Json::nullValue)
		m.songpath = in.songpath;
	if(in.songname.type() != Json::nullValue)
		m.songname = in.songname;
	if(in.songsinger.type() != Json::nullValue)
		m.songsinger = in.songsinger;
	if(in.songmid.type() != Json::nullValue)
		m.songmid = in.songmid;
	if(in.roomno.type() != Json::nullValue)
		m.roomno = in.roomno;
	if(in.roomname.type() != Json::nullValue)
		m.roomname = in.roomname;
	if(in.optionreceive.type() != Json::nullValue)
		m.optionreceive = in.optionreceive;
	if(in.optionkgame.type() != Json::nullValue)
		m.optionkgame = in.optionkgame;
	if(in.optiongame.type() != Json::nullValue)
		m.optiongame = in.optiongame;
	if(in.optiongift.type() != Json::nullValue)
		m.optiongift = in.optiongift;
	if(in.optionmusic.type() != Json::nullValue)
		m.optionmusic = in.optionmusic;
	if(in.users.type() != Json::nullValue)
		m.users = in.users;
	if(in.message.type() != Json::nullValue)
		m.message = in.message;

		
	
	
}

Json::Value BoxSocial::get(int boxid)
{
	boxIn in;
	{
		MutexGuard guard(_mutex);
		if(_mapBox.count(boxid))
			in = _mapBox[boxid];
	}
	Json::Value v;	
	v["boxid"] = boxid;
	v["songpath"] = (in.songpath.type() == Json::nullValue)?"":in.songpath;
	v["songname"] = (in.songname.type() == Json::nullValue)?"":in.songname;
	v["songsinger"] = (in.songsinger.type() == Json::nullValue)?"":in.songsinger;
	v["songmid"] = (in.songmid.type() == Json::nullValue)?0:in.songmid;
	v["roomno"] = (in.roomno.type() == Json::nullValue)?"":in.roomno;
	v["roomname"] = (in.roomname.type() == Json::nullValue)?"":in.roomname;
	v["optionreceive"] = (in.optionreceive.type() == Json::nullValue)?false:in.optionreceive;
	v["optionkgame"] = (in.optionkgame.type() == Json::nullValue)?false:in.optionkgame;
	v["optiongame"] = (in.optiongame.type() == Json::nullValue)?false:in.optiongame;
	v["optiongift"] = (in.optiongift.type() == Json::nullValue)?false:in.optiongift;
	v["optionmusic"] = (in.optionmusic.type() == Json::nullValue)?false:in.optionmusic;
	v["message"] = in.message;
	if(in.users.type() == Json::nullValue)
		v["users"].resize(0);
	else
		v["users"] = in.users;
	
	return v;
}


Json::Value BoxSocial::getAll(int number , int page , int &total)
{
	Json::Value v0;
	v0.resize(0);
	int jump = number * page;
	int i = 0;
	int c = 0;
	total = 0;
	for each(auto k in _mapBox)
	{
		++total;
	}


	MutexGuard guard(_mutex);
	for each(auto k in _mapBox)
	{
		if(i >= jump)
		{
			if(c >= number)
				break;
			{
				const boxIn &in = k.second;
				Json::Value v;	
				v["boxid"] = k.first;
				v["songpath"] = (in.songpath.type() == Json::nullValue)?"":in.songpath;
				v["songname"] = (in.songname.type() == Json::nullValue)?"":in.songname;
				v["songsinger"] = (in.songsinger.type() == Json::nullValue)?"":in.songsinger;
				v["songmid"] = (in.songmid.type() == Json::nullValue)?0:in.songmid;
				v["roomno"] = (in.roomno.type() == Json::nullValue)?"":in.roomno;
				v["roomname"] = (in.roomname.type() == Json::nullValue)?"":in.roomname;
				v["optionreceive"] = (in.optionreceive.type() == Json::nullValue)?false:in.optionreceive;
				v["optionkgame"] = (in.optionkgame.type() == Json::nullValue)?false:in.optionkgame;
				v["optiongame"] = (in.optiongame.type() == Json::nullValue)?false:in.optiongame;
				v["optiongift"] = (in.optiongift.type() == Json::nullValue)?false:in.optiongift;
				v["optionmusic"] = (in.optionmusic.type() == Json::nullValue)?false:in.optionmusic;
				v["message"] = in.message;
				if(in.users.type() == Json::nullValue)
					v["users"].resize(0);
				else
					v["users"] = in.users;
				v0.append(v);
			}
			++c;
			
		}
		++i;
	}

	return v0;

}


void KTVKGame2::add(int fromboxid , int toboxid , bool one)
{
	MutexGuard guard(_mutex);
	GameInfo info = {fromboxid , toboxid , one , GetTickCount()};
	_gameInfo.push_back(info);
}

/*
为啥用反向迭代器，是因为可能 存在没有退出 或没有上传积分 ，错误的gameinfo存在。所以从后面找起。

*/



bool KTVKGame2::check(int boxid)
{
	MutexGuard guard(_mutex);
	std::vector<GameInfo>::reverse_iterator it = std::find_if(_gameInfo.rbegin() , _gameInfo.rend() , PrKTVGame(boxid));
	if(it ==_gameInfo.rend() )
		return false;
	return it->fromupload && it->toupload;
}

void KTVKGame2::update(int boxid , int result_1 , int reulst_2 , int identify ,const Json::Value &info)
{

	MutexGuard guard(_mutex);
	std::vector<GameInfo>::reverse_iterator it = std::find_if(_gameInfo.rbegin() , _gameInfo.rend() , PrKTVGame(boxid));
	if(it == _gameInfo.rend() )
		return ;

	if (it->fromboxid == boxid)
	{
		it->fromupload = true;
		it->from_result_1 = result_1;
		it->from_result_2 = reulst_2;
		it->from_identitfy = identify;
		it->from_info = info;
	}
	else if (it->toboxid == boxid)
	{
		it->toupload = true;
		it->to_result_1 = result_1;
		it->to_result_2 = reulst_2;
		it->to_identity = identify;
		it->to_info	= info;
	}
	
}

void KTVKGame2::del(int boxid)
{
	MutexGuard guard(_mutex);
	auto k = _gameInfo.rbegin();
	while(k != _gameInfo.rend())
	{
		if(k->fromboxid == boxid || k->toboxid == boxid)
		{
			_gameInfo.erase((++k).base());
			break;
		}
		++k;
	}
}

bool KTVKGame2::get(int boxid , GameInfo &info)
{
	MutexGuard guard(_mutex);
	std::vector<GameInfo>::reverse_iterator it = std::find_if(_gameInfo.rbegin() , _gameInfo.rend() , PrKTVGame(boxid));
	if(it == _gameInfo.rend() )
		return false;
	info = *it;
	return true;
}

int KTVKGame2::show(yiqiding::net::tel::ServerSend *srv)
{
	unsigned int ticks(GetTickCount());
	std::ostringstream out;
	
	{
		MutexGuard guard(_mutex);	
		for each(auto k in _gameInfo)
		{
			out << "from:" << k.fromboxid << " to:" << k.toboxid << " type: " << k.one << " fromupload:" 
				<< k.fromupload << " toupload:" << k.toupload << " time:" << (ticks - k.times)/1000 
				<< " fromscore:"<< k.from_result_1 << " toscore:" << k.to_result_1 
				<< "\r\n";
		}
	}

	return srv->teleSend(out.str());
}