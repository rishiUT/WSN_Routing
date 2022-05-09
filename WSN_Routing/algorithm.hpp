#pragma once
#include "node.hpp"
#include "message.hpp"
#include <string>
#include <map>
#include "algorithm_base.h"
#include <cassert>

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

namespace DC
{
	class Algorithm : public AlgorithmBase{
	public:
		struct node_metadata {
								node_metadata() = default;
			std::map<Node*, std::map<Node*, double>> values_;
		};

		struct Signature {
			Node* sender_ = nullptr;
			int     timestamp_ = 0;

			Signature() = default;
			Signature(Signature const& other) = default;
			Signature& operator=(Signature const& other) = default;
			Signature(Signature&& other) = default;
			Signature& operator=(Signature&& other) = default;

			Signature(Node* sender, int timestamp) : sender_{ sender }, timestamp_{ timestamp }{}
		};

		struct msg_metadata {
	        using				SignatureVect = std::vector<Signature>;

	        bool				arrived_ = false;
	        SignatureVect		travelLog_;
			void				push_signature(Signature&& signature);
			Signature			pop_signature() { Signature res = travelLog_.back(); travelLog_.pop_back(); return res; }
			Signature			peek_signature() { return travelLog_.back(); }
			int					hop_count_back_ = 0;
		};


		inline void				on_message_init(MessagePtr msg) override;
		inline void				on_node_init(Node* self) override;
		inline void				on_neighbor_added(Node* self, Node* neighbor) override;

		inline void				operator()(Node* self, MessagePtr sensor_data) override;
		inline void				on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations) override {}
	private:
	    void					update_values(Node* self, Node* destination, Node* neighbor, int distance, int time);
	    inline static Node*		choose_recipient(Node* self, Node* destination);
	};

	inline void Algorithm::operator()(Node* self, MessagePtr sensor_data=nullptr) {
		//if sensor_data != null, push it as a priority message to the outbox once the recipient is chosen
	    if (sensor_data != nullptr) {
	        //Handle this message
			MessagePtr msg = sensor_data;
			Node* dst = msg->destination();
			if (dst != nullptr) {
				//This message needs to be forwarded
				//Find the neighbor most likely to be closest to the destination
				Node* best_n = choose_recipient(self, dst);

				msg->set_hop_source(self);
				msg->set_hop_destination(best_n);
				msg->ext_data<msg_metadata>()->push_signature(Signature(self->id(), self->now()));
				self->push_outbox(msg);
			}
	    }
	    else if (self->inbox_pending()) {
	        MessagePtr msg = self->pop_inbox();
	        self->add_neighbor(*(msg->hop_source()));
	        Node* dst = msg->destination();
	        if (dst != nullptr && dst != self->id()) {
	            //This message needs to be forwarded
	            if (msg->arrived()) {
	                //This message has reached its destination and is now an acknowledgement
	                int distance = msg->ext_data<msg_metadata>()->hop_count_back_ + 1; //This is the number of times the message was forwarded before it arrived at the destination
	                msg->ext_data<msg_metadata>()->hop_count_back_ = distance; //update the hop count
					Signature sig = msg->ext_data<msg_metadata>()->pop_signature();
	                int time_to_dst = msg->arrival_time() - sig.timestamp_; //This is the number of clock ticks it took to arrive
					update_values(self, msg->destination(), msg->hop_source(), distance, time_to_dst);
	                // Were we the sender? If not, forward it back again
	                if (msg->source() != self->id()) {
	                    Node* previous = msg->ext_data<msg_metadata>()->peek_signature().sender_;
						msg->set_hop_destination(previous);
	                    self->push_outbox(msg);
	                }
	            }
	            else {
	                //We still want to get closer to the destination
	                Node* recvr = choose_recipient(self, dst);
					msg->ext_data<msg_metadata>()->push_signature(Signature(self->id(), self->now()));
					msg->set_hop_destination(recvr);
					self->push_outbox(msg);
	            }
	        }
	        else {
	            //This is for us! Read the message, then send it back so the sender knows it was received (and how long it took to get here)
	            const auto ext_data = msg->ext_data<msg_metadata>();
				ext_data->push_signature(Signature(self->id(), self->now()));

	            self->read_msg(msg); // This is where you'd normally do something with the data

	            msg->set_arrival_time(self->now());

	        	Signature sig = ext_data->pop_signature();
	            Node* previous = sig.sender_;

	        	msg->set_hop_destination(previous);
				self->push_outbox(msg);
	        }
	    }
	}

	inline void Algorithm::update_values(Node* self, Node* destination, Node* neighbor, int distance, int time)
	{
		const auto ext_data = self->ext_data<node_metadata>();
		std::map<Node*, double> paths = ext_data->values_[destination];
		paths[neighbor] += 10; //Undo the value edit we made when the message was sent

		const double update_val = -1 * (distance + time);
		const double prev_val = paths[neighbor];
		paths[neighbor] = (prev_val * 0.9) + (update_val * 0.1); //Make a minor update to the expected value
	}

	inline Node* Algorithm::choose_recipient(Node* self, Node* destination) {
		const auto ext_data = self->ext_data<node_metadata>();

		assert(self->neighbors().size() != 0);

		//If one of your neighbors is the destination, send the message to that neighbor
		for (auto& neighbor : self->neighbors())
		{
			if (neighbor->label() == destination->label())
			{
				return neighbor;
			}
		}

		std::map<Node*, double> dest_paths = ext_data->values_[destination];
		Node* best_path = nullptr;
		double best_val = 1;
		for (auto&& n : dest_paths)
		{
			if (best_val == 1 || n.second > best_val)
			{
				best_val = n.second;
				best_path = n.first;
			}
		}
		bool explore = false; //Make this random; if it's true, we'll take an alternate path to see if it's better
								//We might also need to take the best path to make sure the message arrives though
		if (explore)
		{
			//TODO: best_path = random neighbor
			int val = std::rand();
			val %= self->neighbors().size();
			best_path = self->neighbors()[val];
		}

		dest_paths[best_path] -= 10; // Mildly discourage the use of this path until it returns, to prevent overfilling and in case the node went down
		return best_path;
	}

	inline void Algorithm::msg_metadata::push_signature(Signature&& signature)
	{
		travelLog_.push_back(signature);
	}

	inline void Algorithm::on_message_init(MessagePtr msg)
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
			ext_data->values_[d][neighbor] = 0; //-1 = "no value known, no route found, etc."
		}
	}
}
