#pragma once
#include "node.hpp"
#include <map>
#include <list>

namespace DC
{
	class MsgPendingList
	{
	public:
		struct pending_metadata
		{
			MessagePtr message_id_ = nullptr;
			Node* sender_ = nullptr;
			int sent_time_ = 0;
			int timeout_ = 0;

			pending_metadata() = default;
			pending_metadata(MessagePtr id, Node* sender, int sent_time, int timeout) :
				message_id_(id), sender_(sender), sent_time_(sent_time), timeout_(timeout) {}
		};

		MsgPendingList() = default;
		~MsgPendingList() = default;

		void push(MessagePtr msg, Node* sender, int sent_time, int timeout);
		void push(pending_metadata to_add);

		bool is_empty() const;
		pending_metadata peek();
		pending_metadata pop();
		pending_metadata remove(MessagePtr id);
	private:
		std::list<pending_metadata> pending_;
		std::map<MessagePtr, std::list<pending_metadata>::iterator> easy_access_;
	};

	inline void MsgPendingList::push(MessagePtr msg, Node* sender, int sent_time, int timeout)
	{
		push(pending_metadata(msg, sender, sent_time, timeout));
	}

	inline void MsgPendingList::push(pending_metadata to_add)
	{
		auto it = pending_.cbegin();
		while (it != pending_.cend() && (*it).timeout_ <= to_add.timeout_)
		{
			++it;
		}
		pending_.insert(it, to_add);
		//easy_access_[to_add.message_id_] = to_add;

	}

	inline bool MsgPendingList::is_empty() const
	{
		return pending_.empty()
	}

	inline MsgPendingList::pending_metadata MsgPendingList::peek()
	{
		return pending_.front();
	}

	inline MsgPendingList::pending_metadata MsgPendingList::pop()
	{
		pending_metadata result = pending_.front();
		pending_.pop_front();
		return result;
	}

	inline MsgPendingList::pending_metadata MsgPendingList::remove(MessagePtr id)
	{
		auto it = pending_.cbegin();
		while (it != pending_.cend() && (*it).message_id_ != id)
		{
			++it;
		}
		if (it == pending_.cend())
		{
			return nullptr;
		} else
		{
			pending_metadata result = *it;
			pending_.remove(result);
			return result;
		}
	}

	/*
	 *	NOTE:
	 *	Replace signatures with something in the Node Metadata
	 *		Create a datastructure that
	 *			Stores structs containing the "message sender" (null if we sent it), the "send time", and the "timeout time"
	 *			When elements are added, sorts them by timeout time
	 *			Allows for a peek and a pop, to check for timeouts and remove timed-out message receipts
	 *			Allows for a message check, where
	 *				an incoming message is checked against all elements
	 *				if the message is there,
	 *					Remove the message receipt from the list
	 *					return that message receipt
	 *				else return nullptr
	 *					The message must have timed out already
	 *			Note that the timeout functionality doesn't have to be used; in this situation, messages always wait in the queue, and the returning messages will always be found
	 *				If the timeout functionality is used, dead neighbors can be marked
	 */
}
