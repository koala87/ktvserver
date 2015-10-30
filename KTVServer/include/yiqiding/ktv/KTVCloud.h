#pragma once

#include "yiqiding/Thread.h"
#include "json/json.h"
#include "KTVServer.h"
#include "yiqiding/utility/Transcode.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Logger.h"
namespace yiqiding { namespace ktv { namespace cloud{


/*
传说中的云更新，估计是一个非常坑的功能。
没有需求文档，没有产品文档，就两个开发人员，就想完成这个？
2015-7-2 bill 哥终于出场。
*/

//json文件
//状态
//下载 download，解压  执行 execute， 上传 upload , 检测 check.


/*

{"status":"check|download|unzip|request|execute|upload" , "download-server":{"version" :,"url"}, "download-index":, "version": "",type :"music"}
{}

*/


enum CloudState
{
	CloudCheck = 0,
	CloudRequest = 1,
	CloudDownload = 2,
	CloudUnzip = 3,
	CloudExecute = 4,
	CloudUpload = 5,
};



class CloudItem
{
private:

	yiqiding::Mutex		_mutex;
	yiqiding::Condition _cond;
	bool				_result;
protected:
	CloudState			_status;

	bool request_s(){ 
		if(!request()) 
			return false;
		MutexGuard guard(_mutex);
		_cond.wait(_mutex);
		if(_result)
			_status = CloudDownload;
		return _result;
	}

	virtual bool load(yiqiding::ktv::Server *s,const std::string &path)=0;
	virtual bool save() = 0;
	virtual bool check()	{_status = CloudDownload; return true;}
	virtual bool download()	{_status = CloudUnzip ;return true;}
	virtual bool unzip()	{_status = CloudRequest;return true;}
	virtual bool request()	{_status = CloudExecute;return true;}
	virtual bool execute()	{_status = CloudUpload;return true;}
	virtual bool upload()	{_status = CloudCheck;return true;}
	virtual bool finish(bool result)	{return true;}
public:
	bool notify(bool result) {
		 _result = result;
		 _cond.signal();
		 if(result)
		 {
			_status = CloudExecute;
		 }
		 return result;
	 }
	void operator()() 
	{
		bool result;
	 do{
		 result = false;
		 switch(_status)
		 {
		 case CloudCheck:			
			 result = check();	
			 yiqiding::utility::Logger::get("system")->log("cloudupdate [music] check " + yiqiding::utility::toString(result));
			 break;
		 case CloudRequest:			
			 result = request_s();	
			 yiqiding::utility::Logger::get("system")->log("cloudupdate [music] request " +  yiqiding::utility::toString(result));	
			 break;
		 case CloudDownload:		
			 result = download(); 	
			 yiqiding::utility::Logger::get("system")->log("cloudupdate [music] download " +  yiqiding::utility::toString(result));
			 break;
		 case CloudUnzip:			
			 result = unzip();			
			 yiqiding::utility::Logger::get("system")->log("cloudupdate [music] unzip " +  yiqiding::utility::toString(result));
			 break;
		 
		 case CloudExecute:			
			 result = execute();	
			  yiqiding::utility::Logger::get("system")->log("cloudupdate [music] execute " +  yiqiding::utility::toString(result));
			 break;
		 case CloudUpload:			
			 result = upload();			
			 yiqiding::utility::Logger::get("system")->log("cloudupdate [music] upload " +  yiqiding::utility::toString(result));
			 break;
		 }
		
		 if(!result)
		 {
			  finish(result);
			  return;
		 }

	 }while(_status != CloudCheck);
		 
	}
};



class MusicCloud:public CloudItem{
private:
	std::string _path;	
	Json::Value _root;
	yiqiding::ktv::Server *_s;
	std::set<unsigned int> _identifys;
	yiqiding::Mutex _mutex;
public:
	virtual bool load(yiqiding::ktv::Server *s,const std::string &path = "music.cloud");
	virtual bool save();
	virtual bool check();
	virtual bool download();
	virtual bool unzip();
	virtual bool request();
	virtual bool execute();
	virtual bool upload();
	virtual bool finish(bool result);
	bool checkidentify(unsigned int identify) { 
		yiqiding::MutexGuard guard(_mutex);
		if(!_identifys.count(identify))
			return false;

		_identifys.erase(identify);
		return true;
	}
};

class UpLoadItem{
	std::string _type;

public:
	UpLoadItem(const std::string &type):_type(type){}
	bool operator()(const std::string &path , const std::string &filename)
	{
		std::string result;
		int times = 0;
		
		do{
			if(yiqiding::net::DistributeContent::getInstance()->DisFile(path ,
				yiqiding::utility::transcode(filename, yiqiding::utility::CodePage::GBK , yiqiding::utility::CodePage::UTF8) ,
				result,
				_type  , 
			"application/octet-stream"))
			{
				DeleteFileA(path.c_str());
				return true;
			}
			++times;
		}while(times<3);

		return false;
		
		
	}
};


}}}