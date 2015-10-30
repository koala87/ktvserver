/**
 * KTV Database Media Model
 * Lohas Network Technology Co., Ltd
 * @author Shiwei Zhang
 * @date 2014.02.14
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include "yiqiding/ktv/GameTimer.h"

namespace yiqiding { namespace ktv { namespace db { namespace model {

class Game
{
	unsigned int					_kid;
	int								_ktype;
	unsigned int					_kstarttime;
	std::set<unsigned int>			_ksendtime;
	int								_kstate;
	std::string						_ktitle;
	std::string						_kmessage;
	std::vector<yiqiding::ktv::game::MidName>			_kmids;
	std::set<unsigned int>			_kjoins;
	std::vector<yiqiding::ktv::game::BoxScore>			_kscores;

	// Getter
	inline unsigned int							getKId()					const { return _kid; };
	inline int									getKType()					const { return _ktype; };
	inline unsigned int							getKStartTime()				const { return _kstarttime; };
	inline const std::set<unsigned int>&		getKSendTime()				const { return _ksendtime; };
	inline int									getKState()					const { return _kstate; };
	inline const std::string&					getKTitle()					const { return _ktitle; };
	inline const std::string&					getKMessage()				const { return _kmessage; };
	inline const std::set<unsigned int>&		getKJoins()					const { return _kjoins; };
	inline const std::vector<yiqiding::ktv::game::MidName>&			getKMids()					const { return _kmids; };	
	inline const std::vector<yiqiding::ktv::game::BoxScore>&			getKScores()				const { return _kscores; };

	// Setter
	inline void									setKId(unsigned int kid)								{ _kid = kid; };
	inline void									setKType(int ktype)										{ _ktype = ktype; };
	inline void									setKStartTime(unsigned int kstarttime)					{ _kstarttime = kstarttime; };
	inline void									setKSendTime(const std::set<unsigned int> & ksendtime)	{ _ksendtime = ksendtime; };
	inline void									setKState(int kstate)									{ _kstate = kstate; };
	inline void									setKTitle(const std::string &ktitle)					{ _ktitle = ktitle; };
	inline void									setKMessage(const std::string &kmessage)				{ _kmessage = kmessage; };
	inline void									setKJoins(const std::set<unsigned int> &kjoins)			{ _kjoins = kjoins; };
	inline void									setKMids(const std::vector<yiqiding::ktv::game::MidName> &kmids)				{ _kmids = kmids; };	
	inline void									setKScores(const std::vector<yiqiding::ktv::game::BoxScore> &kscores)		{ _kscores = kscores; };

};