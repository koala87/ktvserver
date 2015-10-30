#include "yiqiding/ktv/KTVPCM.h"
#include "ws2tcpip.h"
using namespace yiqiding::ktv;


KTVPCM::PcmItem::PcmItem(yiqiding::ktv::Server *server , uint32_t boxid , std::string roomno , std::string roomname):_serv(server),_boxid(boxid),_status(false),_roomname(roomname) , _roomno(roomno){

}

void KTVPCM::PcmItem:: startPcm(uint32_t time){
	_status = true;
	_times = time;
}
void KTVPCM::PcmItem::stopPcm(){
	if(!_status)
		return;

	_status = false;
	notifyReceiverStop();
	_otherboxids.clear();
}
void KTVPCM::PcmItem::listenPcm(uint32_t boxid , uint32_t &time){
	
	time = timeGetTime() - _times;
	if(_otherboxids.count(boxid))
		return;

	_otherboxids.insert(boxid);
	//alertSenderListen(boxid);
}
void KTVPCM::PcmItem::removePcm(uint32_t boxid , const Json::Value &message){
	if(!_otherboxids.count(boxid))
		return;

	_otherboxids.erase(boxid);
	alertSenderRemove(boxid , message);
}

/*
void KTVPCM::PcmItem::alertSenderListen(uint32_t boxid){
	
	packet::Packet pack(packet::KTV_REQ_BOX_PCM_LISTEN_ALERT);
	Json::Value root;

	std::string roomno;
	std::string roomname;
	std::string strip = box::BoxInfoMan::getInstace()->getIP(boxid);
	{
		yiqiding::MutextReader mr(box::BoxInfoMan::getMutexWR());
		box::BoxInfoItem *item = box::BoxInfoMan::getInstace()->getItem(strip);
		roomno = item->getRoomNo();
		roomname = item->getRoomName();
	}

	root["boxid"] = boxid;
	root["roomno"] = roomno;
	root["roomname"] =roomname;
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	try {
		_serv->getConnectionManager()->sendToBox(boxid , &pack);
	} catch (const std::exception& err) {
		;
	}


}*/

void KTVPCM::PcmItem::alertSenderRemove(uint32_t boxid , const Json::Value &message){

	packet::Packet pack(packet::KTV_REQ_BOX_PCM_STOP_ALERT);
	Json::Value root;

// 	std::string roomno;
// 		std::string roomname;
// 		std::string strip = box::BoxInfoMan::getInstace()->getIP(boxid);
// 		{
// 			yiqiding::MutextReader mr(box::BoxInfoMan::getMutexWR());
// 			box::BoxInfoItem *item = box::BoxInfoMan::getInstace()->getItem(strip);
// 			roomno = item->getRoomNo();
// 			roomname = item->getRoomName();
// 		}
// 	
// 		root["boxid"] = boxid;
// 		root["roomno"] = roomno;
// 		root["roomname"] =roomname;

	root["boxid"] = boxid;
	root["message"] = message;
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	try {
		_serv->getConnectionManager()->sendToBox(_boxid , &pack);
	} catch (const std::exception& err) {
		;
	}

}

void KTVPCM::PcmItem::notifyReceiverStop(){


	packet::Packet pack(packet::KTV_REQ_BOX_PCM_STOP_NOTIFY);
	Json::Value root;
	root["boxid"] = _boxid;
	root["roomno"] = _roomno;
	root["roomname"] =_roomname;
	std::string msg = root.toStyledString();
	pack.setPayload(msg.c_str() , msg.length());
	
	for each(auto k in _otherboxids){
		try {
			_serv->getConnectionManager()->sendToBox(k , &pack);
		} catch (const std::exception& err) {
			;
		}
	}
	


}


void KTVPCM::run(){

	while (true) {
		PcmBuf buf;
		{
			MutexGuard guard(_pcm_data_mutex);
			while (_pcm_data_queue.empty())
				if (_loop)
					_pcm_data_cond.wait(_pcm_data_mutex);
				else
					return;
			buf = _pcm_data_queue.front();
			_pcm_data_queue.pop();
		}
		_ser->getConnectionManager()->sendToMusic(buf._self , buf._data , buf._len);
		buf.release();
	}
}

bool KTVPCM::transmit(uint32_t self , const char *data , int len)
{

	PcmBuf buf(self , data , len);
	{
		yiqiding::MutexGuard	guard(_pcm_data_mutex);
		_pcm_data_queue.push(buf);
		_pcm_data_cond.signal();
	}
	return true;
}

KTVPCM::KTVPCM():_loop(true) , _thread(this){
	_thread.start();
}

KTVPCM::~KTVPCM()
{
	_loop = false;
	_pcm_data_cond.signal();
	_thread.join();
}

bool KTVPCM::startPcm(uint32_t self , uint32_t boxid ,int millSecond ){
	

	_ser->getConnectionManager()->clearMusic(self);
	{

		MutexGuard guard(_pcm_command_mutex);
		PcmItem *pItem;
		auto it = std::find_if(_pcmitems.begin() , _pcmitems.end() , PrPcm(boxid));
		SOCKADDR_IN in;
		std::string strip;
		{
		
				memset(&in , sizeof(in) , 0);
				in.sin_addr.s_addr = self;
				char ip[16];
				inet_ntop(AF_INET , &in.sin_addr.s_addr , ip , 16);
				strip = ip;
		}
		std::string roomno;
		std::string roomname;
		{
			yiqiding::MutextReader mr(box::BoxInfoMan::getMutexWR());
			box::BoxInfoItem *item = box::BoxInfoMan::getInstace()->getItem(strip);
			roomno = item->getRoomNo();
			roomname = item->getRoomName();
		}
		if(it == _pcmitems.end())
		{
			pItem = new PcmItem(_ser , boxid , roomno , roomname);
			pItem->startPcm(timeGetTime() - millSecond);
			_pcmitems.push_back(pItem);
			
		}
		else
		{
			(*it)->startPcm(timeGetTime() - millSecond);
		}
	}

	return true;
}

bool KTVPCM::stopPcm(uint32_t self , uint32_t boxid)
{
	_ser->getConnectionManager()->clearMusic(self);
	
	{
		MutexGuard guard(_pcm_command_mutex);
		PcmItem *pItem;
		auto it = std::find_if(_pcmitems.begin() , _pcmitems.end() , PrPcm(boxid));
		if(it != _pcmitems.end())
		{
			(*it)->stopPcm();
		}
	}
	return true;
}


bool KTVPCM::listenPcm(uint32_t boxid , uint32_t otherboxid , uint32_t selft , uint32_t other , uint32_t &time){

	_ser->getConnectionManager()->listenMusic(selft , other);

	{
		MutexGuard guard(_pcm_command_mutex);
		
		//delete first
		for each(auto k in _pcmitems){
			k->removePcm(otherboxid , Json::Value(Json::nullValue));
		}
		
		
		
		PcmItem *pItem;
		auto it = std::find_if(_pcmitems.begin() , _pcmitems.end() , PrPcm(boxid));
		if(it != _pcmitems.end())
		{
			(*it)->listenPcm(otherboxid ,time);
			return true;
		}
	}
	return false;
}

bool KTVPCM::removePcm(const Json::Value &message ,uint32_t boxid , uint32_t otherboxid , uint32_t self , uint32_t other){
	
	_ser->getConnectionManager()->removeMusic(self ,other);


	{
		MutexGuard guard(_pcm_command_mutex);
		PcmItem *pItem;
		auto it = std::find_if(_pcmitems.begin() , _pcmitems.end() , PrPcm(boxid));
		if(it != _pcmitems.end())
		{
			(*it)->removePcm(otherboxid , message);
			
		}
	}

	return true;
}