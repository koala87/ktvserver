/**
 * KTV Database Cloud Connector
 * Lohas Network Technology Co., Ltd
 * @author Yuchun Zhang
 * @date 2015.07.09
 */

#pragma once
#include "yiqiding/ktv/db/ConfigModel.h"
#include "yiqiding/ktv/db/Definition.h"

namespace yiqiding { namespace ktv { namespace db {


	static const unsigned int INVALID_SQL_ID = 0xffffffff;


	class CloudConnector {
	private:
		ConnectionPool*	_pool;
	
		yiqiding::sql::mysql::Connection	_conn;
		bool		_trans;
		//  serial_id => id
		// 1 media , 2 actor , 3 songlist
		unsigned int getMediaId(unsigned int serialid);
		unsigned int getActorId(unsigned int serialid);
		unsigned int getSonglistId(unsigned int serialid);

		unsigned int getId(const std::string &sql);
	public:
		CloudConnector(ConnectionPool* pool) : _pool(pool),_trans(false){};
		~CloudConnector() {};

		//common
		bool execute(const std::string &sql);

		void setAutoCommit() { _trans = true; _conn = _pool->getConnection(); _conn->setAutoCommit(false);}
		void commit() { _trans = false ; _conn->commit();_pool->returnConnection(_conn); }




		//songlist_detail
			//add
		bool add_songlist_detail(unsigned int serial_lid , unsigned int index , unsigned int serial_mid);
			//del
		bool del_songlist_detail(unsigned int serial_lid , unsigned int index);
			//modify
		bool modify_songlist_detail(unsigned int serial_lid , unsigned int index , unsigned int serial_mid);

		//media_list
			//add
		bool add_media_list(const std::string &type , unsigned int index , unsigned int serial_mid);
			//del
		bool del_media_list(const std::string &type , unsigned int index);
			//modify
		bool modify_media_list(const std::string &type , unsigned int index , unsigned int serial_mid);

		//media
			//add
		bool add_media( int serial_id ,   int actor_serialid_1,
		 int actor_serailid_2, const std::string &name , int language , int type ,
		const std::string &singer , const std::string &pinyin, const std::string &header,
		const std::string &head ,  int words , const std::string &path , const std::string &lyric,
		 int orginal_track ,  int sound_track ,  int start_volume_1 ,  int start_volume_2 ,
		 int prelude,  int effect ,  int version ,	const std::string &create_time,
		double stars , int hot ,	int count , int enable , int black , int match , const std::string &update_time , int resolution ,int quality , int source ,int rhythm, int pitch ,std::string &info);

		
		
			//modify
		bool modify_media( int serial_id ,   int actor_serialid_1,
		 int actor_serailid_2, const std::string &name , int language , int type,
		const std::string &singer , const std::string &pinyin, const std::string &header,
		const std::string &head ,  int words , const std::string &path , const std::string &lyric,
		 int orginal_track ,  int sound_track ,  int start_volume_1 ,  int start_volume_2 ,
		 int prelude,  int effect ,  int version ,	const std::string &create_time,
		double stars , int hot ,	int count , int enable , int black , int match , const std::string  &update_time , int resolution ,int quality , int source ,int rhythm, int pitch ,std::string &info);

		//actor
			//add
		//execute
			//modify
		//execute 

		//songlist
			//add
		//execute
			//modify
		bool modify_songlist(unsigned int serial_lid , const std::string &title);
			//del
		bool del_songlist(unsigned int serial_lid);

	};
}}}
