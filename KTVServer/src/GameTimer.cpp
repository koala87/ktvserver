/*
		K歌比赛的实现
*/
#include "yiqiding/ktv/GameTimer.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/ktv/KTVServer.h"
#include "tinyxml2.h"
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/ktv/PacketProcessor.h"
#include "yiqiding/io/File.h"
#include "yiqiding/ktv/db/MediaModel.h"
#include   <algorithm> 
#include <iostream>

using namespace yiqiding::ktv::game;
using namespace yiqiding;
using namespace yiqiding::utility;
using namespace yiqiding::ktv;



bool GameTimer::canDel()
{
	MutexGuard guard(_mutex);
	return _state <= GAME_JOIN_TIMER;
}

bool GameTimer::canJoin()
{
	MutexGuard guard(_mutex);
	return _state == GAME_JOIN_TIMER;
}

bool GameTimer::canModify()
{
	MutexGuard guard(_mutex);
	return _state <= GAME_JOIN_TIMER;
}

bool GameTimer::canUpLoad()
{
	MutexGuard guard(_mutex);
	 if(_state == GAME_SCORE_TIMER)
	 {
		 yiqiding::time::Timer::stop();
		 start(TIME_END_AFTER_GAME , time::WHEEL_ONESHOT);
		_state = GAME_END_TIMER;
		return true;
	 }
	 else if( _state == GAME_END_TIMER)
		return true;
	 else
		 return false;
}
void GameTimer::sendOpenSingGame(ktv::packet::Processor *pro ,packet::Packet *pac , ktv::KTVConnection *conn , uint32_t kid , bool confirm)
{
	packet::Packet back_pack(pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("kid"));
		node->InsertEndChild(doc.NewText(toString(kid).c_str()));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	back_pack.dispatch(conn);
	pro->setOutPack(&back_pack);
}

void GameTimer::sendModifySingGame(ktv::packet::Processor *pro ,packet::Packet *pac , ktv::KTVConnection *conn , bool confirm)
{
	packet::Packet back_pack(pac->getHeader());
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("status"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("confirm"));
		node->InsertEndChild(doc.NewText(toCString(confirm)));

		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		back_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	back_pack.dispatch(conn);
	pro->setOutPack(&back_pack);

}

void GameTimer::sendNotify( bool flag)
{

	uint32_t now = (uint32_t)::time(NULL);
	if(flag)
	{
		packet::Packet out_pack(packet::KTV_REQ_BOX_GAME_NOTIFY);
		{
			tinyxml2::XMLDocument doc;
			tinyxml2::XMLElement* root;
			tinyxml2::XMLElement* node;
			std::string text;
			doc.InsertEndChild(root = doc.NewElement("root"));
			root->InsertEndChild(node = doc.NewElement("kid"));
			node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
			root->InsertEndChild(node = doc.NewElement("notifymessage"));
			node->InsertEndChild(doc.NewText(_notifymessage.c_str()));
			root->InsertEndChild(node = doc.NewElement("notifytitle"));
			node->InsertEndChild(doc.NewText(_notifytitle.c_str()));
			root->InsertEndChild(node = doc.NewElement("starttime"));
			node->InsertEndChild(doc.NewText(toString(_begin).c_str()));
			root->InsertEndChild(node = doc.NewElement("mids"));
			for each(auto midname in getMids())
			{
				tinyxml2::XMLElement *cnode;
				tinyxml2::XMLElement *croot;
				node->InsertEndChild(croot = doc.NewElement("item"));
				croot->InsertEndChild(cnode = doc.NewElement("mid"));
				cnode->InsertEndChild(doc.NewText(toString(midname._mid).c_str()));
				croot->InsertEndChild(cnode = doc.NewElement("name"));
				cnode->InsertEndChild(doc.NewText(midname._name.c_str()));

			}

			tinyxml2::XMLPrinter printer(0, true);
			doc.Print(&printer);
			out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		}

		auto box_connections = _server->getConnectionManager()->getConnectionBox();
		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			if (conn) {
				try {
					out_pack.dispatch(conn);
				} catch (...) {}
				conn->release();
			}
		}
	}

	std::set<uint32_t>::iterator it = _notifytimes.begin();
	while (it != _notifytimes.end())
	{
		if (*it > now)
		{
			break;
		}
		++it;
	}

	if (it != _notifytimes.end())
	{
		start(*it - now + 1 , time::WHEEL_ONESHOT);
		_state = GAME_NOTIFY_TIMER;

	}
	else if (_begin > now)
	{
		start(_begin - now - TIME_BEFORE_START , time::WHEEL_ONESHOT);
		_state = GAME_JOIN_TIMER;

	}
	else
	{
		Logger::get("system")->log("KGame:" + toString(_kid) + " now:" + toString(now) + " begin:" + toString(_begin), yiqiding::utility::Logger::WARNING);
	}

}

void GameTimer::sendScoreError(int state)
{
	
	_state = GAME_END;
	packet::Packet out_pack(packet::KTV_REQ_ERP_SCORE_UPLOAD);
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	std::string text;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("kid"));
	node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
	root->InsertEndChild(node = doc.NewElement("state"));
	node->InsertEndChild(doc.NewText(toString(state).c_str()));
	
	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);
	// Pack
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);

	try{
		_server->getConnectionManager()->sendToERP(ERP_ROLE_MEDIA , &out_pack);
	}
	catch(const ktv::extended::ERPConnectionLost &)
	{
		yiqiding::utility::Logger::get("system")->log("sendScoreError "+ toString(state) +" "+ toString((int)ERP_ROLE_MEDIA) + "disconnected" , Logger::WARNING);
	}

	{
		{
			yiqiding::ktv::db::model::Game game;
			game.setKId(_kid);
			game.setKMids(_mids);
			game.setKJoins(_joins);
			game.setKMessage(_notifymessage);
			game.setKTitle(_notifytitle);
			game.setKState(state);
			game.setKType(_type);
			game.setKScores(_scores);
			game.setKStartTime(_begin);
			game.setKSendTime(_notifytimes);

			_server->getDatabase()->getMediaConnector()->addGame(game);
		}
	}

	yiqiding::ktv::game::SingerGame::getInstace()->save("game.xml");

}

bool GameTimer::sendJoin( )
{
	uint32_t now = (uint32_t)::time(NULL);
	packet::Packet out_pack(packet::KTV_REQ_BOX_GAME_JOIN);
	{
		// Generate XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("kid"));
		node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
		root->InsertEndChild(node = doc.NewElement("notifymessage"));
		node->InsertEndChild(doc.NewText(_notifymessage.c_str()));
		root->InsertEndChild(node = doc.NewElement("notifytitle"));
		node->InsertEndChild(doc.NewText(_notifytitle.c_str()));
		root->InsertEndChild(node = doc.NewElement("starttime"));
		node->InsertEndChild(doc.NewText(toString(_begin).c_str()));
		root->InsertEndChild(node = doc.NewElement("mids"));
		for each(auto midname in getMids())
		{
			tinyxml2::XMLElement *cnode;
			tinyxml2::XMLElement *croot;
			node->InsertEndChild(croot = doc.NewElement("item"));
			croot->InsertEndChild(cnode = doc.NewElement("mid"));
			cnode->InsertEndChild(doc.NewText(toString(midname._mid).c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("name"));
			cnode->InsertEndChild(doc.NewText(midname._name.c_str()));

		}
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	
	auto box_connections = _server->getConnectionManager()->getConnectionBox();
	
	if (box_connections.size() == 0 )
	{
		return false;		
	}
	else{

		for each (auto box_conn in box_connections) {
			auto conn = box_conn.second->getConnection(_server);
			if (conn) {
				try {
					out_pack.dispatch(conn);
				} catch (...) {}
				conn->release();
			}
		}

		start(TIME_BEFORE_START , time::WHEEL_ONESHOT);
		_state = GAME_START_TIMER;
		return true;
	}


}

void GameTimer::sendStart()
{
	if (_joins.size() == 0)
	{
		sendScoreNoBodyJoin();
	}
	else
	{
		packet::Packet out_pack(packet::KTV_REQ_BOX_GAME_START);
		{
			// Generate XML
			tinyxml2::XMLDocument doc;
			tinyxml2::XMLElement* root;
			tinyxml2::XMLElement* node;
			std::string text;
			doc.InsertEndChild(root = doc.NewElement("root"));
			root->InsertEndChild(node = doc.NewElement("kid"));
			node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
	

			tinyxml2::XMLPrinter printer(0, true);
			doc.Print(&printer);
			// Pack
			out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
		}

		for each (auto boxid in _joins) {
			try{
			_server->getConnectionManager()->sendToBox(boxid , &out_pack);
			}
			catch(const ktv::extended::BoxConnectionLost &)
			{
				yiqiding::utility::Logger::get("system")->log("sendStart boxid:" + toString(boxid + " disconnected") , Logger::WARNING);
			}
		}

		start((uint32_t)_mids.size() * TIME_PER_SING_GAME_TIME , time::WHEEL_ONESHOT);
		_state = GAME_SCORE_TIMER;
	}
}




void GameTimer::sendScoreNormal()
{

	_state = GAME_END;
	sort(_scores.begin() , _scores.end() , std::greater<BoxScore>());
	
	////////////////
	{
		
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		{
			int index = 0;
			tinyxml2::XMLElement *croot;
			tinyxml2::XMLElement *cnode;
			doc.InsertEndChild(root = doc.NewElement("root"));
			root->InsertEndChild(node = doc.NewElement("kid"));
			node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
			root->InsertEndChild(node = doc.NewElement("scores"));


			std::set<int> uploadscores;
			for each(auto score in _scores)
				uploadscores.insert(score._boxid);

			std::auto_ptr<std::map<uint32_t , std::string> > RoomNames = yiqiding::ktv::box::BoxInfoMan::getInstace()->getRoomNames(uploadscores);
		
			for each(auto bc in _scores)
			{	
				node->InsertEndChild(croot = doc.NewElement("item"));
				croot->InsertEndChild(cnode = doc.NewElement("boxid"));
				cnode->InsertEndChild(doc.NewText(toString(bc._boxid).c_str()));
				croot->InsertEndChild(cnode = doc.NewElement("roomname"));
				cnode->InsertEndChild(doc.NewText((*RoomNames)[bc._boxid].c_str()));
				croot->InsertEndChild(cnode = doc.NewElement("score"));
				cnode->InsertEndChild(doc.NewText(toString(bc._score).c_str()));
			
				if (++index > 10)
				{
					break;
				}
			}
			root->InsertEndChild(node = doc.NewElement("selfscore"));
			
			root->InsertEndChild(node = doc.NewElement("sort"));
		}

		int index = 0;
		packet::Packet box_packet(packet::KTV_REQ_BOX_GAME_SORT , 2048);
		for each(auto bc in _scores)
		{			
			++index;
			node = root->FirstChildElement("selfscore");
			root->DeleteChild(node);
			node = root->FirstChildElement("sort");
			root->DeleteChild(node);
			root->InsertEndChild(node = doc.NewElement("selfscore"));
			node->InsertEndChild(doc.NewText(toString(bc._score).c_str()));
			root->InsertEndChild(node = doc.NewElement("sort"));
			node->InsertEndChild(doc.NewText(toString(index).c_str()));

			tinyxml2::XMLPrinter printer(0, true);
			doc.Print(&printer);
			// Pack
			box_packet.setData(printer.CStr(), printer.CStrSize() - 1);

			try{
				_server->getConnectionManager()->sendToBox(bc._boxid , &box_packet);
			}
			catch(const ktv::extended::BoxConnectionLost &)
			{
				Logger::get("system")->log("sendScoreNormal boxid:" +toString(bc._boxid) + " disconnected" , Logger::WARNING);
			}
		}

	}



	packet::Packet erp_packet(packet::KTV_REQ_ERP_SCORE_UPLOAD);
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement *croot;
		tinyxml2::XMLElement *cnode;
		std::string text;
		doc.InsertEndChild(root = doc.NewElement("root"));
		root->InsertEndChild(node = doc.NewElement("kid"));
		node->InsertEndChild(doc.NewText(toString(_kid).c_str()));
		root->InsertEndChild(node = doc.NewElement("state"));
		node->InsertEndChild(doc.NewText("0"));
		root->InsertEndChild(node = doc.NewElement("scores"));

		std::set<int> uploadscores;
		for each(auto score in _scores)
			uploadscores.insert(score._boxid);

		std::auto_ptr<std::map<uint32_t , std::string> > RoomNames = yiqiding::ktv::box::BoxInfoMan::getInstace()->getRoomNames(uploadscores);


		for each(auto bc in _scores)
		{
			node->InsertEndChild(croot = doc.NewElement("item"));
			croot->InsertEndChild(cnode = doc.NewElement("boxid"));
			cnode->InsertEndChild(doc.NewText(toString(bc._boxid).c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("roomname"));
			cnode->InsertEndChild(doc.NewText((*RoomNames)[bc._boxid].c_str()));
			croot->InsertEndChild(cnode = doc.NewElement("score"));
			cnode->InsertEndChild(doc.NewText(toString(bc._score).c_str()));
		}
		
		tinyxml2::XMLPrinter printer(0, true);
		doc.Print(&printer);
		// Pack
		erp_packet.setPayload(printer.CStr(), printer.CStrSize() - 1);
	}
	try{
	_server->getConnectionManager()->sendToERP(ERP_ROLE_MEDIA , &erp_packet);
	}
	catch(const ktv::extended::ERPConnectionLost &)
	{
		Logger::get("system")->log("sendScoreNormal "+ toString((int)ERP_ROLE_MEDIA) + " disconnected" , Logger::WARNING);
	}

	{
		yiqiding::ktv::db::model::Game game;
		game.setKId(_kid);
		game.setKMids(_mids);
		game.setKJoins(_joins);
		game.setKMessage(_notifymessage);
		game.setKTitle(_notifytitle);
		game.setKState(0);
		game.setKType(_type);
		game.setKScores(_scores);
		game.setKStartTime(_begin);
		game.setKSendTime(_notifytimes);
		
		_server->getDatabase()->getMediaConnector()->addGame(game);
	}

	SingerGame::getInstace()->save("game.xml");


}

void GameTimer::process()
{

	MutexGuard guard(_mutex);
	try{
		switch (_state)
		{
		case GAME_NOTIFY_TIMER:
			sendNotify(true);
			break;
		case GAME_JOIN_TIMER:
			{
				if(!sendJoin())
				sendScoreNoBodyOnline();
			}
			break;
		case GAME_START_TIMER:
			sendStart();
			break;
		case GAME_SCORE_TIMER:
			sendScoreNoBodyUpLoad();
			break;
		case GAME_END_TIMER:
			sendScoreNormal();
			break;
		default:
			Logger::get("system")->log(toString(_kid) + "process state error" , Logger::WARNING);
			break;
		}
	}
	catch(const std::exception &e)
	{
		Logger::get("system")->log(e.what() , Logger::WARNING);
	}
}



void GameTimer::beginSingeGame(	bool type ,
	const std::vector<MidName> &mids ,
	const std::set<uint32_t> &notifytimes ,
	const std::string &notifymessage ,
	const std::string &notifytitle, 
	uint32_t begin )   
{
	_type = type;
	_mids = mids;
	_notifytimes = notifytimes;
	_notifymessage = notifymessage;
	_notifytitle = notifytitle;
	_begin = begin;
}

bool GameTimer::modifySingeGame(const std::vector<MidName> &mids ,
	const std::set<uint32_t> &notifytimes ,
	const std::string &notifymessage ,
	const std::string &notifytitle, 
	uint32_t begin )
{
	MutexGuard guard(_mutex);
	if (_state == GAME_NOTIFY_TIMER)
	{
		_mids = mids;
		_notifytimes = notifytimes;
		_notifymessage = notifymessage;
		_notifytitle = notifytitle;
		_begin = begin;
		stop();
		return true;
	}
	return false;
}

void GameTimer::joinSingeGame(uint32_t boxid)
{
	MutexGuard guard(_mutex);
	_joins.insert(boxid);
}

void GameTimer::scoreSingeGame(uint32_t boxid , uint32_t score)
{
	MutexGuard guard(_mutex);
	_scores.push_back(BoxScore(boxid , score));
}

bool GameTimer::checkTime(uint32_t starttime , int midNum)
{
	MutexGuard guard(_mutex);
	uint32_t curmim = starttime - TIME_BEFORE_START;
	uint32_t curmax = starttime + midNum * TIME_PER_SING_GAME_TIME + TIME_END_AFTER_GAME;
	uint32_t nextmin = _begin - TIME_BEFORE_START;
	uint32_t nextmax = _begin + _mids.size() * TIME_PER_SING_GAME_TIME + TIME_END_AFTER_GAME;
	
	

	if ( _state == GAME_END || curmim > nextmax || curmax < nextmin)
		return true;
	return false;

}
///////////////////////////////
//SingerGame


SingerGame * SingerGame::_instance = NULL;
MutexWR SingerGame::_wr;
uint32_t SingerGame::_kid = 0;

SingerGame * SingerGame::getInstace()
{
	if (_instance == NULL)
	{
		MutexWriter guard(_wr);
		if (_instance == NULL)
		{
			_instance = new SingerGame();
		}

	}

	return _instance;
}

void SingerGame::unLoad()
{
	MutexWriter guard(_wr);
	if (_instance != NULL)
	{
		delete _instance;
		_instance = NULL;
	}

}


uint32_t SingerGame::getNewKid()
{
	MutexWriter guard(_wr);
	return ++_kid;

}

void SingerGame::add(GameTimer *gt)
{
	MutexWriter guard(_wr);
	_mapTimer[gt->_kid] = gt;

	GCGameTimer();
}

 GameTimer * SingerGame::get(int kid)
{
	if (_mapTimer.count(kid))
	{
		return _mapTimer[kid];
	}

	return NULL;	
}

bool SingerGame::del(int kid)
{
	MutexWriter guard(_wr);
	std::map<uint32_t , GameTimer *>::iterator it = _mapTimer.find(kid);
	if (it != _mapTimer.end())
	{
		delete it->second;
		_mapTimer.erase(it++);
		return true;
	}
	return false;
	
}

void SingerGame::load(yiqiding::ktv::Server * s ,const std::string &path)
{
	MutexWriter guard(_wr);

	tinyxml2::XMLDocument doc;

	uint32_t maxkid  = s->getDatabase()->getMediaConnector()->getGameId();
	_kid = maxkid;

	doc.LoadFile(path.c_str());
	if (doc.ErrorID())
		return ;


	uint32_t now = (uint32_t)::time(NULL);

	
	{
		tinyxml2::XMLElement* root;
		tinyxml2::XMLElement* node;
		tinyxml2::XMLElement* croot;
		tinyxml2::XMLElement* cnode;
	

		root = doc.FirstChildElement("root");
		if (root == NULL)
			return;
		node = root->FirstChildElement("ktv");
		if (node == NULL)
			return;
		for (auto i = node->FirstChildElement("item") ; i != NULL ; i= i->NextSiblingElement("item"))
		{
			croot = i->FirstChildElement("kid");
			if (croot == NULL || !yiqiding::utility::isULong(croot->GetText()))
				break;
			uint32_t kid = yiqiding::utility::toUInt(croot->GetText());
			croot = i->FirstChildElement("notifytitle");
			if (croot == NULL || !croot->GetText())
				break;
			std::string notifytitle = croot->GetText();
			croot = i->FirstChildElement("notifymessage");
			if (croot == NULL || !croot->GetText())
				break;
			std::string notifymessage = croot->GetText();
			croot = i->FirstChildElement("starttime");
			if (croot == NULL || !yiqiding::utility::isULong(croot->GetText()))
				break;
			uint32_t starttime = yiqiding::utility::toUInt(croot->GetText());
			croot = i->FirstChildElement("typetime");
			if (croot == NULL)
				break;
			bool type =  yiqiding::utility::toBool(croot->GetText());
			std::set<uint32_t> notifytimes;
			croot = i->FirstChildElement("notifytimes");
			if(croot == NULL)
				break;
			for (auto j = croot->FirstChildElement("item"); j != NULL ; j = j->NextSiblingElement("item"))
			{
				if (!yiqiding::utility::isULong(j->GetText()))
					break;
				
				notifytimes.insert(yiqiding::utility::toUInt(j->GetText()));
			}

			croot = i->FirstChildElement("mids");
			if (croot == NULL)
				break;
			std::vector<MidName> mids;
			for (auto k = croot->FirstChildElement("item") ; k != NULL ; k = k->NextSiblingElement("item"))
			{
				cnode = k->FirstChildElement("mid");
				if (cnode == NULL || !yiqiding::utility::isULong(cnode->GetText()))
					break;
				uint32_t mid = yiqiding::utility::toUInt(cnode->GetText());
				cnode = k->FirstChildElement("name");
				if(cnode == NULL || !cnode->GetText())
					break;
				std::string name = cnode->GetText(); 
				
				mids.push_back(MidName(mid , name));
			}

			
			
			if (maxkid < kid)
			{	
				maxkid = kid;
				_kid = maxkid;
			}

			//时间已过
			if (starttime < now)
			{
				yiqiding::ktv::db::model::Game game;
				game.setKId(kid);
				game.setKMessage(notifymessage);
				game.setKStartTime(starttime);
				game.setKSendTime(notifytimes);
				game.setKMids(mids);
				game.setKState(4);
				game.setKTitle(notifytitle);
				game.setKType(type);
				try{
					s->getDatabase()->getMediaConnector()->addGame(game);
				}
				catch(const yiqiding::ktv::db::DBException &db)
				{
					Logger::get("system")->log("Kid " + toString(kid) + " add database error" + db.what(), Logger::WARNING);
				}
				catch(const std::exception &e)
				{
					Logger::get("system")->log(e.what() , Logger::WARNING);
				}
				continue;
			}

			if (_mapTimer.count(kid))
				continue;

			GameTimer *gt = new GameTimer(s , kid);
			gt->beginSingeGame(type , mids , notifytimes , notifymessage , notifytitle , starttime);
			gt->sendNotify(false);
			_mapTimer[kid] = gt;
		}
		
	}
	

	

}

void SingerGame::save(const std::string &path)
{
	MutexWriter guard(_wr);

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	tinyxml2::XMLElement* croot;
	tinyxml2::XMLElement* cnode;
	tinyxml2::XMLElement* droot;
	tinyxml2::XMLElement* dnode;
	std::string text;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("ktv"));
	for each(auto map in _mapTimer)
	{
		if(map.second->_state == GAME_END)
			continue;

		node->InsertEndChild(croot = doc.NewElement("item"));
		croot->InsertEndChild(cnode = doc.NewElement("kid"));
		cnode->InsertEndChild(doc.NewText(toString(map.second->getKid()).c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("notifytitle"));
		cnode->InsertEndChild(doc.NewText(map.second->getNotifyTitle().c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("notifymessage"));
		cnode->InsertEndChild(doc.NewText(map.second->getNotifyMessage().c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("starttime"));
		cnode->InsertEndChild(doc.NewText(toString(map.second->getStartTime()).c_str()));
		croot->InsertEndChild(cnode = doc.NewElement("typetime"));
		cnode->InsertEndChild(doc.NewText(toCString(map.second->getType())));
		croot->InsertEndChild(cnode = doc.NewElement("notifytimes"));
		for each(auto time in map.second->getNotifyTimes())
		{
			cnode->InsertEndChild(droot = doc.NewElement("item"));
			droot->InsertEndChild(doc.NewText(toString(time).c_str()));
		}
		croot->InsertEndChild(cnode = doc.NewElement("mids"));
		for each(auto mid in map.second->getMids())
		{
			cnode->InsertEndChild(droot = doc.NewElement("item"));
			droot->InsertEndChild(dnode = doc.NewElement("name"));
			dnode->InsertEndChild(doc.NewText(mid._name.c_str()));
			droot->InsertEndChild(dnode = doc.NewElement("mid"));
			dnode->InsertEndChild(doc.NewText(toString(mid._mid).c_str()));
		}
	}
	doc.SaveFile(path.c_str());

	//xml
	//<root>
	//<ktv>
	//<item> <kid><notifytitle><notitfymessage><starttime><typetime><notifytimes><mids>
	//</item>
	//</ktv>
	//</root>
}

bool SingerGame::checkTime(uint32_t startTime , int midNum)
{
	MutextReader mr(_wr); 

	for each(auto bc in _mapTimer)
	{
		if(!bc.second->checkTime(startTime , midNum))
			return false;
	}

	return true;

}

bool SingerGame::checkTimeExcept(uint32_t startTime , int midNum , int exceptKid)
{
	MutextReader mr(_wr);
	for each(auto bc in _mapTimer )
	{
		if(!bc.second->checkTime(startTime , midNum) && bc.first != exceptKid)
			return false;
		
	}
	return true;
}

SingerGame::~SingerGame()
{
	std::map<uint32_t , GameTimer*>::iterator it = _mapTimer.begin();
	while(it != _mapTimer.end())
	{	
		delete it->second;
		_mapTimer.erase(it++);	
	}
}

void SingerGame::GCGameTimer()
{
	std::map<uint32_t , GameTimer*>::iterator it = _mapTimer.begin();
	while(it != _mapTimer.end())
	{
		if (it->second->_state == GAME_END)
		{
			delete it->second;
			_mapTimer.erase(it++);
		}
		else
		{
			++it;
		}
	}
}