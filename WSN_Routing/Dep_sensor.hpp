#include "node.hpp"
#include <map>

class SensorNode: public Node
{
private:
    std::map<Node*, std::map<Node*, double>> values;

    Node* choose_destination();
    Node* choose_neighbor(Node*);
public:
    SensorNode(std::vector<Node*>, int, int);
    void add_neighbor(Node*);
    void forward_data(Node*, MessagePtr);
    void send_sensor_data(std::string);
    void update_values(Node*, Node*, int, int);
    void tick();
};

SensorNode::SensorNode(std::vector<Node*> destinations, int x, int y) {
    for (Node* destination : destinations) {
        values[destination] = std::map<Node*, double>();
    }
}

void SensorNode::add_neighbor(Node* neighbor) 
{
    //Prevent duplicate neighbors
    //TODO: Consider replacing this with a set or something that prevents duplicates
    for (Node* n : neighbors_) {
        if (neighbor == n) { return; }
    }
    neighbors_.push_back(neighbor);
    for (const auto& [key, value] : values) {
        value[neighbor] = -1; //-1 means we haven't explored this path to this destination. If we explore the path and don't arrive at the destination, let's store a -2.
    }
}

bool found_data() {
    //TODO: Make this random; when it is true, the sensor generates data
    return false;
}

void SensorNode::update_values(Node* destination, Node* neighbor, int distance, int time) {
    //Calculate a value based on distance and time
    int value = distance + time;
    values[destination][neighbor] *= 0.9;
    values[destination][neighbor] += 0.1 * value;
}


void SensorNode::tick() {
    num_ticks_++;
    if (found_data()) {
        send_sensor_data("This is Data!");
    } else if (!inbox_.empty()) {
        inbox_msg current = inbox_.front();
        inbox_.pop();
        add_neighbor(current.sender);
        MessagePtr msg = current.message;
        Node* dst = msg->destination();
        if (dst != NULL) {
            //This message needs to be forwarded
            if (msg->arrived()) {
                //This message has reached its destination and is now an acknowledgement
                int distance = msg->hop_count(); //This is the number of times the message was forwarded before it arrived at the destination
                int time = msg->travel_time(); //This is the number of clock ticks it took to arrive
                // Were we the sender? If not, forward it back again
                if (msg->source() != id_) {
                    Node* previous = msg->remove_signature();
                    send_message(msg, previous);
                }
            } else {
                //We still want to get closer to the destination
                forward_data(msg->destination(), msg);
            }
        } else {
            //This is for us! Read the message, then send it back so the sender knows it was received (and how long it took to get here)
            msg->contents(); // This is where you'd normally do something with the data
            msg->set_arrival_time(num_ticks_);
            Message::Signature sig = msg->pop_signature();
            Node* previous = sig.sender_;
            send_message(msg, previous);
        }
    }

}

void SensorNode::send_sensor_data(std::string data)
{
    Node* destination = choose_destination();
    MessagePtr msg = new Message(id_, destination, data, num_ticks_);
    forward_data(destination, msg);
}

Node* SensorNode::choose_destination() {
    //TODO: Make this random
    return destinations_[0];
}

Node* SensorNode::choose_neighbor(Node* destination) {
    //TODO: Choose one neighbor to send the message through based on whatever algorithm
    // 
    return NULL;
}

void SensorNode::forward_data(Node* destination, MessagePtr msg) {
    Node* intermediary = choose_neighbor(destination);
    msg->add_signature(id_, num_ticks_);
    send_message(msg, intermediary);
}

/*
The bandit version of the nodes should make decisions based on the values of each "arm" it can take
This means it needs values for each neighbor given the destination it is trying to reach
To find the values of all neighbors given a destination, I can use a map of destinations -> (data structure of values)
To find the value of each neighbor, I can use a map of neighbors -> values
*/

/*
What should I do about lost messages?
They should provide a bad value for the neighbor that lost the message, but how long can I wait before giving up?
How should I keep track of which messages have returned and which haven't?
*/