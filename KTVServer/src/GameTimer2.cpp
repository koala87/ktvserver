#include "yiqiding/ktv/db/GameTimer2.h"
#include "json/json.h"
#include "yiqiding/utility/Utility.h"

using namespace yiqiding::ktv;
using namespace yiqiding::time;



void GameTimer2::calc(unsigned int timeofStart , unsigned int now)
{
	int graps = 30;

	if(timeofStart)
	{
		if(timeofStart - now > 4 *graps * 60) 
		{
			_notifytimes.push(graps * 60);
			_notifytimes.push(graps * 60);
			_notifytimes.push(graps * 60);
			_state = GAME2_NOTIFY;
			start((timeofStart - now - 4*graps*60)*1000 , WHEEL_ONESHOT);
		}
		else if(timeofStart - now > 3 *graps * 60) 
		{
			_notifytimes.push(graps * 60);
			_notifytimes.push(graps * 60);
			_state = GAME2_NOTIFY;
			start((timeofStart - now - 3*graps*60)*1000 , WHEEL_ONESHOT);

		}
		else if(timeofStart - now  > 2 * graps * 60)
		{

			_notifytimes.push(graps * 60);
			_state = GAME2_NOTIFY;
			start((timeofStart - now - 2*graps*60)*1000 , WHEEL_ONESHOT);

		}
		else if( timeofStart - now  > graps *60)
		{
			_state = GAME2_NOTIFY;
			start((timeofStart - now - graps*60)*1000 , WHEEL_ONESHOT);
		}
		else
		{
			_state = GAME2_JOIN;
			start((timeofStart - now)*1000 , WHEEL_ONESHOT);
		}

	}
	else
	{
		if(!doit(packet::KTV_REQ_BOX_INVITE_KGAME2 , _boxids.empty()?0:&_boxids))
		{
			donoscore();
			_state = GAME2_END;
			return ;
		}
		start(60*1000 , WHEEL_ONESHOT);
		_state = GAME2_START;
	}
}

void GameTimer2::add(const std::string &title , const std::string &desc , const std::string &award ,
	const std::string &songs , unsigned int timeofStart ,unsigned int num,const std::string &boxids, unsigned int id)
{
	unsigned int now = ::time(NULL);
	_game.setTitle(title);
	_game.setDesc(desc);
	_game.setAwards(award);
	_game.setSongs(songs);
	_game.setNum(num);
	_game.setTimeofStart(timeofStart?timeofStart: now);
	_game.setBoxids(boxids);
	_game.setScores("[]");
	Json::Value root;
	Json::Reader reader;
	if(reader.parse( boxids,root) && root.isArray())
	{
		for (int i = 0 ; i < root.size() ; ++i)
		{
			if(root[i].isConvertibleTo(Json::intValue))
				_boxids.insert(root[i].asInt());
		}
	}

	if(id == invalid_id)
		_game.setId(_server->getDatabase()->getMediaConnector()->addGame2(_game));
	else
		_game.setId(id);

	calc(timeofStart , now);
	
}



void GameTimer2::donoscore()
{
	_server->getDatabase()->getMediaConnector()->updateGame2(_game.getId() , "[]");
}

bool GameTimer2::doscore()
{


	sort(_results.begin() ,_results.end() , std::greater<GameTimer2::BoxResult>());

	packet::Packet out(packet::KTV_REQ_BOX_SCORE_KGAME2);
	Json::Value score;
	std::set<int> uploadscores;
	{
		Json::Value root;
		root["songs"] = yiqiding::utility::StrToJson(_game.getSongs());
		root["time"] = yiqiding::utility::getDateTime(_game.getTimeofStart()).c_str();
		root["awards"] = yiqiding::utility::StrToJson(_game.getAwards());
		root["title"] = _game.getTitle();
		root["desc"] = _game.getDesc();
		root["id"] = _game.getId();
		root["songs"] = yiqiding::utility::StrToJson(_game.getSongs());
		
		for each(auto b in _results)
			uploadscores.insert(b.boxid);

		std::auto_ptr<std::map<uint32_t , std::string> > RoomNames = yiqiding::ktv::box::BoxInfoMan::getInstace()->getRoomNames(uploadscores);

		for each(auto b in _results)
		{
			Json::Value item;
			item["boxid"] = b.boxid;
			item["roomname"] = (*RoomNames)[b.boxid].c_str();
			item["score"] = b.score;
			score.append(item);
		}
		root["scores"] = score;


		std::string msg = root.toStyledString();
		out.setPayload(msg.c_str() , msg.length());

	}

	_server->getDatabase()->getMediaConnector()->updateGame2(_game.getId() , score.toStyledString());

	auto box_connections = _server->getConnectionManager()->getConnectionBox();

	if (box_connections.size() == 0 )
	{
		return false;		
	}
	else{
		bool flag = false;

		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			if(!uploadscores.count(box_conn.first))
				continue;
			if (conn) {
				try {
					out.dispatch(conn);
					flag = true;
				} catch (...) {}
				conn->release();
			}
		}
		return flag;
	}

}

bool GameTimer2::doit( int request ,std::set
	<unsigned int > *ids)
{
	packet::Packet out((packet::Request)request);
	
	{
		Json::Value root;
		root["songs"] = yiqiding::utility::StrToJson(_game.getSongs());
		root["time"] = yiqiding::utility::getDateTime(_game.getTimeofStart()).c_str();
		root["awards"] = yiqiding::utility::StrToJson(_game.getAwards());
		root["title"] = _game.getTitle();
		root["desc"] = _game.getDesc();
		root["id"] = _game.getId();
		root["num"] = _game.getNum();
		std::string msg = root.toStyledString();

		out.setPayload(msg.c_str() , msg.length());

	}

	auto box_connections = _server->getConnectionManager()->getConnectionBox();

	if (box_connections.size() == 0 )
	{
		return false;		
	}
	else{
		bool flag = false;

		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			//ids null & ids hasn't boxid
			if(ids != NULL && !ids->count(box_conn.first))
				continue;
			if (conn) {
				try {
					out.dispatch(conn);
					flag = true;
				} catch (...) {}
				conn->release();
			}
		}
		return flag;
	}
}

void GameTimer2::process()
{
	unsigned int now = ::time(NULL);
	bool flag = true;
	switch(_state)
	{
	case GAME2_NOTIFY:
		
		doit(packet::KTV_REQ_BOX_NOTIFY_KGAME2 , _boxids.empty()?0:&_boxids);
		
		//Í¨ÖªÍê±Ï
		if(_notifytimes.empty())
		{
			start((_game.getTimeofStart() - now )*1000 , WHEEL_ONESHOT);
			_state = GAME2_JOIN;
		}
		else
		{
			unsigned int time = _notifytimes.front();
			start(time * 1000 , WHEEL_ONESHOT);
			_state = GAME2_NOTIFY;
			_notifytimes.pop();
		}

		break;
	case GAME2_JOIN:


		flag = doit(packet::KTV_REQ_BOX_INVITE_KGAME2 , _boxids.empty()?0:&_boxids);
		if(flag)
		{
			start(60 * 1000 , WHEEL_ONESHOT);
			_state = GAME2_START;
		}
		else
		{
			donoscore();
			_state = GAME2_END;
		}
		break;
	case GAME2_START:

		flag = doit(packet::KTV_REQ_BOX_START_KGAME2 , &_accepts);
		if(flag)
		{
			start(30 * 60 * 1000,WHEEL_ONESHOT);
			_state = GAME2_NO_SCORE;
		
		}
		else{
			donoscore();
			_state = GAME2_END;
		}
		break;
	case GAME2_NO_SCORE:

		donoscore();
		_state = GAME2_END;
		break;
	case GAME2_UPLOAD:
		
		doscore();
		_state = GAME2_END;
		break;

	case GAME2_END:
		break;
	}
}


bool GameTimer2::scores(int boxid , double score)
{
	
	if(_accepts.count(boxid) == 0)
		return false;
	if(_results.empty())
	{
		_state = GAME2_UPLOAD;
		start(15 * 1000 , WHEEL_ONESHOT);
	}
	_accepts.erase(boxid);
	BoxResult res = {score , boxid};
	_results.push_back(res);
	return true;
}

bool GameTimer2::accept(int boxid)
{
	if(!_boxids.empty() && !_boxids.count(boxid))
		return false;

	if(_accepts.count(boxid) != 0)
		return false;

	_accepts.insert(boxid);
	return true;
}


void KTVGameTimer2::add(yiqiding::ktv::Server * server ,const std::string &title , const std::string &desc , const std::string &award,
	const std::string &songs, unsigned int timeofStart,unsigned int num,const std::string &boxids,

	unsigned int id )
{
	MutexGuard guard(_mutex);
	GameTimer2 *game = new GameTimer2(server);
	game->add(title , desc , award , songs , timeofStart , num  ,  boxids, id);
	_vecGame2Timer.push_back(game);
}

bool KTVGameTimer2::del(yiqiding::ktv::Server *server , unsigned int id)
{
	MutexGuard guard(_mutex);
	auto t = std::find_if(_vecGame2Timer.begin() , _vecGame2Timer.end() , GamePr(id));
	if(t != _vecGame2Timer.end() && (*t)->getState() < GameTimer2::GAME2_START )
	{
		(*t)->setState(GameTimer2::GAME2_END);
		(*t)->stop();	
	}
	
	server->getDatabase()->getMediaConnector()->delGame2(id);

	return true;
	
}

bool KTVGameTimer2::scores(int id , int boxid , double score)
{
	MutexGuard guard(_mutex);
	auto t = std::find_if(_vecGame2Timer.begin() , _vecGame2Timer.end() , GamePr(id));
	if(t == _vecGame2Timer.end())
		return false;
	return (*t)->scores(boxid , score);
}

bool KTVGameTimer2::accept(int id , int boxid)
{
	MutexGuard guard(_mutex);
	auto t = std::find_if(_vecGame2Timer.begin() , _vecGame2Timer.end() , GamePr(id));
	if(t == _vecGame2Timer.end())
		return false;

	return (*t)->accept(boxid);
	
}

std::auto_ptr< std::vector<model::Game2> > KTVGameTimer2::getGames()
{ 
	MutexGuard guard(_mutex);
	std::auto_ptr< std::vector<model::Game2> > list(new std::vector<model::Game2>());
	for each(auto k in _vecGame2Timer)
	{
		if(k->getState() <= GameTimer2::GAME2_JOIN)
			list->push_back(k->getGame());
	}
	return list;
}



void KTVGameTimer2::load(yiqiding::ktv::Server *s)
{
	unsigned int now = ::time(NULL);
	bool state = false;
	std::auto_ptr< std::vector<model::Game2> > list(s->getDatabase()->getMediaConnector()->getAllGame2(0, 1000 , &state));
	for each(auto k in *list)
	{
		if(!k.getIsComplete())
		{
			if(k.getTimeofStart() <= now)
				s->getDatabase()->getMediaConnector()->updateGame2(k.getId() , "[]");
			else
				add(s , k.getTitle() , k.getDesc() , k.getAwards() , k.getSongs() , k.getTimeofStart() ,k.getNum(),k.getBoxids(), k.getId());
		}
	}
}





