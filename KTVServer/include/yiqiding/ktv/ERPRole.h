/**
 * ERP Roles Definition
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.11
 */

#pragma once

namespace yiqiding { namespace ktv {
	enum ERPRole {
		ERP_ROLE_NONE			=	0,
		ERP_ROLE_MAIN			=	1001,
		ERP_ROLE_MEDIA			=	1002,
		ERP_ROLE_MARKET			=	1003,
		ERP_ROLE_KITCHEN		=	1004,
	};

	const char* ERPRoleString(ERPRole role);
}}
