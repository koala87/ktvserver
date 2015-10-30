#include "yiqiding/ktv/db/CloudConnector.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Logger.h"
using namespace yiqiding::ktv::db;
using namespace yiqiding::utility;

unsigned int CloudConnector::getId(const std::string &sql)
{
	Connection conn;
	if(!_trans)
		conn = _pool->getConnection();
	else
		 conn = _conn;

	unsigned int ret = INVALID_SQL_ID;
	PreparedStatement stmt;
	ResultSet result;
	try{
		stmt = conn->prepareStatement(sql);
		result = stmt->executeQuery();
	} catch (const std::exception& err) {
		yiqiding::utility::Logger::get("system")->log(sql  + " " + err.what(), Logger::WARNING);
		return INVALID_SQL_ID;
	}

	if (result->next() )
		ret = result->getInt(1);

	if(!_trans)
		_pool->returnConnection(conn);



	return ret;
}


bool CloudConnector::execute(const std::string& sql)
{
	Connection conn;
	if(!_trans)
		conn = _pool->getConnection();
	else
		conn = _conn;
	PreparedStatement stmt;
	try {
		stmt = conn->prepareStatement(sql);
		stmt->execute();
	} catch (const std::exception& err) {
		yiqiding::utility::Logger::get("system")->log(sql + " " + err.what() , Logger::WARNING);
		return false;
	}

	if(!_trans)
		_pool->returnConnection(conn);
	
	return true;
}

unsigned int CloudConnector::getMediaId(unsigned int serialid)
{
	return getId("SELECT `mid` FROM media where serial_id = " + yiqiding::utility::toString(serialid));
}

unsigned int CloudConnector::getActorId(unsigned int serialid)
{
	return getId("select sid from actor where serial_id = " + yiqiding::utility::toString(serialid));
}

unsigned int CloudConnector::getSonglistId(unsigned int serialid)
{
	return getId("select lid from songlist where serial_id = " + yiqiding::utility::toString(serialid));
}



bool CloudConnector::add_songlist_detail(unsigned int serial_lid , unsigned int index , unsigned int serial_mid){
	
	unsigned int lid = getSonglistId(serial_lid);
	unsigned int mid = getMediaId(serial_mid);
	if(lid == INVALID_SQL_ID || mid == INVALID_SQL_ID)
		return false;

	std::string sql = "insert into songlist_detail(`lid` , `index` , mid) values("+ yiqiding::utility::toString(lid) +" , "+ yiqiding::utility::toString(index)+" , "+ yiqiding::utility::toString(mid)+")";
	return execute(sql);
}

bool CloudConnector::modify_songlist_detail(unsigned int serial_lid , unsigned int index , unsigned int serial_mid)
{
	unsigned int lid = getSonglistId(serial_lid);
	unsigned int mid = getMediaId(serial_mid);
	if(lid == INVALID_SQL_ID || mid == INVALID_SQL_ID)
		return false;

	std::string sql = "update songlist_detail set mid = " + yiqiding::utility::toString(mid) + " where lid = " + yiqiding::utility::toString(lid) + " and `index` = " + yiqiding::utility::toString(index);
	return execute(sql);
}

bool CloudConnector::del_songlist_detail(unsigned int serial_lid , unsigned int index)
{
	unsigned int lid = getSonglistId(serial_lid);
	if(lid == INVALID_SQL_ID)
		return false;

	std::string sql = "delete from songlist_detail where lid = " + yiqiding::utility::toString(lid) + " and `index` = " + yiqiding::utility::toString(index);
	return execute(sql);

}

bool CloudConnector::add_media_list(const std::string &type , unsigned int index , unsigned int serial_mid){
	unsigned int mid = getMediaId(serial_mid);
	if(mid == INVALID_SQL_ID)
		return false;

	std::string sql = "insert into media_list(`type` , `index` , mid) values('"+ type +"' , " + yiqiding::utility::toString(index)+" , "+ yiqiding::utility::toString(mid)+")";
	return execute(sql);
}

bool CloudConnector::del_media_list(const std::string &type , unsigned int index){
	std::string sql = "delete from media_list where `index` = " + toString(index) + " and `type` = '" + type + "'";
	return execute(sql);
}

bool CloudConnector::modify_media_list(const std::string &type , unsigned int index , unsigned int serial_mid)
{
	unsigned int mid = getMediaId(serial_mid);
	if(mid == INVALID_SQL_ID)
		return false;

	std::string sql = "update media_list set mid = " + toString(mid) + " where `index` = " + toString(index)  + " and `type` = '" + toString(type) + "'";
	return execute(sql);
}



bool CloudConnector::modify_songlist(unsigned int serial_lid , const std::string &title)
{
	std::string sql = "update songlist set title = '" + title + "' where serial_id = " + toString(serial_lid);
	return execute(sql);
}

bool CloudConnector::del_songlist(unsigned int serial_lid)
{
	std::string sql = "delete from songlist where serial_id = " + toString(serial_lid);
	return execute(sql);
}

bool CloudConnector::add_media( int serial_id ,   int actor_serialid_1,
	 int actor_serailid_2, const std::string &name , int language , int type ,
	const std::string &singer , const std::string &pinyin, const std::string &header,
	const std::string &head ,  int words , const std::string &path , const std::string &lyric,
	 int orginal_track ,  int sound_track ,  int start_volume_1 ,  int start_volume_2 ,
	signed int prelude,  int effect ,  int version ,	const std::string &create_time,
	double stars , int hot ,	int count , int enable ,int black, int match , const std::string & update_time , int resolution ,int quality , int source ,int rhythm, int pitch ,std::string &info){
	
		unsigned int artist_sid_1 = getActorId(actor_serialid_1);
		unsigned int artist_sid_2 = getActorId(actor_serailid_2);
		std::string i_create_time = "null";
		std::string i_update_time = "null";
		if(create_time != "")
			i_create_time = "'" + create_time + "'";
		if(update_time != "")
			i_update_time = "'" + update_time + "'";
		
		std::string sql = "INSERT INTO media(serial_id, name, language,type,singer,artist_sid_1, artist_sid_2,pinyin, header, head, words,path,lyric, original_track, sound_track,start_volume_1,"
		"start_volume_2,prelude,effect,  version,create_time,stars,hot, count,enabled,black, `match`,update_time,resolution,quality, source, rhythm, pitch, info)  VALUES(" ;
		sql += toString(serial_id) + " , '"+ name +"', " + toString(language) + " , " + toString(type) + " , '" + singer + "' ," + (artist_sid_1 == INVALID_SQL_ID ? "null":toString(artist_sid_1)) + " , ";
		sql += (artist_sid_2  == INVALID_SQL_ID ? "null":toString(artist_sid_2)) + " , '" + pinyin + "' , '" + header + "' , '" + head + "' , " + toString(words) + ",'" + path + "','" + lyric + "'," ;
		sql += toString(orginal_track) + " , " + toString(sound_track) + " , " + toString(start_volume_1) +  " , " + toString(start_volume_2) + " , " + toString(prelude) + " , " + toString(effect) + " , ";
		sql += toString(version) + " , " + i_create_time + ", " + toString(stars) + " , " + toString(hot ) + " , " + toString(count) + " , " + toString(enable) +" ," + toString(black) + " , " + toString(match) + " ," + i_update_time + "  , ";
		sql += toString(resolution) + " , " + toString(quality) + " , " + toString(source) + " , " + toString(rhythm) + " , " + toString(pitch) + " , ' " + info + "')";
		
		return execute(sql);
}



bool CloudConnector::modify_media( int serial_id ,   int actor_serialid_1,
	 int actor_serailid_2, const std::string &name , int language , int type ,
	const std::string &singer , const std::string &pinyin, const std::string &header,
	const std::string &head ,  int words , const std::string &path , const std::string &lyric,
	 int orginal_track ,  int sound_track ,  int start_volume_1 ,  int start_volume_2 ,
	 int prelude,  int effect ,  int version ,	const std::string &create_time,
	double stars , int hot ,	int count , int enable , int black , int match , const std::string &update_time , int resolution ,int quality , int source ,int rhythm, int pitch ,std::string &info){
		unsigned int artist_sid_1 = getActorId(actor_serialid_1);
		unsigned int artist_sid_2 = getActorId(actor_serailid_2);

		std::string i_create_time = "null";
		std::string i_update_time = "null";
		if(create_time != "")
			i_create_time = "'" + create_time + "'";
		if(update_time != "")
			i_update_time = "'" + update_time + "'";

		std::string sql = "update media set name = '" + name + "' , language = "+ toString(language)+ " , type = " + toString(type)+ " , singer = '" + singer + "' , artist_sid_1 = " + (artist_sid_1 == INVALID_SQL_ID?"null":toString(artist_sid_1)) + " , ";
		sql += "artist_sid_2 = " +  (artist_sid_2 == INVALID_SQL_ID?"null":toString(artist_sid_2)) + " , ";
		sql += "prelude = " + toString(prelude) + " , effect = " + toString(effect) + " , version = " + toString(version) + " , ";
		sql += "create_time = " + i_create_time + " , stars = " + toString(stars) + " , hot = " + toString(hot) + " , count = " + toString(count)  + " , ";
		sql += "enabled = " + toString(enable) + " , black = " + toString(black) + " , `match` = " + toString(match) + " , update_time = " + i_update_time + "  , ";
		sql += "resolution = " + toString(resolution) + " , quality = " + toString(quality) + " , source = " + toString(source) + " , rhythm = " + toString(rhythm) + " , pitch = " + toString(pitch) + " , ";
		sql += "info ='" + info + "' where serial_id =" + toString(serial_id);

		return execute(sql);
}