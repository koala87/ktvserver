#pragma  once
#include "yiqiding/Thread.h"
#include "KTVServer.h"
#include <cstdint>
#include <map>
#include <set>
#include <queue>
#include <json/json.h>
namespace yiqiding { namespace ktv{








	class KTVPCM:public yiqiding::Runnable
	{
	private:

		struct PcmBuf{
			uint32_t	_self;
			char		*_data;
			int			_len;
			PcmBuf(uint32_t self ,const char *data , int len){
				_self = self;
				_data = new char[len];
				memcpy(_data , data , len);
				_len = len;
			}
			PcmBuf(){}

			void release()
			{
				delete []_data;
			}
		};


	


		class PcmItem{
			uint32_t				_boxid;
			std::string				_roomno;
			std::string				_roomname;
			yiqiding::ktv::Server	*_serv;

			bool					_status;
			uint32_t				_times;
			std::set<uint32_t>		_otherboxids;

			//void alertSenderListen(uint32_t boxid);
			void alertSenderRemove(uint32_t boxid , const Json::Value &message );
			void notifyReceiverStop();

		public:	
			PcmItem(yiqiding::ktv::Server *server , uint32_t boxid , std::string roomno , std::string roomname);
			void startPcm(uint32_t time);
			void stopPcm();
			void listenPcm(uint32_t boxid , uint32_t &time);
			void removePcm(uint32_t boxid , const Json::Value &message);
			
			//getter
			uint32_t getBoxid()  const { return _boxid;}

			//setter
		};

		class PrPcm{
			uint32_t _boxid;
		public:
			PrPcm(uint32_t boxid):_boxid(boxid){

			}
			bool operator()(const PcmItem *item){
				return _boxid == item->getBoxid();
			}
		};


		yiqiding::ktv::Server	*				_ser;

		yiqiding::Mutex							_pcm_data_mutex;
		Condition								_pcm_data_cond;
		Thread									_thread;
		bool									_loop;
		std::queue<PcmBuf>						_pcm_data_queue;
		
		yiqiding::Mutex							_pcm_command_mutex;
		std::vector<PcmItem *>					_pcmitems;

		
		void run();
	public:
		KTVPCM();
		~KTVPCM();
		
		void setServer(yiqiding::ktv::Server *server) { _ser = server;}
		
		bool transmit(uint32_t self , const char *data , int len);
		
		bool startPcm(uint32_t self , uint32_t boxid ,int millSecond );
		
		bool stopPcm(uint32_t self , uint32_t boxid);
		
		bool listenPcm(uint32_t boxid , uint32_t otherboxid , uint32_t selft , uint32_t other , uint32_t &time);
		
		bool removePcm(const Json::Value &message ,uint32_t boxid , uint32_t otherboxid , uint32_t self , uint32_t other);
	};


}}

