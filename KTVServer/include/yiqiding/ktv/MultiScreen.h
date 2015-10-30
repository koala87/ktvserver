#pragma once
/*

∂‡∆µ∑Ω∞∏
2015-7-7

*/
#include <vector>
#include "json/json.h"
#include <string>

namespace yiqiding { namespace ktv{

 static const  int MAX_SLAVE	= 5;


 static const unsigned int INVALID_BOXID = 0xffffffff;



struct ViceScreen
{
	std::string		_slave;
	unsigned int	_boxid;	
};

class ScreenItem
{
	std::string _master;
	ViceScreen _slaves[MAX_SLAVE];
public:
	ScreenItem(const std::string &master , const Json::Value &arrays);
	void getBoxids(int (& boxid)[MAX_SLAVE]) const;
	unsigned int getSlave(const std::string &slave) const ;
	const std::string & getMaster() const { return _master;}
};



class ScreenSlavePt{
	std::string _slave;
	public:
		ScreenSlavePt(const std::string &slave):_slave(slave){}
		bool operator()(const ScreenItem &item){ 
			return  (item.getSlave(_slave) != INVALID_BOXID);
		}
};

class ScreenMasterPt{
	std::string _master;
public:
	ScreenMasterPt(const std::string &master):_master(master){}
	bool operator()(const ScreenItem &item) { return (item.getMaster() == _master);}
};




class MultiScreen
{
private:
	std::vector<ScreenItem> _items;
	void add(const std::string &master , Json::Value arrays);
public:
	void load(const std::string &path = "MultiScreen.json");
	unsigned int getSlave(const std::string& slave); 
	std::string getMasterIp(const std::string & slave);
	void getSlave(const std::string &master , int (& boxid)[MAX_SLAVE]);
};






}}