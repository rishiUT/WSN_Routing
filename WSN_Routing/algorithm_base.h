#pragma once
namespace DC
{
    class Node;
    class Message;

    class AlgorithmBase {
    public:
						AlgorithmBase()                             = default;
		virtual 		~AlgorithmBase()                            = default;
						AlgorithmBase(AlgorithmBase const& other)   = default;
		AlgorithmBase&  operator=(AlgorithmBase const& other)       = default;
						AlgorithmBase(AlgorithmBase&& other)        = default;
		AlgorithmBase&  operator=(AlgorithmBase&& other)            = default;


        virtual void    on_message_init(MessagePtr msg) = 0;
        virtual void    on_node_init(Node* msg) = 0;
        virtual void    on_neighbor_added(Node* self, Node* neighbor) = 0;
        virtual void    on_tick(std::vector<Node*> nodes, std::vector<Node*> destinations) = 0;

    	virtual void    operator()(Node* self, MessagePtr sensor_data) = 0;
    };
}
