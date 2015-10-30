/**
 * KTV Dongle Implementation
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.20
 */

#include <cstring>
#include "elite5.h"
#include "yiqiding/ktv/Dongle.h"
using namespace yiqiding::ktv;
using yiqiding::MutexGuard;

// Fixed IDs
#define KTV_DEVELOPER_ID	0x424A4C48
#define KTV_PRODUCT_ID		0x00000000

#ifdef _DEBUG
#pragma comment(lib, "elite5d")
#else
#pragma comment(lib, "elite5")
#endif

Dongle* yiqiding::ktv::Dongle::_instance = NULL;

void Dongle::open() {
	// Close if already open
	close();

	EVUINT32 ret;

	// Open Dongle
	if ((ret = EVOpen(KTV_DEVELOPER_ID, KTV_PRODUCT_ID, NULL, 0, &_handle)) != EV_ERROR_SUCCESS)
		throw DongleException("Fail to open", ret, __FILE__, __LINE__);

	// Reset device
	if ((ret = EVControl(_handle,EV_CONTROL_CODE_RESET,NULL)) != EV_ERROR_SUCCESS)
		throw DongleException("Fail to reset", ret, __FILE__, __LINE__);

	// Login (user mode)
	if (_usr_pwd.size() != EV_PASSWORD_LENGTH_USER)
		throw DongleException("Invalid password length", __FILE__, __LINE__);
	else if ((ret = EVLogin(_handle, EV_LOGIN_MODE_EXCLUSIVE, EV_PASSWORD_TYPE_USER, (char*)_usr_pwd.c_str(), EV_PASSWORD_LENGTH_USER)) != EV_ERROR_SUCCESS) {
		EVClose(_handle);
		throw DongleException("Fail to login", ret, __FILE__, __LINE__);
	}
}

void Dongle::close() {
	// Basic check
	if (_handle == NULL)
		return;

	// Logout
	EVLogout(_handle);

	// Close
	EVClose(_handle);

	_handle = NULL;
}

Dongle* Dongle::getInstance() {
	if (_instance == NULL)
		_instance = new Dongle();
	return _instance;
}

void Dongle::setUserPassword(const std::string& usr_pwd) {
	if (usr_pwd.size() != EV_PASSWORD_LENGTH_USER)
		throw DongleException("Invalid password length", __FILE__, __LINE__);
	else
		_usr_pwd = usr_pwd;
}

void Dongle::checkPassword() {
	MutexGuard lock(_mutex);
	open();
	close();
}

bool Dongle::checkUDID(uint32_t time , const char *data) {
	MutexGuard lock(_mutex);
	char args[30] = {0};

	if (data != NULL && strlen(data) != 25)
	{
		return false;
	}

	if(data)
	strcpy_s(args, data);
	
	memcpy(args + 26 , &time , 4);

	open();
	uint8_t result;
	EVUINT32 len;
	EVUINT32 ret = EVExecute(_handle, "udid_check.evx", (EVUINT8*)args, 30, (EVUINT8*)&result, 1, &len);
	if (ret != EV_ERROR_SUCCESS)
	{	
		close();
		throw DongleException("Fail to execute", ret, __FILE__, __LINE__);
	}
	close();

	return result == 0;
}

std::string DongleException::toHexString(uint32_t n) {
	char s[11];
	sprintf_s(s, "0x%08x", n);
	s[10] = 0;
	return s;
}
