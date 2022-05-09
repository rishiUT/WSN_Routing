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
//RASER
/*First, check if the message has already arrived;
    if so, and the new iteration has a lower hop count than you,
        drop both iterations
    else
        The other iteration has either the same hop count or a worse one. Either way, we're still forwarding it. Drop the one you just got though
If you haven't seen it before,
	If it has a lower hop count, or has the same hop count and isn't priority,
		Ignore it
	Else
		If it has the same hop count and is priority
			then remove the priority
		mark it as a broadcast (hop_destination = nullptr)
		put it in the temp outbox
You're done with the inbox for this turn. If self.num_ticks is a multiple of your ID number - 1, it's your turn to send a message
	if there is a message in the purgatory box,
		pop the first message from the purgatory box to send
	else
		create a blank message, add your hop count, set hop destination and destination to nullptr, put that in the outbox
		
*/

/*
 * Notes to self:
 * Change broadcast to send several copies of the message instead of the pointer
 * Add a "total hop count" to each message, then increment that in both node send functions
 */


 /*
  *
  * Algorithm has
  *      A graph object storing a graph of all nodes, where all nodes have one path leading to the leader
  *          This is a global variable
  *          There are actually several, one for each destination
  *      a reset_graph function that is called by the environment on occasion (comparable to the sink node re-evaluating the map and sending it to all nodes in the graph)
  *          Actually this is on_tick_end
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

/*
 *  TODO:
 *  Add randomization for destinations - done
 *  Pegasis:
 *      Complete! I think
 *  RASeR:
 *      Complete! Probably
 *  My Algorithm:
 *      Add some randomness to the best_path picker - done
 *      Eventually refactor the signatures out of the nodes
 *
 *  Add more functions to get the data-collecting information out of the nodes and messages
 *  Add functions to handle the data, then to print it to a file
 *      Print to CSV, so I can do excel stuff and maybe python stuff
 *
 *  Make sure all the agents are checking if their neighbors are still active
 *      Active list?
 *      Add a physical neighbor list to the node class which is exclusively updated by the environment
 *
 *
 *  Make sure the environment is taking inactive neighbors off the neighbor lists
 *      RASeR doesn't know anything about the neighbors, it only uses broadcasts
 *      PEGASIS is receiving updated neighbors lists from the environment, which equates to the reverse flooding to determine who is alive
 *      My algorithm does not update the neighbor lists or handle failures... yet
 */

/*
 *  Print Messages
 *      Add a message print function
 *          Prints the ID, the hop count, the start time, and the end time
 *      Add an algorithm-specific on_message_print
 *          Print the message metadata, whatever that looks like
 *
 *  Unrandomize the nodes
 *  Add congestion by changing node message frequencies
 *
 *
 */