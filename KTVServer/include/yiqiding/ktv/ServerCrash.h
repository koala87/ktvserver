/**
 * Server Crash Process
 * @author Yuchun Zhang
 * @date 2014.08.09
 */

#include "yiqiding/crashdump.h"

namespace yiqiding {namespace ktv{

	class ServerCrash:public yiqiding::CrashProcess
	{
	public:
		virtual void process(const std::string &path);
	};
}}