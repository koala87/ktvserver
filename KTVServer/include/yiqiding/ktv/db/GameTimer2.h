/*

K歌大赛2


*/

#include "MediaModel.h"
#include "yiqiding/time/Timer.h"
#include "yiqiding/container/SmartVector.h"
#include "yiqiding/ktv/KTVServer.h"
#pragma once
namespace yiqiding{ namespace ktv{

	const unsigned int invalid_id = 0xffffffff;

	class GameTimer2:public yiqiding::time::Timer{
	public:
		enum Game2State{
			GAME2_NOTIFY,		//通知 发送通知
			GAME2_JOIN,			//发送 邀请
			GAME2_START,		//开始 发送开始
			GAME2_NO_SCORE,		//没有积分上传
			GAME2_UPLOAD,		//已经有积分上传
			GAME2_END
		};

		struct BoxResult{
			double score;
			unsigned int boxid;
			
			bool operator >(const BoxResult &b) const
			{
				return (score > b.score);
			}
					

		};

	private:
		std::queue<unsigned int>			_notifytimes;
		Game2State							_state;
		yiqiding::ktv::db::model::Game2		_game;
		std::set<unsigned int>				_accepts;
		std::vector<BoxResult>				_results;
		yiqiding::ktv::Server			*	_server;
		std::set<unsigned int>				_boxids;
		bool doit(int request , std::set<unsigned int> *ids = NULL);
		
		void process();
		bool doscore();
		void donoscore();
		void calc(unsigned int timeofStart , unsigned int now);
public:
		GameTimer2(yiqiding::ktv::Server * server):_server(server){}
		
		void add(const std::string &title , const std::string &desc , const std::string &award ,
			const std::string &songs , unsigned int timeofStart ,unsigned int num,const std::string &boxids, unsigned int id = 0xffffffff);
		bool accept(int boxid);
		bool scores(int boxid , double score);
		
		unsigned int getId()					{ return _game.getId();}
		Game2State getState()					{ return _state;}
		void	   setState(Game2State state)	{ _state = state;}
		const yiqiding::ktv::db::model::Game2 &getGame() const			{ return _game;}
	

};

	class GamePr
	{
	private:
		unsigned int _id;
	public:
		GamePr(int id):_id(id){}
		bool operator ()(GameTimer2 *game)
		{
			return _id == game->getId();
		}
	};



	class KTVGameTimer2{
		
	private:
		yiqiding::Mutex			 _mutex;
		std::vector<GameTimer2 *> _vecGame2Timer;
	public:
		void	add(yiqiding::ktv::Server * server ,const std::string &title , 
			const std::string &desc , 
			const std::string &award,
			const std::string &songs,
			unsigned int timeofStart , unsigned int num,
			const std::string &boxids,
			unsigned int id = invalid_id);
		bool    del(yiqiding::ktv::Server * server, unsigned int id);	
		bool accept(int id , int boxid);
		std::auto_ptr< std::vector<model::Game2> >  getGames();
		bool scores(int id , int boxid , double score);


		void load(yiqiding::ktv::Server *s);
	};



}}



