/**
 * KTV Storage Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.19
 */

#include "yiqiding/ktv/KTVStorage.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/io/FileWriteMap.h"
#include "yiqiding/crypto/MD5.h"
#include "yiqiding/io/FileDistribute.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/ktv/BoxInfoMan.h"
#include <iostream>
#include <string>
#include <time.h> 
#include <tinyxml2.h>
#include "json/json.h"
#include "yiqiding/Exception.h"
#include "yiqiding/ktv/LogDelayUpLoad.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/BoxData.h"
#include "yiqiding/utility/Transcode.h"
#include "yiqiding/ktv/KTVMPG.h"
#include "yiqiding/ktv/LogDelayUpLoad.h"
#include <fstream>
#include <windows.h>

using yiqiding::crypto::MD5;
using namespace yiqiding::ktv;
using yiqiding::utility::Logger;
using yiqiding::ktv::KTVStorageClientHandler;
using yiqiding::io::FileWriteMap;

//only temp
yiqiding::Mutex KTVStorageClientHandler::_sMutex;


bool KTVStorageClientHandler::processUpLoad(StoragePacket &pac ,const std::string &path , const std::string &filename)
{
	std::string result;
	std::string mime = "application/octet-stream";

	std::string name = filename;

	std::string convertPath = path;

	if(pac.getContent() == TYPE_MEDIA || pac.getContent() == TYPE_IMAGE)
	{
		name =  generateName(name);
	}
	else if(pac.getContent() == TYPE_MEIDA_MPG)
	{
		std::vector<std::string> vec = yiqiding::utility::split(name ,'.');
		if(vec.size() == 2)
			name = vec[0];
		name = generateName(name) + ".mp4";
	}
	else if (pac.getContent() == TYPE_ROM_LOG)
	{
		name = "rom" + generateName(name);
	}
	else if (pac.getContent() == TYPE_APK_LOG)
	{
		name = "box" + generateName(name);
	}
	else if (pac.getContent() == TYPE_MID)
	{
		name = "mid" + generateName(name);
	}
	else if (pac.getContent() == TYPE_BOX_LOG){
		name = "box_log" + generateName(name);
	}

	//convert mpg to mp4
	if(pac.getContent() == TYPE_MEIDA_MPG)
	{
		MutexGuard guard(_sMutex);
		convertPath = path + ".mp4";
		yiqiding::utility::Logger::get("storage")->log("begin to converting ..." + path);
		int ret = KTVMeidaConvert::ConvertMpgToMp4ByFile(path ,  convertPath);
		if(ret != 0)
		{
			yiqiding::utility::Logger::get("storage")->log("Convert Mpg To Mp4 Error:" + yiqiding::utility::toString(ret));
			return sendBack(pac , false , "Convert Mpg To Mp4 Error:" + yiqiding::utility::toString(ret));
		}
		else
		{
			yiqiding::utility::Logger::get("storage")->log("end to converting ..." + path);
		}
	}

	//apk or rom log
	if(pac.getContent() == TYPE_APK_LOG || pac.getContent() == TYPE_ROM_LOG)
	{
		sendBackToBox(pac , true);

		bool ret = yiqiding::net::DistributeContent::UpLoadFile4( Singleton<yiqiding::ktv::LogDelayUpload>::getInstance()->getServer()->getDataServer() ,
			name , mime , path ,box::BoxInfoMan::getInstace()->getShopName());
		if(!ret)
		{	
			yiqiding::utility::Logger::get("storage")->log("UpLoadFile Error" + path + name , yiqiding::utility::Logger::WARNING);
			std::string newpath = "cache/" + name;
			if(MoveFileA(path.c_str() , newpath.c_str()))
			{
				Singleton<LogDelayUpload>::getInstance()->push(newpath);
			}
			else
			{
				yiqiding::utility::Logger::get("storage")->log("MoveFileA file error" + path + name , yiqiding::utility::Logger::WARNING);
			}
			
		}
		else
		{	
			yiqiding::utility::Logger::get("storage")->log("UpLoadFile " + path + " " + name );
		}

		return true;
	}
	//box data
	else if(pac.getContent() == TYPE_BOX_DATA)
	{
		sendBackToBox(pac , true);
		
		
		std::string newpath = Singleton<KTVBoxData>::getInstance()->getDir() + "/" + name;
		if(MoveFileA(path.c_str() , newpath.c_str()))
		{
			std::string path = Singleton<KTVBoxData>::getInstance()->add(name);//gbk path
			std::string filename;
			int pos = path.find_last_of('/');
			if (pos != std::string::npos)
			{
				filename = path.substr(pos + 1);
			}
			else{
				filename = path;
			}


			if(path != "")
			{
				bool ret = yiqiding::net::DistributeContent::UpLoadFile2(yiqiding::net::yiqichang_server2 ,
					yiqiding::utility::transcode(filename , yiqiding::utility::CodePage::GBK , yiqiding::utility::CodePage::UTF8),"application/zip" , path );
				if(!ret)
				{
					Singleton<LogDelayUpload>::getInstance()->push(path);
					return false;
				}
				else{
					DeleteFileA(path.c_str());
				}
			}
		}
		else{
			yiqiding::utility::Logger::get("storage")->log("MoveFileA file error " + path + " " + newpath , yiqiding::utility::Logger::WARNING);
			return false;
		}
		
		return true;
	
	}
	else if(TYPE_MID == pac.getContent())
	{
		sendBackToBox(pac , true);
		std::string newpath = "mid/" + name;
		if(MoveFileA(path.c_str() , newpath.c_str()))
		{
			Singleton<LogDelayUpload>::getInstance()->push(newpath);
			return true;
		}
		else
		{
			yiqiding::utility::Logger::get("storage")->log("MovdFileA " + path + " to " + newpath , Logger::WARNING);
			return false;
		}

	}
	else if(TYPE_NONE < pac.getContent() && pac.getContent() < TYPE_END )
	{
		
	

		bool ret = yiqiding::net::DistributeContent::getInstance()->DisFile(convertPath , name ,result, PackettypeStr[pac.getContent()]  , mime);
		
		if(!ret)
			yiqiding::utility::Logger::get("storage")->log("DisFile Error" + convertPath + name + result);
		else
			yiqiding::utility::Logger::get("storage")->log("DisFile" + convertPath + name + result);

		if(pac.getContent() == TYPE_MEIDA_MPG)
		{
			 DeleteFileA(convertPath.c_str());
		}
		if(result == "")
			return sendBack(pac, ret , "Resource Server Error");
		else
			return sendBack(pac, ret , result);
	}

	return sendBack(pac , false , "type error");
}

std::string KTVStorageClientHandler::generateName(const std::string &md5 , const std::string& suffix)
{
	time_t now = ::time(NULL);
	struct tm timeinfo;
	char time_str[15];
	localtime_s(&timeinfo, &now);
	strftime(time_str,15,"%Y%m%d%H%M%S", &timeinfo);

	return  time_str + yiqiding::utility::generateTempCode() + md5.substr(0 ,4) + "." + suffix;	
}

std::string KTVStorageClientHandler::generateName(const std::string &originName )
{
	time_t now = ::time(NULL);
	struct tm timeinfo;
	char time_str[15];
	localtime_s(&timeinfo, &now);
	strftime(time_str,15,"%Y%m%d%H%M%S", &timeinfo);

	return  time_str + yiqiding::utility::generateTempCode() + originName;	
}


bool KTVStorageClientHandler::sendBackToBox(StoragePacket &pac , bool suc)
{
	std::string msg = suc ?"{\"success\":true}":"{\"success\":false}";
	pac.setLength(msg.length());
	pac.HtoN();

	try {
		_client.write((char *)&pac , packetSize);
		_client.write(msg.c_str(), msg.length());
		return true;
	} catch (...) {}

	return false;

}

bool KTVStorageClientHandler::sendBack( StoragePacket &pac ,bool suc,const std::string &path)
{
	tinyxml2::XMLDocument doc;

	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;
	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("success"));
	node->InsertEndChild(doc.NewText(yiqiding::utility::toString(suc).c_str()));
	root->InsertEndChild(node = doc.NewElement("path"));
	node->InsertEndChild(doc.NewText(path.c_str()));

	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);

	int len = printer.CStrSize() - 1;

	pac.setLength(len);
	pac.HtoN();
	
	try{
		_client.write((char *)&pac , packetSize);
		_client.write(printer.CStr(), printer.CStrSize() - 1);
	}catch(...)
	{
		return false;
	}
	return true;

}

void KTVStorageClientHandler::run() {

	int						nret;
	uint32_t				consumed = 0;
	bool					bcreate = false;
	FileWriteMap					fp;
	StoragePacket			pac;
	std::string				path = yiqiding::utility::toString(_client.native());
	yiqiding::crypto::MD5	md5;
	char					*buff;	
	int nSendBuf = 1024 * 1024 * 4;
	
	linger slinger;
	slinger.l_onoff = 1;
	slinger.l_linger = 3;
	u_long bio = 1;
	setsockopt(_client.native() , SOL_SOCKET , SO_LINGER , (char *)&slinger , sizeof(slinger));
	ioctlsocket(_client.native() , FIONBIO , &bio);
	setsockopt(_client.native(),SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	setsockopt(_client.native(),SOL_SOCKET,SO_RCVBUF,(const char*)&nSendBuf,sizeof(int));
	timeval timeout;
	std::string				filename;
	char					optionBuf[256];
	int						optionLen = 0;
	bool					optionCheck = false;
	bool					optionFlag	= false;
	FD_SET fd;

	//Logger::get("storage")->log("upload start:" + path + " " + _client.getAddress() + " port:" + yiqiding::utility::toString(_client.getPort()));
	Logger::get("server")->log("[FTP upload] upload start:" + path + " " + _client.getAddress() + " port:" + yiqiding::utility::toString(_client.getPort()));
	try{
			while(true)
			{
				FD_ZERO(&fd);
				FD_SET(_client.native() , &fd);
				timeout.tv_sec = 30;
				timeout.tv_usec = 0;
				int count = select(0 , &fd , NULL , NULL , &timeout);
				// timeout
				if (count <= 0)
				{
					//Logger::get("storage")->log(path + " select timeout" , Logger::WARNING);
					Logger::get("server")->log(path + " select timeout" , Logger::WARNING);
					break;
				}
				if(!FD_ISSET(_client.native() , &fd))
				{
					//Logger::get("storage")->log(path + " isset" , Logger::WARNING);
					Logger::get("server")->log(path + " select timeout" , Logger::WARNING);
					break;
				}


				if(consumed < sizeof(StoragePacket))
				{
					nret = ::recv(_client.native() , ((char *)&pac) + consumed , packetSize - consumed , 0);
					if (nret == 0 || (nret == SOCKET_ERROR && GetLastError() != WSAEWOULDBLOCK ))
					{	
						//Logger::get("storage")->log("recv header return " + utility::toString(nret) + (nret == -1 ?(" socket : " + yiqiding::utility::toString(GetLastError())):"") , Logger::WARNING);
						Logger::get("server")->log("recv header return " + utility::toString(nret) + (nret == -1 ?(" socket : " + yiqiding::utility::toString(GetLastError())):"") , Logger::WARNING);
						break;
					}
					else if (nret == SOCKET_ERROR)
					{
						continue;
					}
				
					consumed += nret;

				}
				//have already recved header 

				filename = (const char *)pac.getFormat();
				int optionNameLen = 0;
				if(!optionCheck)
				{
					optionCheck = true;
					char empty[32] = {0};
					if(memcmp(pac.getFormat() , empty , sizeof(empty)) == 0)
					{
						optionFlag = true;
					}
				}

				if (optionFlag && optionLen < sizeof(optionBuf)) // 需要收
				{


					nret = ::recv(_client.native() , optionBuf  + optionLen, sizeof(optionBuf) - optionLen , 0);
					if (nret == 0 || (nret == SOCKET_ERROR && GetLastError() != WSAEWOULDBLOCK ))
					{	
						Logger::get("storage")->log("optional error");
					}
					else if(nret == SOCKET_ERROR)
					{
						continue;
					}
				
					optionLen += nret;
				}

				if(optionFlag && optionLen < sizeof(optionBuf))
				{
					continue;
				}
				else if(optionFlag)
				{
					filename = optionBuf;
				}
			


				if (consumed >= packetSize)
				{
					if(!bcreate)
					{
						pac.NtoH();

						//合理的length
						if(pac.getLength() > MaxStoragePacketSize)
						{
							//Logger::get("storage")->log("upload content:" + path + " type:" + utility::toString(pac.getContent()) + "size :" + utility::toString(pac.getLength()));
							Logger::get("server")->log("upload content:" + path + 
								" type:" + utility::toString(pac.getContent()) + "size :" + utility::toString(pac.getLength()));


							break;
						}

						fp.open(path ,pac.getLength());
						bcreate = true;
						buff = fp.getPData();

						//Logger::get("storage")->log("upload content:" + path + " type:" + utility::toString(pac.getContent()) + 
						//	(char *)pac.getFormat() + " md5:" + std::string((char *)pac.getMD5() , 16) + " length:" + utility::toString(pac.getLength()));
						Logger::get("server")->log("[FTP upload start] _content(" + utility::toString(pac.getContent()) + ")" +
							" _format("+ utility::toString(pac.getFormat()) + ")" +
							" _md5(" + utility::toString(pac.getMD5()) + ")" +
							" _length(" + utility::toString(pac.getLength()) + ")" + 
							" path(" + path + ")" +
							" filename(" + filename + ")" );
					}
					nret = ::recv(_client.native() , buff + consumed - packetSize , pac.getLength() + packetSize - consumed , 0);
					if (nret == 0 || (nret == SOCKET_ERROR && GetLastError() != WSAEWOULDBLOCK ))
					{
						//Logger::get("storage")->log("recv buff return " + utility::toString(nret) + (nret == -1 ?(" socket : " + yiqiding::utility::toString(GetLastError())):"") , Logger::WARNING);
						Logger::get("server")->log("recv buff return " + utility::toString(nret) + (nret == -1 ?(" socket : " + yiqiding::utility::toString(GetLastError())):"") , Logger::WARNING);
						fp.close();
						break;
					}
					else if(nret == SOCKET_ERROR)
					{
						continue;
					}


					md5.update(buff + consumed - packetSize, nret);
					consumed += nret;
				}

				// have finished recing file.
				if (consumed == pac.getLength() + packetSize)
				{
					md5.finalise();
					fp.close();

					//md5 check ok
					if (memcmp((void *)md5.hex().c_str() , (void *)pac.getMD5() , 16) == 0)
					{
				//		char suffix[5];
				//		memcpy(suffix , pac.getFormat() , 4);
				//		suffix[4] = 0;

				//		filename = generateName(md5.hex() , suffix);

						
						Logger::get("server")->log("[FTP upload finished] _content(" + utility::toString(pac.getContent()) + ")" +
							" _format("+ utility::toString(pac.getFormat()) + ")" +
							" _md5(" + utility::toString(pac.getMD5()) + ")" +
							" _length(" + utility::toString(pac.getLength()) + ")" + 
							" path(" + path + ")" +
							" filename(" + filename + ")" );
						
						if(TYPE_BOX_LOG == pac.getContent()){
							// copy file1 and rename
							std::string fname = "boxlog_" + generateName(md5.hex(), "zip");
							
							FILE *in_file, *out_file;
							char data[1024];
							size_t bytes_in, bytes_out;
							long len = 0;

							in_file = fopen(path.c_str(), "rb");
							out_file = fopen(fname.c_str(), "wb");

							int error = 0;
							if(!in_file){
								error = 1;
								Logger::get("server")->log("[FTP upload file] " + path + " can't open!");
								sendBackToBox(pac , false);
							}
							if(!out_file){
								error = 1;
								Logger::get("server")->log("[FTP upload file] " + fname + " can't open!");
								sendBackToBox(pac , false);
							}
							if(!error){
								int len = 0;
								while((bytes_in = fread(data, 1, 1024, in_file))>0){
									bytes_out = fwrite(data, 1, bytes_in, out_file);
									if(bytes_in != bytes_out){
										Logger::get("server")->log("[FTP upload] in size:" + utility::toString(bytes_in) +
											" out size:" + utility::toString(bytes_out));
									}
									len += bytes_out;
								}
								fclose(in_file);
								fclose(out_file);
								Logger::get("server")->log("[FTP upload copy file finished] ");
								sendBackToBox(pac , true);
							}
						}

						if(!processUpLoad(pac , path , filename))
						{
							Logger::get("storage")->log("Distribution failed" , Logger::WARNING);
						}
					}	
					else
					{
						Logger::get("storage")->log("md5 check failed" , Logger::WARNING);
					}
					break;
				}	
			}
	}
	catch(const std::exception &e)
	{
		Logger::get("storage")->log(e.what());
	}
	fp.close();
	DeleteFileA(path.c_str());	
	Logger::get("storage")->log("upload end:" + path + "->" + filename);
}

	

	


void KTVStorage::run() {
	try {
		while (_is_running) {
			_pool->add(new SelfDestruct(new KTVStorageClientHandler(net::tcp::Server::accept())));
		}
	} catch (const std::exception& err) {
		try {
			Logger::get("storage")->log(err.what(), Logger::FATAL);
		} catch(...) {}
	}
}

void KTVStorage::start() {
	if (_is_running)
		return;

	_is_running = true;
	_pool->add(this);
}

void KTVStorage::stop() {
	_is_running = false;
	Socket::close();
}
