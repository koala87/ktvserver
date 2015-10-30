/**
 * Packet Heart
 * @author Yuchun Zhang
 * @date 2015.01.20
 */
#include "yiqiding/time/Timer.h"
#include "yiqiding/net/TCPAD.h"
namespace yiqiding { namespace ktv{



	class HeartManger;



	class PacketHeart:yiqiding::time::Timer
	{

		enum State{FIRST , NORMAL};
		State												_status;
		size_t												_ticks;
		size_t												_id;
		yiqiding::net::tcp::async::ConnectionPool			*_pool;
		HeartManger											*_heartMan;
		yiqiding::Mutex										_mutex;
	public:
		PacketHeart(yiqiding::net::tcp::async::ConnectionPool *pool , HeartManger *heartMan , size_t id);
		virtual void process();
		void set();
		void unset();
	};

	


	class HeartManger
	{
		std::map<size_t , PacketHeart *>					_mapHearts;
		yiqiding::net::tcp::async::ConnectionPool			*_pool;
		yiqiding::Mutex										_mutex;
	public:
		HeartManger(yiqiding::net::tcp::async::ConnectionPool *pool);

		void add(int connId);
		void del(int connId);
		void set(int connId);
	};



}}