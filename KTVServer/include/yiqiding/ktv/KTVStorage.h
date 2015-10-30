/**
 * KTV Storage
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.19
 */

#pragma once
#include <cstdint> 
#include "yiqiding/net/TCP.h"
#include "yiqiding/Thread.h"
#include "yiqiding/io/FileWriteMap.h"
#include "yiqiding/crypto/MD5.h"
#include "yiqiding/io/FileDistribute.h"
#include "yiqiding/net/FileCurlDistribute.h"


namespace yiqiding { namespace ktv {

	const int MaxStoragePacketSize = 1024 * 1024 * 512;

	enum PacketType
	{
		TYPE_NONE,


		TYPE_MEDIA					= 1,
		TYPE_ROM					= 2,
		TYPE_APK					= 3,
		TYPE_IMAGE					= 4,
		TYPE_AVATAR					= 5,
		TYPE_FM						= 6,
		TYPE_LYRIC					= 7,
		TYPE_MEIDA_MPG				= 8,
		TYPE_END,


		TYPE_ROM_LOG					= 100,
		TYPE_APK_LOG					= 101,
		TYPE_BOX_DATA					= 102,
		TYPE_MID						= 103,
		TYPE_BOX_LOG					= 104,
	};

	static const char *PackettypeStr[] = {
		"start",
		"mp4",
		"rom",
		"apk",
		"image",
		"avatar",
		"fm",
		"lyric",
		"mp4",
	};



	class StoragePacket
	{
	private:
		uint32_t _content;
		uint8_t _format[32];
		uint8_t _md5[16];
		uint32_t _length;
	public:
		StoragePacket(){ memset(this , 0 , sizeof(*this));}		
		//Getter
		uint32_t			getContent() const		{ return _content;}
		const uint8_t *		getFormat()  const		{ return _format;}
		const uint8_t *		getMD5()	 const		{ return _md5;	}
		uint32_t			getLength()	 const		{ return _length;}
		//Setter
		void				setLength(uint32_t len) { _length = len ;}

		void NtoH()	
		{
			_content = ntohl(_content);
			_length  = ntohl(_length); 
		}

		void HtoN(){ NtoH();}
	};

	class KTVStorageClientHandler : public Runnable {

	

	private:
		net::tcp::Client _client;
		std::string generateName(const std::string &md5 , const std::string &suffix);
		std::string generateName(const std::string &originName);
	public:
		KTVStorageClientHandler(net::tcp::Client client):_client(client){};
		~KTVStorageClientHandler() {};
		void run();
		bool processUpLoad(StoragePacket & ,const std::string &path , const std::string &name);
		bool sendBack( StoragePacket & ,bool , const std::string &);
		bool sendBackToBox( StoragePacket & ,bool );

		static yiqiding::Mutex _sMutex;
	};

	const int packetSize = sizeof(StoragePacket);

	class KTVStorage : public net::tcp::Server, private Runnable {
	private:
		ThreadPool	*_pool;
		bool		_is_running;
		void run();
	public:
		KTVStorage(ThreadPool *pool) : _pool(pool),_is_running(false) {};
		~KTVStorage() { stop(); };
		void start();
		void stop();
	};

}}
