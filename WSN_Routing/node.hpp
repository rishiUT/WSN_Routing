#pragma once
#include <cassert>
#include "message.hpp"
#include "message_queue.hpp"
#include <queue>
#include "algorithm_base.h"

/*
 *  NOTE: Add environment neighbor list
 *      The current neighbor list contains all neighbors, even if the node shouldn't know about it yet
 *          This should be changed for accuracy's sake, if not for the sake of this simulation
 *          This would require regular "I am alive" broadcasts to keep the node neighbor list up-to-date, though
 *      The environment neighbor list should be used for broadcasts and debugging
 *      The ordinary neighbor list will be used the way it's used right now, except the environment will never add neighbors to it
 *
 */


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

    constexpr double MSG_SEND_COST = 170;
    constexpr double MSG_RECV_COST = 50;
    constexpr double AWAKE_COST = 15;


    class Node
    {
    public:
		inline 					    Node(int label, int x, int y, bool is_actuator, bool is_active, AlgorithmBase& algo, double battery);
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
        inline void                 activate(double new_battery);
        inline void                 deactivate()                                                    { active_ = false; }
        inline void                 read_msg(Message& msg);

        //Algorithm-required functions
        inline bool                 inbox_pending()                                                 { return !inbox_.empty(); }
        inline Message*             pop_inbox()                                                     { return inbox_.pop(); }
        inline void                 push_outbox(Message& new_message)                               { outbox_.push(&new_message); }
        inline std::vector<Node*>&  neighbors()                                                     { return neighbors_; }
        inline std::vector<Node*>&  destinations()                                                  { return destinations_; }
        inline void                 set_ext_data(void* ptr)                                         { ext_data_ = std::shared_ptr<void>(ptr); }
        inline double               battery_remaining_mA() const                                    { return battery_remaining_mA_; }

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
        int                 num_destinations_{};
        int                 num_ticks_;
        double              battery_remaining_mA_;

        Node* choose_destination() const;
        Message* package_sensor_data(std::string);

        int sent_msg_count = 0;
        int recv_msg_count = 0;

        AlgorithmBase* algo_;
    };

    inline Node::Node(int label, int x, int y, bool has_sensor, bool active, AlgorithmBase& algo, double battery) :
        label_{ label }, active_{ active }, has_sensor_{ has_sensor }, num_ticks_{ 0 }, battery_remaining_mA_{battery},
        algo_{&algo}
    {
        id_ = this;
        ed.location_ = coordinates(x, y);
    }

    inline void Node::receive_message(Message& msg)
    {
        battery_remaining_mA_ -= MSG_RECV_COST;

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
        msg.increment_hop();
        recipient->receive_message(msg);
        sent_msg_count++;
        battery_remaining_mA_ -= MSG_SEND_COST;
    }

    inline void Node::broadcast(Message& msg)
    {
        msg.set_hop_source(id_);
        msg.increment_hop();
        //msg.set_hop_destination(nullptr); //Use this if the neighbor should know it's a broadcast
        for (Node* neighbor : neighbors_) {
            Message new_msg = Message(msg);
			new_msg.set_hop_destination(neighbor);
            neighbor->receive_message(new_msg);
        }
        sent_msg_count++;
        battery_remaining_mA_ -= MSG_SEND_COST;
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
        if(!active_)
        {
            return; //The node is either asleep or dead; it can't do anything
        }

        num_ticks_++;
        battery_remaining_mA_ -= AWAKE_COST; //This is the cost of listening for messages
        Message* sensor_data = nullptr;
        if (trigger_sensor) {
            //This sensor node had a sensor activation
            sensor_data = package_sensor_data("This is data! Very Important");
        }

        (* algo_)(this, sensor_data);

        if (battery_remaining_mA_ <= 0)
        {
            //The node died while receiving the message and cannot continue
            active_ = false;
            return;
        }

        if (!outbox_.empty()) {
            Message* to_send = outbox_.pop();
            if (to_send->hop_destination() == nullptr) {
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

    inline void Node::activate(double new_battery)
    {
        if (new_battery != -1.0)
        {
            battery_remaining_mA_ = new_battery;
        }

        if (battery_remaining_mA_ <= 0)
        {
            active_ = true;
        }
    }

    inline void Node::read_msg(Message& msg)
    {
        recv_msg_count++;
        std::cout << "Node " << label_ << " Received this message: " << msg.contents() << std::endl; //Read the contents
        std::cout << "Hop Count was " << msg.hop_count() << std::endl;
    }
}
