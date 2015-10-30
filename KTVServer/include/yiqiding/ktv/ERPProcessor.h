/**
 * Box Request Processor
 * @author Shiwei Zhang
 * @date 2014.02.13
 */

#pragma once

#include "yiqiding/ktv/PacketProcessor.h"
#include "yiqiding/ktv/ERPRole.h"

namespace yiqiding { namespace ktv {

	



	class ERPProcessor : public packet::Processor {
	protected:

		//
		void processDongle();

		// ERP_ROLE_RECEPTION
		void finishServiceCall();
		void processControlRoom();
		void processBroadcastMessage();
		void processSingleMessage();
		void processUploadBoxInfo();
		void processChangeBox();
		void processSetMsgRule();
		void processSetFireInfo();
		void processSetAdUrls();
		void processGetOnlineBox();

		// ERP_ROLE_MEDIA
		void processGetVolume();
		void processResource();
		void processSetVolume();
		void processControlBlacklist();
		void processAddMusic();
		void processAddSinger();
		void processAddActivity();
		void processAddPlayList();
		void processGetMusicList();
		void processGetSingerList();
		void processGetActivityList();
		void processGetPlayList();
		void processGetPlayListMusic();
		void processEditMusic();
		void processEditSinger();
		void processEditActivityList();
		void processEditPlayList();
		void processEditRankList();
		void processQueryMusic();
		void processQuerySinger();
		void processQueryPlayList();

		// SING_GAME
		void processOpenSingGame();
		void processCloseSingGame();
		void processModifySingGame();
		void processAllSingGame();
		void processDetailSingGame();
		void processAllDataSingGame();

		//kgame2
		void processAddKGame2();
		void processDelKGame2();
		void processGetGame2();

		//cloud
		void processCloudConfirm();

		// Misc
		inline bool isRoleValid(ERPRole role) { return (getConnection()->getRole() == _pac->getDeviceID()) ? true : (sendErrorMessage("Authentication failure"), false); };
		static std::string getPinyinHead(const std::string& pinyin);
		static std::string removeSplit(const std::string & pinyin , char split = '_');
	public:
		ERPProcessor(Server* server, ERPConnection* conn, Packet* pac , const std::string &ip , int port); 
		virtual ~ERPProcessor()	{};
		virtual void onReceivePacket();
		virtual std::string getDir() { return "./erp/";}
		inline ERPConnection*	getConnection()	{ return (ERPConnection*)_conn; };
	};
}}
