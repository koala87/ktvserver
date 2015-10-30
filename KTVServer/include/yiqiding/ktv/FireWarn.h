#pragma once
#include <string>
#include <queue>
#include "yiqiding/Thread.h"

namespace yiqiding{ namespace ktv{ namespace fire{

const  std::string path = "fire.json";
class FireWarn
{
	bool _fireStatus;	
public:
	FireWarn():_fireStatus(false){}


	bool load(const std::string &path);
	bool save(const std::string &path ,const std::string &fireImageUrl , const std::string &fireVideoUrl);
	bool getStatus() const {yiqiding::MutexGuard guard(_mutex);  return _fireStatus;}
	void setStatus(bool fireStatus){yiqiding::MutexGuard guard(_mutex); _fireStatus = fireStatus;}
	std::string  getImageUrl() const {yiqiding::MutexGuard guard(_mutex);return _fireImageUrl;}
	std::string  getVideoUrl() const {yiqiding::MutexGuard guard(_mutex);return _fireVideoUrl;}
	 
private:
	std::string _fireImageUrl;
	std::string _fireVideoUrl;
	mutable yiqiding::Mutex _mutex;
};



}}}

