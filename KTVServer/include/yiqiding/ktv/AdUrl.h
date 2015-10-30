#include <string>
#include <queue>
#include "yiqiding/Thread.h"


namespace yiqiding { namespace ktv{ 


	const std::string AdPath = "adurl.json";

	class AdUrl
	{
	public:
		void load(const std::string &path);
		bool save(const std::string &path , const std::string &urls);
		std::string getUrls() const ;
	private:
		std::string _urls;
		mutable yiqiding::Mutex _mutex;
	};

} }