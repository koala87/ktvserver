/**
 * KTV Database Media Connector
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.17
 */

#pragma once

#include <memory>
#include <map>
#include "yiqiding/ktv/db/Definition.h"
#include "yiqiding/ktv/db/MediaModel.h"
#include "yiqiding/container/SmartVector.h"

namespace yiqiding { namespace ktv { namespace db {
	class MediaConnector {
	private:
		ConnectionPool*	_pool;
	public:
		MediaConnector(ConnectionPool* pool) : _pool(pool) {};
		~MediaConnector() {};

		//////////////////////////////////////////////////////////////////////////

		/// Insert a row into activity.
		/// aid and serial_id will be generated automatically.
		void add(const model::Activity& activity);

		/// Update activity with aid
		/// serial_id will NOT be changed
		void update(const model::Activity& activity);
		
		/// Get activity by aid
		std::auto_ptr<model::Activity> getActivity(unsigned int aid);

		/// Get activity
		std::auto_ptr<container::SmartVector<model::Activity> >	getActivity(size_t pos = 0, size_t n = 0);

		/// Get number of records in activity
		size_t countActivity();

		//////////////////////////////////////////////////////////////////////////

		/// Insert a row into signer.
		/// sid and serial_id will be generated automatically.
		void add(const model::Actor& actor);

		/// Update Actor with sid
		/// serial_id will NOT be changed
		void update(const model::Actor& actor);

		/// Get actor by sid
		std::auto_ptr<model::Actor> getActor(unsigned int sid);

		/// Get actor by name
		std::auto_ptr<container::SmartVector<model::Actor> > getActorByName(const std::string& name);

		/// Get actor
		std::auto_ptr<container::SmartVector<model::Actor> > getActor(unsigned int type, size_t pos, size_t n);

		/// Get actor type id by media type name
		unsigned int	getActorTypeId(const std::string& name);

		/// Get All
		std::map<unsigned int, std::string> getActorType();

		/// Get number of records in Actor
		size_t countActor(unsigned int type = 0);

		//////////////////////////////////////////////////////////////////////////

		/// Insert a row into media.
		/// mid and serial_id will be generated automatically.
		void add(const model::Media& media);

		/// Add media record
		void addMediaRecord(unsigned int boxid, unsigned int mid);

		/// Update Media with mid
		/// serial_id will NOT be changed
		void update(const model::Media& media);

		/// Get Media by mid
		std::auto_ptr<model::Media> getMedia(unsigned int mid);

		/// Get Media by name and match
		std::auto_ptr<container::SmartVector<model::Media> > getMediaByName(const std::string& name , bool match = false);

		/// Get Media
		std::auto_ptr<container::SmartVector<model::Media> > getMedia(unsigned int type , unsigned int language, size_t pos = 0, size_t n = 0);

		/// Get media type id by media type name
		unsigned int getMediaTypeId(const std::string& name);

		/// Get media language id by media language name
		unsigned int getMediaLanguageId(const std::string& name);

		/// Get All
		std::map<unsigned int, std::string> getMediaType();
		std::map<unsigned int, std::string> getMediaLanguage();

		/// Get number of records in Media
		size_t countMedia(unsigned int type = 0, unsigned int language = 0);
		size_t countMedia(const std::string& list_name, unsigned int type = 0, unsigned int language = 0);

		//////////////////////////////////////////////////////////////////////////

		/// Insert a row into songlist.
		/// lid and serial_id will be generated automatically.
		void add(const model::SongList& list);

		/// Update songlist with lid
		/// serial_id will NOT be changed
		void update(const model::SongList& list);

		/// Get songlist by lid
		std::auto_ptr<model::SongList> getSongList(unsigned int lid);

		/// Get songlist by name
		std::auto_ptr<model::SongList> getSongListByTitle(const std::string& title);

		/// Get songlist
		std::auto_ptr<container::SmartVector<model::SongList> > getSongList(const std::string& type, size_t pos = 0, size_t n = 0);
		std::auto_ptr<container::SmartVector<model::Media> > getSongListMedia(unsigned int lid, size_t pos = 0, size_t n = 0);

		/// Get number of records in activity
		size_t countSongList(const std::string& type);

		//////////////////////////////////////////////////////////////////////////

		void updateMediaList(const std::string& list_name, const std::vector<unsigned int>& songs);
		
		/// Get Media
		std::auto_ptr<container::SmartVector<model::Media> > getMediaListMedia(const std::string& list_name, unsigned int type = 0, unsigned int language = 0, size_t pos = 0, size_t n = 0);

		////////////////////////////////////////////////////////////////////////////

		/// Get GameList
		std::auto_ptr<container::SmartVector<model::Game> > getGameList(size_t pos = 0 , size_t n = 0);
		
		/// Get Game
		std::auto_ptr<model::Game> getGame(unsigned int kid);
	
		/// Insert Game
		void addGame(const model::Game &game);

		uint32_t getGameId();

		uint32_t countGameList();


		/// Game2
		unsigned int addGame2(const model::Game2 &game);
		void delGame2(int id);
		 std::auto_ptr< std::vector<model::Game2> > getAllGame2(int page , int num = 5 , bool *pStatus = NULL);
		 int										getGame2Count();
		 std::auto_ptr< std::vector<model::Game2> > getHisGame2(int num = 3);
		void updateGame2(int id , const std::string &score);
	};
}}}
