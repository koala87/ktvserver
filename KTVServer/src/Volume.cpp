#include "yiqiding/ktv/Volume.h"
#include "yiqiding/io/File.h"

using namespace yiqiding::ktv;
using namespace yiqiding::io;
void Volume::load(const std::string &path)
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
		_volumeInfo = pdata.get();
	} catch (...) {}
}

std::string Volume::getVolumeInfo() const
{
	MutexGuard guard(_mutex);
	return _volumeInfo;
}

bool Volume::save(const std::string &path , const std::string &volumeInfo)
{
	File fp(path);
	try {
		MutexGuard guard(_mutex);
		_volumeInfo = volumeInfo;
		fp.open(yiqiding::io::File::CREATE|yiqiding::io::File::WRITE | yiqiding::io::File::READ);
		fp.write(volumeInfo.c_str() , volumeInfo.length());
		fp.close();
		return true;
	} catch (...) {
		return false;
	}
}


