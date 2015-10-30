/**
 * Connection Content Manager.
 * @warning Do not mix up with Virtual Connections defined in Connection.h
 * @author Shiwei Zhang , Yuchun Zhang
 * @date 2014.03.10
 */

#pragma once

#include "yiqiding/net/TCPAD.h"
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/ktv/ERPRole.h"

namespace yiqiding { namespace ktv {
	using net::tcp::async::Connection;

	enum Type {
		CONNECTION_NONE			=	0,
		CONNECTION_INI,
		CONNECTION_BOX,
		CONNECTION_ERP,
		CONNECTION_APP,
		CONNECTION_ERP_UPLOAD
	};

	class KTVConnection : public Connection {
	private:
		Type	_type;
		Packet*	_packet;
	public:
		KTVConnection(net::tcp::async::SocketListener* listener, net::tcp::async::ConnectionPool* pool, net::tcp::async::EventListener* event_listener, Type type = CONNECTION_NONE) : net::tcp::async::Connection(listener, pool, event_listener), _packet(new Packet), _type(type) {};
		virtual ~KTVConnection() { delete _packet; };

		// Memory Operation
		inline Packet*	releasePacket()		{ Packet* packet = _packet; _packet = NULL; return packet; };
		inline Packet*	reallocPacket()		{ Packet* packet = _packet; _packet = new Packet; return packet; };
		inline void		allocBoxContent();
		inline void		deallocBoxContent();

		// Getter
		inline Packet*			getPacket()			{ return _packet; };
		inline const Packet*	getPacket() const	{ return _packet; };
		inline Type				getType()	const	{ return _type; };	

		// Setter
		inline void	setPacket(Packet* packet)	{ delete _packet; _packet = packet; };
	};

 	class BoxConnection : public KTVConnection {
	private:
		size_t	_box_id;
		bool	_registered;
		bool	_allocated;
	public:
		BoxConnection(net::tcp::async::SocketListener* listener, net::tcp::async::ConnectionPool* pool, net::tcp::async::EventListener* event_listener) : KTVConnection(listener, pool, event_listener, CONNECTION_BOX), _box_id(0), _registered(false), _allocated(false) {};
		~BoxConnection() {};

		// Getter
		inline size_t	getBoxID()		const	{ return _box_id; };
		/// Has been binded to a box id or not
		inline bool		isRegistered()	const	{ return _registered; };
		/// Has been allocated a resource server or not
		inline bool		isAllocated()	const	{ return _allocated; };
		inline bool		isBoxIDValid()	const	{ return _registered || _allocated; };

		// Setter
		inline void setBoxID(size_t box_id)			{ _box_id = box_id; _registered = true; };
		inline void setAllocated(bool val = true)	{ _allocated = val; };
	};
	class IniConnection:public KTVConnection{

	public:
		IniConnection(net::tcp::async::SocketListener * listener , 
			net::tcp::async::ConnectionPool *pool , 
			net::tcp::async::EventListener * event_listener ) : KTVConnection(listener , pool , event_listener , CONNECTION_INI){};
		~IniConnection(){}

	};

	class AppConnection:public KTVConnection{
	private:
		size_t		_uid;
		bool		_registered;
	public:
		AppConnection(net::tcp::async::SocketListener * listener , net::tcp::async::ConnectionPool *pool , net::tcp::async::EventListener * event_listener) : KTVConnection(listener , pool , event_listener , CONNECTION_APP) ,_uid(0) , _registered(false){};
		~AppConnection(){}
		//Getter
		inline size_t	getAppID()		const	{	return _uid ; };
		/// Has been binded to a app or not
		inline bool		isRegistered()	const	{	return _registered ;};	
		
		//Setter
		inline	void	setAppID( size_t id)	{	 _uid = id; _registered = true;	};
	};


	class ERPConnection : public KTVConnection {
	private:
		ERPRole	_role;
		bool	_registered;
	public:
		ERPConnection(net::tcp::async::SocketListener* listener, net::tcp::async::ConnectionPool* pool, net::tcp::async::EventListener* event_listener) : KTVConnection(listener, pool, event_listener, CONNECTION_ERP), _role(ERP_ROLE_NONE), _registered(false) {};
		~ERPConnection() {};

		// Getter
		inline ERPRole	getRole()		const	{ return _role; };
		/// Has been binded to a role or not
		inline bool		isRegistered()	const	{ return _registered; };
		inline bool		isRoleValid()	const	{ return _registered; };

		// Setter
		inline void setRole(ERPRole role)		{ _role = role; _registered = true; };
	};
}}
