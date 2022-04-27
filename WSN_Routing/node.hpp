#pragma once
#include <cassert>
#include "message.hpp"
#include "message_queue.hpp"
#include <queue>
#include "algorithm_base.h"

namespace DC
{

    struct inbox_msg {
        Message* message;
        Node* sender;

        inbox_msg(Message* msg, Node* sndr) {
            sender = sndr;
            message = msg;
        }
    };

    struct coordinates {
        int x_ = 0;
    	int y_ = 0;

        coordinates() = default;
        coordinates(int x, int y) { x_ = x; y_ = y; }
    };



    struct env_data {
        coordinates location_;

        env_data() = default;
    };

    class Node
    {
    public:
		inline 					    Node(int label, int x, int y, bool is_actuator, bool is_active, AlgorithmBase& algo);
        inline                      Node(Node const& other) = delete;
        inline Node&                operator=(Node const& other) = delete;
        inline                      Node(Node&& other) = delete;
        inline Node&                operator=(Node&& other) = delete;


        inline void                 receive_message(Message& msg);
		inline void                 add_destination(Node& destination);
        inline void                 add_neighbor(Node& neighbor);
        inline void                 send_message(Message& msg);
        inline void                 broadcast(Message& msg);
        inline void                 tick(bool trigger_sensor);
        inline int                  distance_to(Node& other) const;
        inline int                  label() const                                                   { return label_; }
        inline bool                 has_sensor() const                                              { return has_sensor_; }

        //Algorithm-required functions
        inline bool                 inbox_pending()                                                  { return !inbox_.empty(); }
        inline Message*             pop_inbox()                                                      { return inbox_.pop(); }
        inline void                 push_outbox(Message& new_message)                                { outbox_.push(&new_message); }
        inline std::vector<Node*>&  neighbors()                                                      { return neighbors_; }
        inline std::vector<Node*>&  destinations()                                                   { return destinations_; }
        inline void                 read_message()                                                  { recv_msg_count++; }
        inline void                 set_ext_data(void* ptr)                                         { ext_data_ = std::shared_ptr<void>(ptr); }

        template<typename T> inline std::shared_ptr<T> ext_data()                                   { return std::static_pointer_cast<T>(ext_data_); }
        inline int now() const                                                                      { return num_ticks_; }
		inline Node* id() const                                                                     { return id_; }

    protected:
        friend class        Environment;

        std::vector<Node*>  neighbors_;
        std::vector<Node*>  destinations_;
        MessageQueue        inbox_;
        MessageQueue        outbox_;
        Node*               id_;
        int                 label_ = 0;

        bool active_         = false;
        bool has_sensor_     = false;
        env_data            ed;
        std::shared_ptr<void>   ext_data_;

    	/* data */
        int                 num_destinations_;
        int                 num_ticks_;

        Node* choose_destination() const;
        Message* package_sensor_data(std::string);

        int sent_msg_count = 0;
        int recv_msg_count = 0;

        AlgorithmBase* algo_;
    };

    inline Node::Node(int label, int x, int y, bool has_sensor, bool active, AlgorithmBase& algo) :
        label_{ label }, active_{ active }, has_sensor_{ has_sensor }, num_ticks_{ 0 }, algo_{ &algo }
    {
        id_ = this;
        ed.location_ = coordinates(x, y);
    }

    inline void Node::receive_message(Message& msg)
    {
        auto src = msg.source();
        auto dest = msg.destination();
        auto hopSrc = msg.hop_source();
        auto hopDest = msg.hop_destination();
        if(src->label() < 1 || src->label() > 10 ||
            dest->label() < 1 || dest->label() > 10 ||
            hopSrc->label() < 1 || hopSrc->label() > 10 ||
            hopDest->label() < 1 || hopDest->label() > 10
            )
        {
            std::cout << "invalid" << std::endl;
        }

	    inbox_.push(&msg);
    }

    inline void Node::add_destination(Node& destination)
    {
        destinations_.push_back(&destination);
    }

    inline void Node::add_neighbor(Node& neighbor)
    {
        //Prevent duplicate neighbors
        //TODO: Consider replacing this with a set or something that prevents duplicates
        for (Node* n : neighbors_) {
            if (&neighbor == n) { return; }
        }
        neighbors_.push_back(&neighbor);
        algo_->on_neighbor_added(this, &neighbor);
    }

    inline void Node::send_message(Message& msg)
    {
        Node* recipient = msg.hop_destination();
        msg.set_hop_source(id_);
        recipient->receive_message(msg);
        sent_msg_count++;
    }

    inline void Node::broadcast(Message& msg)
    {
        msg.set_hop_source(this);
        msg.set_hop_destination(nullptr);
        for (Node* neighbor : neighbors_) {
            send_message(msg);
        }
        sent_msg_count++;
    }

    inline Node* Node::choose_destination() const
    {
        //Make this random
        return destinations_[0];
    }

    inline Message* Node::package_sensor_data(std::string data)
    {
        Node* destination = choose_destination();
        const auto msg = new Message(this, destination, data, num_ticks_);
        algo_->on_message_init(msg);
        return msg;
    }

    inline void Node::tick(bool trigger_sensor)
    {
        assert(has_sensor_ || !trigger_sensor);

        num_ticks_++;
        Message* sensor_data = nullptr;
        if (trigger_sensor) {
            //This sensor node had a sensor activation
            sensor_data = package_sensor_data("This is data! Very Important");
        }

        (* algo_)(this, sensor_data);

        if (!outbox_.empty()) {
            Message* to_send = outbox_.pop();
            if (to_send->destination() == nullptr) {
                // This is a broadcast
                broadcast(*to_send);
            }
            else {
                //Assumption; if it's in the outbox, the algorithm has already provided a recipient
                send_message(*to_send);
            }
        }
    }
    inline int Node::distance_to(Node& other) const
    {
        int x_dist = ed.location_.x_ - other.ed.location_.x_;
        int y_dist = ed.location_.y_ - other.ed.location_.y_;
        double result = (x_dist * x_dist) + (y_dist * y_dist);
        result = std::sqrt(result);
        return static_cast<int>(result);
    }
}