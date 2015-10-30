/**
 * ERP Roles Definition
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.08
 */

#include "yiqiding/ktv/ERPRole.h"

const char* yiqiding::ktv::ERPRoleString(ERPRole role) {
	using namespace yiqiding::ktv;

	switch (role) {
	case ERP_ROLE_MAIN:
		return "Main";
	case ERP_ROLE_MEDIA:
		return "Media";
	case ERP_ROLE_MARKET:
		return "Market";
	case ERP_ROLE_KITCHEN:
		return "Kitchen";
	default:
		return "Unknown";
	}
}
