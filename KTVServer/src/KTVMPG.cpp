#include "yiqiding/ktv/KTVMPG.h"
#include "Convertion.h"

using namespace yiqiding::ktv;

int KTVMeidaConvert::ConvertMpgToMp4ByFile(const std::string &srcFile , const std::string &dstFile)
{
	return Convertion(srcFile.c_str() ,dstFile.c_str());
}

