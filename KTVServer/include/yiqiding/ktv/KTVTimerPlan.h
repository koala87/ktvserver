
#include "yiqiding/utility/Logger.h"
#include "yiqiding/net/SocketPool.h"
#include "yiqiding/ktv/LogDelayUpLoad.h"
#include "yiqiding/io/File.h"
namespace yiqiding { namespace ktv{

class UpLoadCoreLog
{
	class DoIt
	{
	public:
		void operator()() const {		

			if(yiqiding::io::File::getLength("core.log") == 0)
				return;
			std::string newpath = "cache/boxlog" + yiqiding::utility::toString(::time(NULL)) + yiqiding::utility::generateTempCode(6) +  ".log";	
			if(MoveFileA("core.log" , newpath.c_str()))
			{
				Singleton<yiqiding::ktv::LogDelayUpload>::getInstance()->push(newpath);
			}
			else
			{
				yiqiding::utility::Logger::get("system")->log("MoveFile Error UploadCoreLog" , yiqiding::utility::Logger::WARNING);
			}
		}
	};
public:
	void operator ()(){
		yiqiding::utility::Logger::get("core")->doit(DoIt());
	}
};


}};