#pragma once
#include <stdint.h>
#include <list>
#include "yiqiding/Thread.h"
#include "yiqiding/time/Timer.h"
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/net/TelNet.h"


namespace yiqiding{ namespace ktv{

	
	class Server;


	struct ScoreJiePai
	{
		int _perfect;
		int _great;
		int _good;
		int _ok;
		int _miss;
	};

	
	struct ChallengeScore
	{
		int _normal;		//1 0 -1
		int _score;
		int _yinzhun;
		int _wending;
		int _biaoxianli;
		int _jiezou;
		ScoreJiePai _jiepai;
		int _jiqiao;

	};
	
	
	
	class KTVChallenge:public yiqiding::time::Timer
	{

	public:
		enum GameState{
			INIT_TIMEOUT,
			JOIN_TIMEOUT,
			UPLOAD_TIMEOUT,
			END_TIMEOUT,
		};

	private:
		uint32_t						_fromboxid;
		ChallengeScore					_fromscore;
		uint32_t						_toboxid;
		ChallengeScore					_toscore;
		uint32_t						_mid;
		GameState						_status;
		yiqiding::Mutex					_mutex;
		yiqiding::ktv::Server			*_server;

	public:
		KTVChallenge(yiqiding::ktv::Server *s , uint32_t fromboxid , uint32_t toboxid , uint32_t mid);
		uint32_t getFromBoxid()				{return _fromboxid;}
		uint32_t getToBoxid()				{return _toboxid;}
		GameState getStatus()				{return _status;}
		virtual void process();
		void addtimer(GameState status);
		
		//返回是否结束,如果都上传积分。
		const ChallengeScore & getfromscore() { return _fromscore;}
		const ChallengeScore & gettoscore()	  { return _toscore;}
		bool setfromscore(const ChallengeScore &fromscore ); 
		bool settoscore(const ChallengeScore &toscore);		
		uint32_t	getmid()				{ return _mid;}
};


	class FindExistPred
	{
		uint32_t _boxid;
	public:
		FindExistPred(uint32_t boxid):_boxid(boxid){}

		//id 在 发送者 或 已接受的接受者 中。
		bool operator()(KTVChallenge *challenge)
		{
			return (challenge->getFromBoxid() == _boxid || (challenge->getToBoxid() == _boxid && challenge->getStatus() != KTVChallenge::INIT_TIMEOUT));
		}
	};

	class FindExistFPred
	{
		uint32_t _fromboxid;
	public:
		FindExistFPred(uint32_t boxid):_fromboxid(boxid){}
		bool operator()(KTVChallenge *challenge)
		{
			return (challenge->getFromBoxid() == _fromboxid && challenge->getStatus() == KTVChallenge::INIT_TIMEOUT);
		}
	};


	class FindExistFTPred
	{
		uint32_t _toboxid;
		uint32_t _fromboxid;
	public:
		FindExistFTPred( uint32_t fromboxid,uint32_t toboxid):_toboxid(toboxid),_fromboxid(fromboxid){}
		bool operator()(KTVChallenge *challenge)
		{
			return challenge->getFromBoxid() == _fromboxid && challenge->getToBoxid() == _toboxid;
		}

	};

	class RemoveEndPred
	{
	public:
		bool operator()(KTVChallenge * challenge)
		{
			return challenge->getStatus() == KTVChallenge::END_TIMEOUT;
		}

	};	




	class ManKTVChallenge
	{
	private:
		std::list<KTVChallenge *>		_lstChallenge;
		yiqiding::Mutex					_mutex;
		
		bool isInternelExist(uint32_t boxid);

	public:
		//添加
		bool addChallenge(yiqiding::ktv::Server *s ,uint32_t mid , uint32_t fromboxid , uint32_t toboxid);
		//取消
		bool cancelChallenge(uint32_t fromboxid );

		//是否存在
		bool isExist(uint32_t boxid);
		//加入
		bool joinChanllenge(uint32_t fromboxid , uint32_t toboxid);
		//拒绝
		bool rejectChanllenge(uint32_t fromboxid);
		//上传
		bool uploadChanllenge(Server *s , const ChallengeScore &score , uint32_t boxid);
		//获得对端的boxid
		int getPeerBoxid(uint32_t boxid);
		//显示
		int showAllKGame(yiqiding::net::tel::ServerSend *srv);
		
		static packet::Packet * generatorNotifyJoin(int join);
		static packet::Packet * generatorNotifyScoreTimerout();
		
};


}}
