#include "yiqiding/ktv/AdUrl.h"
#include "yiqiding/io/File.h"


using namespace yiqiding::ktv;
using namespace yiqiding::io;
void AdUrl::load(const std::string &path)
{
	File fp(path);
	try {
		
		MutexGuard guard(_mutex);
		fp.open(File::READ);
		DWORD low = GetFileSize(fp.native() , &low);
		std::auto_ptr<char> pdata(new char[low + 1]);
		fp.read(pdata.get() , low);
		pdata.get()[low] = 0;
		fp.close();
		_urls = pdata.get();
	} catch (...) {}
}

std::string AdUrl::getUrls() const
{
	MutexGuard guard(_mutex);
	return _urls;
}

bool AdUrl::save(const std::string &path , const std::string &urls)
{
	File fp(path);
	try {
		MutexGuard guard(_mutex);
		_urls = urls;
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		fp.write(urls.c_str() , urls.length());
		fp.close();
		return true;
	} catch (...) {
		return false;
	}
}