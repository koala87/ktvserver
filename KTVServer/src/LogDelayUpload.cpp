#include "yiqiding/ktv/LogDelayUpLoad.h"
#include "yiqiding/net/FileCurlDistribute.h"
#include "yiqiding/ktv/BoxInfoMan.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/utility/Transcode.h"

using namespace yiqiding::ktv;


LogDelayUpload::LogDelayUpload():_loop(true)
{
	_thread = Thread(this);

	CreateDirectoryA("cache" , NULL);
	CreateDirectoryA("zip" , NULL);
	CreateDirectoryA("mid" , NULL);
	_thread.start();
	
}

LogDelayUpload::~LogDelayUpload()
{
	_loop = false;
	_cond.signal();
	_thread.join();
}

void LogDelayUpload::load(const std::string &dir)
{
	WIN32_FIND_DATAA FindFileData;
	std::string path = dir + "/" + "*.*";
	int finder;
	HANDLE hFind = FindFirstFileA(path.c_str(), &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		yiqiding::utility::Logger::get("system")->log("FindFirstFileA path" , yiqiding::utility::Logger::WARNING);
		return;
	}

	do 
	{
		finder = FindNextFileA(hFind , &FindFileData);
		if(finder && (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
		{
			push(dir + "/" + FindFileData.cFileName);
		}
	} while(finder);

	FindClose(hFind);
}

void LogDelayUpload::push(const std::string &filepath)
{
	MutexGuard guard(_mutex);
	_queuepaths.push(filepath);
	if(_queuepaths.size() == 1)
	{
		_cond.signal();
	}
}

void LogDelayUpload::run()
{
	size_t errcount = 0;
	while(_loop)
	{
		std::string filename;
		std::string path;
		{
			MutexGuard guard(_mutex);
			if(_queuepaths.empty())
			{
				if(_loop)
					_cond.wait(_mutex);
				else
					break;
			}
			 path =  _queuepaths.front();
			_queuepaths.pop();	 
		}
			
		int pos = path.find_last_of('/');
		if(pos == std::string::npos)
		{
			filename = path;
		}
		else
		{
			filename = path.substr(pos + 1);
		}

		//bool type = (path[0] == 'c');//direction:cache or zip or mid

		if(path[0] == 'c' && yiqiding::net::DistributeContent::UpLoadFile(yiqiding::net::yiqichang_server ,filename,
			"text/plain",path , yiqiding::ktv::box::BoxInfoMan::getInstace()->getShopName()))
		{
			errcount = 0;
			DeleteFileA(path.c_str());
		}
		else if(path[0] == 'z' && /*yiqiding::net::DistributeContent::UpLoadFile2(yiqiding::net::yiqichang_server2 ,yiqiding::utility::transcode(filename,yiqiding::utility::CodePage::GBK ,yiqiding::utility::CodePage::UTF8),
								  "application/zip",path)*/ yiqiding::net::DistributeContent::UpLoadFile4(_server->getDataServer() ,
								  yiqiding::utility::transcode(filename,yiqiding::utility::CodePage::GBK ,yiqiding::utility::CodePage::UTF8) , "application/zip" ,	path ,box::BoxInfoMan::getInstace()->getShopName()))
		{
			errcount = 0;
			DeleteFileA(path.c_str());
		}
		else if (path[0] == 'm' && yiqiding::net::DistributeContent::UpLoadFile3(filename , path))
		{
			errcount = 0;
			DeleteFileA(path.c_str());
		}
		else
		{
			++errcount;
			push(path);
			yiqiding::utility::Logger::get("system")->log(path + " upload error");
		}

		//·ÀÖ¹´íÎó·¢Éú,cpuÕ¼Âú
		if(errcount)
		{
			int sleeptime = errcount > max_diff?max_diff : errcount;
			Sleep(sleeptime * 10);
		}
	}
}
