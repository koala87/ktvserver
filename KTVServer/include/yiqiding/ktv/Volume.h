/*
@author Zhangyuchun
@Date	2014-12-24
@Content //待到机顶盒有时间 可以把adurl firewarn messagerule volume 几个功能相近的设置合并。
*/

#pragma once
#include <string>
#include "yiqiding/Thread.h"

namespace yiqiding { namespace ktv{ 


	const std::string VlPath = "volume.json";

	class Volume
	{
	public:
		void load(const std::string &path);
		bool save(const std::string &path , const std::string &volumeInfo);
		std::string getVolumeInfo() const ;
	private:
		std::string _volumeInfo;
		mutable yiqiding::Mutex _mutex;
	};

} }