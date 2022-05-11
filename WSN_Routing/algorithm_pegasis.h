#pragma once
#include "algorithm_base.h"
#include "node.hpp"
#include <map>

namespace DC
{
    class AlgorithmPegasis : public AlgorithmBase {
    public:
	    struct node_metadata {
	        node_metadata();
	        std::map<Node*, Node*> nearest_neighbors_left;
	        std::map<Node*, Node*> nearest_neighbors_right;
            std::map<Node*, bool> is_leader;
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
        inline auto get_closest_node(std::vector<Node*> nodes, Node* destination, bool needs_disconnected = true);
        bool has_node(std::vector<Node*> node_list, Node* node);
        inline void find_forwarding_node(Node* node, Node* dest);
	    void connected_create_chain(std::vector<Node*> nodes, Node* dest);

	    int breakCounter_ = 0;

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
            ext_data->is_leader[n] = false;
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

    inline auto AlgorithmPegasis::get_closest_node(std::vector<Node*> nodes, Node* destination, bool needs_disconnected)
    {
        //Gets the furthest disconnected node from the current destination
        Node* best_node = nullptr;
        int best_distance = INT_MAX;

        for (auto curr_node = nodes.begin(); curr_node != nodes.end(); ++curr_node)
        {
            auto&& nodeMetadata = *(*curr_node)->ext_data<node_metadata>();
            auto disconnected = (*curr_node)->ext_data<node_metadata>()->disconnected;
            auto distance = curr_node.operator*()->distance_to(*destination);

            if (((*curr_node)->ext_data<node_metadata>()->disconnected || !needs_disconnected) && curr_node.operator*()->distance_to(*destination) < best_distance)
            {
                best_node = *curr_node;
                best_distance = (*curr_node)->distance_to(*destination);
            }
        }

        return best_node;
    }

    bool AlgorithmPegasis::has_node(std::vector<Node*> node_list, Node* node)
    {
	    for (auto& other : node_list)
	    {
		    if (other->id() == node->id())
		    {
                return true;
		    }
	    }

        return false;
    }

    inline void AlgorithmPegasis::find_forwarding_node(Node* node, Node* dest)
    {
        Node* best_option = nullptr;
        node->ext_data<node_metadata>()->disconnected = false; //This node is about to be connected

	    if (has_node(node->neighbors(), dest))
	    {
            //The destination is one of your neighbors, forward the message directly
            best_option = &(*dest);
	    }
	    else if (has_disconnected_node(node->neighbors()))
        {
	        //There are still nodes that aren't in a chain in your vicinity, forward the message to them
            best_option = get_closest_node(node->neighbors(), dest, true);
            //Since the new neighbor isn't in a chain, 
        }

        node->ext_data<node_metadata>()->nearest_neighbors_right[&(*dest)] = best_option;
    }

    inline void AlgorithmPegasis::connected_create_chain(std::vector<Node*> nodes, Node* dest)
    {
	    Node* furthest = get_furthest_node(nodes, dest);
        furthest->ext_data<node_metadata>()->disconnected = false;
        furthest->ext_data<node_metadata>()->nearest_neighbors_left[dest] = nullptr;
        while (has_disconnected_node(nodes))
        {
            Node* next = get_furthest_node(nodes, dest);
            furthest->ext_data<node_metadata>()->nearest_neighbors_right[dest] = next;
            next->ext_data<node_metadata>()->nearest_neighbors_left[dest] = furthest;
            next->ext_data<node_metadata>()->disconnected = false;
            furthest = next;
        }

        furthest->ext_data<node_metadata>()->nearest_neighbors_right[dest] = nullptr;
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
	        }

            for (auto& node : nodes)
            {
                //A broadcast from every node simulates flooding, which is used to determine which nodes are still alive
                std::string txt = "I'm Alive";
                MessagePtr msg{ new Message(node, nullptr, txt, breakCounter_) };
                node->broadcast(msg);
            }
        }

        if (breakCounter_ % nodes.size() == 0)
        {
	        //It's the beginning of a round; select a leader node
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
                Node* best_n = self->ext_data<node_metadata>()->nearest_neighbors[dst];

                msg->set_hop_source(self);
                msg->set_hop_destination(best_n);
                self->push_outbox(msg);
            }
        }
        else if (self->inbox_pending()) {
            MessagePtr msg = self->pop_inbox();
            Node* dst = msg->destination();
            if (dst != nullptr && dst->label() != self->label()) {
                if (self->label() == 1)
                {
                    std::cout << dst->label() << "\n";
                }
                //This message needs to be forwarded
                //Find the neighbor most likely to be closest to the destination
                Node* best_n = self->ext_data<node_metadata>()->nearest_neighbors[dst];

                msg->set_hop_source(self);
                msg->set_hop_destination(best_n);
                self->push_outbox(msg);
            }
            else {
                //This is for us! If it isn't an alive message, read the message, then do nothing.
                if (dst != nullptr) { self->read_msg(msg); }
                msg->set_arrival_time(self->now());
            }
        }
    }

    inline void AlgorithmPegasis::on_end(std::ostream& os)
    {
        logger_.print(os);
    }
}



/*
 *
 * Algorithm has
 *      a reset_graph function that is called by the environment on occasion (comparable to the sink node re-evaluating the map and sending it to all nodes in the graph)
 *          Actually this is on_tick
 *          This function takes in a list of nodes which all have node_lists of neighbors
 *          For each destination:
 *              Mark all nodes as "disconnected"
 *              While there is a disconnected node:
 *                  Find the node with the greatest physical distance from the destination
 *                  For that node, see if the destination is within messaging distance
 *                      If so, mark it as the "node to forward" for that destination
 *                  If not, find the closest disconnected node
 *                      then mark them as the "node to forward" for that destination
 *                      And repeat the "find connection" algorithm for that node
 *                  If there isn't a disconnected node within reach:
 *                      Find the nearest connected node and mark mark as the "node to forward" for that destination
 *
 *      On node init
 *          Add node_metadata
 *          Run the "find nearest disconnected? neighbor" function used in the algorithm reset call
 *      on message init
 *          nothing
 *      on neighbor added
 *          Nothing
 *      operator()
 *          Get the nearest neighbor for the necessary destination, then forward them the packet
 *
 *
 */