#include "yiqiding/ktv/KTVCloud.h"
#include "yiqiding/io/File.h"
#include "yiqiding/utility/Utility.h"
#include "unzip.h"
# include <direct.h>
# include <io.h>
#include "yiqiding/io/FileDirPt.h"
#include "yiqiding/net/DynamicData.h"
#include "yiqiding/net/FileCurlDistribute.h"
#include "json/json.h"
#include "tinyxml2.h"

/*
	cloud update 2
*/

#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)

//
using namespace yiqiding::ktv::cloud;
using namespace yiqiding::io;
using namespace yiqiding::utility;

bool MusicCloud::load(yiqiding::ktv::Server *s ,const std::string &path){
	_s = s;
	_path = path;
	_root = yiqiding::utility::FileToJson(path);
	if(!_root.isObject())
		return false;
	if(!_root.isMember("status") || !_root["status"].isConvertibleTo(Json::intValue))
		return false;
	_status = (CloudState)_root["status"].asInt();
	yiqiding::utility::Logger::get("system")->log("cloudupdate [music] load " + toString(_status));
	return true;
}

bool MusicCloud::save(){
	File fp(_path);
	try {
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		std::string json = _root.toStyledString();
		fp.write(json.c_str() , json.length());
		fp.close();
		return true;
	} catch (...) { }
	return false;
}

bool MusicCloud::check(){
	/*
	真想屎，去他妈的程序员，脑子让狗吃了。
	*/
	std::string version = _root["version"].asString();
	std::string type = _root["type"].asString();
	std::string url = "http://testyun.17chang.com/check.php";

	std::string post = "version=" + version + "&type=" + type;

	CURL *curl = curl_easy_init();
	yiqiding::net::DynamicData data;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());				// 
	//curl_easy_setopt(curl , CURLOPT_SSL_VERIFYPEER , FALSE);
	//curl_easy_setopt(curl,CURLOPT_CAINFO,"cacert.pem");
	//curl_easy_setopt(curl , CURLOPT_VERBOSE , 1);
	//curl_easy_setopt(curl , CURLOPT_SSL_VERIFYHOST  , FALSE);
	curl_easy_setopt(curl, CURLOPT_POST, 1);  
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);						// 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, yiqiding::net::DistributeThread::write_data);	// 
	curl_easy_setopt(curl, CURLOPT_TIMEOUT , 10);

	CURLcode code = curl_easy_perform(curl);
	if (CURLE_OK != code)
	{
		curl_easy_cleanup(curl);
		yiqiding::utility::Logger::get("system")->log("MusicCloud check curl " + url + " error");
		return false;
	}

//	const  char *v = data.getData();
//	if((unsigned char)v[0] == 0xef && (unsigned char)v[1] == 0xbb && (unsigned char)v[2] == 0xbf)
//		v = data.getData() + 3;
//	
//	std::string vData = std::string(v, data.getData()  + data.getLenth());
	std::string strData(data.getData() , data.getData() + data.getLenth()); 
	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(strData , root) && !root.isObject())
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud check parse error");
		return false;
	}

	bool status = root["status"].asBool();
	if(status == false)
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud check status error");
		return false;
	}
	Json::Value result = root["result"];
	if(!result.isObject())
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud check result is not object");
		return false;
	}
	bool bupdate = result["require_update"].asBool();
	if(bupdate == false)
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud check require_update is false");
		return false;
	}

	Json::Value updates = result["updates"];
	if(!updates.isArray() || updates.size() == 0)
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud check no updates");
		return false;
	}

	_root["updates"] = result["updates"];
	_root["update-index"] = 0;
	_status = CloudRequest;
	_root["status"] = CloudRequest;

	save();

	curl_easy_cleanup(curl);
	return true;
}

bool MusicCloud::download(){
	
	std::string filename = "music.zip";
	std::string username = "song";
	std::string pwd = "yiqiding1314";
	std::string buckname = "yiqichang-song";
	std::string api_url = "http://v0.api.upyun.com";
	
	
	int index = _root["update-index"].asInt();
	if(index >= _root["updates"].size())
	{
		yiqiding::utility::Logger::get("system")->log("MusicCloud download curl index error");
		return false;
	}
	std::string uri = _root["updates"][index]["url"].asString();
	//"2015-05-08/mid20150508112050axsfe4-1780920312.aac";
	//std::string url = array[index]["url"].asString();

	//std::string uri = "/" + buckname + "/" + url; 

	std::string date;
	{
		char mytime[100];
		time_t lt =	::time(NULL);
		struct tm ptm;
		gmtime_s(&ptm , &lt);
		strftime(mytime , 100 , "%a, %d %b %Y %H:%M:%S GMT" , &ptm);
		date = mytime;
	}

	bool flag = true;

	int len = 0;
	yiqiding::crypto::MD5 md5("GET&" + uri + "&" + date + "&" + yiqiding::utility::toString(len) + "&" + yiqiding::crypto::MD5(pwd).hex());
	std::string sign = md5.hex();
	std::string hdate = "Date: " + date;
	std::string hauth = "Authorization: UpYun " + username + ":" + sign;  

	struct curl_slist	 *headerlist = NULL;
	CURL * curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, hdate.c_str());
	headerlist = curl_slist_append(headerlist, hauth.c_str());
	
	FILE *fp = fopen(filename.c_str() , "wb");
	
	uri = api_url + uri;
	curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());		
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);						// 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, yiqiding::net::DistributeThread::write_file);	// 
	curl_easy_setopt(curl , CURLOPT_LOW_SPEED_LIMIT , 64);
	curl_easy_setopt(curl , CURLOPT_LOW_SPEED_TIME , 8);
	curl_easy_setopt(curl , CURLOPT_CONNECTTIMEOUT_MS , 3000);

	CURLcode code =curl_easy_perform(curl);
	if(code != CURLE_OK)
	{
		yiqiding::utility::Logger::get("system")->log("MusicCloud download curl " + uri + " error");
		flag = false;
	}
	if(code == CURLE_OK)
	{
		long retcode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode); 
		if(retcode != 200)
		{
			yiqiding::utility::Logger::get("system")->log("MusicCloud download curl " + uri + " not 200 ok");
			flag = false;
		}
	}

	if(flag)
	{
		_status = CloudUnzip;
		_root["status"] = CloudUnzip;
		save();
	}

	fclose(fp);
	curl_slist_free_all(headerlist);  
	curl_easy_cleanup(curl);
	return flag;
}

bool MusicCloud::execute()
{
	
	std::string sql;
	try{
		File fp("music/json/sql.json");
		fp.open(yiqiding::io::File::READ);
		DWORD low = GetFileSize(fp.native() , &low);
		std::auto_ptr<char> pdata(new char[low + 1 ]);
		fp.read(pdata.get() , low);
		pdata.get()[low] = 0;
		fp.close();
		sql = "[" + std::string(pdata.get()) + "{}]";
	}catch(...){
		yiqiding::utility::Logger::get("system")->log("MusicCloud execute file sql.json error");
		return false;
	}
	Json::Value root = yiqiding::utility::StrToJson(sql);
	if(!root.isArray())
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud execute file sql.json not json");
		return false;
	}
	
	
	for(int i = 0 ; i < root.size() - 1; ++i){
		if( !root[i].isMember("type") || !root[i]["type"].isConvertibleTo(Json::intValue))
			continue;



		int type = root[i]["type"].asInt();
		
		if(type == 0)
		{
			_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());
		}
		else if(type == 1)
		{
			std::string op = root[i]["op"].asString();
			if(op == "insert")
				_s->getDatabase()->getCloudConnector()->add_songlist_detail(root[i]["serial_id_lid"].asInt() , root[i]["index"].asInt(), root[i]["serial_id_mid"].asInt());
			else if(op == "delete")
				_s->getDatabase()->getCloudConnector()->del_songlist_detail(root[i]["serial_id_lid"].asInt() , root[i]["index"].asInt());
			else if(op == "update")
				_s->getDatabase()->getCloudConnector()->modify_songlist_detail(root[i]["serial_id_lid"].asInt() , root[i]["index"].asInt() , root[i]["serial_id_mid"].asInt());
		}
		else if(type == 2)
		{
			std::string op = root[i]["op"].asString();
			if( op == "insert")
				_s->getDatabase()->getCloudConnector()->add_media_list(root[i]["class"].asString() , root[i]["index"].asInt() , root[i]["serial_id"].asInt());
			else if(op == "delete")
				_s->getDatabase()->getCloudConnector()->del_media_list(root[i]["class"].asString() , root[i]["index"].asInt());
			else if(op == "update")
				_s->getDatabase()->getCloudConnector()->modify_media_list(root[i]["class"].asString() , root[i]["index"].asInt() , root[i]["serial_id"].asInt());
		}
		else if(type == 100){
			std::string op = root[i]["op"].asString();
			Json::Value &item = root[i]["media"];
			Json::Value &father = root[i];
			if( op == "insert")
				_s->getDatabase()->getCloudConnector()->add_media(item["serial_id"].asInt() ,
				father["actor_serialid_1"].asInt() , father["actor_serialid_2"].asInt() ,item["name"].asString() , item["language"].asInt() ,
				item["type"].asInt() , item["singer"].asString() , item["pinyin"].asString() , item["header"].asString() ,item["head"].asString(),item["words"].asInt(),
				item["path"].asString() , item["lyric"].asString() , item["original_track"].asInt()  , item["sound_track"].asInt() ,item["start_volume_1"].asInt() , 
				item["start_volumn_2"].asInt(),item["prelude"].asInt() , item["effect"].asInt() , item["version"].asInt() , item["create_time"].asString() , item["stars"].asInt() ,
				item["hot"].asInt() , item["count"].asInt() , item["enabled"].asInt() , item["black"].asInt() , item["match"].asInt() , item["update_time"].asString() , item["resolution"].asInt() ,
				item["quality"].asInt() , item["source"].asInt() , item["rhythm"].asInt() , item["pitch"].asInt() , item["info"].asString());

				
			else if( op == "delete")
				_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());

			else if(op == "update")
				_s->getDatabase()->getCloudConnector()->modify_media(item["serial_id"].asInt() ,
				father["actor_serialid_1"].asInt() , father["actor_serialid_2"].asInt() ,item["name"].asString() , item["language"].asInt() ,
				item["type"].asInt() , item["singer"].asString() , item["pinyin"].asString() , item["header"].asString() ,item["head"].asString(),item["words"].asInt(),
				item["path"].asString() , item["lyric"].asString() , item["original_track"].asInt()  , item["sound_track"].asInt() ,item["start_volume_1"].asInt() , 
				item["start_volumn_2"].asInt(),item["prelude"].asInt() , item["effect"].asInt() , item["version"].asInt() , item["create_time"].asString() , item["stars"].asInt() ,
				item["hot"].asInt() , item["count"].asInt() , item["enabled"].asInt() , item["black"].asInt() , item["match"].asInt() , item["update_time"].asString() , item["resolution"].asInt() ,
				item["quality"].asInt() , item["source"].asInt() , item["rhythm"].asInt() , item["pitch"].asInt() , item["info"].asString());

		}
		else if(type == 101){
			std::string op = root[i]["op"].asString();
			if( op == "insert")
				_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());
			else if( op == "update")
				_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());
			else if( op == "delete")
				_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());
		}
		else if(type == 102){
			std::string op = root[i]["op"].asString();
			if(op == "insert")
				_s->getDatabase()->getCloudConnector()->execute(root[i]["sql"].asString());
			else if(op == "update")
				_s->getDatabase()->getCloudConnector()->modify_songlist(root[i]["serial_id"].asInt() , root[i]["new_title"].asString());
			else if( op == "delete")
				_s->getDatabase()->getCloudConnector()->del_songlist(root[i]["serial_id"].asInt());
		}
		
	}

	
	_status = CloudUpload;
	save();
	DeleteFileA("music/json/sql.json");
	return true;
}

bool MusicCloud::finish(bool result){
	
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;



	std::string version;
	std::string failmsg = "";
	if(result){
		version = _root["version"].asCString();
	}
	else{
		int index = _root["update-index"].asInt();

		if(index >= _root["updates"].size())
		{
			yiqiding::utility::Logger::get("system")->log("MusicCloud finish index error");
			return false;
		}
		
		version = _root["updates"][index]["version"].asCString();
		switch(_status){
		case CloudDownload:
			failmsg = "download file error.";
			break;
		case CloudUnzip:
			failmsg = "Unzip file error.";
			break;
		case CloudExecute:
			failmsg = "Execute sql error.";
			break;
		case CloudUpload:
			failmsg = "Upload file error.";
			break;
		default:
			return false;
		}
	}


	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("name"));
	node->InsertEndChild(doc.NewText(_root["type"].asCString()));
	root->InsertEndChild(node = doc.NewElement("version"));
	node->InsertEndChild(doc.NewText(version.c_str()));
	root->InsertEndChild(node = doc.NewElement("success"));
	node->InsertEndChild(doc.NewText(yiqiding::utility::toCString(result)));
	root->InsertEndChild(node = doc.NewElement("failmsg"));
	node->InsertEndChild(doc.NewText(failmsg.c_str()));

	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);
	packet::Packet out_pack(packet::KTV_REQ_ERP_CLOUD_FINISH);

	// Pack
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	try{
		_s->getConnectionManager()->sendToERP( ERP_ROLE_MAIN , &out_pack);
	}catch(...){
		yiqiding::utility::Logger::get("system")->log("finish request error");
		return false;
	}
	yiqiding::utility::Logger::get("system")->log("finish request ok");
	return true;
}

bool MusicCloud::upload()
{

	if(!FileDirPt<UpLoadItem>("music/mp4/08" , UpLoadItem("mp4")).execute())
		return false;
	if(!FileDirPt<UpLoadItem>("music/image" , UpLoadItem("image")).execute())
		return false;
	if(!FileDirPt<UpLoadItem>("music/avatar" , UpLoadItem("avatar")).execute())
		return false;
	if(!FileDirPt<UpLoadItem>("music/fm" , UpLoadItem("fm")).execute())
		return false;
	if(!FileDirPt<UpLoadItem>("music/lyric" , UpLoadItem("lyric")).execute())
		return false;

	int index = _root["update-index"].asInt();

	if(index >= _root["updates"].size())
	{
		yiqiding::utility::Logger::get("system")->log("MusicCloud upload index error");
		return false;
	}

	_root["version"] = _root["updates"][index]["version"];
	_root["update-index"] = ++index;
	int size = _root["updates"].size();
	if(index < _root["updates"].size())
		_status = CloudRequest;
	else
		_status = CloudCheck;
	_root["status"] = _status;
	save();
	finish(true);
	return true;
}

bool MusicCloud::request()
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root;
	tinyxml2::XMLElement* node;



	int index = _root["update-index"].asInt();

	if(index >= _root["updates"].size())
	{
		yiqiding::utility::Logger::get("system")->log("MusicCloud request index error");
		return false;
	}

	doc.InsertEndChild(root = doc.NewElement("root"));
	root->InsertEndChild(node = doc.NewElement("name"));
	node->InsertEndChild(doc.NewText(_root["type"].asCString()));
	root->InsertEndChild(node = doc.NewElement("version"));
	node->InsertEndChild(doc.NewText(_root["updates"][index]["version"].asCString()));
	root->InsertEndChild(node = doc.NewElement("remark"));
	node->InsertEndChild(doc.NewText(_root["updates"][index]["name"].asCString()));

	tinyxml2::XMLPrinter printer(0, true);
	doc.Print(&printer);
	packet::Packet out_pack(packet::KTV_REQ_ERP_CLOUD_UPDATE);

	// Pack
	out_pack.setPayload(printer.CStr(), printer.CStrSize() - 1);
	try{
		_s->getConnectionManager()->sendToERP( ERP_ROLE_MAIN , &out_pack);
		{
			yiqiding::MutexGuard guard(_mutex);
			_identifys.insert(out_pack.getIdentifier());
		}
	}catch(...){
			yiqiding::utility::Logger::get("system")->log("MusicCloud request error");
			return false;
	}
	yiqiding::utility::Logger::get("system")->log("MusicCloud request ok " + yiqiding::utility::toString(out_pack.getIdentifier()));
	return true;

}

void change_file_date(
	const char *filename,
uLong dosdate,
tm_unz tmu_date)
{

	HANDLE hFile;
	FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

	hFile = CreateFileA(filename,GENERIC_READ | GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,0,NULL);
	GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
	DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
	LocalFileTimeToFileTime(&ftLocal,&ftm);
	SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
	CloseHandle(hFile);

}


int mymkdir(
	const char* dirname)
{
	int ret = _mkdir(dirname);

	return ret;
}


int makedir (
	const char *newdir)
{
	char *buffer ;
	char *p;
	int  len = (int)strlen(newdir);

	if (len <= 0)
		return 0;

	buffer = (char*)malloc(len+1);
	if (buffer==NULL)
	{
		printf("Error allocating memory\n");
		return UNZ_INTERNALERROR;
	}
	strcpy(buffer,newdir);

	if (buffer[len-1] == '/') {
		buffer[len-1] = '\0';
	}
	if (mymkdir(buffer) == 0)
	{
		free(buffer);
		return 1;
	}

	p = buffer+1;
	while (1)
	{
		char hold;

		while(*p && *p != '\\' && *p != '/')
			p++;
		hold = *p;
		*p = 0;
		if ((mymkdir(buffer) == -1) && (errno == ENOENT))
		{
			printf("couldn't create directory %s\n",buffer);
			free(buffer);
			return 0;
		}
		if (hold == 0)
			break;
		*p++ = hold;
	}
	free(buffer);
	return 1;
}

int do_extract_currentfile(
	unzFile uf,
const int* popt_extract_without_path,
int* popt_overwrite,
const char* password)
{
	char filename_inzip[256];
	char* filename_withoutpath;
	char* p;
	int err=UNZ_OK;
	FILE *fout=NULL;
	void* buf;
	uInt size_buf;

	unz_file_info64 file_info;
	uLong ratio=0;
	err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if (err!=UNZ_OK)
	{
		printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
		return err;
	}

	size_buf = 8192;//WRITEBUFFERSIZE;
	buf = (void*)malloc(size_buf);
	if (buf==NULL)
	{
		printf("Error allocating memory\n");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

	if ((*filename_withoutpath)=='\0')
	{
		if ((*popt_extract_without_path)==0)
		{
			printf("creating directory: %s\n",filename_inzip);
			mymkdir(filename_inzip);
		}
	}
	else
	{
		const char* write_filename;
		int skip=0;

		if ((*popt_extract_without_path)==0)
			write_filename = filename_inzip;
		else
			write_filename = filename_withoutpath;

		err = unzOpenCurrentFilePassword(uf,password);
		if (err!=UNZ_OK)
		{
			printf("error %d with zipfile in unzOpenCurrentFilePassword\n",err);
		}

		if (((*popt_overwrite)==0) && (err==UNZ_OK))
		{
			char rep=0;
			FILE* ftestexist;
			ftestexist = FOPEN_FUNC(write_filename,"rb");
			if (ftestexist!=NULL)
			{
				fclose(ftestexist);
				do
				{
					char answer[128];
					int ret;

					printf("The file %s exists. Overwrite ? [y]es, [n]o, [A]ll: ",write_filename);
					ret = scanf("%1s",answer);
					if (ret != 1)
					{
						exit(EXIT_FAILURE);
					}
					rep = answer[0] ;
					if ((rep>='a') && (rep<='z'))
						rep -= 0x20;
				}
				while ((rep!='Y') && (rep!='N') && (rep!='A'));
			}

			if (rep == 'N')
				skip = 1;

			if (rep == 'A')
				*popt_overwrite=1;
		}

		if ((skip==0) && (err==UNZ_OK))
		{
			fout=FOPEN_FUNC(write_filename,"wb");
			/* some zipfile don't contain directory alone before file */
			if ((fout==NULL) && ((*popt_extract_without_path)==0) &&
				(filename_withoutpath!=(char*)filename_inzip))
			{
				char c=*(filename_withoutpath-1);
				*(filename_withoutpath-1)='\0';
				makedir(write_filename);
				*(filename_withoutpath-1)=c;
				fout=FOPEN_FUNC(write_filename,"wb");
			}

			if (fout==NULL)
			{
				printf("error opening %s\n",write_filename);
			}
		}

		if (fout!=NULL)
		{
	//		printf(" extracting: %s\n",write_filename);

			do
			{
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)
				{
					printf("error %d with zipfile in unzReadCurrentFile\n",err);
					break;
				}
				if (err>0)
					if (fwrite(buf,err,1,fout)!=1)
					{
						printf("error in writing extracted file\n");
						err=UNZ_ERRNO;
						break;
					}
			}
			while (err>0);
			if (fout)
				fclose(fout);

			if (err==0)
				change_file_date(write_filename,file_info.dosDate,
				file_info.tmu_date);
		}

		if (err==UNZ_OK)
		{
			err = unzCloseCurrentFile (uf);
			if (err!=UNZ_OK)
			{
				printf("error %d with zipfile in unzCloseCurrentFile\n",err);
			}
		}
		else
			unzCloseCurrentFile(uf); /* don't lose the error */
	}

	free(buf);
	return err;
}




bool do_extract(
	unzFile uf,
int opt_extract_without_path,
int opt_overwrite,
const char* password)
{
	uLong i;
	unz_global_info64 gi;
	int err;
	FILE* fout=NULL;

	err = unzGetGlobalInfo64(uf,&gi);
	if (err!=UNZ_OK)
		return false;//printf("error %d with zipfile in unzGetGlobalInfo \n",err);

	for (i=0;i<gi.number_entry;i++)
	{
		if (do_extract_currentfile(uf,&opt_extract_without_path,
			&opt_overwrite,
			password) != UNZ_OK)
			return false;//break;

		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				//printf("error %d with zipfile in unzGoToNextFile\n",err);
				return false;//break;
			}
		}
	}

	return true;
}



bool MusicCloud::unzip()
{
	unzFile uf = unzOpen("music.zip");
	if(uf == NULL)
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud unzip music.zip exsit");
		return false;
	}
	int index = _root["update-index"].asInt();

	if(index >= _root["updates"].size())
	{
		yiqiding::utility::Logger::get("system")->log("MusicCloud unzip index error");
		return false;
	}

	std::string pwd = _root["updates"][index]["remark"].asString();
	if(!do_extract(uf, 0, 1, pwd.c_str()))
	{	
		yiqiding::utility::Logger::get("system")->log("MusicCloud unzip music.zip error");
		return false;
	}
	unzClose(uf);

	DeleteFileA("music.zip");
	_status = CloudExecute;
	_root["status"] = CloudExecute;
	save();
	return true;
}