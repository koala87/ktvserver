/**
 * Balance Module
 * @author Shiwei Zhang
 * @date 2014.01.26
 */

#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <set>
#include "yiqiding/Thread.h"
#include "yiqiding/sql/MySQL.h"

namespace yiqiding { namespace ktv {
	/// Balancer a.k.a. Resource Server Manager
	/// Thread-Safety: getNode(); request(); release(); are thread-safe.
	class Balancer {
	public:
		class Node {
		private:
			size_t						_id;
			std::string					_hostname;	//	Memory-Speed Trade off <- Efficiency requried.
			size_t						_load;
			
			friend class Balancer;	// Allow Balancer to direct control. Originally, it was "struct Node". Just for provide access control.
		public:
			inline size_t						getID()				const	{ return _id; };
			inline const std::string&			getHostname()		const	{ return _hostname; };
			inline size_t						getLoad()			const	{ return _load; };
		};
	private:
		Mutex					_mutex;
		/// Node Container
		std::vector<Node*>		_node;
		/// Node id | Node
		std::map<size_t, Node*>	_node_index;
		/// Box id | Node
		/// Full scan required when node deletion
		std::map<size_t, Node*>	_in_use;
	public:
		Balancer() {};
		~Balancer();

		/// Add node. If node exists, update it.
		/// Assuming the resource server contains a database replica...
		void addNode(size_t id, const std::string& hostname);
		
		inline const std::vector<Node*>&	getNodes()	const	{ return _node; };
		inline const std::map<size_t , Node *> getMapNodes() const {return _in_use;}

		const Node* get(size_t box_id);
		const Node* random();
		const Node* request(size_t box_id);
		const Node* request(size_t box_id, const std::set<size_t>& exclude);
		void release(size_t box_id);
	};
}}
