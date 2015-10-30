/*
 save Message Rule .Base64
*/

#include "yiqiding/ktv/MessageRule.h"
#include "yiqiding/io/File.h"

using namespace yiqiding::ktv;
using namespace yiqiding::io;


MessageRule * MessageRule::_instance;
yiqiding::Mutex MessageRule::_mutex;

MessageRule * MessageRule::getInstance()
{
	if (_instance == NULL)
	{
		MutexGuard guard(_mutex);
		if (_instance == NULL)
		{
			_instance = new MessageRule();
		}

	}

	return _instance;
}

void MessageRule::unLoad()
{
	MutexGuard guard(_mutex);
	if (_instance != NULL)
	{
		delete _instance;
		_instance = NULL;
	}
}

bool MessageRule::load(const std::string &path)
{
	File fp(path);
	try{
		MutexGuard guard(_mutex);
		fp.open(File::READ);
		DWORD low = GetFileSize(fp.native() , &low);
		std::auto_ptr<char> pdata(new char[low + 1]);
		fp.read(pdata.get() , low);
		pdata.get()[low] = 0;
		_content = pdata.get();
		fp.close();
		return true;
	}catch(...){ }
	return false;
}

bool MessageRule::save(const std::string &path ,const std::string &content)
{
	File fp(path);
	try {
		MutexGuard guard(_mutex);
		_content = content;
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		fp.write(content.c_str() , content.length());
		fp.close();
		return true;
	} catch (...) { }
	return false;


}
