#pragma once
#include <string>
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/Thread.h"
namespace yiqiding{ namespace ktv{


const int max_diff = 60 * 10 * 100;

class LogDelayUpload:public yiqiding::Runnable
{
public:
	void load(const std::string &dir);
	void push(const std::string &filepath);
	void setServer(Server *server) {_server = server;}
	Server * getServer()		{ return _server;}
	LogDelayUpload();
	~LogDelayUpload();
private:
	virtual void run();
	Server	*_server;

private:
	std::queue<std::string > _queuepaths;
	yiqiding::Condition	_cond;
	yiqiding::Mutex		_mutex;
	yiqiding::Thread	_thread;
	bool				_loop;
};

}}