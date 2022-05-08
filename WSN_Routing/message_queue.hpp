#pragma once
#include "message.hpp"
#include <deque>
namespace DC
{
	class MessageQueue
	{
	public:
		MessageQueue();
		void push(Message* msg);
		void priority_push(Message* msg);
		Message* pop();
		bool empty();
		bool contains(Message* msg);
		bool remove(Message* msg);
	private:
		std::deque<Message*> msgs;
	};

	MessageQueue::MessageQueue() {
	}

	void MessageQueue::push(Message* msg) {
		msgs.push_back(msg);
	}

	void MessageQueue::priority_push(Message* msg) {
		//msg->set_priority(true); //If this message doesn't already have priority, it better have priority now
		assert(msg->priority());
		auto it = msgs.cbegin();
		while (it != msgs.cend() && (*it)->priority())
		{
			++it;
		}

		if (it != msgs.cend())
		{
			//We found a non-priority message, add this one in front of it
			msgs.insert(it, msg);
		}
		else
		{
			//They are all priority messages, so put it at the end
			msgs.push_back(msg);
		}
	}

	Message* MessageQueue::pop() {
		_ASSERT(!empty());
		Message* val = msgs.front();
		msgs.pop_front();
		return val;
	}

	bool MessageQueue::empty() {
		return msgs.empty();
	}

	inline bool MessageQueue::contains(Message* msg)
	{
		auto it = msgs.cbegin();
		while (it != msgs.end() && (*it)->get_id() != msg->get_id())
		{
			++it;
		}

		if (it != msgs.cend())
		{
			return true;
		}

		return false;
	}

	inline bool MessageQueue::remove(Message* msg)
	{
		//Removes the messaage if it is in the queue
		//Returns true if the message was there, false otherwise
		auto it = msgs.cbegin();
		while (it != msgs.cend() && (*it)->get_id() != msg->get_id())
		{
			++it;
		}

		if (it != msgs.cend())
		{
			msgs.erase(it);
			return true;
		}

		return false;
	}
}
