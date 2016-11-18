/*
	Binary tree class
*/

#pragma once

#include <vector>
#include <ostream>
#include <istream>
#include <cassert>

template<
	typename value_t,
	typename = is_default_constructible<value_t>::type
>
class BinaryTree
{
public:

	typedef unsigned int NodeId;

private:

	struct Node
	{
		//Value of node
		value_t value;

		//Index of left child node
		NodeId left = 0;
		//Index of right child node
		NodeId right = 0;

		Node() :
			value(value_t())
		{}

		Node(const value_t& v) :
			value(v)
		{}

		Node(value_t&& v) :
			value(v)
		{}
	};

	std::vector<Node> m_nodes;

	bool validateId(NodeId id) const
	{
		bool expr = (id <= m_nodes.size() && (id > 0));

		//if (!expr) throw logic_error("Invalid tree node id");

		return expr;
	}

	bool getNode(NodeId id, Node& node) const
	{
		if (!validateId(id))
			return false;

		node = m_nodes.at((size_t)id - 1);

		return true;
	}

	bool setNode(NodeId id, Node node)
	{
		if (!validateId(id))
			return false;

		m_nodes.at((size_t)id - 1) = node;

		return true;
	}

public:


	NodeId allocNode(const value_t& value)
	{
		m_nodes.push_back(Node(value));
		return (NodeId)(m_nodes.size());
	}

	NodeId allocNode(value_t&& value)
	{
		m_nodes.push_back(Node(value));
		return (NodeId)(m_nodes.size());
	}

	void linkNodeLeft(NodeId parentid, NodeId leftnodeid)
	{
		Node parent;
		getNode(parentid, parent);
		parent.left = leftnodeid;
		setNode(parentid, parent);
	}

	void linkNodeRight(NodeId parentid, NodeId rightnodeid)
	{
		Node parent;
		getNode(parentid, parent);
		parent.right = rightnodeid;
		setNode(parentid, parent);
	}

	void linkNode(NodeId parentid, NodeId childid, bool isright)
	{
		if (isright)
			this->linkNodeRight(parentid, childid);
		else
			this->linkNodeLeft(parentid, childid);
	}

	//Returns true if a node has no children
	bool isNodeLeaf(NodeId node) const
	{
		Node n;

		if (getNode(node, n))
			return (!n.left && !n.right);
		else
			return false;
	}

	//Returns true if a node has at least one child
	bool isNodeBranch(NodeId node) const
	{
		return !isNodeLeaf(node);
	}

	//Returns true if the node exists
	bool isNode(NodeId node) const
	{
		return validateId(node);
	}

	NodeId getChildNodeLeft(NodeId id) const
	{
		Node n;
		getNode(id, n);
		return n.left;
	}

	NodeId getChildNodeRight(NodeId id) const
	{
		Node n;
		getNode(id, n);
		return n.right;
	}

	NodeId getChildNode(NodeId id, bool isright) const
	{
		if (isright)
			return this->getChildNodeRight(id);
		else
			return this->getChildNodeLeft(id);
	}

	bool getNodeValue(NodeId id, value_t& value) const
	{
		Node n;

		if (!this->getNode(id, n))
			return false;

		value = n.value;
		return true;
	}
};
