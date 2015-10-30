/**
 * Server Crash Process
 * @author Yuchun Zhang
 * @date 2014.08.09
 */
#include "yiqiding/ktv/KTVServer.h"
#include "yiqiding/ktv/ServerCrash.h"
#include "yiqiding/net/smtpmail.h"
#include "yiqiding/utility/Logger.h"
#include "yiqiding/net/FileCurlDistribute.h"
using namespace yiqiding::ktv;
using namespace yiqiding::net::mail;

void ServerCrash::process(const std::string &path)
{
	std::string usr = "125712105@qq.com";
	std::string pwd = "XJDTSS02041219";
	std::string host = "smtp.qq.com";
	int			port = 465;
	std::string to = "jinyingqi@yiqiding.com";
	
	do{
		SmtpMail mail(MAIL_SECURE);
		if(!mail.Login(usr , pwd , host , port))
			break;
		if(!mail.sendBody(to , "ktv server crash" , yiqiding::ktv::Version))
			break;

		mail.sendAttach(path , "ktvserver.dump");
		mail.sendAttach("box.xml" , "box.xml");
		mail.sendAttach("ktvserver.conf" , "ktvserver.conf");
		yiqiding::utility::Logger::unload();
		mail.sendAttach("ktvserver.log" , "ktvserver.log" , 1024 * 10);
		mail.sendAttach("request.log" , "request.log" , 1024 * 100);
		mail.sendEnd();
	}while(0);

	char time_str[15];
	{
		time_t now = ::time(NULL);
		struct tm timeinfo;
		
		localtime_s(&timeinfo, &now);
		strftime(time_str,15,"%Y%m%d%H%M%S", &timeinfo);
	}

	yiqiding::net::DistributeContent::UpLoadFile(yiqiding::net::yiqichang_server , 
		yiqiding::utility::toString("server") + time_str + yiqiding::utility::generateTempCode() ,
		"text/plain" , "ktvserver.log" ,box::BoxInfoMan::getInstace()->getShopName());

	//先停止服务
	//system("net stop LoHasServer & net start LoHasServer");
	
	STARTUPINFOA si = { sizeof(si) };   
	PROCESS_INFORMATION pi;   

	si.dwFlags = STARTF_USESHOWWINDOW;   
	si.wShowWindow = FALSE; //TRUE表示显示创建的进程的窗口

	CreateProcessA("..\\server.bat",
		NULL,
		NULL ,
		NULL ,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi);


}