/*
 * BoxData record user data
 * @author YuChun Zhang
 * @date 2015.01.22
 */

#include <string>
#include <vector>
#include "yiqiding/Thread.h"
#include <WinSock2.h>
#include <Windows.h>
#include "zip.h"
 
namespace yiqiding { namespace ktv{


	const std::string data_dir = "./zip";
	
	struct ServerInfo{
	 std::string ktv_sid;
	 std::string ktv_name;
	 std::string server_serial;
	 std::string server_ver;
	 std::string database_ver;
	 std::string erp_ver;
	};

	class KTVBoxData
	{
	private:
		std::string						_dir;
		int								_packNum;
		std::vector<std::string>		_filelst;
		ServerInfo						_info;
		yiqiding::Mutex					_mutex;
		char							_buf[4096];
	private:
		bool PackFile(std::string &path);
		int filetime(const char *f, tm_zip *tmzip,uLong * dt);
		bool wirteInZip(const char *path , zipFile file);

	public:
		const std::string & getDir() {return _dir;}
		void load(const std::string &dir , const ServerInfo  &info,  int defaultNum = 20);
		std::string add(const std::string &fileName);
		
	};



}}
