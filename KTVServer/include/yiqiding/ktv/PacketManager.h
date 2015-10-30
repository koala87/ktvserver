/**
 * Packet Manager
 * @author Shiwei Zhang
 * @date 2014.01.22
 */

#pragma once

#include "yiqiding/ktv/KTVConnection.h"

namespace yiqiding { namespace ktv { namespace packet {
	// Packet Listener
	class Listener {
	public:
		/// @warning The listener should take the responsibility of deletion of the packet
		virtual void onReceivePacket(KTVConnection* conn, Packet* pac) = 0;

	};



	// Packet Listener
	class Manager : public net::tcp::async::EventListener {
	private:
		Listener*	_listener;
	public:
		Manager(Listener* listener);
		~Manager();
		void onCompleteRead(net::tcp::async::Connection* conn, char* data, size_t size);
	};
}}}

namespace yiqiding { namespace ktv {
	typedef packet::Manager PacketManager;
}}
