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

#define MICROPHONE_DEBUG 1

#define MICROPHONE_MIX_DUMP 1

using namespace yiqiding;

void KtvMicrophoneService::DataHandleThread::run() {
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread::run", yiqiding::utility::Logger::NORMAL);
	}
	std::vector<RecvDataInfo *> vecData;
	std::vector<AacDecApi *> aac;

	FILE* fp = NULL;
	uint32_t test_tmp = 0;
	uint32_t test_tmp2 = 0;
	if(MICROPHONE_MIX_DUMP) {
		char* file = "dest_test.pcm";
		fp = fopen(file, "wb+");
	}

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
					std::map<uint32_t ,std::queue<RecvDataInfo*> *>* pSingleKtvBoxDataMap = i.second;
					for (uint32_t n = 0; n < MAX_MICROPHONE_QUEUE_COUNT; n++) {
						vecData.clear();
						aac.clear();
						for each(auto j in (*pSingleKtvBoxDataMap)) {
							phoneId = j.first;
							if(!j.second->empty()) {
								RecvDataInfo *data = j.second->front();
								j.second->pop();
								vecData.push_back(data);
								std::map<uint32_t , AacDecApi *>* pSingleKtvBoxAacMap
									= mpOwn->mMultiMapaac[ktvBoxId];
								aac.push_back((*pSingleKtvBoxAacMap)[phoneId]);
								if(MICROPHONE_DEBUG){
									yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread push_back n = " + utility::toString(n), yiqiding::utility::Logger::NORMAL);
								}
							}
						}
						char mix[MAX_MICROPHONE_COUNT][MIX_MAX_LEN];
						char dest[MIX_MAX_LEN];
						unsigned char *pBufOut;
						int bufOutLen;
						int consumed;
						for(uint32_t k = 0 ; k < aac.size() ; k++){
							if(MICROPHONE_DEBUG){
								yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread vecData[" + utility::toString(k) +\
									"->mMicrophoneDataLen = " + utility::toString(vecData[k]->mMicrophoneDataLen), yiqiding::utility::Logger::NORMAL);
							}
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
						if(MICROPHONE_DEBUG){
							yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread ktvBoxId = " +\
								utility::toString(ktvBoxId), yiqiding::utility::Logger::NORMAL);
						}
						if(MICROPHONE_MIX_DUMP) {
							if (!test_tmp2) {
								fwrite((unsigned char *)dest,1,MIX_MAX_LEN,fp);
								test_tmp++;
							}else {
								if(MICROPHONE_DEBUG){
									yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread DONE", yiqiding::utility::Logger::NORMAL);
								}
							}
							if (test_tmp > 3000) {
								test_tmp = 0;
								if (!test_tmp2) {
									fclose(fp);
								}
								test_tmp2 = 1;
							}
						}
						mpOwn->mConn->sendDataToBox(ktvBoxId, dest, MIX_MAX_LEN);
					}
					mpOwn->mMultiKtvboxState[ktvBoxId] = false;
				}
			}
		}
	}
}

void KtvMicrophoneService::UdpServerRecvThread::run() {

	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::UdpServerRecvThread::run", yiqiding::utility::Logger::NORMAL);
	}
	SOCKET socketFd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); 

	struct sockaddr_in servaddr;
	char on = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == SOCKET_ERROR) {
		if(MICROPHONE_DEBUG){
			yiqiding::utility::Logger::get("server")->log("reuseaddr error", yiqiding::utility::Logger::NORMAL);
		}
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
		if(MICROPHONE_DEBUG){
			yiqiding::utility::Logger::get("server")->log("bind " + utility::toString(UDP_PORT) + " error", yiqiding::utility::Logger::NORMAL);
		}
	}

	char pBuf[4096];

	struct sockaddr_in clientaddr;
	int fromLen = sizeof(clientaddr);

	while(true){
		int nRet = recvfrom(socketFd , pBuf , sizeof(pBuf) , 0 , (struct sockaddr *)&clientaddr , &fromLen);
		if(nRet <= 4 ) {
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
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::~KtvMicrophoneService start", 
		yiqiding::utility::Logger::NORMAL); 
	}
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
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::~KtvMicrophoneService end", 
		yiqiding::utility::Logger::NORMAL); 
	}
}

void KtvMicrophoneService::init() {
	yiqiding::net::Socket::Driver::load();

	mpUdpServerRecvThread = new UdpServerRecvThread(this);
	mpDataHandleThread = new DataHandleThread(this);
	mpUdpServerRecvThread->start();
	mpDataHandleThread->start();
}

void KtvMicrophoneService::dispatchData(RecvDataInfo* pRecvDataInfo) {
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData start", 
			yiqiding::utility::Logger::NORMAL); 
	}
	char verCode[VER_CODE_LEN] = {0};
	memcpy(verCode , pRecvDataInfo->mpData , VER_CODE_LEN);

	if(MICROPHONE_DEBUG){
	yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData verCode[0] = " +\
		utility::toString(verCode[0]) + " verCode[1] = " + utility::toString(verCode[1]) + " verCode[2] = " +\
		utility::toString(verCode[2]) + " verCode[3] = " + utility::toString(verCode[3]), 
		yiqiding::utility::Logger::NORMAL);
	}

	pRecvDataInfo->mpMicrophoneData = pRecvDataInfo->mpData + VER_CODE_LEN;
	pRecvDataInfo->mMicrophoneDataLen = pRecvDataInfo->mLen - VER_CODE_LEN;
	pRecvDataInfo->mPhoneId = 0;
	for(uint32_t i = 0; i < VER_CODE_LEN; i++) {
		pRecvDataInfo->mPhoneId = (int32_t)(pRecvDataInfo->mPhoneId | ((verCode[i] & 0x000000ff) << ((VER_CODE_LEN - 1 - i) * 8)));
	}
	
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData mPhoneId = " +\
			utility::toString(pRecvDataInfo->mPhoneId), 
			yiqiding::utility::Logger::NORMAL);
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData mMicrophoneDataLen = " +\
			utility::toString(pRecvDataInfo->mMicrophoneDataLen), 
			yiqiding::utility::Logger::NORMAL);
	}
	
	pRecvDataInfo->mKtvBoxId = mConn->getBoxIdFromAppId(pRecvDataInfo->mPhoneId);
	{
		yiqiding::MutexGuard guard(mHandleMutexLock);
		if(!mMultiMapaac.count(pRecvDataInfo->mKtvBoxId)) {
			std::map<uint32_t , AacDecApi *>* pKtvBoxAacDecApiMap = new std::map<uint32_t, AacDecApi *>();
			mMultiMapaac[pRecvDataInfo->mKtvBoxId] = pKtvBoxAacDecApiMap;

			std::map<uint32_t ,std::queue<RecvDataInfo*> *> * pKtvBoxDataMap
				= new std::map<uint32_t ,std::queue<RecvDataInfo*> *>();

			mMultiMapdatas[pRecvDataInfo->mKtvBoxId] = pKtvBoxDataMap;
			mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = false;
		}

		if (mMultiMapaac[pRecvDataInfo->mKtvBoxId]->size() >= MAX_MICROPHONE_COUNT) {
			free(pRecvDataInfo->mpData);
			delete pRecvDataInfo;
			if(MICROPHONE_DEBUG){
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData end 0", 
				yiqiding::utility::Logger::NORMAL); 
			}
			return;
		}

		if(!mMultiMapaac[pRecvDataInfo->mKtvBoxId]->count(pRecvDataInfo->mPhoneId)) {
			AacDecApi *pAac = new AacDecApi;
			pAac->Initialize();
			pAac->StartDecode((unsigned char *)pRecvDataInfo->mpMicrophoneData ,pRecvDataInfo->mMicrophoneDataLen);
			(*mMultiMapaac[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId] = pAac;

			std::queue<RecvDataInfo *> *pDataQueue = new std::queue<RecvDataInfo *>();
			pDataQueue->push(pRecvDataInfo);
			(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId] = pDataQueue;
			if(MICROPHONE_DEBUG){
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData end 1", 
				yiqiding::utility::Logger::NORMAL); 
			}
		} else {
			if (mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] == true) {
				free(pRecvDataInfo->mpData);
				delete pRecvDataInfo;
				return;
			}

			(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->push(pRecvDataInfo);
			
			if(MICROPHONE_DEBUG){
				yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData size = " +
					utility::toString((*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size()),
					yiqiding::utility::Logger::NORMAL);
			}
			if ((*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size() == MAX_MICROPHONE_QUEUE_COUNT) {
				if(MICROPHONE_DEBUG){
					yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData MAX_MICROPHONE_QUEUE_COUNT",
						yiqiding::utility::Logger::NORMAL);
				}
				mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = true;
			}
		}
	}
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData end 2", 
		yiqiding::utility::Logger::NORMAL); 
	}
}

void KtvMicrophoneService::noticeConnectLostApp(uint32_t phoneId) {
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::noticeConnectLostApp start", 
		yiqiding::utility::Logger::NORMAL); 
	}
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
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::noticeConnectLostApp end", 
		yiqiding::utility::Logger::NORMAL); 
	}
}