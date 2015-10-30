/**
 * KTV Database Media Model
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.14
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <cstdint>

namespace yiqiding { namespace ktv { 
	
	
	class BoxScore
	{
	public:
		uint32_t _boxid;
		uint32_t _score;
		BoxScore(uint32_t boxid , uint32_t score):_boxid(boxid),_score(score){}
		bool operator >(const BoxScore &b) const
		{
			return (_score > b._score) || (_score == b._score && _boxid < b._boxid);
		}
	};

	class MidName
	{
	public: 
		uint32_t _mid;
		std::string _name;
		MidName & operator = (const MidName &m)
		{
			if(this != &m)
			{_mid = m._mid;
			_name = m._name;
			}
			return *this;
		}
		MidName(uint32_t mid , const std::string &name)
		{
			_mid = mid;
			_name = name;
		}
	};
	
	
	
	
	namespace db { namespace model {
	static const std::string EMPTY_STRING;



	class Activity {
	private:
		unsigned int	_aid;
		int				_serial_id;
		std::string		_title;
		std::string		_thumb;
		std::string		_type;
		int				_member_count;
		std::string		_start_date;
		std::string		_end_date;
		std::string		_start_time;
		std::string		_end_time;
		std::string		_status;
		std::string		_address;
		double			_fee;
		std::string*	_sponsor;
		std::string*	_photos;
		std::string*	_description;
	public:
		Activity() : _aid(0), _serial_id(0), _title(), _thumb(), _type(), _member_count(0), _start_date(), _end_date(), _start_time(), _end_time(), _status(), _address(), _fee(0), _sponsor(NULL), _photos(NULL), _description(NULL) {};
		~Activity() { delete _sponsor; delete _photos; delete _description;};

		// Is Null?
		inline bool isSponsorNull()		const { return _sponsor == NULL; };
		inline bool isPhotosNull()		const { return _photos == NULL; };
		inline bool isDescriptionNull()	const { return _description == NULL; };

		// Getter
		inline unsigned int			getAid()			const { return _aid; };
		inline int					getSerialId()		const { return _serial_id; };
		inline const std::string&	getTitle()			const { return _title; };
		inline const std::string&	getThumb()			const { return _thumb; };
		inline const std::string&	getType()			const { return _type; };
		inline int					getMemberCount()	const { return _member_count; };
		inline const std::string&	getStartDate()		const { return _start_date; };
		inline const std::string&	getEndDate()		const { return _end_date; };
		inline const std::string&	getStartTime()		const { return _start_time; };
		inline const std::string&	getEndTime()		const { return _end_time; };
		inline const std::string&	getStatus()			const { return _status; };
		inline const std::string&	getAddress()		const { return _address; };
		inline double				getFee()			const { return _fee; };
		inline const std::string&	getSponsor()		const { return _sponsor ? *_sponsor : EMPTY_STRING; };
		inline const std::string&	getPhotos()			const { return _photos ? *_photos : EMPTY_STRING; };
		inline const std::string&	getDescription()	const { return _description ? *_description : EMPTY_STRING; };

		// Setter
		inline void setAid(unsigned int aid)						{ _aid = aid; };
		inline void setSerialId(int serial_id)						{ _serial_id = serial_id; };
		inline void setTitle(const std::string& title)				{ _title = title; };
		inline void setThumb(const std::string& thumb)				{ _thumb = thumb; };
		inline void setType(const std::string& type)				{ _type = type; };
		inline void setMemberCount(int member_count)				{ _member_count = member_count; };
		inline void setStartDate(const std::string& start_date)		{ _start_date = start_date; };
		inline void setEndDate(const std::string& end_date)			{ _end_date = end_date; };
		inline void setStartTime(const std::string& start_time)		{ _start_time = start_time; };
		inline void setEndTime(const std::string& end_time)			{ _end_time = end_time; };
		inline void setStatus(const std::string& status)			{ _status = status; };
		inline void setAddress(const std::string& address)			{ _address = address; };
		inline void setFee(double fee)								{ _fee = fee; };
		inline void setSponsor()									{ delete _sponsor; _sponsor = NULL; };
		inline void setSponsor(const std::string& sponsor)			{ if (!_sponsor) _sponsor = new std::string; *_sponsor = sponsor; };
		inline void setPhotos()										{ delete _photos; _photos = NULL; };
		inline void setPhotos(const std::string& photos)			{ if (!_photos) _photos = new std::string; *_photos = photos; };
		inline void setDescription()								{ delete _description; _description = NULL; };
		inline void setDescription(const std::string& description)	{ if (!_description) _description = new std::string; *_description = description; };
	};

	class Actor {
	private:
		unsigned int	_sid;
		int				_serial_id;
		std::string		_name;
		float*			_stars;
		unsigned int	_type;
#ifndef REMOVE_SQL_IMAGE
		std::string		_nation;
		std::string*	_image;
#endif
		std::string		_pinyin;
		int				_song_count;
		int				_count;
		std::string		_header;
		char*			_head;
	public:
		Actor() : _sid(0), _serial_id(0), _name(), _stars(NULL), _type(0), 
#ifndef REMOVE_SQL_IMAGE	
			_nation(),
			_image(NULL),
#endif			
			_pinyin(), _song_count(0), _count(0), _header(), _head(NULL) {};
		~Actor() { delete _stars;
#ifndef REMOVE_SQL_IMAGE	
		delete _image;
#endif
		delete _head; };

		// Is Null?
		inline bool isStarsNull()	const { return _stars == NULL; };
#ifndef REMOVE_SQL_IMAGE		
		inline bool isImageNull()	const { return _image == NULL; };
#endif		
		inline bool isHeadNull()	const { return _head == NULL; };

		// Getter
		inline unsigned int			getSid()		const { return _sid; };
		inline int					getSerialId()	const { return _serial_id; };
		inline const std::string&	getName()		const { return _name; };
		inline float				getStars()		const { return _stars ? *_stars : 0; };
		inline unsigned int			getType()		const { return _type; };
#ifndef REMOVE_SQL_IMAGE
		inline const std::string&	getNation()		const { return _nation; };
		inline const std::string&	getImage()		const { return _image ? *_image : EMPTY_STRING; };
#endif	
		inline const std::string&	getPinyin()		const { return _pinyin; };
		inline int					getSongCount()	const { return _song_count; };
		inline int					getCount()		const { return _count; };
		inline const std::string&	getHeader()		const { return _header; };
		inline char					getHead()		const { return _head ? *_head : 0; };

		// Setter
		inline void setSid(unsigned int sid)				{ _sid = sid; };
		inline void setSerialId(int serial_id)				{ _serial_id = serial_id; };
		inline void setName(const std::string& name)		{ _name = name; };
		inline void setStars()								{ delete _stars; _stars = NULL; };
		inline void setStars(float stars)					{ if (!_stars) _stars = new float; *_stars = stars; };
		inline void setType(unsigned int type)				{ _type = type; };
#ifndef REMOVE_SQL_IMAGE	

		inline void setNation(const std::string& nation)	{ _nation = nation; };
		inline void setImage()								{ delete _image; _image = NULL; };
		inline void setImage(const std::string& image)		{ if (!_image) _image = new std::string; *_image = image; };
#endif		
		inline void setPinyin(const std::string& pinyin)	{ _pinyin = pinyin; };
		inline void setSongCount(int song_count)			{ _song_count = song_count; };
		inline void setCount(int count)						{ _count = count; };
		inline void setHeader(const std::string& header)	{ _header = header; };
		inline void setHead()								{ delete _head; };
		inline void setHead(char head)						{ if (!_head) _head = new char; *_head = head; };
	};
	
	class Media {
	private:
		unsigned int	_mid;
		int				_serial_id;
		std::string		_name;
		std::string		_singer;
#ifndef REMOVE_SQL_IMAGE		
		std::string*	_image;
#endif
		unsigned int	_language;
		unsigned int	_type;
		float			_stars;
		int				_count;
		std::string		_path;
		int*			_artist_sid_1;
		int*			_artist_sid_2;
		std::string		_pinyin;
		std::string*	_lyric;
		std::string		_header;
		int				_original_track;
		int				_sound_track;
		int				_words;
		bool			_black;
		bool			_hot;
		char*			_head;
	public:
		Media() : _mid(0), _serial_id(0), _name(), _singer(),
#ifndef REMOVE_SQL_IMAGE			
			_image(NULL),
#endif			
			_language(0), _type(0), _stars(0), _count(0), _path(), _artist_sid_1(NULL), _artist_sid_2(NULL), _pinyin(), _lyric(NULL), _header(), _original_track(0), _sound_track(0), _words(0), _black(0), _hot(0), _head(NULL) {};
		~Media() { 
#ifndef REMOVE_SQL_IMAGE			
			delete _image;
#endif			
			delete _artist_sid_1; delete _artist_sid_2; delete _lyric; delete _head; };

		// Is Null?
#ifndef REMOVE_SQL_IMAGE
		inline bool isImageNull()		const { return _image == NULL; };
#endif		
		inline bool isArtistSid1Null()	const { return _artist_sid_1 == NULL; };
		inline bool isArtistSid2Null()	const { return _artist_sid_2 == NULL; };
		inline bool isLyricNull()		const { return _lyric == NULL; };
		inline bool isHeadNull()		const { return _head == NULL; };

		// Getter
		inline unsigned int			getMid()			const { return _mid; };
		inline int					getSerialId()		const { return _serial_id; };
		inline const std::string&	getName()			const { return _name; };
		inline const std::string&	getSinger()			const { return _singer; };
#ifndef REMOVE_SQL_IMAGE
		inline const std::string&	getImage()			const { return _image ? *_image : EMPTY_STRING; };
#endif
		inline unsigned int			getLanguage()		const { return _language; };
		inline unsigned int			getType()			const { return _type; };
		inline float				getStars()			const { return _stars; };
		inline int					getCount()			const { return _count; };
		inline const std::string&	getPath()			const { return _path; };
		inline int					getArtistSid1()		const { return _artist_sid_1 ? *_artist_sid_1 : 0; };
		inline int					getArtistSid2()		const { return _artist_sid_2 ? *_artist_sid_2 : 0; };
		inline const std::string&	getPinyin()			const { return _pinyin; };
		inline const std::string&	getLyric()			const { return _lyric ? *_lyric : EMPTY_STRING; };
		inline const std::string&	getHeader()			const { return _header; };
		inline int					getOriginalTrack()	const { return _original_track; };
		inline int					getSoundTrack()		const { return _sound_track; };
		inline int					getWords()			const { return _words; };
		inline bool					getBlack()			const { return _black; };
		inline bool					getHot()			const { return _hot; };
		inline char					getHead()			const { return _head ? *_head : 0; };

		// Setter
		inline void setMid(unsigned int mid)				{ _mid = mid; };
		inline void setSerialId(int serial_id)				{ _serial_id = serial_id; };
		inline void setName(std::string name)				{ _name = name; };
		inline void setSinger(std::string singer)			{ _singer = singer; };
#ifndef REMOVE_SQL_IMAGE		
		inline void setImage()								{ delete _image; _image = NULL; };
		inline void setImage(const std::string& image)		{ if (!_image) _image = new std::string; *_image = image; };
#endif		
		inline void setLanguage(unsigned int language)		{ _language = language; };
		inline void setType(unsigned int type)				{ _type = type; };
		inline void setStars(float stars)					{ _stars = stars; };
		inline void setCount(int count)						{ _count = count; };
		inline void setPath(std::string path)				{ _path = path; };
		inline void setArtistSid1()							{ delete _artist_sid_1; _artist_sid_1 = NULL; };
		inline void setArtistSid1(int artist_sid_1)			{ if (!_artist_sid_1) _artist_sid_1 = new int; *_artist_sid_1 = artist_sid_1; };
		inline void setArtistSid2()							{ delete _artist_sid_2; _artist_sid_2 = NULL; };
		inline void setArtistSid2(int artist_sid_2)			{ if (!_artist_sid_2) _artist_sid_2 = new int; *_artist_sid_2 = artist_sid_2; };
		inline void setPinyin(std::string pinyin)			{ _pinyin = pinyin; };
		inline void setLyric()								{ delete _lyric; _lyric = NULL; };
		inline void setLyric(const std::string& lyric)		{ if (!_lyric) _lyric = new std::string; *_lyric = lyric; };
		inline void setHeader(std::string header)			{ _header = header; };
		inline void setOriginalTrack(int original_track)	{ _original_track = original_track; };
		inline void setSoundTrack(int sound_track)			{ _sound_track = sound_track; };
		inline void setWords(int words)						{ _words = words; };
		inline void setBlack(bool black)					{ _black = black; };
		inline void setHot(bool hot)						{ _hot = hot; };
		inline void setHead()								{ delete _head; _head = NULL; };
		inline void setHead(char head)						{ if (!_head) _head = new char; *_head = head; };
	};

	class SongList {
	private:
		unsigned int				_lid;
		int							_serial_id;
		std::string					_title;
		std::string					_image;
		std::string					_type;
		int							_count;
		bool						_special;
		std::vector<unsigned int>	_songs;
	public:
		SongList() : _lid(0), _serial_id(0), _title(), _image(), _type(), _count(0), _special(0) {};
		~SongList()	{};

		// Getter
		inline unsigned int						getLid()		const { return _lid; };
		inline int								getSerialId()	const { return _serial_id; };
		inline const std::string&				getTitle()		const { return _title; };
		inline const std::string&				getImage()		const { return _image; };
		inline const std::string&				getType()		const { return _type; };
		inline int								getCount()		const { return _count; };
		inline bool								getSpecial()	const { return _special; };
		inline const std::vector<unsigned int>&	getSongs()		const { return _songs; };

		// Setter
		inline void setLid(unsigned int lid)							{ _lid = lid; };
		inline void setSerialId(int serial_id)							{ _serial_id = serial_id; };
		inline void setTitle(const std::string& title)					{ _title = title; };
		inline void setImage(const std::string& image)					{ _image = image; };
		inline void setType(const std::string& type)					{ _type = type; };
		inline void setCount(int count)									{ _count = count; };
		inline void setSpecial(bool special)							{ _special = special; };
		inline void setSongs(const std::vector<unsigned int>& songs)	{ _songs = songs; };
	};
	

	class Game
	{
		unsigned int								_kid;
		int											_ktype;
		unsigned int								_kstarttime;
		std::set<unsigned int>						_ksendtime;
		int											_kstate;
		std::string									_ktitle;
		std::string									_kmessage;
		std::vector<yiqiding::ktv::MidName>			_kmids;
		std::set<unsigned int>						_kjoins;
		std::vector<yiqiding::ktv::BoxScore>		_kscores;
	public:
		Game():_kid(0), _ktype(0), _kstarttime(0), _kstate(){};
		~Game()	{};

		// Getter
		inline unsigned int									getKId()					const { return _kid; };
		inline int											getKType()					const { return _ktype; };
		inline unsigned int									getKStartTime()				const { return _kstarttime; };
		inline const std::set<unsigned int>&				getKSendTime()				const { return _ksendtime; };
		inline int											getKState()					const { return _kstate; };
		inline const std::string&							getKTitle()					const { return _ktitle; };
		inline const std::string&							getKMessage()				const { return _kmessage; };
		inline const std::set<unsigned int>&				getKJoins()					const { return _kjoins; };
		inline const std::vector<yiqiding::ktv::MidName>&	getKMids()					const { return _kmids; };	
		inline const std::vector<yiqiding::ktv::BoxScore>&	getKScores()				const { return _kscores; };

		// Setter
		inline void									setKId(unsigned int kid)										{ _kid = kid; };
		inline void									setKType(int ktype)												{ _ktype = ktype; };
		inline void									setKStartTime(unsigned int kstarttime)							{ _kstarttime = kstarttime; };
		inline void									setKSendTime(const std::set<unsigned int> & ksendtime)			{ _ksendtime = ksendtime; };
		inline void									setKState(int kstate)											{ _kstate = kstate; };
		inline void									setKTitle(const std::string &ktitle)							{ _ktitle = ktitle; };
		inline void									setKMessage(const std::string &kmessage)						{ _kmessage = kmessage; };
		inline void									setKJoins(const std::set<unsigned int> &kjoins)					{ _kjoins = kjoins; };
		inline void									setKMids(const std::vector<yiqiding::ktv::MidName> &kmids)		{ _kmids = kmids; };	
		inline void									setKScores(const std::vector<yiqiding::ktv::BoxScore> &kscores)	{ _kscores = kscores; };

	};

	class Game2
	{
		unsigned int _id;				//游戏编号
		unsigned int _timeofStart;		//开始时间
		std::string _title;				//比赛主题
		std::string _songs;				//比赛歌曲
		std::string _scores;				//分数
		std::string _awards;				//奖品
		std::string _desc;				//描述
		bool _isComplete;				//是否结束
		unsigned int _num;				//数目
		std::string _boxids;			//机顶盒编号
	public:
		Game2():_isComplete(false){}

		//getter
		inline const std::string& getSongs() const		{return _songs;}
		inline const std::string& getScores() const		{return _scores;}
		inline const std::string& getAwards() const		{return _awards;}
		inline const std::string& getDesc() const		{return _desc;}
		inline bool				  getIsComplete() const	{return _isComplete;}
		inline int				  getId() const			{return _id;}
		inline const std::string& getTitle() const		{return _title;}
		inline unsigned int		  getTimeofStart() const{return _timeofStart;}
		inline unsigned int		  getNum() const		{return _num;}
		inline const std::string& getBoxids()const		{return _boxids;}
		//setters
		
		inline void setId(unsigned int id)				 {_id = id;}
		inline void setSongs(const std::string &songs)	 { _songs = songs;}
		inline void setScores(const std::string &scores) { _scores = scores;}
		inline void setAwards(const std::string &awards) { _awards = awards;}
		inline void setDesc(const std::string &desc)     {_desc = desc;}
		inline void setComplete(bool isComplete)		 { _isComplete = isComplete;}
		inline void setTitle(const std::string &title)	 {_title = title;}
		inline void setTimeofStart(unsigned int timeofStart) {_timeofStart = timeofStart;}
		inline void setNum(unsigned int num)				 {_num = num;}
		inline void setBoxids(const std::string &boxids)	{_boxids = boxids;}
	};

}}}}

namespace yiqiding { namespace ktv { namespace model {
	using namespace yiqiding::ktv::db::model;
}}}
