#include "yiqiding/ktv/BoxData.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/utility/Transcode.h"

using namespace yiqiding::ktv;


bool KTVBoxData::wirteInZip(const char *path , zipFile file)
{	
	
	FILE *fp = fopen(path , "rb");
	if(fp == NULL)
		return false;
	int ret;
	int err;
	do{
		ret = fread(_buf , 1 , sizeof(_buf) , fp);
		err = zipWriteInFileInZip(file , _buf , ret);
		if(err != ZIP_OK)
		{	
			fclose(fp);
			return false;
		}
	}while(!feof(fp));

	fclose(fp);
	return true;
}

int KTVBoxData::filetime(const char *f, tm_zip *tmzip,uLong * dt)
{
	int ret = 0;
	{
		FILETIME ftLocal;
		HANDLE hFind;
		WIN32_FIND_DATAA ff32;

		hFind = FindFirstFileA(f,&ff32);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
			FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
			FindClose(hFind);
			ret = 1;
		}
	}
	return ret;
}

bool KTVBoxData::PackFile(std::string &filePath){

	std::string ownFile = utility::transcode(_info.ktv_name , yiqiding::utility::CodePage::UTF8 , yiqiding::utility::CodePage::GBK) + "_" +  yiqiding::utility::toString(::time(NULL)) + "_info";
	std::string path = "zip//" + ownFile + ".zip";
	ownFile += ".txt";
	filePath = "";


	zipFile file = zipOpen64(path.c_str() , APPEND_STATUS_CREATE);
	if(file == NULL)
	{
		yiqiding::utility::Logger::get("system")->log("zipOpen64 " +  path);
		return false;
	}

	int err;
	zip_fileinfo zi;


	zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
		zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;

	
	for each(auto k in _filelst){

		std::string path = _dir + "/" + k;

		if(!filetime(path.c_str(),&zi.tmz_date,&zi.dosDate))	//maybe,this file not exist.
		{
			yiqiding::utility::Logger::get("system")->log("filetime " + path , yiqiding::utility::Logger::WARNING);
			continue;
		}
		err = zipOpenNewFileInZip(file , k.c_str(),&zi ,NULL,0,NULL,0,NULL, Z_DEFLATED,Z_BEST_COMPRESSION);
		if(err != ZIP_OK)
		{
			zipClose(file , NULL);
			yiqiding::utility::Logger::get("system")->log("zipOpenNewFileInZip " +  k , yiqiding::utility::Logger::WARNING);
			return false;
		}
		if(!wirteInZip(path.c_str() , file))
		{
			zipClose(file , NULL);
			yiqiding::utility::Logger::get("system")->log("wirteInZip " + k);
			return false;
		}
	}

	//for last

	err = zipOpenNewFileInZip(file , "info.txt" ,NULL ,NULL,0,NULL,0,NULL, Z_DEFLATED,Z_BEST_COMPRESSION);

	if(err != ZIP_OK)
	{
		zipClose(file , NULL);
		yiqiding::utility::Logger::get("system")->log("zipOpenNewFileInZip ownFile " + ownFile , yiqiding::utility::Logger::WARNING);
		return false;
	}
	
	std::ostringstream out;
	out << "#ktv_sid:"<<_info.ktv_sid << "\r\n"
		<< "#ktv_name:" << _info.ktv_name << "\r\n"
		<< "#server_serial:"<<_info.server_serial <<"\r\n"
		<< "#server_ver:"<<_info.server_ver << "\r\n"
		<< "#database_ver:"<<_info.database_ver << "\r\n"
		<< "#erp_ver:"<<_info.erp_ver << "\r\n";

	std::string content = out.str();

	err = zipWriteInFileInZip(file , content.c_str() , content.length());
	if(err != ZIP_OK)
	{
		zipClose(file , NULL);
		yiqiding::utility::Logger::get("system")->log("zipWriteInFileInZip  " + content , yiqiding::utility::Logger::WARNING);
		return false;
	}

	err = zipClose(file , NULL);
	if(err != ZIP_OK)
	{
		yiqiding::utility::Logger::get("system")->log("zipClose  " , yiqiding::utility::Logger::WARNING);
		return false;
	}

	filePath = path;
	for each(auto k in _filelst){
		std::string path = _dir + "/" + k;
		DeleteFileA(path.c_str());
	}
	_filelst.clear();
	return true;
}

void KTVBoxData::load(const std::string &dir ,const ServerInfo  &info, int defaultNum )
{
	yiqiding::MutexGuard gurad(_mutex);
	CreateDirectoryA(dir.c_str() , NULL);
	_dir = dir;
	_info = info;
	_packNum = defaultNum;
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
			_filelst.push_back(FindFileData.cFileName);
		}
	} while(finder);

	FindClose(hFind);
}
		
std::string KTVBoxData::add(const std::string &fileName)
{
	std::string path;
	yiqiding::MutexGuard gurad(_mutex);
	_filelst.push_back(fileName);
	if(_filelst.size() >= _packNum)
	{
		if(!PackFile(path))
		{
			yiqiding::utility::Logger::get("system")->log("PackFile " + fileName , yiqiding::utility::Logger::WARNING);
		}
		yiqiding::utility::Logger::get("system")->log("PackFile num:" + yiqiding::utility::toString(_packNum));
	}
	return path;
}



