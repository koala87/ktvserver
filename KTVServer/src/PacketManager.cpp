/**
 * Packet Manager Implementation
 * @author Shiwei Zhang
 * @date 2014.01.22
 */

#include <memory>
#include "yiqiding/ktv/PacketManager.h"
#include "yiqiding/utility/Logger.h"
using namespace yiqiding::ktv::packet;
using yiqiding::ktv::KTVConnection;

Manager::Manager(Listener* listener) : _listener(listener) {}

Manager::~Manager() {}

void Manager::onCompleteRead(yiqiding::net::tcp::async::Connection* conn, char* data, size_t size) {	
	size_t index = 0;


	try {
			do{	
				Packet* pac = ((KTVConnection*)conn)->getPacket();
				index += pac->feed(data + index , size - index);
				if (pac->isFull())
					_listener->onReceivePacket((KTVConnection*)conn, ((KTVConnection*)conn)->reallocPacket());

			}while(index != size);

	} catch (const BadPacket& e) {
		
		yiqiding::utility::Logger::get("system")->log(conn->getAddress() + "bad packet and shutdown" ,yiqiding::utility::Logger::WARNING);
		conn->shutdown();
		throw e;
	}
	


}
