#include "yiqiding/ktv/FireWarn.h"
#include "yiqiding/io/File.h"
#include "json/json.h"

using namespace yiqiding::ktv::fire;
using namespace yiqiding::io;

bool FireWarn::load(const std::string &path)
{
	File fp(path);
	try{
		MutexGuard guard(_mutex);
		fp.open(File::READ);
		DWORD low = GetFileSize(fp.native() , &low);
		std::auto_ptr<char> pdata(new char[low + 1]);
		fp.read(pdata.get() , low);
		pdata.get()[low] = 0;
		fp.close();
		Json::Value root;
		Json::Reader reader;
		if(!reader.parse( pdata.get() ,root))
			return false;
		if(!root.isMember("fireImageUrl") || !root["fireImageUrl"].isString() ||
		   !root.isMember("fireVideoUrl") || !root["fireVideoUrl"].isString())
		{
			return false;
		}
		_fireImageUrl = root["fireImageUrl"].asString();
		_fireVideoUrl = root["fireVideoUrl"].asString();
		return true;
	}catch(...){ }
	return false;
}

bool FireWarn::save(const std::string &path ,const std::string &fireImageUrl,const std::string &fireVideoUrl)
{
	File fp(path);
	try {
		MutexGuard guard(_mutex);
		_fireImageUrl = fireImageUrl;
		_fireVideoUrl = fireVideoUrl;
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		std::string json = "{\"fireImageUrl\" : \""+ fireImageUrl + "\",\"fireVideoUrl\":\"" + fireVideoUrl + "\"}";
		fp.write(json.c_str() , json.length());
		fp.close();
		return true;
	} catch (...) { }
	return false;
}