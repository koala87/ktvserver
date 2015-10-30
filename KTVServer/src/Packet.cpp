/**
 * Protocol & Packet Implementation
 * @author Shiwei Zhang
 * @date 2014.01.20
 */

#include <cstdlib>
#include <exception>
#include "yiqiding/ktv/Packet.h"
#include "yiqiding/utility/memman.h"
#include "yiqiding/utility/Logger.h"
using namespace yiqiding::ktv::packet;

//////////////////////////////////////////////////////////////////////////
// Packet
//////////////////////////////////////////////////////////////////////////

void Packet::toNetwork() {
	Header* header = (Header*)_data;
	header->auth = htonl(header->auth);
	header->version = htonl(header->version);
	header->request = htonl(header->request);
	header->identifier = htonl(header->identifier);
	header->length = htonl(header->length);
	header->device_id = htonl(header->device_id);
}

void Packet::toHost() {
	Header* header = (Header*)_data;
	header->auth = ntohl(header->auth);
	header->version = ntohl(header->version);
	header->request = ntohl(header->request);
	header->identifier = ntohl(header->identifier);
	header->length = ntohl(header->length);
	header->device_id = ntohl(header->device_id);
}

Packet::Packet() : _consumed(0),_checked(false) {
	_data = (char*)Lohas_MALLOC(HEADER_SIZE);
	if (_data == NULL)
		throw std::bad_alloc();
	Header* header = (Header*)_data;
	header->auth			=	PACKET_AUTH;
	header->version		=	PACKET_VERSION;
}

Packet::Packet(Request request) : _consumed(HEADER_SIZE),_checked(false) {
	_data = (char*)Lohas_MALLOC(HEADER_SIZE);
	if (_data == NULL)
		throw std::bad_alloc();

	Header* header = (Header*)_data;

	header->auth			=	PACKET_AUTH;
	header->version		=	PACKET_VERSION;
	header->request		=	request;
	header->identifier	=	rand();
	header->length		=	0;
	header->device_id	=	0;
}

Packet::Packet(Request request , size_t length) : _consumed(HEADER_SIZE),_checked(false) {
	_data = (char*)Lohas_MALLOC(length);
	if (_data == NULL)
		throw std::bad_alloc();

	Header* header = (Header*)_data;

	header->auth			=	PACKET_AUTH;
	header->version		=	PACKET_VERSION;
	header->request		=	request;
	header->identifier	=	rand();
	header->length		=	0;
	header->device_id	=	0;
}

Packet::Packet(const Header& src) : _consumed(HEADER_SIZE),_checked(false) {
	_data = (char*)Lohas_MALLOC(HEADER_SIZE);
	if (_data == NULL)
		throw std::bad_alloc();

	Header* header = (Header*)_data;
	*header = src;
	header->length = 0;
}

Packet::Packet(const Packet& src) : _consumed(src._consumed),_checked(src._checked) {
	size_t length = HEADER_SIZE + src.getLength();
	
	_data = (char*)Lohas_MALLOC(length);
	if (_data == NULL)
		throw std::bad_alloc();

	memcpy(_data, src._data, length);
}

Packet::~Packet() {
	Lohas_FREE(_data);
}

Packet& Packet::operator=(const Packet& src) {
	if (this == &src)
		return *this;
	
	size_t length = HEADER_SIZE + src.getLength();
	_data = (char*)Lohas_REALLOC(_data, length);
	if (_data == NULL)
		throw std::bad_alloc();

	memcpy(_data, src._data, length);
	_consumed = src._consumed;
	_checked = src._checked;

	return *this;
}




size_t Packet::feed(const char* data, size_t size)
{
	size_t used = 0;
	if(_consumed < HEADER_SIZE)
	{
		size_t consuming = size > (HEADER_SIZE - _consumed)?(HEADER_SIZE - _consumed):size;	
		memcpy(_data + _consumed , data + used , consuming);
		_consumed += consuming;
		used += consuming;
	}
		
	if(_consumed >= HEADER_SIZE)
	{
		if(!_checked)
		{
			toHost();
			if(getAuth() != PACKET_AUTH)
				throw BadPacket(this, "Bad Authentication Code:"+ yiqiding::utility::toString(getAuth()) +"\r\n" , __FILE__, __LINE__);
			if(getVersion() != PACKET_VERSION)
				throw BadPacket(this, "Version does not match: " + yiqiding::utility::toString(getVersion()) +"\r\n" , __FILE__, __LINE__);
			if(getLength() > PACKET_MAX_LENGTH)
				throw BadPacket(this , "req:" + utility::toString(getRequest()) +" len:"+ utility::toString(getLength()) + " Len does not right" , __FILE__ , __LINE__);

			if(getLength() != 0)
				_data = (char*)Lohas_REALLOC(_data, HEADER_SIZE + getLength());

			_checked = true;
		}
		
		//full pack
		if( size - used >= HEADER_SIZE + getLength() - _consumed )
		{
			size_t consuming = HEADER_SIZE + getLength() - _consumed;
			memcpy(_data + _consumed , data + used , consuming);
			_consumed += consuming;
			used += consuming;		 
		}
		else
		{
			size_t consuming = size - used;
			memcpy(_data + _consumed , data + used , consuming);
			_consumed += consuming;
			used += consuming;
		}

		
	}

	return used;

}



/*
size_t Packet::feed(const char* data, size_t size)
{
	size_t index = 0;
	if (_consumed < HEADER_SIZE)
	{
		size_t nSize = (size > HEADER_SIZE - _consumed)? (HEADER_SIZE - _consumed) : size;

		memcpy(_data + _consumed , data , nSize);
		size -= nSize;
		index += nSize;
		_consumed += nSize;
		
	}
	if(size > 0)
	{
		if (_consumed == HEADER_SIZE )
		{
			toHost();	
			
			// Check for basic validity
			if (getAuth() != PACKET_AUTH)
				throw BadPacket(this, "Bad Authentication Code", __FILE__, __LINE__);
			else if (getVersion() != PACKET_VERSION)
				throw BadPacket(this, "Version does not match", __FILE__, __LINE__);
			// Payload memory allocation
			uint32_t len = getLength();
			char* new_data = (char*)realloc(_data, HEADER_SIZE + len);
			if (new_data == NULL)
				throw std::bad_alloc();
			_data = new_data;
		}
		int len = getLength();

		if (size  + _consumed < len + HEADER_SIZE)
		{
			memcpy(_data + _consumed , data + index , size);
			index += size;
			_consumed += size;
			size -= size;
			
		}
		else 
		{
			memcpy(_data + _consumed , data + index , len + HEADER_SIZE - _consumed );		
			index += (len + HEADER_SIZE - _consumed);
			size -= (len + HEADER_SIZE  - _consumed);
			_consumed +=  (len  + HEADER_SIZE- _consumed);
			
		}
	}

	return index;
	

}*/

/*

size_t Packet::feed_origin(const char* data, size_t size) {
	size_t max_length;
	size_t consuming;

	if (_consumed < HEADER_SIZE)
		max_length = HEADER_SIZE;
	else
		max_length = getLength() + HEADER_SIZE;
	if (_consumed + size < max_length)
		consuming = size;
	else
		consuming = max_length - _consumed;
	
	// Fill
	if(consuming != 0)
	{
		memcpy(_data + _consumed, data, consuming);
		_consumed += consuming;
	}
	
	// Fill Payload if Header was just filled
	if (_consumed == HEADER_SIZE && size != 0) {
		// Prepare
		toHost();

		// Check for basic validity
		if (getAuth() != PACKET_AUTH)
			throw BadPacket(this, "Bad Authentication Code:"+ yiqiding::utility::toString(getAuth()) +"\r\n" + this->toLogString(), __FILE__, __LINE__);
		else if (getVersion() != PACKET_VERSION)
			throw BadPacket(this, "Version does not match: " + yiqiding::utility::toString(GetVersion()) +"\r\n" + this->toLogString(), __FILE__, __LINE__);

		// Payload memory allocation
		uint32_t len = getLength();
		if (len >= PACKET_MAX_LENGTH)
			throw BadPacket(this , "req:" + utility::toString(getRequest()) +" len:"+ utility::toString(len) + " Len does not right" , __FILE__ , __LINE__);
		
		char* new_data = (char*)realloc(_data, HEADER_SIZE + len );

		if (new_data == NULL)
			throw std::bad_alloc();
		_data = new_data;

		// Fill payload
		consuming += feed_origin(data + consuming, size - consuming);
	}

	return consuming;
}*/


void Packet::dispatch(yiqiding::net::tcp::async::Connection *conn , const char *data , int len)
{
	conn->transmit(data , len);
}

void Packet::dispatch(yiqiding::net::tcp::async::Connection* conn) const {
	((Packet*)this)->toNetwork();
	__try {
#ifdef USE_SECURE_SSL
		if(conn->isSecured())			//only once , test
		{
			int index = 0;
			int ret = 0;
			int len = 0;
			while(len < _consumed)
			{
				ret = conn->sslWrite(_data + len , _consumed - len);
				if(ret <= 0 )
				{	
					throw std::exception("sslWrite Error in dispatch");
					break;
				}
				len += ret;
			}
		}
		else{ conn->transmit(_data ,_consumed); }
#else
		conn->transmit(_data, _consumed);
#endif
	} __finally {
		((Packet*)this)->toHost();
	}
}

void Packet::dispatch(yiqiding::io::ostream& out) const {
	((Packet*)this)->toNetwork();
	__try {
		out.write(_data, _consumed);
	} __finally {
		((Packet*)this)->toHost();
	}
}

void Packet::setPayload(const char* payload, size_t length) {

	char* new_data = (char*)Lohas_REALLOC(_data, HEADER_SIZE + length);
	if (new_data == NULL)
		throw std::bad_alloc();
	_data = new_data;
	getHeaderM().length = (uint32_t)length;
	memcpy(_data + HEADER_SIZE, payload, length);
	_consumed = HEADER_SIZE + length;
}

void Packet::setData(const char *data , size_t length)
{
	getHeaderM().length = length;
	memcpy(_data + HEADER_SIZE , data , length);
	_consumed = HEADER_SIZE + length;
}

std::string Packet::toLogString() const
{
	std::ostringstream out;
	std::string content(getPayload() , getPayload() + getLength());
	out << "request:" << getRequest()<<" body:"<< content ;

	return out.str();
}

std::string Packet::toString() const
{
	std::ostringstream out;
	const char * str = getPayload();
    std::vector<char> newstr; 
	while(str != getPayload() + getLength())
	{
		if(*str == '\n')
		{
			newstr.push_back('\r');
		}
		newstr.push_back(*str++);
	}
	newstr.push_back(0);
	std::string tmp(newstr.begin() , newstr.end());
	out << "request:" << getRequest() << "\r\n";
	out << "identifier:" << getIdentifier() << "\r\n";
	out << "length:"<<getLength() << "\r\n";
	out <<"body:" << "\r\n" << tmp << "\r\n";

	return out.str();
}