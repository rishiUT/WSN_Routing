#pragma once
#include "algorithm_base.h"
#include "node.hpp"
#include <map>

namespace DC
{

    class AlgorithmRaser : public AlgorithmBase {
        using Base = AlgorithmBase;
    public:
	    struct node_metadata {
	        node_metadata() = default;
	        std::map<Node*, int> hop_counts_;
	        MessageQueue temp_inbox_;
	        MessageQueue received_msgs;
	    };

	    struct msg_metadata {
            std::map<Node*, int> sender_hop_counts_;
	    };
        AlgorithmRaser() = default;
        ~AlgorithmRaser() = default;
        AlgorithmRaser(AlgorithmRaser const& other) = default;
        AlgorithmRaser& operator=(AlgorithmRaser const& other) = default;
        AlgorithmRaser(AlgorithmRaser&& other) = default;
        AlgorithmRaser& operator=(AlgorithmRaser&& other) = default;


        inline void                     on_message_init(MessagePtr msg) override;
        inline void                     on_node_init(Node* self) override;
        inline void                     on_neighbor_added(Node* self, Node* neighbor) override;
        inline void				        on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations) override;
        inline void                     on_end(std::ostream& os) override;

        void    operator()(Node* node, MessagePtr sensor_data) override;

    private:
        int num_ticks_ = 0;
        int num_nodes_ = 0;

    };

    inline void AlgorithmRaser::on_message_init(MessagePtr msg)
    {
        auto ext_data = new msg_metadata();
        msg->set_ext_data(ext_data);
    }

    inline void AlgorithmRaser::on_node_init(Node* self)
    {
        auto ext_data = new node_metadata();
        for (Node* n : self->destinations())
        {
            ext_data->hop_counts_[n] = INT_MAX; //Initialize to worst-case scenario
            if (n->label() == self->label())
            {
	            //This is me
                ext_data->hop_counts_[n] = 0; //Initialize to worst-case scenario
            }
        }
        self->set_ext_data(ext_data);
    }

    inline void AlgorithmRaser::on_neighbor_added(Node* self, Node* neighbor)
    {
    }

    inline void AlgorithmRaser::on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations)
    {
        num_nodes_ = static_cast<int>(nodes.size());
        num_ticks_++;
    }

    inline void AlgorithmRaser::operator()(Node* node, MessagePtr sensor_data)
    {
        //if sensor_data != null, push it as a priority message to the outbox once the recipient is chosen
        if (sensor_data != nullptr) {
            //Handle this message
            MessagePtr msg = sensor_data;
            Node* dst = msg->destination();
            for (auto& dest : node->destinations())
            {
                msg->ext_data<msg_metadata>()->sender_hop_counts_[dest] = node->ext_data<node_metadata>()->hop_counts_[dest];
            }
            msg->set_hop_source(node);
            msg->set_hop_destination(nullptr); //This is a broadcast
            node->push_outbox(msg);
        }
        else if (node->inbox_pending()) {
            MessagePtr msg = node->pop_inbox();
            node->add_neighbor(*(msg->hop_source()));
            Node* dst = msg->destination();

            node_metadata node_mtdt = *node->ext_data<node_metadata>();
            node_metadata sender_mtdt = *msg->hop_source()->ext_data<node_metadata>();
            msg_metadata msg_mtdt = *msg->ext_data<msg_metadata>();

            for (auto& dest : node->destinations())
            {
                int sender_hop_count = msg->ext_data<msg_metadata>()->sender_hop_counts_[dest];
                int receiver_hop_count = node->ext_data<node_metadata>()->hop_counts_[dest];
                if (sender_hop_count != INT_MAX && (receiver_hop_count == INT_MAX || sender_hop_count + 1 < receiver_hop_count))
	            {
	                //This is either the shortest or the only path we've seen to this destination
	                node->ext_data<node_metadata>()->hop_counts_[dest] = sender_hop_count + 1;
	            }
            }
            
            if (dst != nullptr && dst != node->id()) {
                if (node->ext_data<node_metadata>()->temp_inbox_.contains(msg))
                {
                    if (msg->ext_data<msg_metadata>()->sender_hop_counts_[dst] < node->ext_data<node_metadata>()->hop_counts_[dst])
                    {
                        node->ext_data<node_metadata>()->temp_inbox_.remove(msg);
                    } else
                    {
                        // The other iteration has either the same hop count or a worse one.Either way, we're still forwarding it. Drop the one you just got though
                    }
                } else
                {
                    int sender_hop_count = msg->ext_data<msg_metadata>()->sender_hop_counts_[dst];
                    int receiver_hop_count = node->ext_data<node_metadata>()->hop_counts_[dst];
                    if (sender_hop_count < receiver_hop_count || (sender_hop_count == receiver_hop_count && !msg->priority()))
                    {
	                    // Ignore it
                    }
                    else
                    {
	                    if (sender_hop_count == receiver_hop_count && msg->priority())
	                    {
                            msg->set_priority(false);
	                    }

                        msg->set_hop_destination(nullptr);
                        if (msg->priority())
                        {
                            node->ext_data<node_metadata>()->temp_inbox_.priority_push(msg);
                        }
                    	else
                        {
                            node->ext_data<node_metadata>()->temp_inbox_.push(msg);
                        }
                    }
                }
            }
            else {
                //This is for us! Read the message, and determine if it's a duplicate.
                const auto ext_data = msg->ext_data<msg_metadata>();
                if (dst != nullptr && !node->ext_data<node_metadata>()->received_msgs.contains(msg))
                {
                    node->read_msg(msg);
					node->ext_data<node_metadata>()->received_msgs.push(msg);
                }
            }
        }
        if (num_ticks_ % num_nodes_ == node->label() - 1)
        {
            if (!node->ext_data<node_metadata>()->temp_inbox_.empty(node->now()))
            {
                node->push_outbox(node->ext_data<node_metadata>()->temp_inbox_.pop(node->now()));
            } else
            {
                std::string content = "Alive";
                MessagePtr to_send{ new Message{ node, nullptr, content, node->now() } };
                on_message_init(to_send);
                for (auto& dest : node->destinations())
                {
                    to_send->ext_data<msg_metadata>()->sender_hop_counts_[dest] = node->ext_data<node_metadata>()->hop_counts_[dest];
                }
                node->push_outbox(to_send);
            }
        }
    }

	inline void AlgorithmRaser::on_end(std::ostream & os)
    {
        logger_.print(os);
    }
}