/*
 save Message Rule .Base64
*/

#include <string>
#include "yiqiding/Thread.h"

namespace yiqiding { namespace ktv{
	
	const std::string MSG_PATH = "msg.rule";

	class MessageRule
	{
		static Mutex			_mutex;
		static MessageRule	*	_instance;
		std::string _content;

		MessageRule(){};

	public:
		std::string getContent(){ MutexGuard guard(_mutex); return _content; }
		bool load(const std::string &path);
		bool save(const std::string &path , const std::string &content);

		static MessageRule *getInstance();
		static void unLoad();
	};

}}


