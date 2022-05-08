#pragma once
#include "node.hpp"
#include "message.hpp"
#include "algorithm_base.h"
namespace DC
{
	class AlgorithmTest : public AlgorithmBase {
	public:
		AlgorithmTest() = default;
		~AlgorithmTest() override = default;
		AlgorithmTest(AlgorithmTest const& other) = default;
		AlgorithmTest& operator=(AlgorithmTest const& other) = default;
		AlgorithmTest(AlgorithmTest&& other) = default;
		AlgorithmTest& operator=(AlgorithmTest&& other) = default;


		inline void				on_message_init(Message* msg) override {}
		inline void				on_node_init(Node* msg) override {}
		inline void				on_neighbor_added(Node* self, Node* neighbor) override {}
		inline void				on_tick_end(std::vector<Node*> nodes, std::vector<Node*> destinations) override {}

		inline void				operator()(Node* self, Message* sensor_data) override;
	};

	inline void AlgorithmTest::operator()(Node* self, Message* sensor_data)
	{
        if (sensor_data != nullptr) {
            //send_sensor_data("This is Data!");
            //Handle this message
			Message* msg = sensor_data;
			Node* dst = msg->destination();
			if (dst != nullptr) {
				//This message needs to be forwarded
				//Find the neighbor most likely to be closest to the destination
				Node* best_n = nullptr;
				for (Node* neighbor : self->neighbors())
				{
					if (best_n == nullptr || (best_n->label() > neighbor->label() && neighbor->label() > msg->destination()->label()))
					{
						best_n = neighbor;
					}
				}

				msg->set_hop_source(self);
				msg->set_hop_destination(best_n);
				self->push_outbox(*msg);
			}
        }
        else if (self->inbox_pending()) {
            Message* msg = self->pop_inbox();
            Node* dst = msg->destination();
            if (dst != nullptr && dst->label() != self->label()) {
				if (self->label() == 1)
				{
					std::cout << dst->label() << "\n";
				}
                //This message needs to be forwarded
				//Find the neighbor most likely to be closest to the destination
            	Node* best_n = nullptr;
				for (Node* neighbor : self->neighbors())
				{
					if (best_n == nullptr || (best_n->label() > neighbor->label() && neighbor->label() > msg->destination()->label()))
					{
						best_n = neighbor;
					}
				}

				msg->set_hop_source(self);
				msg->set_hop_destination(best_n);
				self->push_outbox(*msg);
            }
            else {
                //This is for us! Read the message, then do nothing.
				self->read_msg(*msg);
            }
        }
	}
}
