#include "yiqiding/ktv/PacketHeart.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/utility/Utility.h"
/**
 * Packet Heart
 * @author Yuchun Zhang
	连接上3s没有收到请求，则断开连接;之后30s没有收到请求，则断开连接。
 * @date 2015.01.20
 **/
using namespace yiqiding::ktv;

#define FIRST_TIMEOUT			(3000)
#define NORMAL_TIMEOUT			(30 * 1000)
#define DEVIATION_TIMEOUT		(100)

PacketHeart::PacketHeart(yiqiding::net::tcp::async::ConnectionPool *pool , HeartManger *heartMan ,  size_t id):_pool(pool),_heartMan(heartMan),_ticks(timeGetTime()),_id(id),_status(FIRST)
{
	start(FIRST_TIMEOUT , yiqiding::time::WHEEL_ONESHOT);
}

void PacketHeart::set()
{
	MutexGuard guard(_mutex);
	_ticks = timeGetTime();
}

void PacketHeart::unset()
{
	yiqiding::net::tcp::async::Connection *conn = _pool->get(_id);
	if(conn != NULL)
	{	
		conn->shutdown();
		conn->release();
		yiqiding::utility::Logger::get("system")->log("shutdown release");
	}


	_heartMan->del(_id);
}

void PacketHeart::process()
{
	size_t n;
	{
		MutexGuard guard(_mutex);
		n = timeGetTime() - _ticks;
	}

	int id = _id;

	switch(_status)
	{
	case FIRST:
		if(n >= FIRST_TIMEOUT - DEVIATION_TIMEOUT)
		{
			unset();
			yiqiding::utility::Logger::get("system")->log(yiqiding::utility::toString(id) + "first timeout");
		}
		else
		{
			_status = NORMAL;
			start(NORMAL_TIMEOUT, yiqiding::time::WHEEL_PERIODIC);		
		}
		break;
	case NORMAL:
		if( n >= NORMAL_TIMEOUT- DEVIATION_TIMEOUT)
		{
			unset();
			yiqiding::utility::Logger::get("system")->log(yiqiding::utility::toString(id) + "normal timeout");
		}
		break;
	}
	
	

}

HeartManger::HeartManger(yiqiding::net::tcp::async::ConnectionPool *pool):_pool(pool)
{

}


void HeartManger::add(int connId)
{
	MutexGuard guard(_mutex);
	if(_mapHearts.count(connId))
		return;
	_mapHearts[connId] = new PacketHeart(_pool , this,connId);
}


void HeartManger::del(int connId)
{
	MutexGuard guard(_mutex);
	if(!_mapHearts.count(connId))
		return;
	delete _mapHearts[connId];
	_mapHearts.erase(connId);
}

void HeartManger::set(int connId)
{
	MutexGuard guard(_mutex);
	if (!_mapHearts.count(connId))
		return;
	_mapHearts[connId]->set();
}
