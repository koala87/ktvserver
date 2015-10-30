/**
 * KTV Database Config Model
 * Lohas Network Technology Co., Ltd
 * @author Yuchun Zhang
 * @date 2014.05.06
 */

#pragma once
#include <map>

namespace yiqiding { namespace ktv { namespace db { namespace model {

	const std::string longitude = "longitude";
	const std::string latitude = "latitude";
	class ConfigItem
	{
	private:
		int				_cid;
		std::string		_name;
		std::string		_value;
		std::string		_detail;

	public:
		void setCid(int cid) { _cid = cid;}
		void setName(const std::string &name) { _name = name;}
		void setValue(const std::string &value){ _value = value;}
		void setDetail(const std::string &detail) { _detail = detail;}

		int				getCid() const { return _cid;}
		std::string		getName() const { return _name;}
		std::string		getValue() const { return _value;}
		std::string		getDetail() const { return _detail;}
	};

}}}}
