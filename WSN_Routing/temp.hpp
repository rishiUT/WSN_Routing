/*
Node either creates or receives a message
Sends message to message handling
checks if it is the destination using get_destination
if it is:
    read the message (skip for now, because the contents aren't valuable)
    call arrived to mark it as having reached the destination
    remove the last signature to know which node sent it
    if that node isn't in your neighbors:
        Add that node to neighbors, because obviously they're a neighbor
    Send that node a response (this message, which has reached the destination)
if it isn't, check if this node has reached its destination using reached_destination. If so:
    If distance information is important, use get_distance to determine the number of forwards to the destination
    call get_time to get the time between this node and the destination given the chosen path
    if this node isn't the sender (use get_sender):
        remove_signature to get the next node to send it to, then forward the message
    else:
        Do nothing? Can't think of anything
else:
    Choose how to decide which node to send it to using some algorithm
    add_signature (and timestamp)
    send message to selected node
*/

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


/*
All nodes track:
    Incoming messages
    Outgoing Messages
    coordinates
    neighbors
    destinations
    ID
    current time (clock? num_ticks?)
*/

/*
The node's tick function will first check if it has sensed data
If it has,
    It will create a message with the sensor data as content, and will select one destination as the message's destination
    It will priority_push the message to the front of the inbox
Once that is resolved, it will call select_action on the algorithm it is using
It will then check the outbox to see if there are any pending messages
If there are,
    It will send one message; if it has a destination, it will send it to that destination
    If the destination is null, it will send the message to all neighbors (broadcast)
*/
#pragma once
