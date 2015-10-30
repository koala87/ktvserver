#include "yiqiding/ktv/MultiScreen.h"
#include "yiqiding/utility/Utility.h"
#include "yiqiding/time/Timer.h"

using namespace yiqiding::ktv;



ScreenItem::ScreenItem(const std::string &master , const Json::Value &arrays):_master(master)
{
	static unsigned int now = 112358;
	for(int i = 0 ; i < arrays.size(); ++i)
	{	
		_slaves[i]._slave = arrays[i].asString();
		_slaves[i]._boxid = ++now;
	}
}

void ScreenItem::getBoxids(int (& boxid)[MAX_SLAVE]) const
{
	for(int i = 0 ; i < MAX_SLAVE ; ++i)
	{
		if(_slaves[i]._slave == "")
			break;
		boxid[i] = _slaves[i]._boxid;
	}
}

unsigned int ScreenItem::getSlave(const std::string &slave) const
{
	for(int i = 0 ; i < MAX_SLAVE;++i)
	{
		if(_slaves[i]._slave == slave)
			return _slaves[i]._boxid;
	}

	return INVALID_BOXID;
}

void MultiScreen::add(const std::string &master , Json::Value arrays) 
{
	ScreenItem item(master , arrays);
	_items.push_back(item);
}

void MultiScreen::load(const std::string &path /* = "MultiScreen.json" */)
{
	Json::Value root = yiqiding::utility::FileToJson(path);
	if(!root.isArray())
		return;
	for(int i = 0 ; i < root.size(); ++i)
	{
		add(root[i]["master"].asString() , root[i]["slave"]);
	}
}

unsigned int MultiScreen::getSlave(const std::string& slave)
{
	auto f = std::find_if(_items.begin() , _items.end() , ScreenSlavePt(slave));
	if(f==_items.end())
		return INVALID_BOXID;

	return f->getSlave(slave);
	
}

std::string MultiScreen::getMasterIp(const std::string & slave)
{
	auto f = std::find_if(_items.begin() , _items.end() , ScreenSlavePt(slave));
	if(f==_items.end())
		return "";

	return f->getMaster();
}

void MultiScreen::getSlave(const std::string &master , int (& boxid)[5])
{
	auto f= std::find_if(_items.begin() , _items.end() , ScreenMasterPt(master));
	if(f == _items.end())
		return ;

	f->getBoxids(boxid);
}


