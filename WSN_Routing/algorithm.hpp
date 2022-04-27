#pragma once
#include "node.hpp"
#include "message.hpp"
#include <string>
#include <map>
#include "algorithm_base.h"

namespace DC
{
	class Algorithm : public AlgorithmBase{
	public:
		struct node_metadata {
								node_metadata() = default;
			std::map<Node*, std::map<Node*, double>> values_;
		};

		struct msg_metadata {
	        using				Signature = Message::Signature;
	        using				SignatureVect = std::vector<Signature>;

	        bool				arrived_ = false;
	        SignatureVect		travelLog_;
			void				push_signature(Signature&& signature);
			Signature			pop_signature() { Signature res = travelLog_.back(); travelLog_.pop_back(); return res; }
			Signature			peek_signature() { return travelLog_.back(); }

		};

		inline void				on_message_init(Message* msg) override;
		inline void				on_node_init(Node* self) override;
		inline void				on_neighbor_added(Node* self, Node* neighbor) override;

		inline void				operator()(Node* self, Message* sensor_data) override;
	private:
	    void					update_values(Node* self, Node* destination, Node* neighbor, int distance, int time);
	    inline static Node*		choose_recipient(Node* self, Node* destination);
	};

	inline void Algorithm::operator()(Node* self, Message* sensor_data=nullptr) {
		//if sensor_data != null, push it as a priority message to the outbox once the recipient is chosen
	    if (sensor_data != nullptr) {
	        //send_sensor_data("This is Data!");
	        //Handle this message
			Message* msg = sensor_data;
			Node* dst = msg->destination();
			if (dst != nullptr) {
				//This message needs to be forwarded
				//Find the neighbor most likely to be closest to the destination
				Node* best_n = choose_recipient(self, dst);

				msg->set_hop_source(self);
				msg->set_hop_destination(best_n);
				msg->ext_data<msg_metadata>()->push_signature(Message::Signature(self->id(), self->now()));
				self->push_outbox(*msg);
			}
	    }
	    else if (self->inbox_pending()) {
	        Message* msg = self->pop_inbox();
	        self->add_neighbor(*(msg->hop_source()));
	        Node* dst = msg->destination();
	        if (dst != nullptr && dst != self->id()) {
	            //This message needs to be forwarded
	            if (msg->arrived()) {
	                //This message has reached its destination and is now an acknowledgement
	                int distance = msg->hop_count(); //This is the number of times the message was forwarded before it arrived at the destination
					Message::Signature sig = msg->ext_data<msg_metadata>()->pop_signature();
	                int time_to_dst = msg->arrival_time() - sig.timestamp_; //This is the number of clock ticks it took to arrive
					update_values(self, msg->destination(), msg->hop_source(), distance, time_to_dst);
	                // Were we the sender? If not, forward it back again
	                if (msg->source() != self->id()) {
	                    Node* previous = msg->ext_data<msg_metadata>()->peek_signature().sender_;
						msg->set_hop_destination(previous);
	                    self->push_outbox(*msg);
	                }
	            }
	            else {
	                //We still want to get closer to the destination
	                Node* recvr = choose_recipient(self, dst);
					msg->ext_data<msg_metadata>()->push_signature(Message::Signature(self->id(), self->now()));
					msg->set_hop_destination(recvr);
					self->push_outbox(*msg);
	            }
	        }
	        else {
	            //This is for us! Read the message, then send it back so the sender knows it was received (and how long it took to get here)
	            const auto ext_data = msg->ext_data<msg_metadata>();
				ext_data->push_signature(Message::Signature(self->id(), self->now()));

	            msg->contents(); // This is where you'd normally do something with the data
				self->read_message();

	            msg->set_arrival_time(self->now());

	        	Message::Signature sig = ext_data->pop_signature();
	            Node* previous = sig.sender_;

	        	msg->set_hop_destination(previous);
				self->push_outbox(*msg);
	        }
	    }
	}

	inline void Algorithm::update_values(Node* self, Node* destination, Node* neighbor, int distance, int time)
	{
		const double update_val = -1 * (distance + time);
		const auto ext_data = self->ext_data<node_metadata>();

		std::map<Node*, double> paths = ext_data->values_[destination];
		const double prev_val = paths[neighbor];
		paths[neighbor] = (prev_val * 0.9) + (update_val * 0.1); //Make a minor update to the expected value
	}

	inline Node* Algorithm::choose_recipient(Node* self, Node* destination) {
		const auto ext_data = self->ext_data<node_metadata>();

		const std::map<Node*, double> dest_paths = ext_data->values_[destination];
		Node* best_path = nullptr;
		double best_val = 0;
		for (auto&& n : dest_paths)
		{
			if (best_val == 0 || n.second > best_val)
			{
				best_val = n.second;
				best_path = n.first;
			}
		}
		bool explore = false; //Make this random; if it's true, we'll take an alternate path to see if it's better
								//We might also need to take the best path to make sure the message arrives though
		return best_path;
	}

	inline void Algorithm::msg_metadata::push_signature(Signature&& signature)
	{
		travelLog_.push_back(signature);
	}

	inline void Algorithm::on_message_init(Message* msg)
	{
		auto ext_data = new msg_metadata();
		msg->set_ext_data(ext_data);
	}

	inline void Algorithm::on_node_init(Node* self)
	{
		auto ext_data = new node_metadata();
		for (Node* d : self->destinations())
		{
			for (Node* n : self->neighbors())
			{
				ext_data->values_[d][n] = 0; //0 = "no value known, no route found, etc."
			}
		}
		
		self->set_ext_data(ext_data);
	}

	inline void Algorithm::on_neighbor_added(Node* self, Node* neighbor)
	{
		//TODO: Implement this
		// If there is any other neighbor-related metadata, add it. The actual adding to the neighbor list is already done
		const auto ext_data = self->ext_data<node_metadata>();
		for (Node* d : self->destinations())
		{
			ext_data->values_[d][neighbor] = -1; //-1 = "no value known, no route found, etc."
		}
	}
}
