#include "yiqiding/ktv/KtvMicrophoneService.h"

#include "yiqiding/net/Socket.h"
#include "yiqiding/io/PCMAudio.h"
#include "yiqiding/net/TCP.h"
#include "AudioMixing.h"
#include <windows.h>
#include "yiqiding/utility/Logger.h"

#define TCP_PORT (36629)
#define UDP_PORT (36628)
#define VER_CODE_LEN (4)
#define IPADDR_LEN (16)
#define MIX_MAX_LEN (4096)
#define MAX_MICROPHONE_COUNT (8)
#define MAX_MICROPHONE_QUEUE_COUNT (3)

#define DEBUG_MICROPHONE 

using namespace yiqiding;

void KtvMicrophoneService::DataHandleThread::run() {
	//printf("KtvMicrophoneService::DataHandleThread::run\n");
	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread::run", yiqiding::utility::Logger::NORMAL);
	// yiqiding::ktv::audio::PCMAudioOutPut output;
	std::vector<RecvDataInfo *> vecData;
	std::vector<AacDecApi *> aac;
	// output.start(2 , 44100 , 16);

	while(true) {
		if (mpOwn->mMultiMapdatas.empty() || mpOwn->mMultiMapaac.empty()) {
			Sleep(50);
			continue;
		}
		uint32_t ktvBoxId = 0;
		uint32_t phoneId = 0;
		{
			yiqiding::MutexGuard guard(mpOwn->mHandleMutexLock);
			for each(auto i in mpOwn->mMultiMapdatas) {
				ktvBoxId = i.first;
				if (mpOwn->mMultiKtvboxState[ktvBoxId] == false) {
					continue;
				}
				if(!i.second->empty()) {
					yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 1");
					std::map<uint32_t ,std::queue<RecvDataInfo*> *>* pSingleKtvBoxDataMap = i.second;
					yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 2");
					for (uint32_t n = 0; n < MAX_MICROPHONE_QUEUE_COUNT; n++) {
						yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 3");
						vecData.clear();
						aac.clear();
						yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 4");
						for each(auto j in (*pSingleKtvBoxDataMap)) {
							yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 5");
							phoneId = j.first;
							yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 6");
							if(!j.second->empty()) {
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 7");
								RecvDataInfo *data = j.second->front();
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 8");
								j.second->pop();
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 9");
								vecData.push_back(data);
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 10");
								std::map<uint32_t , AacDecApi *>* pSingleKtvBoxAacMap
									= mpOwn->mMultiMapaac[ktvBoxId];
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 11");
								aac.push_back((*pSingleKtvBoxAacMap)[phoneId]);
								//printf("KtvMicrophoneService::DataHandleThread push_back n = %d\n", n);
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread push_back n = " + utility::toString(n), yiqiding::utility::Logger::NORMAL);
							}
						}
						char mix[MAX_MICROPHONE_COUNT][MIX_MAX_LEN];
						char dest[MIX_MAX_LEN];
						unsigned char *pBufOut;
						int bufOutLen;
						int consumed;
						for(uint32_t k = 0 ; k < aac.size() ; k++){
							//printf("KtvMicrophoneService::DataHandleThread vecData[%d]->mMicrophoneDataLen = %d\n", k, vecData[k]->mMicrophoneDataLen);
							yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread vecData[" + utility::toString(k) +\
								"->mMicrophoneDataLen = " + utility::toString(vecData[k]->mMicrophoneDataLen), yiqiding::utility::Logger::NORMAL);
							aac[k]->DecodeBuffer((unsigned char *)vecData[k]->mpMicrophoneData ,
								vecData[k]->mMicrophoneDataLen , &pBufOut , &bufOutLen , &consumed);
							memcpy(((char *)mix )+ MIX_MAX_LEN * k  , pBufOut , bufOutLen);
						}
						uint32_t nRet =  AudioMixing(mix , MIX_MAX_LEN , aac.size() , dest);
						// delete vecData
						auto bg = vecData.begin();
						while(bg != vecData.end()) {
							free((*bg)->mpData);
							delete *bg;
							bg = vecData.erase(bg);
						}
						//printf("KtvMicrophoneService::DataHandleThread ktvBoxId = %d\n", ktvBoxId);
						yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread ktvBoxId = " +\
							utility::toString(ktvBoxId), yiqiding::utility::Logger::NORMAL);
						mpOwn->mConn->sendDataToBox(ktvBoxId, dest, MIX_MAX_LEN);
						yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread debug 0");
					}
					mpOwn->mMultiKtvboxState[ktvBoxId] = false;
				}
			}
		}
	}
}

void KtvMicrophoneService::UdpServerRecvThread::run() {
	printf("KtvMicrophoneService::UdpServerRecvThread::run\n");
	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::UdpServerRecvThread::run", yiqiding::utility::Logger::NORMAL);

	SOCKET socketFd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); 

	struct sockaddr_in servaddr;
	char on = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == SOCKET_ERROR) {
		//printf("reuseaddr error\n");
		yiqiding::utility::Logger::get("server")->log("reuseaddr error", yiqiding::utility::Logger::NORMAL);
	}
	u_long bio = 1;
	uint32_t nSendBuf = 1024 * 1024 * 4;
	ioctlsocket(socketFd , FIONBIO , &bio);
	setsockopt(socketFd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	setsockopt(socketFd,SOL_SOCKET,SO_RCVBUF,(const char*)&nSendBuf,sizeof(int));

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);

	if (bind(socketFd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) {
		//printf("bind %d error\n", UDP_PORT);
		yiqiding::utility::Logger::get("server")->log("bind " + utility::toString(UDP_PORT) + " error", yiqiding::utility::Logger::NORMAL);
	}

	char pBuf[4096];

	struct sockaddr_in clientaddr;
	int fromLen = sizeof(clientaddr);

	while(true){
		// int nRet = recvfrom(socketFd , pBuf , sizeof(pBuf) , 0 , (struct sockaddr *)&clientaddr , &fromLen);
		Sleep(10);
		for (int i = 0; i < 400; i++) {
			pBuf[i] = 10;
		}
		int nRet = 400;
		if(nRet <= 0 ) {
			continue;
		}
		{
			yiqiding::MutexGuard guard(mpOwn->mRecvMutexLock);
			RecvDataInfo* pRecvDataInfo = new RecvDataInfo;
			char* pData = (char *)malloc(nRet);
			memset(pData, 0, nRet);
			memcpy(pData , pBuf , nRet);
			pRecvDataInfo->mLen = nRet;
			pRecvDataInfo->mpData = pData;
			mpOwn->dispatchData(pRecvDataInfo);
		}
	}
}

KtvMicrophoneService::KtvMicrophoneService(yiqiding::ktv::ConnectionManager* conn) {
	mConn = conn;
	init();
}

KtvMicrophoneService::~KtvMicrophoneService(void) {
	mpDataHandleThread->exit();
	mpUdpServerRecvThread->exit();
	delete(mpDataHandleThread);
	delete(mpUdpServerRecvThread);
	if (!mMultiMapaac.empty()) {
		for each(auto i in mMultiMapaac){
			std::map<uint32_t , AacDecApi *>* pSingleKtvBoxAacMap = i.second;
			if (!pSingleKtvBoxAacMap->empty()) {
				for each(auto j in (*pSingleKtvBoxAacMap)){
					delete j.second;
				}
			}
			delete pSingleKtvBoxAacMap;
		}
	}
	if (!mMultiMapdatas.empty()) {
		for each(auto i in mMultiMapdatas){
			std::map<uint32_t ,std::queue<RecvDataInfo*> *>* pSingleKtvBoxDataMap = i.second;
			if (!pSingleKtvBoxDataMap->empty()) {
				for each(auto j in (*pSingleKtvBoxDataMap)){
					for (uint32_t k = 0; k < j.second->size(); k++) {
						RecvDataInfo* pData = j.second->front();
						j.second->pop();
						delete pData->mpData;
						delete pData;
					}
					delete j.second;
				}
			}
			delete pSingleKtvBoxDataMap;
		}
	}
	yiqiding::net::Socket::Driver::unload();
}

void KtvMicrophoneService::init() {
	yiqiding::net::Socket::Driver::load();

	mpUdpServerRecvThread = new UdpServerRecvThread(this);
	mpDataHandleThread = new DataHandleThread(this);
	mpUdpServerRecvThread->start();
	mpDataHandleThread->start();
}

void KtvMicrophoneService::dispatchData(RecvDataInfo* pRecvDataInfo) {
	char verCode[VER_CODE_LEN] = {0};
	memcpy(verCode , pRecvDataInfo->mpData , VER_CODE_LEN);
	//printf("KtvMicrophoneService::dispatchData verCode[0] = %x, verCode[1] = %x,verCode[2] = %x, verCode[3] = %x\n",
	// 	verCode[0],verCode[1],verCode[2],verCode[3]);

	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData verCode[0] = " +\
		utility::toString(verCode[0]) + " verCode[1] = " + utility::toString(verCode[1]) + " verCode[2] = " +\
		utility::toString(verCode[2]) + " verCode[3] = " + utility::toString(verCode[3]), 
		yiqiding::utility::Logger::NORMAL);

	pRecvDataInfo->mpMicrophoneData = pRecvDataInfo->mpData + VER_CODE_LEN;
	pRecvDataInfo->mMicrophoneDataLen = pRecvDataInfo->mLen - VER_CODE_LEN;
	pRecvDataInfo->mPhoneId = 0;
	for(uint32_t i = 0; i < VER_CODE_LEN; i++) {
		pRecvDataInfo->mPhoneId = (int32_t)(pRecvDataInfo->mPhoneId | ((verCode[i] & 0x000000ff) << ((VER_CODE_LEN - 1 - i) * 8)));
	}
	//printf("KtvMicrophoneService::dispatchData mPhoneId = %d\n", pRecvDataInfo->mPhoneId);

	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData mPhoneId = " +\
		utility::toString(pRecvDataInfo->mPhoneId), 
		yiqiding::utility::Logger::NORMAL);

	//printf("KtvMicrophoneService::dispatchData mMicrophoneDataLen = %d\n", pRecvDataInfo->mMicrophoneDataLen);

	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData mMicrophoneDataLen = " +\
		utility::toString(pRecvDataInfo->mMicrophoneDataLen), 
		yiqiding::utility::Logger::NORMAL);

	// pRecvDataInfo->mKtvBoxId = mConn->getBoxIdFromAppId(pRecvDataInfo->mPhoneId);
	pRecvDataInfo->mKtvBoxId = 51;
	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData pRecvDataInfo->mKtvBoxId = " +\
		utility::toString(pRecvDataInfo->mKtvBoxId), 
		yiqiding::utility::Logger::NORMAL);
	{
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 0.1");
		yiqiding::MutexGuard guard(mHandleMutexLock);
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 0.2");
		if(!mMultiMapaac.count(pRecvDataInfo->mKtvBoxId)) {
			std::map<uint32_t , AacDecApi *>* pKtvBoxAacDecApiMap = new std::map<uint32_t, AacDecApi *>();
			mMultiMapaac[pRecvDataInfo->mKtvBoxId] = pKtvBoxAacDecApiMap;

			std::map<uint32_t ,std::queue<RecvDataInfo*> *> * pKtvBoxDataMap
				= new std::map<uint32_t ,std::queue<RecvDataInfo*> *>();

			mMultiMapdatas[pRecvDataInfo->mKtvBoxId] = pKtvBoxDataMap;
			mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = false;
		}

		if (mMultiMapaac[pRecvDataInfo->mKtvBoxId]->size() >= MAX_MICROPHONE_COUNT) {
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 0");
			free(pRecvDataInfo->mpData);
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 1");
			delete pRecvDataInfo;
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 2");
			return;
		}

		if(!mMultiMapaac[pRecvDataInfo->mKtvBoxId]->count(pRecvDataInfo->mPhoneId)) {
			AacDecApi *pAac = new AacDecApi;
			pAac->Initialize();
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 3");
			pAac->StartDecode((unsigned char *)pRecvDataInfo->mpMicrophoneData ,pRecvDataInfo->mMicrophoneDataLen);
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 4");
			(*mMultiMapaac[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId] = pAac;
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 5");
			std::queue<RecvDataInfo *> *pDataQueue = new std::queue<RecvDataInfo *>();
			pDataQueue->push(pRecvDataInfo);
			(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId] = pDataQueue;
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 6");
		} else {
			if (mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] == true) {
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 7");
				free(pRecvDataInfo->mpData);
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 8");
				delete pRecvDataInfo;
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 9");
				return;
			}
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData debug 10");
			(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->push(pRecvDataInfo);
			//printf("KtvMicrophoneService::dispatchData size = %d\n", (*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size());
			yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData size = " +
				utility::toString((*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size()),
				yiqiding::utility::Logger::NORMAL);
			if ((*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size() == MAX_MICROPHONE_QUEUE_COUNT) {
				//printf("KtvMicrophoneService::dispatchData MAX_MICROPHONE_QUEUE_COUNT\n");
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData MAX_MICROPHONE_QUEUE_COUNT",
					yiqiding::utility::Logger::NORMAL);
				mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = true;
			}
		}
	}
}

void KtvMicrophoneService::noticeConnectLostApp(uint32_t phoneId) {

	yiqiding::MutexGuard guard(mHandleMutexLock);
	uint32_t ktvBoxId = mConn->getBoxIdFromAppId(phoneId);
	if (mMultiMapaac.count(ktvBoxId)) {
		if (mMultiMapaac[ktvBoxId]->count(phoneId)) {
			delete (*mMultiMapaac[ktvBoxId])[phoneId];
			mMultiMapaac[ktvBoxId]->erase(phoneId);
		}
	}

	if (mMultiMapdatas.count(ktvBoxId)) {
		if (mMultiMapdatas[ktvBoxId]->count(phoneId)) {
			std::queue<RecvDataInfo *> *pDataQueue = (*mMultiMapdatas[ktvBoxId])[phoneId];
			while(!pDataQueue->empty()) {
				RecvDataInfo* pRecvDataInfo = pDataQueue->front();
				pDataQueue->pop();
				delete pRecvDataInfo->mpData;
				delete pRecvDataInfo;
			}
			delete pDataQueue;
			mMultiMapdatas[ktvBoxId]->erase(phoneId);
		}
	}

}