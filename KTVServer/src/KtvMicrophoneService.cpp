#include "yiqiding/ktv/KtvMicrophoneService.h"

#include "yiqiding/net/Socket.h"
#include "yiqiding/io/PCMAudio.h"
#include "yiqiding/net/TCP.h"
#include "AudioMixing.h"
#include <windows.h>
#include <iostream>
#include <iterator>
#include "yiqiding/utility/Logger.h"

#define TCP_PORT (36629)
#define UDP_PORT (36628)
#define VER_CODE_LEN (4)
#define IPADDR_LEN (16)
#define MIX_MAX_LEN (4096)
#define MAX_MICROPHONE_COUNT (8)
#define MAX_MICROPHONE_QUEUE_COUNT (50)  // 其中包含一个头结点，数据结点只有49个
#define HANDLE_MICROPHONE_QUEUE_COUNT (49)

#define DEBUG_MICROPHONE 

#define MICROPHONE_DEBUG 0

#define MICROPHONE_MIX_DUMP 0

using namespace yiqiding;

void KtvMicrophoneService::DataHandleThread::run() {
	if(MICROPHONE_DEBUG){
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::DataHandleThread::run", yiqiding::utility::Logger::NORMAL);
	}

	char* file = "dest_test.pcm";
	FILE* fp = NULL;
	if(MICROPHONE_MIX_DUMP) {
		fp = fopen(file, "wb+");
	}
	uint32_t test_tmp = 0;
	uint32_t test_tmp2 = 0;

	std::map<uint32_t ,std::list<RecvDataInfo*> *> ktvBoxMapdatas;

	while(true) {
		if (mpOwn->mMultiMapdatas.empty()) {
			Sleep(10);
			continue;
		}
		uint32_t ktvBoxId = 0;
		uint32_t phoneId = 0;
		{
			yiqiding::MutexGuard guard(mpOwn->mDropHandleMutexLock);
			{
				yiqiding::MutexGuard guard(mpOwn->mHandleMutexLock);
				bool neadHandle = false;
				for each(auto i in mpOwn->mMultiMapdatas) {
					ktvBoxId = i.first;
					if (mpOwn->mMultiKtvboxState[ktvBoxId] == true) {
						neadHandle = true;
						break;
					}
				}
				if (!neadHandle) {
					continue;
				}
				ktvBoxMapdatas.clear();
				std::map<uint32_t ,std::list<RecvDataInfo*> *>* pSingleKtvBoxDataMap = mpOwn->mMultiMapdatas[ktvBoxId];
				for each(auto j in (*pSingleKtvBoxDataMap)) {
					phoneId = j.first;
					if(j.second->size() > 1) {
						std::list<RecvDataInfo *> *pDataList = new std::list<RecvDataInfo *>();
						std::copy(j.second->begin(), j.second->end(), std::back_inserter(*pDataList)); 
						ktvBoxMapdatas[phoneId] = pDataList;
						RecvDataInfo* pHead = j.second->front();
						j.second->pop_front();
						for(int i = 0; i < HANDLE_MICROPHONE_QUEUE_COUNT; i++) {
							if (j.second->size() == 0) {
								break;
							}
							j.second->pop_front();
						}
						j.second->push_back(pHead);
					}
				}
				mpOwn->mMultiKtvboxState[ktvBoxId] = false;
			}

			char mix[MAX_MICROPHONE_COUNT][MIX_MAX_LEN] = {0};
			char dest[MIX_MAX_LEN] = {0};
			unsigned char *pBufOut = NULL;
			int bufOutLen = 0;
			int consumed = 0;

			for (uint32_t n = 0; n < HANDLE_MICROPHONE_QUEUE_COUNT; n++) {
				int k = 0;
				memset(mix, 0, sizeof(char)*MAX_MICROPHONE_COUNT*MIX_MAX_LEN); 
				for each(auto j in ktvBoxMapdatas) {
					if (j.second->size() > 1) {

						RecvDataInfo* pHead = j.second->front();
						j.second->pop_front();
						AacDecApi *pAac = pHead->mpAacDecApi;

						RecvDataInfo* pRecvDataInfo = j.second->front();
						j.second->pop_front();
						pAac->DecodeBuffer((unsigned char *)pRecvDataInfo->mpMicrophoneData ,
							pRecvDataInfo->mMicrophoneDataLen , &pBufOut , &bufOutLen , &consumed);

						free(pRecvDataInfo->mpData);
						delete pRecvDataInfo;

						j.second->push_front(pHead);

						memcpy(mix[k]  , pBufOut , bufOutLen);
						k++;
					}
				}
				if (k > 0) {
					memset(dest, 0, MIX_MAX_LEN);
					uint32_t nRet =  AudioMixing(mix , MIX_MAX_LEN , k, dest);
					if(MICROPHONE_MIX_DUMP) {
						if (!test_tmp2) {
							fwrite((unsigned char *)dest,1,MIX_MAX_LEN,fp);
							test_tmp++;
						}

						if (test_tmp > 800) {
							test_tmp = 0;
							if (!test_tmp2) {
								fclose(fp);
							}
							test_tmp2 = 1;
						}
					}
					mpOwn->mConn->sendDataToBox(ktvBoxId, dest, MIX_MAX_LEN);
				}
			}
		}
		// delete ktvBoxMapdatas start
		for each(auto d in ktvBoxMapdatas) {
			delete d.second;
		}
		// delete ktvBoxMapdatas end
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
		memset(pBuf, 0, 4096);
		int nRet = recvfrom(socketFd , pBuf , sizeof(pBuf) , 0 , (struct sockaddr *)&clientaddr , &fromLen);
		if(nRet <= 4 ) {
			continue;
		}
		{
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
	if (!mMultiMapdatas.empty()) {
		for each(auto i in mMultiMapdatas){
			std::map<uint32_t ,std::list<RecvDataInfo*> *>* pSingleKtvBoxDataMap = i.second;
			if (!pSingleKtvBoxDataMap->empty()) {
				for each(auto j in (*pSingleKtvBoxDataMap)){
					for (uint32_t k = 0; k < j.second->size(); k++) {
						RecvDataInfo* pData = j.second->front();
						j.second->pop_front();
						if (k == 0) {
							delete pData->mpAacDecApi;
							delete pData;
						} else {
							free(pData->mpData);
							delete pData;
						}
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
	int ret = mConn->getBoxIdFromAppId(pRecvDataInfo->mPhoneId);
	if (ret < 0) {
		yiqiding::utility::Logger::get("server")->log("KtvMicrophoneService::dispatchData error mPhoneId = " +\
			utility::toString(pRecvDataInfo->mPhoneId), 
			yiqiding::utility::Logger::NORMAL);
		free(pRecvDataInfo->mpData);
		delete pRecvDataInfo;
		return;
	}

	pRecvDataInfo->mKtvBoxId = ret;
	{
		yiqiding::MutexGuard guard(mDropDispatchMutexLock);
		bool isKtvBoxExist = mMultiMapdatas.count(pRecvDataInfo->mKtvBoxId);

		if(!isKtvBoxExist) {
			std::map<uint32_t ,std::list<RecvDataInfo*> *> * pKtvBoxDataMap
				= new std::map<uint32_t ,std::list<RecvDataInfo*> *>();

			AacDecApi *pAac = new AacDecApi;
			pAac->Initialize();
			pAac->StartDecode((unsigned char *)pRecvDataInfo->mpMicrophoneData ,pRecvDataInfo->mMicrophoneDataLen);

			std::list<RecvDataInfo *> *pDataList = new std::list<RecvDataInfo *>();

			RecvDataInfo* pHead = new RecvDataInfo;
			pHead->mpAacDecApi = pAac;
			pDataList->push_back(pHead);
			pDataList->push_back(pRecvDataInfo);

			(*pKtvBoxDataMap)[pRecvDataInfo->mPhoneId] = pDataList;

			{
				yiqiding::MutexGuard guard(mHandleMutexLock);
				mMultiMapdatas[pRecvDataInfo->mKtvBoxId] = pKtvBoxDataMap;
				mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = false;
			}
			return;
		} else {
			if (mMultiMapdatas[pRecvDataInfo->mKtvBoxId]->size() >= MAX_MICROPHONE_COUNT || 
				mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] == true) {
					free(pRecvDataInfo->mpData);
					delete pRecvDataInfo;
					return;
			}
		}

		bool isPhoneExist = mMultiMapdatas[pRecvDataInfo->mKtvBoxId]->count(pRecvDataInfo->mPhoneId);

		if(!isPhoneExist) {
			AacDecApi *pAac = new AacDecApi;
			pAac->Initialize();
			pAac->StartDecode((unsigned char *)pRecvDataInfo->mpMicrophoneData ,pRecvDataInfo->mMicrophoneDataLen);

			RecvDataInfo* pHead = new RecvDataInfo;
			pHead->mpAacDecApi = pAac;

			std::list<RecvDataInfo *> *pDataList = new std::list<RecvDataInfo *>();
			pDataList->push_back(pHead);
			pDataList->push_back(pRecvDataInfo);
			{
				yiqiding::MutexGuard guard(mHandleMutexLock);
				(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId] = pDataList;
			}
		} else {
			yiqiding::MutexGuard guard(mHandleMutexLock);
			(*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->push_back(pRecvDataInfo);
			if ((*mMultiMapdatas[pRecvDataInfo->mKtvBoxId])[pRecvDataInfo->mPhoneId]->size() == MAX_MICROPHONE_QUEUE_COUNT) {
				mMultiKtvboxState[pRecvDataInfo->mKtvBoxId] = true;
			}
		}
	}
}

void KtvMicrophoneService::noticeConnectLostApp(uint32_t phoneId) {
	yiqiding::MutexGuard guard(mDropDispatchMutexLock);
	yiqiding::MutexGuard guardHandle(mDropHandleMutexLock);
	uint32_t ktvBoxId = mConn->getBoxIdFromAppId(phoneId);
	if (mMultiMapdatas.count(ktvBoxId)) {
		if (mMultiMapdatas[ktvBoxId]->count(phoneId)) {
			std::list<RecvDataInfo *> *pDataList = (*mMultiMapdatas[ktvBoxId])[phoneId];
			int i = 0;
			while(!pDataList->empty()) {
				RecvDataInfo* pRecvDataInfo = pDataList->front();
				pDataList->pop_front();
				if (i == 0) {
					delete pRecvDataInfo->mpAacDecApi;
					delete pRecvDataInfo;
				} else {
					free(pRecvDataInfo->mpData);
					delete pRecvDataInfo;
				}
				i = 1;
			}
			delete pDataList;
			mMultiMapdatas[ktvBoxId]->erase(phoneId);
		}
	}
}