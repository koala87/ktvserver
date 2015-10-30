/*
	K歌比赛实现
	2015-6-12 当时实现咋这么复杂。真是有些脑残。新的见GameTimer2.
*/

#include "yiqiding/time/Timer.h"
#include "yiqiding/Thread.h"
#include "yiqiding/ThreadW1Rn.h"
#include "yiqiding/ktv/PacketProcessor.h"
#include "KTVServer.h"
#include <string>
#include <set>
#include <map>
#include <vector>
#pragma  once

namespace yiqiding{ namespace ktv{
	class MidName;
	class BoxScore;
}}



namespace yiqiding{ namespace ktv{ namespace game{



enum EventState
{
	GAME_INIT = 0,				//初始状态，
	GAME_NOTIFY_TIMER = 1,		//定时器通知状态，
	GAME_JOIN_TIMER = 2,		//定时器加入状态	
	GAME_START_TIMER = 3,		//定时器开始状态
	GAME_SCORE_TIMER = 4,		//定时器第一个积分状态
	GAME_END_TIMER  = 5,		//定时器结束状态
	GAME_END	=	6,			//结束
};



const uint32_t TIME_START_AFTER_NOW = 5 * 60;			//开始时间大于当前时间5分钟
const uint32_t TIME_BEFORE_START = 1 * 60;				//开始时间前1分钟
const uint32_t TIME_END_AFTER_GAME = 15;				//上传积分后15秒
const uint32_t TIME_PER_SING_GAME_TIME = 10 * 60;		//每首歌的时间为10分钟


class GameTimer:public yiqiding::time::Timer
{
private:
	yiqiding::Mutex							_mutex;

	EventState								_state;
	yiqiding::ktv::Server*					_server;	
		

	uint32_t								_kid;					//k歌比赛的id
	bool									_type;					//类型 true 立即
	std::vector<MidName>					_mids;					//k歌比赛的歌曲id组
	std::set<uint32_t>						_notifytimes;			//通知时间组,有序
	uint32_t								_begin;					//开始时间
	std::string								_notifymessage;			//内容
	std::string								_notifytitle;			//标题


	std::set<uint32_t> _joins;						//参加列表
	std::vector<BoxScore> _scores;					//积分
	friend class SingerGame;

public:
	GameTimer(yiqiding::ktv::Server * s , uint32_t kid ):_server(s) , _kid(kid){ }
	~GameTimer(){}

	
	
	void beginSingeGame(bool type ,
		const std::vector<MidName> &mids ,
		const std::set<uint32_t> &notifytimes ,
		const std::string &notifymessage ,
		const std::string &notifytitle, 
		uint32_t begin );


	bool start(uint32_t second , yiqiding::time::WheelTimer t) { return yiqiding::time::Timer::start(second * 1000 , t);}


	static void sendOpenSingGame(yiqiding::ktv::packet::Processor *pro , packet::Packet *pac , ktv::KTVConnection *conn , uint32_t kid , bool confirm);
	static void sendModifySingGame(yiqiding::ktv::packet::Processor *pro , packet::Packet *pac , ktv::KTVConnection *conn , bool confirm);
	void sendNotify(bool flag );
	bool sendJoin( );
	void sendStart();
	
	void sendScoreError(int state);

	
	void sendScoreNoBodyJoin(){ sendScoreError(1);}
	void sendScoreNoBodyUpLoad(){sendScoreError(2);}
	void sendScoreNoBodyOnline(){ sendScoreError(3); }
	void sendScoreNormal();

	//Getter
	uint32_t getKid() const { return _kid; }
	const std::vector<MidName> & getMids() const{ return _mids;}
	bool getType() const {return _type;}
	const std::set<uint32_t> & getNotifyTimes() const{ return _notifytimes;}
	const std::string & getNotifyTitle() const { return _notifytitle;}
	const std::string & getNotifyMessage() const { return _notifymessage;}
	uint32_t getStartTime() const {return _begin;}
	EventState	getState() const { return _state;}

	//@thread safe
	void process();
	void joinSingeGame(uint32_t boxid);	
	void scoreSingeGame(uint32_t boxid , uint32_t score);
	bool modifySingeGame(const std::vector<MidName> &mids ,
		const std::set<uint32_t> &notifytimes ,
		const std::string &notifymessage ,
		const std::string &notifytitle, 
		uint32_t begin );
	bool checkTime(uint32_t starttime , int midNum);
	

	bool canDel();
	bool canJoin();
	bool canUpLoad();
	bool canModify();


	 
};


class SingerGame
{
private:
	// single
	static SingerGame*				_instance;	
	static yiqiding::MutexWR		_wr;
	static uint32_t					_kid;
	std::map<uint32_t , GameTimer *>  _mapTimer;
	
	~SingerGame();
	 void GCGameTimer();
	
public: 

	//@thread safe

	// single 
	static SingerGame* getInstace();
	static void unLoad();
	static yiqiding::MutexWR & getMutexWR () {return _wr;}
	static uint32_t getNewKid();

	// file i/o
	void load(yiqiding::ktv::Server *s ,const std::string &path);
	void save(const std::string &path);

	//  operator
	void add(GameTimer *gt);
	bool del(int kid);

	bool checkTime(uint32_t startTime , int midNum);

	bool checkTimeExcept(uint32_t startTime , int midNum , int exceptKid);

	//call start_get @non thread , must use with getMutexWR
	 GameTimer * get(int kid);

	
	
	 std::map<uint32_t , GameTimer *> & getTimers() { return _mapTimer;}
};

}}}
