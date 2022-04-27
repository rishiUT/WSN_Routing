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
	private:
		std::deque<Message*> msgs;
	};

	MessageQueue::MessageQueue() {
	}

	void MessageQueue::push(Message* msg) {
		msgs.push_back(msg);
	}

	void MessageQueue::priority_push(Message* msg) {
		msgs.push_front(msg);
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
}
