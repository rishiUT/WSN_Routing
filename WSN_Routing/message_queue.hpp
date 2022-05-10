#pragma once
#include "message.hpp"
#include <deque>
namespace DC
{
	class MessageQueue
	{
	public:
		MessageQueue();
		void push(MessagePtr msg);
		void priority_push(MessagePtr msg);
		MessagePtr pop(int curr_time);
		bool empty(int curr_time);
		bool contains(MessagePtr msg);
		bool remove(MessagePtr msg);
	private:
		std::deque<MessagePtr> msgs;
	};

	MessageQueue::MessageQueue() {
	}

	void MessageQueue::push(MessagePtr msg) {
		msgs.push_back(msg);
	}

	void MessageQueue::priority_push(MessagePtr msg) {
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

	MessagePtr MessageQueue::pop(int curr_time) {
		_ASSERT(!empty(curr_time));
		MessagePtr val = msgs.front();
		msgs.pop_front();
		return val;
	}

	bool MessageQueue::empty(int curr_time) {
		return msgs.empty() || (msgs.front()->hop_timestamp() >= curr_time);
	}

	inline bool MessageQueue::contains(MessagePtr msg)
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

	inline bool MessageQueue::remove(MessagePtr msg)
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
