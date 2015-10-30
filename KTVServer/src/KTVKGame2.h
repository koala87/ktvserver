#pragma once
/*
新版本的更新包厢信息，获取包厢信息，获取列表。
k歌新的版本
*/

#include <map>
#include "json/json.h"
#include "yiqiding/Thread.h"
#include "yiqiding/net/TelNet.h"
#include <vector>
//json
namespace yiqiding{ namespace social{




struct boxIn{

	Json::Value songpath;
	Json::Value songname;
	Json::Value songsinger;
	Json::Value songmid;
	Json::Value roomno;
	Json::Value roomname;
	Json::Value	optionreceive;
	Json::Value optionkgame;
	Json::Value	optiongame;
	Json::Value	optiongift;
	Json::Value optionmusic;
	Json::Value message;
	Json::Value	users;
};



class BoxSocial
{
	std::map<int , boxIn> _mapBox;
	yiqiding::Mutex _mutex;

public:
	void update(int boxid  , const boxIn &in);
	Json::Value get(int boxid);
	Json::Value getAll(int number , int page , int &tatol);


};


struct GameInfo{
	int fromboxid;
	int toboxid;
	bool one;
	unsigned int times;
	bool fromupload;
	bool toupload;
	int from_identitfy;
	int from_result_1;
	int from_result_2;
	int to_result_1;
	int to_result_2;
	int to_identity;
	Json::Value from_info;
	Json::Value to_info;
};


class KTVKGame2
{
private:

	yiqiding::Mutex _mutex;
	std::vector<GameInfo> _gameInfo;
public:
	void add(int fromboxid , int toboxid , bool one);// when another accept one's request.
	void update(int boxid , int result_1 , int reulst_2,  int identify ,const Json::Value &info);// when one upload score.
	void del(int boxid);  // del when exit or after send two scores. 
	bool check(int boxid);//check if all upload scores.
	bool get(int boxid , GameInfo &info);// get the scores when send two scores.
	int show(yiqiding::net::tel::ServerSend *srv);//in telnet show
};

class PrKTVGame
{
	int _boxid;
	unsigned int _ticks;
public:
	PrKTVGame(int boxid):_boxid(boxid),_ticks(GetTickCount()){
	}
	 bool operator ()(const GameInfo &game){
		return ((_boxid == game.fromboxid || _boxid == game.toboxid) && _ticks - game.times < 10 * 60 * (game.one ? 1 : 2)*1000);	 
	 }
};

}};