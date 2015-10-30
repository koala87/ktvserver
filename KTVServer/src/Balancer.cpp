/**
 * Balance Module Implementation
 * @author Shiwei Zhang
 * @date 2014.01.26
 */

#include "yiqiding/ktv/Balancer.h"
#include "yiqiding/Exception.h"
using namespace yiqiding::ktv;
using namespace yiqiding;

Balancer::~Balancer() {
	MutexGuard lock(_mutex);
	_in_use.clear();
	_node_index.clear();
	for (auto i = _node.begin(); i != _node.end(); ++i)
		delete *i;
	_node.clear();
}

void Balancer::addNode(size_t id, const std::string& hostname) {
	Node* node;
	MutexGuard lock(_mutex);
	auto node_itr = _node_index.find(id);
	if (node_itr == _node_index.end()) {
		node = new Node;
		_node.push_back(node);	// For Memory
		_node_index[id] = node;	// For index
	} else
		node = node_itr->second;

	node->_id = id;
	node->_hostname = hostname;
	node->_load = 0;
}


const Balancer::Node * Balancer::get(size_t box_id)
{
	MutexGuard lock(_mutex);
	if(!_in_use.count(box_id))
		return NULL;
	return _in_use[box_id];
}

const Balancer::Node *Balancer::random()
{
	MutexGuard lock(_mutex);
	int k = _node.size();
	int i = rand() % k;
	
	return _node[i];
}

const Balancer::Node* Balancer::request(size_t box_id) {	
	release(box_id);

	MutexGuard lock(_mutex);
	if (_node.empty())
		throw Exception("Balancer", "No resource server found", __FILE__, __LINE__);

	// Linear search used for searching with efficiency of O(n)
	// Can be improved later to efficiency of O(log n) by using e.g. heap.
	Node* best_node = *_node.begin();
	for (auto i = ++(_node.begin()); i != _node.end(); ++i)
		if ((*i)->_load < best_node->_load)
			best_node = *i;
	best_node->_load++;
	_in_use[box_id] = best_node;
	return best_node;
}

const Balancer::Node* Balancer::request(size_t box_id, const std::set<size_t>& exclude) {
	release(box_id);

	MutexGuard lock(_mutex);
	if (_node.empty())
		throw Exception("Balancer", "No resource server found", __FILE__, __LINE__);

	// Linear search used for searching with efficiency of O(n)
	// Do not modify the algorithm to a sorting algorithm. It is worse than O(n)
	Node* best_node = *_node.begin();
	for (auto i = ++(_node.begin()); i != _node.end(); ++i) {
		if (exclude.count((*i)->_id))
			continue;
		else if ((*i)->_load < best_node->_load || exclude.count(best_node->_id))
			best_node = (*i);
	}
	if (exclude.count(best_node->_id))
		throw Exception("Balancer", "No resource server available", __FILE__, __LINE__);
	best_node->_load++;
	_in_use[box_id] = best_node;
	return best_node;
}

void Balancer::release(size_t box_id) {
	MutexGuard lock(_mutex);
	
	auto in_use_itr = _in_use.find(box_id);
	if (in_use_itr != _in_use.end()) {
		in_use_itr->second->_load--;
		_in_use.erase(in_use_itr);
	}
}
