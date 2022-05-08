#pragma once
#include "algorithm_base.h"
#include "node.hpp"
#include <map>

namespace DC
{

    class AlgorithmRaser : public AlgorithmBase {
    public:
	    struct node_metadata {
	        node_metadata() = default;
	        std::map<Node*, int> hop_counts_;
	        MessageQueue temp_inbox_;
	        MessageQueue received_msgs;
	    };

	    struct msg_metadata {
	        int sender_hop_count_ = 0;
	    };
        AlgorithmRaser() = default;
        ~AlgorithmRaser() = default;
        AlgorithmRaser(AlgorithmRaser const& other) = default;
        AlgorithmRaser& operator=(AlgorithmRaser const& other) = default;
        AlgorithmRaser(AlgorithmRaser&& other) = default;
        AlgorithmRaser& operator=(AlgorithmRaser&& other) = default;


        inline void                     on_message_init(Message* msg) override;
        inline void                     on_node_init(Node* self) override;
        inline void                     on_neighbor_added(Node* self, Node* neighbor) override;
        inline void				        on_tick_end(std::vector<Node*> nodes, std::vector<Node*> destinations) override {}

        void    operator()(Node* self, Message* sensor_data) override;
    };

    inline void AlgorithmRaser::on_message_init(Message* msg)
    {
        auto ext_data = new msg_metadata();
        msg->set_ext_data(ext_data);
    }

    inline void AlgorithmRaser::on_node_init(Node* self)
    {
        auto ext_data = new node_metadata();
        for (Node* n : self->neighbors())
        {
            ext_data->hop_counts_[n] = -1; //-1 = "no known path"
        }
        self->set_ext_data(ext_data);
    }

    inline void AlgorithmRaser::on_neighbor_added(Node* self, Node* neighbor)
    {
    }

    inline void AlgorithmRaser::operator()(Node* self, Message* sensor_data)
    {
        //if sensor_data != null, push it as a priority message to the outbox once the recipient is chosen
        if (sensor_data != nullptr) {
            //Handle this message
            Message* msg = sensor_data;
            Node* dst = msg->destination();
            msg->ext_data<msg_metadata>()->sender_hop_count_ = self->ext_data<node_metadata>()->hop_counts_[dst];
            msg->set_hop_source(self);
            msg->set_hop_destination(nullptr); //This is a broadcast
            self->push_outbox(*msg);
        }
        else if (self->inbox_pending()) {
            Message* msg = self->pop_inbox();
            self->add_neighbor(*(msg->hop_source()));
            Node* dst = msg->destination();
            if (msg->ext_data<msg_metadata>()->sender_hop_count_ != -1 && (self->ext_data<node_metadata>()->hop_counts_[dst] == -1 || msg->ext_data<msg_metadata>()->sender_hop_count_ + 1 < self->ext_data<node_metadata>()->hop_counts_[dst]))
            {
                //This is either the shortest or the only path we've seen to this destination
                self->ext_data<node_metadata>()->hop_counts_[dst] = msg->ext_data<msg_metadata>()->sender_hop_count_ + 1;
            }
            if (dst != nullptr && dst != self->id()) {
                if (self->ext_data<node_metadata>()->temp_inbox_.contains(msg))
                {
                    if (msg->ext_data<msg_metadata>()->sender_hop_count_ < self->ext_data<node_metadata>()->hop_counts_[dst])
                    {
                        self->ext_data<node_metadata>()->temp_inbox_.remove(msg);
                    } else
                    {
                        // The other iteration has either the same hop count or a worse one.Either way, we're still forwarding it. Drop the one you just got though
                    }
                } else
                {
                    if (msg->ext_data<msg_metadata>()->sender_hop_count_ < self->ext_data<node_metadata>()->hop_counts_[dst] || (msg->ext_data<msg_metadata>()->sender_hop_count_ == self->ext_data<node_metadata>()->hop_counts_[dst] && !msg->priority()))
                    {
	                    // Ignore it
                    }
                    else
                    {
	                    if (msg->ext_data<msg_metadata>()->sender_hop_count_ == self->ext_data<node_metadata>()->hop_counts_[dst] && msg->priority())
	                    {
                            msg->set_priority(false);
	                    }

                        msg->set_hop_destination(nullptr);
                        if (msg->priority())
                        {
                            self->ext_data<node_metadata>()->temp_inbox_.priority_push(msg);
                        }
                    	else
                        {
                            self->ext_data<node_metadata>()->temp_inbox_.push(msg);
                        }
                    }
                }
            }
            else {
                //This is for us! Read the message, the determine if it's a duplicate.
                const auto ext_data = msg->ext_data<msg_metadata>();
                self->read_msg(*msg);
                msg->set_arrival_time(self->now());
                if (!self->ext_data<node_metadata>()->received_msgs.contains(msg))
                {
                    self->ext_data<node_metadata>()->received_msgs.push(msg);
                }
            }
        }
    }
}
