/**
 * KTV Database Definition
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.08
 */

#pragma once

#include "yiqiding/sql/MySQL.h"
#include "yiqiding/Exception.h"

namespace yiqiding { namespace ktv { namespace db {
	using namespace yiqiding::sql::mysql;

	// Exception
	class DBException : public Exception {
	public:
		explicit DBException(const std::string& reason, const std::string& src_file, line_t line_no) : yiqiding::Exception("Database", reason, src_file, line_no) {};
		virtual ~DBException() throw() {};
	};
}}}
