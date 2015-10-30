/**
 * KTV Dongle
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.19
 */

#pragma once

#include <cstdint>
#include <string>
#include "yiqiding/Thread.h"
#include "yiqiding/Exception.h"

namespace yiqiding { namespace ktv {
	static const char DEFAULT_DONGLE_USER_PASSWORD[]	=	"00000000";

	class Dongle {
	private:
		static Dongle* _instance;

		Mutex		_mutex;
		std::string	_usr_pwd;
		void*		_handle;	/* Type: EVHANDLE */

		void open();
		void close();
	public:
		// Get singleton
		static Dongle* getInstance();
		static void unload()	{ delete _instance; };

		// Basic stuffs
		Dongle() : _handle(NULL) {};
		~Dongle() { close(); };

		void setUserPassword(const std::string& usr_pwd);

		// Logic section
		void checkPassword();
		bool checkUDID(uint32_t time , const char *data);
	};

	class DongleException : public Exception {
	private:
		static std::string toHexString(uint32_t n);
	public:
		explicit DongleException(const std::string& reason, const std::string& src_file, line_t line_no) : yiqiding::Exception("Dongle", reason, src_file, line_no) {};
		explicit DongleException(const std::string& reason, uint32_t return_code, const std::string& src_file, line_t line_no) : yiqiding::Exception("Dongle", reason + ": " + toHexString(return_code), src_file, line_no) {};
		virtual ~DongleException() throw() {};
	};
}}
