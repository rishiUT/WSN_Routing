#include "node.hpp"

class ActuatorNode: public Node
{
private:

public:
    bool active = false;
    void tick();
};

void ActuatorNode::tick()
{
    num_ticks_++;
    if (!inbox_.empty()) {
        inbox_msg current = inbox_.front();
        inbox_.pop();
        add_neighbor(current.sender);
        if (current.message->destination() != NULL && current.message->destination() != id_) {
            //This message is for some other actuator
            //For now, we'll assume actuators are input-only, so it won't forward the message.
        } else {
            //This is for us! Read the message, then send it back so the sender knows it was received (and how long it took to get here)
            current.message->contents(); // This is where you'd normally do something with the data
            current.message->set_arrival_time(num_ticks_);
            Node* previous = current.message->remove_signature();
            send_message(current.message, previous);
        }
    }
}