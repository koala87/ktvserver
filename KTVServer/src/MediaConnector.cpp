/**
 * KTV Database Media Connector Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.17
 */

#include <sstream>
#include <memory>

#include "yiqiding/ktv/db/MediaConnector.h"
#include "yiqiding/utility/Utility.h"
using namespace yiqiding::ktv::db;
using namespace yiqiding::utility;
using yiqiding::ktv::db::DataType;

//////////////////////////////////////////////////////////////////////////
/// Activity
//////////////////////////////////////////////////////////////////////////

void MediaConnector::add(const model::Activity& activity) {
	Connection conn = _pool->getConnection();
	{
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `activity` (`serial_id`, `title`, `thumb`, `type`, `member_count`, `start_date`, `end_date`, `start_time`, `end_time`, `status`, `address`, `fee`, `sponsor`, `photos`, `description`) VALUES (0,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		stmt->setString		(1,  activity.getTitle());
		stmt->setString		(2,  activity.getThumb());
		stmt->setString		(3,  activity.getType());
		stmt->setInt			(4,  activity.getMemberCount());
		stmt->setString		(5,  activity.getStartDate());
		stmt->setString		(6,  activity.getEndDate());
		stmt->setString		(7,  activity.getStartTime());
		stmt->setString		(8,  activity.getEndTime());
		stmt->setString		(9,  activity.getStatus());
		stmt->setString		(10, activity.getAddress());
		stmt->setDouble		(11, activity.getFee());
		activity.isSponsorNull()?
			stmt->setNull	(12, DataType::LONGVARCHAR):
		stmt->setString		(12, activity.getSponsor());
		activity.isPhotosNull()?
			stmt->setNull	(13, DataType::LONGVARCHAR):
		stmt->setString		(13, activity.getPhotos());
		activity.isDescriptionNull()?
			stmt->setNull	(14, DataType::LONGVARCHAR):
		stmt->setString		(14, activity.getDescription());
		stmt->execute();
	}
	{
		Statement stmt = conn->createStatement();
		stmt->execute("UPDATE activity SET serial_id = aid + 1000000000 WHERE serial_id = 0");
	}

	_pool->returnConnection(conn);
}

void MediaConnector::update(const model::Activity& activity) {
	Connection conn = _pool->getConnection();
	
	{
		PreparedStatement stmt = conn->prepareStatement("UPDATE `activity` SET `title`=?, `thumb`=?, `type`=?, `member_count`=?, `start_date`=?, `end_date`=?, `start_time`=?, `end_time`=?, `status`=?, `address`=?, `fee`=?, `sponsor`=?, `photos`=?, `description`=? WHERE `aid`=?");
		stmt->setString		(1,  activity.getTitle());
		stmt->setString		(2,  activity.getThumb());
		stmt->setString		(3,  activity.getType());
		stmt->setInt			(4,  activity.getMemberCount());
		stmt->setString		(5,  activity.getStartDate());
		stmt->setString		(6,  activity.getEndDate());
		stmt->setString		(7,  activity.getStartTime());
		stmt->setString		(8,  activity.getEndTime());
		stmt->setString		(9,  activity.getStatus());
		stmt->setString		(10, activity.getAddress());
		stmt->setDouble		(11, activity.getFee());
		activity.isSponsorNull()?
			stmt->setNull	(12, DataType::LONGVARCHAR):
		stmt->setString		(12, activity.getSponsor());
		activity.isPhotosNull()?
			stmt->setNull	(13, DataType::LONGVARCHAR):
		stmt->setString		(13, activity.getPhotos());
		activity.isDescriptionNull()?
			stmt->setNull	(14, DataType::LONGVARCHAR):
		stmt->setString		(14, activity.getDescription());
		stmt->setUInt		(15, activity.getAid());
		stmt->execute();
	}

	_pool->returnConnection(conn);
}

std::auto_ptr<model::Activity> MediaConnector::getActivity(unsigned int aid) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `aid`, `serial_id`, `title`, `thumb`, `type`, `member_count`, `start_date`, `end_date`, `start_time`, `end_time`, `status`, `address`, `fee`, `sponsor`, `photos`, `description` FROM `activity` WHERE `aid`=?");
	stmt->setInt(1, aid);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Activity: sid " + toString(aid) + " not found" , __FILE__, __LINE__);

	std::auto_ptr<model::Activity> activity(new model::Activity);
	activity->setAid			(result->getUInt(1));
	activity->setSerialId	(result->getInt(2));
	activity->setTitle		(result->getString(3));
	activity->setThumb		(result->getString(4));
	activity->setType		(result->getString(5));
	activity->setMemberCount(result->getInt(6));
	activity->setStartDate	(result->getString(7));
	activity->setEndDate		(result->getString(8));
	activity->setStartTime	(result->getString(9));
	activity->setEndTime		(result->getString(10));
	activity->setStatus		(result->getString(11));
	activity->setAddress		(result->getString(12));
	activity->setFee			(result->getDouble(13));
	result->isNull(14)?
		activity->setSponsor(): 
	activity->setSponsor		(result->getString(14));
	result->isNull(15)?
		activity->setPhotos():
	activity->setPhotos		(result->getString(15));
	result->isNull(16)?
		activity->setDescription():
	activity->setDescription	(result->getString(16));

	_pool->returnConnection(conn);

	return activity;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Activity> > MediaConnector::getActivity(size_t pos, size_t n) {
	Connection conn = _pool->getConnection();
	
	PreparedStatement stmt;
	std::string sql = "SELECT `aid`, `serial_id`, `title`, `thumb`, `type`, `member_count`, `start_date`, `end_date`, `start_time`, `end_time`, `status`, `address`, `fee`, `sponsor`, `photos`, `description` FROM activity order by `sort` desc";
	if (pos == 0 && n == 0)
		stmt = conn->prepareStatement(sql);
	else {
		sql += " LIMIT ?, ?";
		stmt = conn->prepareStatement(sql);
		stmt->setUInt64(1, pos);
		stmt->setUInt64(2, n);
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Activity> > list(new yiqiding::container::SmartVector<model::Activity>);
	std::auto_ptr<model::Activity> activity;
	while (result->next()) {
		activity.reset(new model::Activity);

		activity->setAid			(result->getUInt(1));
		activity->setSerialId	(result->getInt(2));
		activity->setTitle		(result->getString(3));
		activity->setThumb		(result->getString(4));
		activity->setType		(result->getString(5));
		activity->setMemberCount(result->getInt(6));
		activity->setStartDate	(result->getString(7));
		activity->setEndDate		(result->getString(8));
		activity->setStartTime	(result->getString(9));
		activity->setEndTime		(result->getString(10));
		activity->setStatus		(result->getString(11));
		activity->setAddress		(result->getString(12));
		activity->setFee			(result->getDouble(13));
		result->isNull(14)?
			activity->setSponsor(): 
		activity->setSponsor		(result->getString(14));
		result->isNull(15)?
			activity->setPhotos():
		activity->setPhotos		(result->getString(15));
		result->isNull(16)?
			activity->setDescription():
		activity->setDescription	(result->getString(16));

		list->push_back(activity.release());
	}
	_pool->returnConnection(conn);

	return list;
}

size_t MediaConnector::countActivity() {
	Connection conn = _pool->getConnection();

	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("SELECT COUNT(*) FROM `activity`");
	if (!result->next())
		throw DBException("Fail to count activity", __FILE__, __LINE__);
	size_t count = result->getUInt64(1);

	_pool->returnConnection(conn);

	return count;
}

//////////////////////////////////////////////////////////////////////////
/// Actor
//////////////////////////////////////////////////////////////////////////

void MediaConnector::add(const model::Actor& actor) {
	Connection conn = _pool->getConnection();
	{
#ifndef REMOVE_SQL_IMAGE
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `actor` (`serial_id`, `name`, `stars`, `type`, `nation`, `image`, `pinyin`, `song_count`, `count`, `header`, `head`) VALUES (0,?,?,?,?,?,?,?,?,?,?)");
		stmt->setString		(1,  actor.getName());
		actor.isStarsNull()?
			stmt->setNull	(2,  DataType::DOUBLE):
		stmt->setDouble		(2,  actor.getStars());
		stmt->setUInt		(3,  actor.getType());
		stmt->setString		(4,  actor.getNation());
		actor.isImageNull()?
			stmt->setNull	(5,  DataType::VARCHAR):
		stmt->setString		(5,  actor.getImage());
		stmt->setString		(6,  actor.getPinyin());
		stmt->setInt			(7,  actor.getSongCount());
		stmt->setInt			(8,  actor.getCount());
		stmt->setString		(9,  actor.getHeader());
		if (actor.isHeadNull())
			stmt->setNull	(10, DataType::VARCHAR);
		else {
			std::string head;
			head = actor.getHead();
			stmt->setString	(10, head);
		}
#else
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `actor` (`serial_id`, `name`, `stars`, `type`, `pinyin`, `song_count`, `count`, `header`, `head`) VALUES (0,?,?,?,?,?,?,?,?,?)");
		stmt->setString		(1,  actor.getName());
		actor.isStarsNull()?
			stmt->setNull	(2,  DataType::DOUBLE):
		stmt->setDouble		(2,  actor.getStars());
		stmt->setUInt		(3,  actor.getType());
	
		
		stmt->setString		(4,  actor.getPinyin());
		stmt->setInt			(5,  actor.getSongCount());
		stmt->setInt			(6,  actor.getCount());
		stmt->setString		(7,  actor.getHeader());
		if (actor.isHeadNull())
			stmt->setNull	(8, DataType::VARCHAR);
		else {
			std::string head;
			head = actor.getHead();
			stmt->setString	(8, head);
		}
#endif
		stmt->execute();
	}
	{
		Statement stmt = conn->createStatement();
		stmt->execute("UPDATE actor SET serial_id = sid + 1000000000 WHERE serial_id = 0");
	}

	_pool->returnConnection(conn);
}

void MediaConnector::update(const model::Actor& actor) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE	
	PreparedStatement stmt = conn->prepareStatement("UPDATE `actor` SET `name`=?, `stars`=?, `type`=?, `nation`=?, `image`=?, `pinyin`=?, `song_count`=?, `count`=?, `header`=?, `head`=? WHERE `sid`=?");
	stmt->setString		(1,  actor.getName());
	actor.isStarsNull()?
		stmt->setNull	(2,  DataType::DOUBLE):
	stmt->setDouble		(2,  actor.getStars());
	stmt->setUInt		(3,  actor.getType());
	stmt->setString		(4,  actor.getNation());
	actor.isImageNull()?
		stmt->setNull	(5,  DataType::VARCHAR):
	stmt->setString		(5,  actor.getImage());
	stmt->setString		(6,  actor.getPinyin());
	stmt->setInt			(7,  actor.getSongCount());
	stmt->setInt			(8,  actor.getCount());
	stmt->setString		(9,  actor.getHeader());
	if (actor.isHeadNull())
		stmt->setNull	(10, DataType::VARCHAR);
	else {
		std::string head;
		head = actor.getHead();
		stmt->setString	(10, head);
	}
	stmt->setUInt		(11, actor.getSid());

#else
	PreparedStatement stmt = conn->prepareStatement("UPDATE `actor` SET `name`=?, `stars`=?, `type`=?, `pinyin`=?, `song_count`=?, `count`=?, `header`=?, `head`=? WHERE `sid`=?");
	stmt->setString		(1,  actor.getName());
	actor.isStarsNull()?
		stmt->setNull	(2,  DataType::DOUBLE):
	stmt->setDouble		(2,  actor.getStars());
	stmt->setUInt		(3,  actor.getType());

	
	stmt->setString		(4,  actor.getPinyin());
	stmt->setInt			(5,  actor.getSongCount());
	stmt->setInt			(6,  actor.getCount());
	stmt->setString		(7,  actor.getHeader());
	if (actor.isHeadNull())
		stmt->setNull	(8, DataType::VARCHAR);
	else {
		std::string head;
		head = actor.getHead();
		stmt->setString	(8, head);
	}
	stmt->setUInt		(9, actor.getSid());
#endif
	stmt->execute();

	_pool->returnConnection(conn);
}

std::auto_ptr<model::Actor> MediaConnector::getActor(unsigned int sid) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt = conn->prepareStatement("SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `nation`, `image`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor` WHERE `sid`=?");
	stmt->setInt(1, sid);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Actor: sid " + toString(sid) + " not found" , __FILE__, __LINE__);

	std::auto_ptr<model::Actor> actor(new model::Actor);
	actor->setSid		(result->getUInt(1));
	actor->setSerialId	(result->getInt(2));
	actor->setName		(result->getString(3));
	result->isNull(4)?
		actor->setStars	():
	actor->setStars		((float)result->getDouble(4));
	actor->setType		(result->getUInt(5));
	actor->setNation		(result->getString(6));
	result->isNull(7)?
		actor->setImage	():
	actor->setImage		(result->getString(7));
	actor->setPinyin		(result->getString(8));
	actor->setSongCount	(result->getInt(9));
	actor->setCount		(result->getInt(10));
	actor->setHeader		(result->getString(11));
	result->isNull(12)?
		actor->setHead	():
	actor->setHead		(result->getString(12)[0]);
#else
	PreparedStatement stmt = conn->prepareStatement("SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor` WHERE `sid`=?");
	stmt->setInt(1, sid);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Actor: sid " + toString(sid) + " not found" , __FILE__, __LINE__);

	std::auto_ptr<model::Actor> actor(new model::Actor);
	actor->setSid		(result->getUInt(1));
	actor->setSerialId	(result->getInt(2));
	actor->setName		(result->getString(3));
	result->isNull(4)?
		actor->setStars	():
	actor->setStars		((float)result->getDouble(4));
	actor->setType		(result->getUInt(5));
	
	
	actor->setPinyin		(result->getString(6));
	actor->setSongCount	(result->getInt(7));
	actor->setCount		(result->getInt(8));
	actor->setHeader		(result->getString(9));
	result->isNull(10)?
		actor->setHead	():
	actor->setHead		(result->getString(10)[0]);
#endif
	_pool->returnConnection(conn);

	return actor;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Actor> >  MediaConnector::getActorByName(const std::string& name) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt = conn->prepareStatement("SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `nation`, `image`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Actor> > list(new yiqiding::container::SmartVector<model::Actor>);
	std::auto_ptr<model::Actor> actor;
	while (result->next()) {
		actor.reset(new model::Actor);

		actor->setSid		(result->getUInt(1));
		actor->setSerialId	(result->getInt(2));
		actor->setName		(result->getString(3));
		result->isNull(4)?
			actor->setStars	():
		actor->setStars		((float)result->getDouble(4));
		actor->setType		(result->getUInt(5));
		actor->setNation		(result->getString(6));
		result->isNull(7)?
			actor->setImage	():
		actor->setImage		(result->getString(7));
		actor->setPinyin		(result->getString(8));
		actor->setSongCount	(result->getInt(9));
		actor->setCount		(result->getInt(10));
		actor->setHeader		(result->getString(11));
		result->isNull(12)?
			actor->setHead	():
		actor->setHead		(result->getString(12)[0]);

		list->push_back(actor.release());
	}
#else
	PreparedStatement stmt = conn->prepareStatement("SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Actor> > list(new yiqiding::container::SmartVector<model::Actor>);
	std::auto_ptr<model::Actor> actor;
	while (result->next()) {
		actor.reset(new model::Actor);

		actor->setSid		(result->getUInt(1));
		actor->setSerialId	(result->getInt(2));
		actor->setName		(result->getString(3));
		result->isNull(4)?
			actor->setStars	():
		actor->setStars		((float)result->getDouble(4));
		actor->setType		(result->getUInt(5));
		
		
		actor->setPinyin		(result->getString(6));
		actor->setSongCount	(result->getInt(7));
		actor->setCount		(result->getInt(8));
		actor->setHeader		(result->getString(9));
		result->isNull(10)?
			actor->setHead	():
		actor->setHead		(result->getString(10)[0]);

		list->push_back(actor.release());
	}
#endif
	_pool->returnConnection(conn);

	return list;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Actor> >  MediaConnector::getActor(unsigned int type, size_t pos, size_t n) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt;
#ifndef REMOVE_SQL_IMAGE
	std::string sql = "SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `nation`, `image`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor`";
#else
	std::string sql = "SELECT `sid`, `serial_id`, `name`, `stars`, `type`, `pinyin`, `song_count`, `count`, `header`, `head` FROM `actor`";
#endif
	if (type == 0)
		sql += " WHERE `type` = ?";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		if (type != 0)
			stmt->setUInt(++i, type);
		if (pos != 0 || n != 0) {
			stmt->setUInt64(++i, pos);
			stmt->setUInt64(++i, n);
		}
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Actor> > list(new yiqiding::container::SmartVector<model::Actor>);
	std::auto_ptr<model::Actor> actor;
	while (result->next()) {
		actor.reset(new model::Actor);
#ifndef REMOVE_SQL_IMAGE
		actor->setSid		(result->getUInt(1));
		actor->setSerialId	(result->getInt(2));
		actor->setName		(result->getString(3));
		result->isNull(4)?
			actor->setStars	():
		actor->setStars		((float)result->getDouble(4));
		actor->setType		(result->getUInt(5));
		actor->setNation		(result->getString(6));
		result->isNull(7)?
			actor->setImage	():
		actor->setImage		(result->getString(7));
		actor->setPinyin		(result->getString(8));
		actor->setSongCount	(result->getInt(9));
		actor->setCount		(result->getInt(10));
		actor->setHeader		(result->getString(11));
		result->isNull(12)?
			actor->setHead	():
		actor->setHead		(result->getString(12)[0]);
#else
		actor->setSid		(result->getUInt(1));
		actor->setSerialId	(result->getInt(2));
		actor->setName		(result->getString(3));
		result->isNull(4)?
			actor->setStars	():
		actor->setStars		((float)result->getDouble(4));
		actor->setType		(result->getUInt(5));
	
		
		actor->setPinyin		(result->getString(6));
		actor->setSongCount	(result->getInt(7));
		actor->setCount		(result->getInt(8));
		actor->setHeader		(result->getString(9));
		result->isNull(10)?
			actor->setHead	():
		actor->setHead		(result->getString(10)[0]);
#endif
		list->push_back(actor.release());
	}

	_pool->returnConnection(conn);

	return list;
}

unsigned int MediaConnector::getActorTypeId(const std::string& name) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `id` FROM `actor_type` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Actor Type: Name '" + name + "' not found" , __FILE__, __LINE__);

	unsigned int id = result->getUInt(1);

	_pool->returnConnection(conn);

	return id;
}

std::map<unsigned int, std::string> MediaConnector::getActorType() {
	Connection conn = _pool->getConnection();

	std::map<unsigned int, std::string> actor_type;
	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("SELECT `id`, `name` FROM `actor_type`");
	while (result->next())
		actor_type[result->getUInt(1)] = result->getString(2);

	_pool->returnConnection(conn);

	return actor_type;
}

size_t MediaConnector::countActor(unsigned int type) {
	Connection conn = _pool->getConnection();

	std::string sql = "SELECT COUNT(*) FROM `actor`";
	PreparedStatement stmt;
	if (type == 0)
		stmt = conn->prepareStatement(sql);
	else {
		sql += " WHERE `type` = ?";
		stmt = conn->prepareStatement(sql);
		stmt->setUInt(1, type);
	}
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Fail to count actor", __FILE__, __LINE__);
	size_t count = result->getUInt64(1);

	_pool->returnConnection(conn);

	return count;
}

//////////////////////////////////////////////////////////////////////////
/// Media
//////////////////////////////////////////////////////////////////////////

void MediaConnector::add(const model::Media& media) {
	Connection conn = _pool->getConnection();

	{
#ifndef REMOVE_SQL_IMAGE
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `media` (`serial_id`, `name`, `singer`, `image`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head`) VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
		stmt->setString		(1,  media.getName());
		stmt->setString		(2,  media.getSinger());	
		media.isImageNull()?
			stmt->setNull	(3,  DataType::VARCHAR):
		stmt->setString		(3,  media.getImage());
		stmt->setUInt		(4,  media.getLanguage());
		stmt->setUInt		(5,  media.getType());
		stmt->setDouble		(6,  media.getStars());
		stmt->setInt			(7,  media.getCount());
		stmt->setString		(8,  media.getPath());
		media.isArtistSid1Null()?
			stmt->setNull	(9,  DataType::INTEGER):
		stmt->setInt			(9,  media.getArtistSid1());
		media.isArtistSid2Null()?
			stmt->setNull	(10, DataType::INTEGER):
		stmt->setInt			(10, media.getArtistSid2());
		stmt->setString		(11, media.getPinyin());
		media.isLyricNull()?
			stmt->setNull	(12, DataType::VARCHAR):
		stmt->setString		(12, media.getLyric());
		stmt->setString		(13, media.getHeader());
		stmt->setInt			(14, media.getOriginalTrack());
		stmt->setInt			(15, media.getSoundTrack());
		stmt->setInt			(16, media.getWords());
		stmt->setBoolean		(17, media.getBlack());
		stmt->setBoolean		(18, media.getHot());
		if (media.isHeadNull())
			stmt->setNull	(19, DataType::CHAR);
		else {
			std::string head;
			head = media.getHead();
			stmt->setString (19, head);
		}
#else
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `media` (`serial_id`, `name`, `singer`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head`) VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
		stmt->setString		(1,  media.getName());
		stmt->setString		(2,  media.getSinger());	
		stmt->setUInt		(3,  media.getLanguage());
		stmt->setUInt		(4,  media.getType());
		stmt->setDouble		(5,  media.getStars());
		stmt->setInt			(6,  media.getCount());
		stmt->setString		(7,  media.getPath());
		media.isArtistSid1Null()?
			stmt->setNull	(8,  DataType::INTEGER):
		stmt->setInt			(8,  media.getArtistSid1());
		media.isArtistSid2Null()?
			stmt->setNull	(9, DataType::INTEGER):
		stmt->setInt			(9, media.getArtistSid2());
		stmt->setString		(10, media.getPinyin());
		media.isLyricNull()?
			stmt->setNull	(11, DataType::VARCHAR):
		stmt->setString		(11, media.getLyric());
		stmt->setString		(12, media.getHeader());
		stmt->setInt			(13, media.getOriginalTrack());
		stmt->setInt			(14, media.getSoundTrack());
		stmt->setInt			(15, media.getWords());
		stmt->setBoolean		(16, media.getBlack());
		stmt->setBoolean		(17, media.getHot());
		if (media.isHeadNull())
			stmt->setNull	(18, DataType::CHAR);
		else {
			std::string head;
			head = media.getHead();
			stmt->setString (18, head);
		}
#endif
		stmt->execute();
	}
	{
		Statement stmt = conn->createStatement();
		stmt->execute("UPDATE media SET serial_id = mid + 1000000000 WHERE serial_id = 0");
	}

	_pool->returnConnection(conn);
}

void MediaConnector::addMediaRecord(unsigned int boxid, unsigned int mid) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("INSERT INTO `media_record` (`boxid`, `mid`) VALUES (?, ?)");
	stmt->setUInt(1, boxid);
	stmt->setUInt(2, mid);
	stmt->execute();

	_pool->returnConnection(conn);
}

void MediaConnector::update(const model::Media& media) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt = conn->prepareStatement("UPDATE `media` SET `name`=?, `singer`=?, `image`=?, `language`=?, `type`=?, `stars`=?, `count`=?, `path`=?, `artist_sid_1`=?, `artist_sid_2`=?, `pinyin`=?, `lyric`=?, `header`=?, `original_track`=?, `sound_track`=?, `words`=?, `black`=?, `hot`=?, `head`=? WHERE `mid`=?");
	stmt->setString		(1,  media.getName());
	stmt->setString		(2,  media.getSinger());
	media.isImageNull()?
		stmt->setNull	(3,  DataType::VARCHAR):
	stmt->setString		(3,  media.getImage());
	stmt->setUInt		(4,  media.getLanguage());
	stmt->setUInt		(5,  media.getType());
	stmt->setDouble		(6,  media.getStars());
	stmt->setInt			(7,  media.getCount());
	stmt->setString		(8,  media.getPath());
	media.isArtistSid1Null()?
		stmt->setNull	(9,  DataType::INTEGER):
	stmt->setInt			(9,  media.getArtistSid1());
	media.isArtistSid2Null()?
		stmt->setNull	(10, DataType::INTEGER):
	stmt->setInt			(10, media.getArtistSid2());
	stmt->setString		(11, media.getPinyin());
	media.isLyricNull()?
		stmt->setNull	(12, DataType::VARCHAR):
	stmt->setString		(12, media.getLyric());
	stmt->setString		(13, media.getHeader());
	stmt->setInt			(14, media.getOriginalTrack());
	stmt->setInt			(15, media.getSoundTrack());
	stmt->setInt			(16, media.getWords());
	stmt->setBoolean		(17, media.getBlack());
	stmt->setBoolean		(18, media.getHot());
	if (media.isHeadNull())
		stmt->setNull	(19, DataType::CHAR);
	else {
		std::string head;
		head = media.getHead();
		stmt->setString (19, head);
	}
	stmt->setUInt		(20, media.getMid());
#else
	PreparedStatement stmt = conn->prepareStatement("UPDATE `media` SET `name`=?, `singer`=?, `language`=?, `type`=?, `stars`=?, `count`=?, `path`=?, `artist_sid_1`=?, `artist_sid_2`=?, `pinyin`=?, `lyric`=?, `header`=?, `original_track`=?, `sound_track`=?, `words`=?, `black`=?, `hot`=?, `head`=? WHERE `mid`=?");
	stmt->setString		(1,  media.getName());
	stmt->setString		(2,  media.getSinger());
	stmt->setUInt		(3,  media.getLanguage());
	stmt->setUInt		(4,  media.getType());
	stmt->setDouble		(5,  media.getStars());
	stmt->setInt			(6,  media.getCount());
	stmt->setString		(7,  media.getPath());
	media.isArtistSid1Null()?
		stmt->setNull	(8,  DataType::INTEGER):
	stmt->setInt			(8,  media.getArtistSid1());
	media.isArtistSid2Null()?
		stmt->setNull	(9, DataType::INTEGER):
	stmt->setInt			(9, media.getArtistSid2());
	stmt->setString		(10, media.getPinyin());
	media.isLyricNull()?
		stmt->setNull	(11, DataType::VARCHAR):
	stmt->setString		(11, media.getLyric());
	stmt->setString		(12, media.getHeader());
	stmt->setInt			(13, media.getOriginalTrack());
	stmt->setInt			(14, media.getSoundTrack());
	stmt->setInt			(15, media.getWords());
	stmt->setBoolean		(16, media.getBlack());
	stmt->setBoolean		(17, media.getHot());
	if (media.isHeadNull())
		stmt->setNull	(18, DataType::CHAR);
	else {
		std::string head;
		head = media.getHead();
		stmt->setString (18, head);
	}
	stmt->setUInt		(19, media.getMid());
#endif

	stmt->execute();

	_pool->returnConnection(conn);
}

std::auto_ptr<model::Media> MediaConnector::getMedia(unsigned int mid) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`, `image`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `mid`=?");
	stmt->setInt(1, mid);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Media: mid " + toString(mid) + " not found" , __FILE__, __LINE__);

	std::auto_ptr<model::Media> media(new model::Media);
	media->setMid			(result->getUInt(1));
	media->setSerialId		(result->getInt(2));
	media->setName			(result->getString(3));
	media->setSinger			(result->getString(4));
	result->isNull(5)?
		media->setImage():
	media->setImage			(result->getString(5));
	media->setLanguage		(result->getUInt(6));
	media->setType			(result->getUInt(7));
	media->setStars			((float)result->getDouble(8));
	media->setCount			(result->getInt(9));
	media->setPath			(result->getString(10));
	result->isNull(11)?
		media->setArtistSid1():
	media->setArtistSid1		(result->getInt(11));
	result->isNull(12)?
		media->setArtistSid2():
	media->setArtistSid2		(result->getInt(12));
	media->setPinyin			(result->getString(13));
	result->isNull(14)?
		media->setLyric():
	media->setLyric			(result->getString(14));
	media->setHeader			(result->getString(15));
	media->setOriginalTrack	(result->getInt(16));
	media->setSoundTrack		(result->getInt(17));
	media->setWords			(result->getInt(18));
	media->setBlack			(result->getBoolean(19));
	media->setHot			(result->getBoolean(20));
	result->isNull(21)?
		media->setHead():
	media->setHead			(result->getString(21)[0]);
#else
	PreparedStatement stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`,  `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `mid`=?");
	stmt->setInt(1, mid);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Media: mid " + toString(mid) + " not found" , __FILE__, __LINE__);

	std::auto_ptr<model::Media> media(new model::Media);
	media->setMid			(result->getUInt(1));
	media->setSerialId		(result->getInt(2));
	media->setName			(result->getString(3));
	media->setSinger			(result->getString(4));
	media->setLanguage		(result->getUInt(5));
	media->setType			(result->getUInt(6));
	media->setStars			((float)result->getDouble(7));
	media->setCount			(result->getInt(8));
	media->setPath			(result->getString(9));
	result->isNull(10)?
		media->setArtistSid1():
	media->setArtistSid1		(result->getInt(10));
	result->isNull(11)?
		media->setArtistSid2():
	media->setArtistSid2		(result->getInt(11));
	media->setPinyin			(result->getString(12));
	result->isNull(13)?
		media->setLyric():
	media->setLyric			(result->getString(13));
	media->setHeader			(result->getString(14));
	media->setOriginalTrack	(result->getInt(15));
	media->setSoundTrack		(result->getInt(16));
	media->setWords			(result->getInt(17));
	media->setBlack			(result->getBoolean(18));
	media->setHot			(result->getBoolean(19));
	result->isNull(20)?
		media->setHead():
	media->setHead			(result->getString(20)[0]);
#endif
	_pool->returnConnection(conn);

	return media;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Media> > MediaConnector::getMediaByName(const std::string& name , bool match) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt;
	
#ifndef REMOVE_SQL_IMAGE
	if(match)
		stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`, `image`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `name`=? and `match` = 1");
	else
		stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`, `image`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
	
		result->isNull(5)?
			media->setImage():
		media->setImage			(result->getString(5));

		media->setLanguage		(result->getUInt(6));
		media->setType			(result->getUInt(7));
		media->setStars			((float)result->getDouble(8));
		media->setCount			(result->getInt(9));
		media->setPath			(result->getString(10));
		result->isNull(11)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(11));
		result->isNull(12)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(12));
		media->setPinyin			(result->getString(13));
		result->isNull(14)?
			media->setLyric():
		media->setLyric			(result->getString(14));
		media->setHeader			(result->getString(15));
		media->setOriginalTrack	(result->getInt(16));
		media->setSoundTrack		(result->getInt(17));
		media->setWords			(result->getInt(18));
		media->setBlack			(result->getBoolean(19));
		media->setHot			(result->getBoolean(20));
		result->isNull(21)?
			media->setHead():
		media->setHead			(result->getString(21)[0]);

		list->push_back(media.release());
	}
#else
	if(match)
		stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `name`=? and `match` = 1");
	else
		stmt = conn->prepareStatement("SELECT `mid`, `serial_id`, `name`, `singer`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));

	

		media->setLanguage		(result->getUInt(5));
		media->setType			(result->getUInt(6));
		media->setStars			((float)result->getDouble(7));
		media->setCount			(result->getInt(8));
		media->setPath			(result->getString(9));
		result->isNull(10)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(10));
		result->isNull(11)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(11));
		media->setPinyin			(result->getString(12));
		result->isNull(13)?
			media->setLyric():
		media->setLyric			(result->getString(13));
		media->setHeader			(result->getString(14));
		media->setOriginalTrack	(result->getInt(15));
		media->setSoundTrack		(result->getInt(16));
		media->setWords			(result->getInt(17));
		media->setBlack			(result->getBoolean(18));
		media->setHot			(result->getBoolean(19));
		result->isNull(20)?
			media->setHead():
		media->setHead			(result->getString(20)[0]);

		list->push_back(media.release());
	}
#endif
	_pool->returnConnection(conn);

	return list;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Media> > MediaConnector::getMedia(unsigned int type, unsigned int language, size_t pos, size_t n) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt;
	std::string sql = "SELECT `mid`, `serial_id`, `name`, `singer`, `image`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM media";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
		if (pos != 0 || n != 0) {
			stmt->setUInt64(++i, pos);
			stmt->setUInt64(++i, n);
		}
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		result->isNull(5)?
			media->setImage():
		media->setImage			(result->getString(5));
		media->setLanguage		(result->getUInt(6));
		media->setType			(result->getUInt(7));
		media->setStars			((float)result->getDouble(8));
		media->setCount			(result->getInt(9));
		media->setPath			(result->getString(10));
		result->isNull(11)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(11));
		result->isNull(12)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(12));
		media->setPinyin			(result->getString(13));
		result->isNull(14)?
			media->setLyric():
		media->setLyric			(result->getString(14));
		media->setHeader			(result->getString(15));
		media->setOriginalTrack	(result->getInt(16));
		media->setSoundTrack		(result->getInt(17));
		media->setWords			(result->getInt(18));
		media->setBlack			(result->getBoolean(19));
		media->setHot			(result->getBoolean(20));
		result->isNull(21)?
			media->setHead():
		media->setHead			(result->getString(21)[0]);

		list->push_back(media.release());
	}
#else
	PreparedStatement stmt;
	std::string sql = "SELECT `mid`, `serial_id`, `name`, `singer`, `language`, `type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM media";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
		if (pos != 0 || n != 0) {
			stmt->setUInt64(++i, pos);
			stmt->setUInt64(++i, n);
		}
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		media->setLanguage		(result->getUInt(5));
		media->setType			(result->getUInt(6));
		media->setStars			((float)result->getDouble(7));
		media->setCount			(result->getInt(8));
		media->setPath			(result->getString(9));
		result->isNull(10)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(10));
		result->isNull(11)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(11));
		media->setPinyin			(result->getString(12));
		result->isNull(13)?
			media->setLyric():
		media->setLyric			(result->getString(13));
		media->setHeader			(result->getString(14));
		media->setOriginalTrack	(result->getInt(15));
		media->setSoundTrack		(result->getInt(16));
		media->setWords			(result->getInt(17));
		media->setBlack			(result->getBoolean(18));
		media->setHot			(result->getBoolean(19));
		result->isNull(20)?
			media->setHead():
		media->setHead			(result->getString(20)[0]);

		list->push_back(media.release());
	}
#endif
	_pool->returnConnection(conn);

	return list;
}

unsigned int MediaConnector::getMediaTypeId(const std::string& name) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `id` FROM `media_type` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Media Type: Name '" + name + "' not found" , __FILE__, __LINE__);
	
	unsigned int id = result->getUInt(1);

	_pool->returnConnection(conn);

	return id;
}

unsigned int MediaConnector::getMediaLanguageId(const std::string& name) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT `id` FROM `media_language` WHERE `name`=?");
	stmt->setString(1, name);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Media Language: Name '" + name + "' not found" , __FILE__, __LINE__);

	unsigned int id = result->getUInt(1);

	_pool->returnConnection(conn);

	return id;
}

std::map<unsigned int, std::string> MediaConnector::getMediaType() {
	Connection conn = _pool->getConnection();

	std::map<unsigned int, std::string> media_type;
	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("SELECT `id`, `name` FROM `media_type`");
	while (result->next())
		media_type[result->getUInt(1)] = result->getString(2);

	_pool->returnConnection(conn);

	return media_type;
}

std::map<unsigned int, std::string> MediaConnector::getMediaLanguage() {
	Connection conn = _pool->getConnection();

	std::map<unsigned int, std::string> media_language;
	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("SELECT `id`, `name` FROM `media_language`");
	while (result->next())
		media_language[result->getUInt(1)] = result->getString(2);

	_pool->returnConnection(conn);

	return media_language;
}

size_t MediaConnector::countMedia(unsigned int type, unsigned int language) {
	Connection conn = _pool->getConnection();

	std::string sql = "SELECT COUNT(*) FROM `media`";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	PreparedStatement stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
	}
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Fail to count media", __FILE__, __LINE__);
	size_t count = result->getUInt64(1);

	_pool->returnConnection(conn);

	return count;
}

size_t MediaConnector::countMedia(const std::string& list_name, unsigned int type, unsigned int language) {
	Connection conn = _pool->getConnection();

	std::string sql = "SELECT COUNT(*) FROM `media_list` INNER JOIN `media` ON `media_list`.`type` = ? AND `media_list`.`mid` = `media`.`mid`";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `media`.`type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	PreparedStatement stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		stmt->setString(++i, list_name);
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
	}
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Fail to count media", __FILE__, __LINE__);
	size_t count = result->getUInt64(1);

	_pool->returnConnection(conn);

	return count;
}

//////////////////////////////////////////////////////////////////////////
/// SongList
//////////////////////////////////////////////////////////////////////////

void MediaConnector::add(const model::SongList& list) {
	Connection conn = _pool->getConnection();
	{
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `songlist` (`serial_id`, `title`, `image`, `type`, `count`, `special`) VALUES (0,?,?,?,?,?)");
		stmt->setString		(1, list.getTitle());
		stmt->setString		(2, list.getImage());
		stmt->setString		(3, list.getType());
		stmt->setInt			(4, list.getCount());
		stmt->setBoolean		(5, list.getSpecial());
		stmt->execute();
	}
	unsigned int lid;
	{
		Statement stmt = conn->createStatement();
		ResultSet result = stmt->executeQuery("SELECT LAST_INSERT_ID();");
		if (!result->next())
			throw DBException("Fail insertion", __FILE__, __LINE__);
		lid = result->getUInt(1);
	}
	{
		Statement stmt = conn->createStatement();
		stmt->execute("UPDATE songlist SET serial_id = lid + 1000000000 WHERE serial_id = 0");
	}
	{
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `songlist_detail` (`lid`, `index`, `mid`) VALUES (?, ?, ?)");
		const std::vector<unsigned int>& songs = list.getSongs();
		size_t size = songs.size();
		for (size_t i = 0; i < size; ++i) {
			stmt->setUInt(1, lid);
			stmt->setUInt64(2, i);
			stmt->setUInt(3, songs[i]);
			stmt->execute();
		}
	}

	_pool->returnConnection(conn);
}

void MediaConnector::update(const model::SongList& list) {
	Connection conn = _pool->getConnection();
	
	unsigned int lid = list.getLid();
	{
		PreparedStatement stmt = conn->prepareStatement("UPDATE `songlist` SET `title`=?, `image`=?, `type`=?, `count`=?, `special`=? WHERE `lid`=?");
		stmt->setString		(1, list.getTitle());
		stmt->setString		(2, list.getImage());
		stmt->setString		(3, list.getType());
		stmt->setInt			(4, list.getCount());
		stmt->setBoolean		(5, list.getSpecial());
		stmt->setUInt		(6, lid);
		stmt->execute();
	}
	{
		PreparedStatement stmt = conn->prepareStatement("DELETE FROM `songlist_detail` WHERE `lid` = ?");
		stmt->setUInt(1, lid);
		stmt->execute();
	}
	{
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `songlist_detail` (`lid`, `index`, `mid`) VALUES (?, ?, ?)");
		const std::vector<unsigned int>& songs = list.getSongs();
		size_t size = songs.size();
		for (size_t i = 0; i < size; ++i) {
			stmt->setUInt(1, lid);
			stmt->setUInt64(2, i);
			stmt->setUInt(3, songs[i]);
			stmt->execute();
		}
	}
	_pool->returnConnection(conn);
}

std::auto_ptr<model::SongList> MediaConnector::getSongList(unsigned int lid) {
	Connection conn = _pool->getConnection();
	std::auto_ptr<model::SongList> songlist(new model::SongList); 

	{
		PreparedStatement stmt = conn->prepareStatement("SELECT `lid`, `serial_id`, `title`, `image`, `type`, `count`, `special` FROM `songlist` WHERE `lid`=?");
		stmt->setInt(1, lid);
		ResultSet result = stmt->executeQuery();
		if (!result->next())
			throw DBException("SongList: lid " + toString(lid) + " not found" , __FILE__, __LINE__);

		songlist->setLid			(result->getUInt(1));
		songlist->setSerialId	(result->getInt(2));
		songlist->setTitle		(result->getString(3));
		songlist->setImage		(result->getString(4));
		songlist->setType		(result->getString(5));
		songlist->setCount		(result->getInt(6));
		songlist->setSpecial		(result->getBoolean(7));
	}
	{
		std::vector<unsigned int> songs;
		PreparedStatement stmt = conn->prepareStatement("SELECT `mid` FROM `songlist_detail` WHERE `lid` = ? ORDER BY `index` ASC");
		stmt->setInt(1, lid);
		ResultSet result = stmt->executeQuery();
		while (result->next())
			songs.push_back(result->getUInt(1));
		songlist->setSongs(songs);
	}

	_pool->returnConnection(conn);

	return songlist;
}

std::auto_ptr<model::SongList> MediaConnector::getSongListByTitle(const std::string& title) {
	Connection conn = _pool->getConnection();
	std::auto_ptr<model::SongList> songlist(new model::SongList); 

	{
		PreparedStatement stmt = conn->prepareStatement("SELECT `lid`, `serial_id`, `title`, `image`, `type`, `count`, `special` FROM `songlist` WHERE `title`=?");
		stmt->setString(1, title);
		ResultSet result = stmt->executeQuery();
		if (!result->next())
			throw DBException("SongList: title " + title + " not found" , __FILE__, __LINE__);

		songlist->setLid			(result->getUInt(1));
		songlist->setSerialId	(result->getInt(2));
		songlist->setTitle		(result->getString(3));
		songlist->setImage		(result->getString(4));
		songlist->setType		(result->getString(5));
		songlist->setCount		(result->getInt(6));
		songlist->setSpecial		(result->getBoolean(7));
	}
	{
		std::vector<unsigned int> songs;
		PreparedStatement stmt = conn->prepareStatement("SELECT `mid` FROM `songlist_detail` WHERE `lid` = ? ORDER BY `index` ASC");
		stmt->setInt(1, songlist->getLid());
		ResultSet result = stmt->executeQuery();
		while (result->next())
			songs.push_back(result->getUInt(1));
		songlist->setSongs(songs);
	}

	_pool->returnConnection(conn);

	return songlist;
}

std::auto_ptr<yiqiding::container::SmartVector<model::SongList> > MediaConnector::getSongList(const std::string& type, size_t pos, size_t n) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt;
	std::string sql = "SELECT `lid`, `serial_id`, `title`, `image`, `type`, `count`, `special` FROM `songlist` WHERE `type` = ?";
	if (pos == 0 && n == 0)
		stmt = conn->prepareStatement(sql);
	else {
		sql += " LIMIT ?, ?";
		stmt = conn->prepareStatement(sql);
		stmt->setUInt64(2, pos);
		stmt->setUInt64(3, n);
	}
	stmt->setString(1, type);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::SongList> > list(new yiqiding::container::SmartVector<model::SongList>);
	std::auto_ptr<model::SongList> songlist;
	while (result->next()) {
		songlist.reset(new model::SongList);

		songlist->setLid			(result->getUInt(1));
		songlist->setSerialId	(result->getInt(2));
		songlist->setTitle		(result->getString(3));
		songlist->setImage		(result->getString(4));
		songlist->setType		(result->getString(5));
		songlist->setCount		(result->getInt(6));
		songlist->setSpecial		(result->getBoolean(7));
		{
			std::vector<unsigned int> songs;
			PreparedStatement stmt = conn->prepareStatement("SELECT `mid` FROM `songlist_detail` WHERE `lid` = ? ORDER BY `index` ASC");
			stmt->setInt(1, songlist->getLid());
			ResultSet result = stmt->executeQuery();
			while (result->next())
				songs.push_back(result->getUInt(1));
			songlist->setSongs(songs);
		}
		list->push_back(songlist.release());
	}

	_pool->returnConnection(conn);

	return list;
}

std::auto_ptr<yiqiding::container::SmartVector<model::Media> > MediaConnector::getSongListMedia(unsigned int lid, size_t pos, size_t n) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt;
	std::string sql = "SELECT `media`.`mid`, `serial_id`, `name`, `singer`, `image`, `language`, `media`.`type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `songlist_detail` INNER JOIN `media` ON `songlist_detail`.`lid` = ? AND `songlist_detail`.`mid` = `media`.`mid` ORDER BY `songlist_detail`.`index` ASC";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	stmt->setUInt(1, lid);
	if (pos != 0 || n != 0) {
		stmt->setUInt64(2, pos);
		stmt->setUInt64(3, n);
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		result->isNull(5)?
			media->setImage():
		media->setImage			(result->getString(5));
		media->setLanguage		(result->getUInt(6));
		media->setType			(result->getUInt(7));
		media->setStars			((float)result->getDouble(8));
		media->setCount			(result->getInt(9));
		media->setPath			(result->getString(10));
		result->isNull(11)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(11));
		result->isNull(12)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(12));
		media->setPinyin			(result->getString(13));
		result->isNull(14)?
			media->setLyric():
		media->setLyric			(result->getString(14));
		media->setHeader			(result->getString(15));
		media->setOriginalTrack	(result->getInt(16));
		media->setSoundTrack		(result->getInt(17));
		media->setWords			(result->getInt(18));
		media->setBlack			(result->getBoolean(19));
		media->setHot			(result->getBoolean(20));
		result->isNull(21)?
			media->setHead():
		media->setHead			(result->getString(21)[0]);

		list->push_back(media.release());
	}
#else
	PreparedStatement stmt;
	std::string sql = "SELECT `media`.`mid`, `serial_id`, `name`, `singer`, `language`, `media`.`type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `songlist_detail` INNER JOIN `media` ON `songlist_detail`.`lid` = ? AND `songlist_detail`.`mid` = `media`.`mid` ORDER BY `songlist_detail`.`index` ASC";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	stmt->setUInt(1, lid);
	if (pos != 0 || n != 0) {
		stmt->setUInt64(2, pos);
		stmt->setUInt64(3, n);
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		media->setLanguage		(result->getUInt(5));
		media->setType			(result->getUInt(6));
		media->setStars			((float)result->getDouble(7));
		media->setCount			(result->getInt(8));
		media->setPath			(result->getString(9));
		result->isNull(10)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(10));
		result->isNull(11)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(11));
		media->setPinyin			(result->getString(12));
		result->isNull(13)?
			media->setLyric():
		media->setLyric			(result->getString(13));
		media->setHeader			(result->getString(14));
		media->setOriginalTrack	(result->getInt(15));
		media->setSoundTrack		(result->getInt(16));
		media->setWords			(result->getInt(17));
		media->setBlack			(result->getBoolean(18));
		media->setHot			(result->getBoolean(19));
		result->isNull(20)?
			media->setHead():
		media->setHead			(result->getString(20)[0]);

		list->push_back(media.release());
	}
#endif
	_pool->returnConnection(conn);

	return list;
}

size_t MediaConnector::countSongList(const std::string& type) {
	Connection conn = _pool->getConnection();

	PreparedStatement stmt = conn->prepareStatement("SELECT COUNT(*) FROM `songlist` WHERE `type` = ?");
	stmt->setString(1, type);
	ResultSet result = stmt->executeQuery();
	if (!result->next())
		throw DBException("Fail to count songlist", __FILE__, __LINE__);
	size_t count = result->getUInt64(1);

	_pool->returnConnection(conn);

	return count;
}

//////////////////////////////////////////////////////////////////////////

void MediaConnector::updateMediaList(const std::string& list_name, const std::vector<unsigned int>& songs) {
	Connection conn = _pool->getConnection();

	// Delete first
	PreparedStatement stmt = conn->prepareStatement("DELETE FROM `media_list` WHERE `type` = ?");
	stmt->setString(1, list_name);
	stmt->execute();

	// Then Insert
	stmt = conn->prepareStatement("INSERT INTO `media_list` (`type`, `index`, `mid`) VALUES (?, ?, ?)");
	stmt->setString(1, list_name);
	size_t count = 0;
	for each (auto mid in songs) {
		stmt->setUInt64(2, count++);
		stmt->setUInt(3, mid);
		stmt->execute();
	}

	_pool->returnConnection(conn);
}

std::auto_ptr<yiqiding::container::SmartVector<model::Media> > MediaConnector::getMediaListMedia(const std::string& list_name, unsigned int type, unsigned int language, size_t pos, size_t n) {
	Connection conn = _pool->getConnection();
#ifndef REMOVE_SQL_IMAGE
	PreparedStatement stmt;
	std::string sql = "SELECT `media`.`mid`, `serial_id`, `name`, `singer`, `image`, `language`, `media`.`type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media_list` INNER JOIN `media` ON `media_list`.`type` = ? AND `media_list`.`mid` = `media`.`mid`";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `media`.`type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	sql += "ORDER BY `index` ASC";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		stmt->setString(++i, list_name);
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
		if (pos != 0 || n != 0) {
			stmt->setUInt64(++i, pos);
			stmt->setUInt64(++i, n);
		}
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		result->isNull(5)?
			media->setImage():
		media->setImage			(result->getString(5));
		media->setLanguage		(result->getUInt(6));
		media->setType			(result->getUInt(7));
		media->setStars			((float)result->getDouble(8));
		media->setCount			(result->getInt(9));
		media->setPath			(result->getString(10));
		result->isNull(11)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(11));
		result->isNull(12)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(12));
		media->setPinyin			(result->getString(13));
		result->isNull(14)?
			media->setLyric():
		media->setLyric			(result->getString(14));
		media->setHeader			(result->getString(15));
		media->setOriginalTrack	(result->getInt(16));
		media->setSoundTrack		(result->getInt(17));
		media->setWords			(result->getInt(18));
		media->setBlack			(result->getBoolean(19));
		media->setHot			(result->getBoolean(20));
		result->isNull(21)?
			media->setHead():
		media->setHead			(result->getString(21)[0]);

		list->push_back(media.release());
	}
#else
	PreparedStatement stmt;
	std::string sql = "SELECT `media`.`mid`, `serial_id`, `name`, `singer`, `language`, `media`.`type`, `stars`, `count`, `path`, `artist_sid_1`, `artist_sid_2`, `pinyin`, `lyric`, `header`, `original_track`, `sound_track`, `words`, `black`, `hot`, `head` FROM `media_list` INNER JOIN `media` ON `media_list`.`type` = ? AND `media_list`.`mid` = `media`.`mid`";
	if (type != 0 || language != 0)
		sql += " WHERE";
	if (type != 0)
		sql += " `media`.`type` = ?";
	if (language != 0) {
		if (type != 0)
			sql += " AND";
		sql += " `language` = ?";
	}
	sql += "ORDER BY `index` ASC";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	stmt = conn->prepareStatement(sql);
	{
		int i = 0;
		stmt->setString(++i, list_name);
		if (type != 0)
			stmt->setUInt(++i, type);
		if (language != 0)
			stmt->setUInt(++i, language);
		if (pos != 0 || n != 0) {
			stmt->setUInt64(++i, pos);
			stmt->setUInt64(++i, n);
		}
	}
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Media> > list(new yiqiding::container::SmartVector<model::Media>);
	std::auto_ptr<model::Media> media;
	while (result->next()) {
		media.reset(new model::Media);

		media->setMid			(result->getUInt(1));
		media->setSerialId		(result->getInt(2));
		media->setName			(result->getString(3));
		media->setSinger			(result->getString(4));
		media->setLanguage		(result->getUInt(5));
		media->setType			(result->getUInt(6));
		media->setStars			((float)result->getDouble(7));
		media->setCount			(result->getInt(8));
		media->setPath			(result->getString(9));
		result->isNull(10)?
			media->setArtistSid1():
		media->setArtistSid1		(result->getInt(10));
		result->isNull(11)?
			media->setArtistSid2():
		media->setArtistSid2		(result->getInt(11));
		media->setPinyin			(result->getString(12));
		result->isNull(13)?
			media->setLyric():
		media->setLyric			(result->getString(13));
		media->setHeader			(result->getString(14));
		media->setOriginalTrack	(result->getInt(15));
		media->setSoundTrack		(result->getInt(16));
		media->setWords			(result->getInt(17));
		media->setBlack			(result->getBoolean(18));
		media->setHot			(result->getBoolean(19));
		result->isNull(20)?
			media->setHead():
		media->setHead			(result->getString(20)[0]);

		list->push_back(media.release());
	}
#endif
	_pool->returnConnection(conn);

	return list;
}


std::auto_ptr<yiqiding::container::SmartVector<model::Game> > MediaConnector::getGameList(size_t pos /* = 0  */, size_t n /* = 0 */)
{
	Connection conn = _pool->getConnection();

	PreparedStatement stmt;
	std::string sql = "SELECT kid , ktype , kstarttime , ksendtime , kstate , ktitle , kmessage , kmids , kjoins , kscores from yiqiding_info.ktv_game";
	if (pos != 0 || n != 0)
		sql += " LIMIT ?, ?";
	
	stmt = conn->prepareStatement(sql);

	if (pos != 0 || n != 0) {
		stmt->setUInt64(1, pos);
		stmt->setUInt64(2, n);
	}

	ResultSet result = stmt->executeQuery();
	std::auto_ptr<yiqiding::container::SmartVector<model::Game> > list(new yiqiding::container::SmartVector<model::Game>);
	std::auto_ptr<model::Game> game;
	while(result->next())
	{
		game.reset(new model::Game);

		game->setKId(result->getUInt(1));
		game->setKType(result->getInt(2));
		game->setKStartTime(result->getUInt(3));
		{
			std::string ksendtime = result->getString(4);
			if(ksendtime != "")
			{
				std::vector<std::string>  vec = yiqiding::utility::split(ksendtime , ',');
				std::set<uint32_t> ksendtimes;
				for  each(auto t in vec)
				{
					if(!yiqiding::utility::isULong(t))
						break;
					ksendtimes.insert(yiqiding::utility::toUInt(t));
				}
				game->setKSendTime(ksendtimes);
			}
		}
		game->setKState(result->getInt(5));
		game->setKTitle(result->getString(6));
		game->setKMessage(result->getString(7));
		{
			std::string kmids = result->getString(8);
			if(kmids != "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(kmids, '/');
				std::vector<MidName> mids;
				std::vector<std::string>::iterator it = vec.begin();
				if (vec.size() % 2 != 0)
					break;
				while(it != vec.end())
				{
					if (!yiqiding::utility::isULong(*it))	
						break;
			
					mids.push_back(MidName(yiqiding::utility::toUInt(*it) , *(it + 1)));
					++it;
					++it;
				}
				game->setKMids(mids);
			}
		}
		{
			std::string kjoins = result->getString(9);
			if(kjoins != "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(kjoins , ',');
				std::set<uint32_t> joins;
				for each(auto j in vec)
				{
					if (!yiqiding::utility::isULong(j))
						break;
					joins.insert(yiqiding::utility::isULong(j));
				}
				game->setKJoins(joins);
			}
		}
		{
			std::string scores = result->getString(10);
			if(scores != "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(scores , ',');
				std::vector<BoxScore> kscores;
				if(vec.size() % 2 != 0)
					break;
				std::vector<std::string>::iterator it = vec.begin();
				while(it != vec.end())
				{
					if (!yiqiding::utility::isULong(*it) || !yiqiding::utility::isULong(*(it + 1)))
						break;

					kscores.push_back(BoxScore(yiqiding::utility::toUInt(*it) , yiqiding::utility::toUInt(*(it + 1))));
					++it;
					++it;
				}

				game->setKScores(kscores);
			}
		}
		list->push_back(game.release());
	}

	_pool->returnConnection(conn);

	return list;

}

std::auto_ptr<model::Game> MediaConnector::getGame(unsigned int kid)
{
	Connection conn = _pool->getConnection();

	PreparedStatement stmt;
	std::string sql = "SELECT kid , ktype , kstarttime , ksendtime , kstate , ktitle , kmessage , kmids , kjoins , kscores from ktv_game where kid = ?";
	stmt = conn->prepareStatement(sql);
	stmt->setUInt(1, kid);
	ResultSet result = stmt->executeQuery();
	std::auto_ptr<model::Game> game(new model::Game);
	if (!result->next())
		 throw DBException("Game: kid " + toString(kid) + "not find" , __FILE__, __LINE__);  
	
	{
		game->setKId(result->getUInt(1));
		game->setKType(result->getInt(2));
		game->setKStartTime(result->getUInt(3));
		{
			std::string ksendtime = result->getString(4);
			if(ksendtime != "")
			{
				std::vector<std::string>  vec = yiqiding::utility::split(ksendtime , ',');
				std::set<uint32_t> ksendtimes;
				for  each(auto t in vec)
				{
					if(!yiqiding::utility::isULong(t))
						break;
					ksendtimes.insert(yiqiding::utility::toUInt(t));
				}
				game->setKSendTime(ksendtimes);
			}
		}
		game->setKState(result->getInt(5));
		game->setKTitle(result->getString(6));
		game->setKMessage(result->getString(7));
		{
			std::string kmids = result->getString(8);
			if(kmids !=  "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(kmids, '/');
				std::vector<MidName> mids;
				std::vector<std::string>::iterator it = vec.begin();
				if (vec.size() % 2 != 0)
					 throw DBException("Game: kid " + toString(kid) + "Data  Error" , __FILE__, __LINE__);
				while(it != vec.end())
				{
					if (!yiqiding::utility::isULong(*it))	
						break;

					mids.push_back(MidName(yiqiding::utility::toUInt(*it) , *(it + 1)));
					++it;
					++it;
				}
				game->setKMids(mids);
			}
		}
		{
			std::string kjoins = result->getString(9);
			if(kjoins != "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(kjoins , ',');
				std::set<uint32_t> joins;
				for each(auto j in vec)
				{
					if (!yiqiding::utility::isULong(j))
						break;
					joins.insert(yiqiding::utility::isULong(j));
				}
				game->setKJoins(joins);
			}
		}
		{
			std::string scores = result->getString(10);
			if(scores != "")
			{
				std::vector<std::string> vec = yiqiding::utility::split(scores , ',');
				std::vector<BoxScore> kscores;
				if(vec.size() % 2 != 0)
					throw DBException("Game: kid " + toString(kid) + "Data  Error" , __FILE__, __LINE__);
				std::vector<std::string>::iterator it = vec.begin();
				while(it != vec.end())
				{
					if (!yiqiding::utility::isULong(*it) || !yiqiding::utility::isULong(*(it + 1)))
						break;

					kscores.push_back(BoxScore(yiqiding::utility::toUInt(*it) , yiqiding::utility::toUInt(*(it + 1))));
					++it;
					++it;
				}

				game->setKScores(kscores);
			}
		}
	}


	_pool->returnConnection(conn);

	return game;
}

void MediaConnector::addGame(const model::Game &game)
{
	Connection conn = _pool->getConnection();
	{
		PreparedStatement stmt = conn->prepareStatement("INSERT INTO `yiqiding_info`.`ktv_game`"
			"(`kid`, `ktype`, `kstarttime`, `ksendtime`, `kstate`, `ktitle`,"
			"`kmessage`, `kmids`, `kjoins`, `kscores`) VALUES(?,?,?,?,?,?,?,?,?,?)");
		
		stmt->setUInt		(1,  game.getKId());
		stmt->setInt			(2,  game.getKType());
		stmt->setUInt		(3,  game.getKStartTime());
		{
			std::ostringstream out;
			int index = 0;
			for each(auto t in game.getKSendTime())
			{
				if (index++ == 0)
				{
					out << t;
				}
				else
				{	
					out << "," << t ;
				}
			}
			stmt->setString	(4 , out.str());	
		}
		stmt->setInt(5	,	game.getKState());
		stmt->setString(6 , game.getKTitle());
		stmt->setString(7 , game.getKMessage());
		{
			std::ostringstream out;
			int index = 0;
			for each(auto t in game.getKMids())
			{
				if (index++ == 0)
				{
					out << t._mid ;
				}
				else 
				{
					out << "/" << t._mid;
				}

				out << "/" << t._name ;
			}
			stmt->setString(8 , out.str());
		}
		{
			std::ostringstream out;
			int index = 0;
			for each(auto t in game.getKJoins())
			{
				if (index++ == 0)
				{
					out << t ;
				}
				else
				{
					out << "," << t;
				}
			}
			stmt->setString(9 , out.str());
		}
		{
			std::ostringstream out;
			int index = 0;
			for each(auto t in game.getKScores())
			{
				if (index++ == 0)
				{
					out << t._boxid ;
				}
				else
				{
					out << ","<<t._boxid;
				}
				out << "," << t._score;
			}

			stmt->setString(10 , out.str());
		}
		stmt->execute();
	}
	
	_pool->returnConnection(conn);

}

uint32_t MediaConnector::getGameId()
{
	Connection conn = _pool->getConnection();
	{
		uint32_t id = 0;
		PreparedStatement stmt = conn->prepareStatement("select max(kid) from yiqiding_info.ktv_game");
		ResultSet result = stmt->executeQuery();
		if (result->next())
		{
			id = result->getUInt(1);
		}

		_pool->returnConnection(conn);
		return id;
	}
}

uint32_t MediaConnector::countGameList()
{
	Connection conn = _pool->getConnection();

	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("SELECT COUNT(*) FROM `yiqiding_info`.`ktv_game`");
	if (!result->next())
		throw DBException("Fail to count activity", __FILE__, __LINE__);
	uint32_t count = result->getUInt(1);

	_pool->returnConnection(conn);

	return count;
}

int MediaConnector::getGame2Count()
{
	Connection conn = _pool->getConnection();

	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("select count(*) from yiqiding_info.ktv_game2");
	if (!result->next())
		throw DBException("Fail to count activity", __FILE__, __LINE__);
	uint32_t count = result->getUInt(1);

	_pool->returnConnection(conn);

	return count;
}
unsigned int MediaConnector::addGame2(const model::Game2 &game)
{
	Connection conn = _pool->getConnection();
	{
		PreparedStatement stmt = conn->prepareStatement("insert into yiqiding_info.ktv_game2(kname , kdesc ,ksongs, kaward , kstarttime , knum , kboxids) values(?,?,?,?,?,?,?)");
		stmt->setString(1 , game.getTitle());
		stmt->setString(2 , game.getDesc());
		stmt->setString(3 , game.getSongs());
		stmt->setString(4 , game.getAwards());
		stmt->setUInt(5 , game.getTimeofStart());
		stmt->setUInt(6 , game.getNum());
		stmt->setString(7 , game.getBoxids());
		stmt->execute();
	}

	
	Statement stmt = conn->createStatement();
	ResultSet result = stmt->executeQuery("select last_insert_id()");
	if (!result->next())
		throw DBException("fail get insert id" , __FILE__ , __LINE__);
	unsigned int id = result->getUInt(1);
	
	_pool->returnConnection(conn);

	return id;
}

 void MediaConnector::delGame2(int id)
 {
	 Connection conn = _pool->getConnection();
	 PreparedStatement stmt = conn->prepareStatement("delete from yiqiding_info.ktv_game2 where kid = ?");
	 stmt->setUInt(1  , id);
	 stmt->execute();

	 _pool->returnConnection(conn);
 }

 std::auto_ptr< std::vector<model::Game2> > MediaConnector::getHisGame2(int num)
 {
	 std::auto_ptr< std::vector<model::Game2> > list(new std::vector<model::Game2>());
	 Connection conn = _pool->getConnection();

	 Statement stmt = conn->createStatement();
	 ResultSet result = stmt->executeQuery("select kid,kname,kdesc,kaward,kstarttime,kscores,kstate,ksongs,knum from yiqiding_info.ktv_game2 where kstate = 1 order by kaddtime desc limit " + yiqiding::utility::toString(num));
	 while(result->next())
	 {
		 model::Game2 game;
		 game.setId(result->getUInt("kid"));
		 game.setTitle(result->getString("kname"));
		 game.setTimeofStart(result->getUInt("kstarttime"));
		 game.setScores(result->getString("kscores"));
		 game.setComplete(result->getBoolean("kstate"));
		 game.setDesc(result->getString("kdesc"));
		 game.setAwards(result->getString("kaward"));
		 game.setSongs(result->getString("ksongs"));
		 game.setNum(result->getUInt("knum"));
		 list->push_back(game);
	 }

	 _pool->returnConnection(conn);

	 return list;
 }

 std::auto_ptr< std::vector<model::Game2> > MediaConnector::getAllGame2(int page , int num  , bool *pStatus)
 {
	 std::auto_ptr< std::vector<model::Game2> > list(new std::vector<model::Game2>());
	 Connection conn = _pool->getConnection();

	 Statement stmt = conn->createStatement();
	 
	 std::string sql = "select kid,kname,kdesc,kaward,kstarttime,kscores,kstate,ksongs,knum from yiqiding_info.ktv_game2";
	 
	 if(pStatus != NULL)
	 {	
		 sql += " where kstate = ";
		 sql += (*pStatus ? "1" : "0");
	 }
	 sql += " order by kaddtime desc limit " + yiqiding::utility::toString(page * num) + " , " + yiqiding::utility::toString(num);

	
	 ResultSet result = stmt->executeQuery(sql);
	 while(result->next())
	 {
		 model::Game2 game;
		 game.setId(result->getUInt("kid"));
		 game.setTitle(result->getString("kname"));
		 game.setTimeofStart(result->getUInt("kstarttime"));
		 game.setScores(result->getString("kscores"));
		 game.setComplete(result->getBoolean("kstate"));
		 game.setDesc(result->getString("kdesc"));
		 game.setAwards(result->getString("kaward"));
		 game.setSongs(result->getString("ksongs"));
		 game.setNum(result->getUInt("knum"));
		 list->push_back(game);
	 }

	 _pool->returnConnection(conn);

	 return list;
 }

 void MediaConnector::updateGame2(int id ,const std::string &score)
 {

	 Connection conn = _pool->getConnection();
	 PreparedStatement stmt = conn->prepareStatement("update yiqiding_info.ktv_game2 set kscores = ? , kstate = 1 where kid = ?");
	 stmt->setString(1 , score.c_str());
	 stmt->setUInt(2 , id);
	 stmt->execute();
	 _pool->returnConnection(conn);
 }


