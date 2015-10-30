/**
 * KTV Distribute Files
 * Lohas Network Technology Co., Ltd
 * @author Yuchun zhang
 * @date 2014.03.10
 */

#include "yiqiding/ktv/KTVDistribute.h"

std::vector<std::string>::iterator KTVDistribute::findPath(std::string path)
{
	std::vector<std::string>::iterator it = _vecPath.begin();
	while(it != _vecPath.end())
	{
		if(*it == path)
		{
			return it;
		}
		++it;
	}
	return _vecPath.end();
}

void KTVDistribute::addPath(std::string path)
{	
	if(findPath(path) != _vecPath.end())
	{
		_vecPath.push_back(path);
	}
}

void KTVDistribute:: delPath(std::string path)
{
	std::vector<std::string>::iterator itor = findPath(path);
	if (itor != _vecPath.end())
	{
		_vecPath.erase(itor);
	}
}

void KTVDistribute::add(std::string pathname , std::string filename)
{

}

void KTVDistribute::del(std::string filename)
{

}