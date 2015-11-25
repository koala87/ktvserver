/**
 * Box Request Processor
 * @author Shiwei Zhang
 * @date 2014.03.10
 */

#pragma once

#include "yiqiding/ktv/PacketProcessor.h"

namespace yiqiding { namespace ktv {
	class BoxProcessor : public packet::Processor {
	protected:
		void finishControlRoom();
		void processApkPath();
		void processOpenRoom();
		void processAllocServer();
		void processService();
		void processReallocServer();
		void processUpdateSongCount();
		void processGetGPS();
		void processChangeBox();
		void processMsgRule();
		void processAdUrl();
		void processFireInfo();
		void processVolume();

		void processGetAddress();

		void processCheckData();

		//wireless
		void processTurnMsgToApp();
		void processTempCode();
		void processTurnMsgToAllApp();

		//social
		void processTurnMsgToAll();

		//game
		void processGameJoin();
		void processGameUpload();
		void processGetKtvGame();
		void processGameReq();
		void processGetHisGame();
		

		//challenge
		void processAddGame();
		void processAnswerGame();
		void processUploadScore();
		void processCancelGame();

		//logcore
		void processLogCore();

		//soc
		void processUpdateBoxInfo();
		void processGetBoxList();
		void processGetOneBox();
		
		//game2
		void processGame2Start();
		void processGame2Exit();
		void processGame2Score();
		void processGame2Inv();

		//kgame2
		void processKGame2Accept();
		void processKGame2Socre();
		void processKGame2Get();

		//reward
		void processAddReward();
		void processAddRecord();
		void processSingReward();
		void processCancelReward();
		void processGetReward();

		//reward2
		void processAddReward2();
		void processReadyReward2();
		void processAddRecord2();
		void processGetMyReward2();
		void processGetAllReward2();
		void processCancelReward2();
		void processSelectReward2();

		//gift
		void processSendGift();
		void processRecvGift();

		//pcm
		void processPCMStart();
		void processPCMStop();
		void processPCMListen();
		void processPCMRemove();
		void processPCMConfirmListen();

		void processGetAccountsList();
		
		//notify songs to box , when it has. used in open , /* or hearted */
		void notifySongsToBox();

		inline bool isBoxIDValid(uint32_t box_id) { return (box_id == _pac->getDeviceID()) ? true : (sendErrorMessage("Authentication failure"), false); };
	public:
		BoxProcessor(Server* server, BoxConnection* conn, Packet* pac , const std::string &ip , int port); 
		virtual ~BoxProcessor()	{};
		virtual void onReceivePacket();
		virtual std::string getDir() { return "./box/"; }
		inline BoxConnection*	getConnection()	{ return (BoxConnection*)_conn; };
	};
}}
