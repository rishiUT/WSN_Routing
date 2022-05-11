#pragma once
#include "algorithm_base.h"
#include "node.hpp"
#include <map>

namespace DC
{
    class AlgorithmPegasis : public AlgorithmBase {
    public:
	    struct node_metadata {
	        node_metadata() = default;
	        std::map<Node*, Node*> nearest_neighbors_left;
	        std::map<Node*, Node*> nearest_neighbors_right;
	        bool disconnected = true;
	    };

	    struct msg_metadata {
	    };

        AlgorithmPegasis()      = default;
        ~AlgorithmPegasis()     = default;
        AlgorithmPegasis(AlgorithmPegasis const& other) = default;
        AlgorithmPegasis& operator=(AlgorithmPegasis const& other) = default;
        AlgorithmPegasis(AlgorithmPegasis&& other) = default;
        AlgorithmPegasis& operator=(AlgorithmPegasis&& other) = default;

        inline void             on_message_init(MessagePtr msg) override;
        inline void             on_node_init(Node* msg) override;
	    inline void             on_neighbor_added(Node* self, Node* neighbor) override {}
        inline void				on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations) override;
        inline void             on_end(std::ostream& os) override;

        inline void             operator()(Node* self, MessagePtr sensor_data) override;

    private:
        bool has_disconnected_node(std::vector<Node*> nodes);
        inline auto get_furthest_node(std::vector<Node*> nodes, Node* destination, bool needs_disconnected = true);
	    void connected_create_chain(std::vector<Node*> nodes, Node* dest);

	    int breakCounter_ = 0;

        std::map<Node*, Node*> leaders;
        std::map<Node*, Node*> current_nodes;
        std::map<Node*, Node*> leftmost;
        std::map<Node*, Node*> rightmost;
        std::map<Node*, bool> moving_left;
        std::map<Node*, bool> second_round;

    };

    inline void AlgorithmPegasis::on_message_init(MessagePtr msg)
    {
        auto ext_data = new msg_metadata();
        msg->set_ext_data(ext_data);
    }

    inline void AlgorithmPegasis::on_node_init(Node* self)
    {
        auto ext_data = new node_metadata();
        for (Node* n : self->destinations())
        {
            ext_data->nearest_neighbors_left[n] = nullptr; //null_ptr = "no known path"
            ext_data->nearest_neighbors_right[n] = nullptr; //null_ptr = "no known path"
        }
        self->set_ext_data(ext_data);
    }

    inline bool AlgorithmPegasis::has_disconnected_node(std::vector<Node*> nodes)
    {
        for (auto node = nodes.begin(); node != nodes.end(); ++node)
        {
            if ((*node)->ext_data<node_metadata>()->disconnected)
            {
                return true;
            }
        }

        return false;
    }

    inline auto AlgorithmPegasis::get_furthest_node(std::vector<Node*> nodes, Node* destination, bool needs_disconnected)
    {
        //Gets the furthest disconnected node from the current destination
        Node* best_node = nullptr;
        int best_distance = 0;

        for (auto curr_node = nodes.begin(); curr_node != nodes.end(); ++curr_node)
        {
            auto&& nodeMetadata = *(*curr_node)->ext_data<node_metadata>();
            auto disconnected = (*curr_node)->ext_data<node_metadata>()->disconnected;
            auto distance = curr_node.operator*()->distance_to(*destination);

	        if (((*curr_node)->ext_data<node_metadata>()->disconnected || !needs_disconnected) && curr_node.operator*()->distance_to(*destination) > best_distance)
	        {
                best_node = *curr_node;
                best_distance = (*curr_node)->distance_to(*destination);
	        }
        }

        return best_node;
    }

    inline void AlgorithmPegasis::connected_create_chain(std::vector<Node*> nodes, Node* dest)
    {
        Node* furthest = get_furthest_node(nodes, dest);
        furthest->ext_data<node_metadata>()->disconnected = false;
        furthest->ext_data<node_metadata>()->nearest_neighbors_left[dest] = nullptr;
        leftmost[dest] = furthest;
        while (has_disconnected_node(nodes))
        {
            Node* next = get_furthest_node(nodes, dest);
            furthest->ext_data<node_metadata>()->nearest_neighbors_right[dest] = next;
            next->ext_data<node_metadata>()->nearest_neighbors_left[dest] = furthest;
            next->ext_data<node_metadata>()->disconnected = false;
            furthest = next;
        }

        furthest->ext_data<node_metadata>()->nearest_neighbors_right[dest] = nullptr;
        rightmost[dest] = furthest;
    }

    inline void AlgorithmPegasis::on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations)
    {
        if (breakCounter_ % 2000 == 0)
        {
	        for (auto dest = destinations.begin(); dest != destinations.end(); ++dest)
	        {
		        for (auto node = nodes.begin(); node != nodes.end(); ++node)
		        {
	                (*node)->ext_data<node_metadata>()->disconnected = ((*node)->id() != (*dest)->id());
		        }

	            //while (has_disconnected_node(nodes))
	            //{
	            //    Node* destNode = *dest;
	            //    auto furthest_node = get_furthest_node(nodes, destNode);
	            //    find_forwarding_node(furthest_node, *dest);
	            //}
                connected_create_chain(nodes, *dest);
                Node* furthest = get_furthest_node(nodes, *dest);
                leaders[*dest] = furthest;
                current_nodes[*dest] = furthest;
                moving_left[*dest] = true;
                second_round[*dest] = false;
	        }

            for (auto& node : nodes)
            {
                //A broadcast from every node simulates flooding, which is used to communicate configuration to all nodes
                std::string txt = "I'm Alive";
                MessagePtr msg{ new Message(node, nullptr, txt, breakCounter_) };
                //TODO: Comment this back in once the leader is receiving messages
            	//node->broadcast(msg);
            }
        }
        breakCounter_++;
    }


    inline void AlgorithmPegasis::operator()(Node* self, MessagePtr sensor_data)
    {
        if (sensor_data != nullptr) {
            //send_sensor_data("This is Data!");
            //Handle this message
            MessagePtr msg = sensor_data;
            Node* dst = msg->destination();
            if (dst != nullptr) {
                //This message needs to be forwarded
                //Find the neighbor most likely to be closest to the destination
                Node* best_n = self->ext_data<node_metadata>()->nearest_neighbors_left[dst];

                msg->set_hop_source(self);
                msg->set_hop_destination(best_n);
                self->push_outbox(msg);
            }
        }

        for (Node* dest : self->destinations())
        {
            if (self != dest)
		    {
		        if (current_nodes[dest] == self)
		        {
			        if (leaders[dest] != self)
			        {
		        		if (moving_left[dest])
		        		{
		        			Node* neighbor = self->ext_data<node_metadata>()->nearest_neighbors_left[dest];
		        			if (neighbor == nullptr)
		        			{
		        				neighbor = self->ext_data<node_metadata>()->nearest_neighbors_right[dest];
		        				moving_left[dest] = false;
		        			}
		        			while (self->inbox_pending())
		        			{
		        				//NOTE: Currently only supports one destination
		        				//If attempting to support multiple, add one inbox per destination to the node_metadata
		        				MessagePtr msg = self->pop_inbox();
		        				msg->set_hop_source(self);
		        				msg->set_hop_destination(neighbor);
		        				self->send_message(msg);
		        			}
		        			current_nodes[dest] = neighbor;
		        		}
			        } else
			        {
		        		bool leader_handoff = false;
		        		if (moving_left[dest])
		        		{
		        			if (second_round[dest])
		        			{
		        				leader_handoff = true;
		        			} else
		        			{
		        				Node* neighbor = self->ext_data<node_metadata>()->nearest_neighbors_left[dest];
		        				if (neighbor == nullptr)
		        				{
		        					neighbor = self->ext_data<node_metadata>()->nearest_neighbors_right[dest];
		        					moving_left[dest] = false;
		        					second_round[dest] = true;
		        				}
		        				while (self->inbox_pending())
		        				{
		        					//NOTE: Currently only supports one destination
		        					//If attempting to support multiple, add one inbox per destination to the node_metadata
		        					MessagePtr msg = self->pop_inbox();
		        					msg->set_hop_source(self);
		        					msg->set_hop_destination(neighbor);
		        					self->send_message(msg);
		        				}
		        				current_nodes[dest] = neighbor;
		        			}
		        		} else
		        		{
		        			second_round[dest] = true;
		        			Node* neighbor = self->ext_data<node_metadata>()->nearest_neighbors_right[dest];
		        			if (neighbor == nullptr)
		        			{
		        				leader_handoff = true;
		        			} else
		        			{
		        				while (self->inbox_pending())
		        				{
		        					//NOTE: Currently only supports one destination
		        					//If attempting to support multiple, add one inbox per destination to the node_metadata
		        					MessagePtr msg = self->pop_inbox();
		        					msg->set_hop_source(self);
		        					msg->set_hop_destination(neighbor);
		        					self->send_message(msg);
		        				}
		        				current_nodes[dest] = neighbor;
		        			}
		        		}
		        		if (leader_handoff)
		        		{
		        			moving_left[dest] = true;
		        			second_round[dest] = false;
		        			while (self->inbox_pending())
		        			{
		        				//NOTE: Currently only supports one destination
		        				//If attempting to support multiple, add one inbox per destination to the node_metadata
		        				MessagePtr msg = self->pop_inbox();
		        				msg->set_hop_source(self);
		        				msg->set_hop_destination(dest);
		        				self->send_message(msg);
		        			}
		        			Node* left_neighbor = self->ext_data<node_metadata>()->nearest_neighbors_left[dest];
		        			leaders[dest] = left_neighbor ? left_neighbor : rightmost[dest];
		        		}
			        }
		        }
	        } else
	        {
		        while (self->inbox_pending())
		        {
                    self->read_msg(self->pop_inbox());
		        }
	        }
        }
    }

    inline void AlgorithmPegasis::on_end(std::ostream& os)
    {
        logger_.print(os);
    }
}