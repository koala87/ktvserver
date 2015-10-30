/**
 * Baisc Database Exception
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.01.26
 */

#pragma once

#include <sstream>
#include "yiqiding/Exception.h"
#include "yiqiding/Thread.h"
#include "yiqiding/ktv/Balancer.h"

namespace yiqiding { namespace ktv { namespace db {
	// Exception
	class DatabaseException : public yiqiding::Exception {
	public:
		explicit DatabaseException(const std::string& connector_name, const std::string& what, const std::string& src_file, line_t line_no) : yiqiding::Exception("Database", connector_name + ": " + what, src_file, line_no) {};
		virtual ~DatabaseException() throw() {};
	};
}}}
