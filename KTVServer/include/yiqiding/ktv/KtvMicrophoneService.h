#pragma once

#include <map>
#include "yiqiding/Thread.h"
#include "AacDecApi.h"
#include <stdint.h>
#include "yiqiding/ktv/Connection.h"


namespace yiqiding {
	class KtvMicrophoneService
	{
	public:
		struct RecvDataInfo {
			char* mpData;
			char* mpMicrophoneData;
			uint32_t mLen;
			uint32_t mKtvBoxId;
			uint32_t mPhoneId;
			uint32_t mMicrophoneDataLen;
		};
	public:
		KtvMicrophoneService(yiqiding::ktv::ConnectionManager*);
		~KtvMicrophoneService(void);
		void init();
		void dispatchData(RecvDataInfo* pRecvDataInfo);
		void noticeConnectLostApp(uint32_t appCode);

	public:
		yiqiding::Mutex mHandleMutexLock;
		yiqiding::Mutex mRecvMutexLock;

	private:
		class DataHandleThread : public yiqiding::Thread {
		public:
			DataHandleThread(KtvMicrophoneService* pOwn) : mpOwn(pOwn) {}
		protected:
			void run();  // handle mircophone data
		private:
			KtvMicrophoneService* mpOwn;
		};

		class UdpServerRecvThread : public yiqiding::Thread {
		public:
			UdpServerRecvThread(KtvMicrophoneService* pOwn) : mpOwn(pOwn) {}
		protected:
			void run();  // recvice ktvbox socket
		private:
			KtvMicrophoneService* mpOwn;
		};

	private:
		std::map<uint32_t , std::map<uint32_t ,std::queue<RecvDataInfo*> *> *> mMultiMapdatas;
		std::map<uint32_t , std::map<uint32_t , AacDecApi *> *> mMultiMapaac;
		std::map<uint32_t , bool> mMultiKtvboxState;
		UdpServerRecvThread* mpUdpServerRecvThread;
		DataHandleThread* mpDataHandleThread;
		yiqiding::ktv::ConnectionManager* mConn;
	};
}

