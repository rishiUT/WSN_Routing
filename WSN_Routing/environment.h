#pragma once
#include<vector>
#include<fstream>
#include<iostream>
#include "node.hpp"
#include "algorithm_base.h"

namespace DC
{
	class Environment
	{
		//static constexpr int comm_range = 4;
		using NodeUnqPtr		= std::unique_ptr<Node>;
		using NodeVector		= std::vector<NodeUnqPtr>;
	public:
		inline					Environment(AlgorithmBase& algorithm, int node_distance, int x_dim, int y_dim, int actuator_count, int comm_range, int sensor_period, std::string file_name);
		int						get_sensory_probability(int x, int y);
		int						get_sensor_period(int x, int y);
		void					run_timesteps(int update_timeframe, int loop_count);
		void update_stats();
		void run_messages(int update_timeframe, int message_count);
		void					print_layout();
	private:
		AlgorithmBase*			algorithm_;
		NodeVector				nodes_;
		std::vector<Node*>		destinations_;

		int x_dim_;
		int y_dim_;

		std::string file_name_;

		bool					partitioned();
		void					print_nodes();
	};

	inline Environment::Environment(AlgorithmBase& algorithm, int node_distance, int x_dim, int y_dim, int actuator_count, int comm_range, int sensor_period, std::string file_name):
		algorithm_ { &algorithm }, x_dim_(x_dim), y_dim_(y_dim), file_name_(file_name)
	{
		assert(node_distance <= comm_range);
		std::srand(15);  // NOLINT(cert-msc51-cpp)
		int node_count = 0; //Number of nodes

		for (int x = 0; x < x_dim; x += node_distance)
		{
			for (int y = 0; y < y_dim; y += node_distance)
			{
				bool has_sensor = (node_count >= actuator_count);
				bool is_active = true;

				NodeUnqPtr node = std::make_unique<Node>(node_count + 1, x, y, has_sensor, is_active, *algorithm_, MSG_SEND_COST * 1000, sensor_period);
				nodes_.push_back(std::move(node));

				++node_count;
			}
		}

		assert(actuator_count < node_count);

		for (int act_ndx = 0; act_ndx < actuator_count; ++act_ndx)
		{
			destinations_.push_back(&(*nodes_[act_ndx]));
			nodes_[act_ndx]->has_sensor_ = false;
		}

		for (int i = 0; i < node_count; ++i)
		{
			for (int act_ndx = 0; act_ndx < actuator_count; ++act_ndx)
			{
				nodes_[i]->add_destination(*nodes_[act_ndx]);
			}
		}

		for(auto& srcNode : nodes_)
		{
			int const label = srcNode->label();

			for (auto& destNode : nodes_)
			{
				if(&srcNode != &destNode)
				{
					if(srcNode->distance_to(*destNode) <= comm_range)
					{
						srcNode->add_neighbor(*destNode);
					}
				}
			}
			algorithm_->on_node_init(srcNode.get());
		}
	}

	inline int Environment::get_sensor_period(int x, int y)
	{
		return 200;
		const int probs_x_dim = 10;
		const int probs_y_dim = 10;
		int probs[probs_x_dim][probs_y_dim];

		for (int i = 0; i < probs_x_dim; ++i)
			for (int j = 0; j < probs_y_dim; ++j)
			{
				if (i > 2 && i < 5 && j > 2 && j < 5)
				{
					probs[i][j] = 10;
				}
				else
				{
					probs[i][j] = 2;
				}
			}

		int probs_x_coor = (int)(x * probs_x_dim / x_dim_);
		int probs_y_coor = (int)(x * probs_y_dim / y_dim_);
		return probs[probs_x_coor][probs_y_coor];
	}

	inline int Environment::get_sensory_probability(int x, int y)
	{
		const int probs_x_dim = 10;
		const int probs_y_dim = 10;
		int probs[probs_x_dim][probs_y_dim];

		for (int i = 0; i < probs_x_dim; ++i)
			for (int j = 0; j < probs_y_dim; ++j)
			{
				if (i > 2 && i < 5 && j > 2 && j < 5)
				{
					probs[i][j] = 50;
				}
				else
				{
					probs[i][j] = 10;
				}
			}

		int probs_x_coor = (int)(x * probs_x_dim / x_dim_);
		int probs_y_coor = (int)(x * probs_y_dim / y_dim_);
		return probs[probs_x_coor][probs_y_coor];
	}

	inline void Environment::run_timesteps(int update_timeframe, int loop_count)
	{
		for (int i = 0; i < loop_count; i++)
		{
			std::vector<Node*> node_list;
			for (auto& node : nodes_)
			{
				if (node->active_)
				{
					node_list.push_back(&(*node));
				}
			}
			algorithm_->on_tick(node_list, node_list.back()->destinations());

			for (auto& node : nodes_)
			{
				bool sensed = (i + node->label()) % node->sensor_period_ == 0;
				sensed = sensed && node->has_sensor();
				node->tick(sensed);
			}
			if (i % update_timeframe == 0)
			{
				update_stats();
			}
		}
		print_nodes();
	}

	inline void Environment::update_stats()
	{
		std::cout << "Print the Update Here" << std::endl;
		/*
		 * Update Statistics:
		 *	Total number of Messages created
		 *	Total number of messages sent
		 *	Total number of messages received
		 *	Total number of messages arrived at destination
		 *	For the messages that arrived since the last update:
		 *		Average hop count
		 *		Average num_timesteps between sending and delivery
		 */
	}

	inline void Environment::run_messages(int update_timeframe, int message_count)
	{
		int num_messages_created = 0;
		int num_messages_arrived = 0;
		int cooldown_timer = 0;
		int max_cooldown = 10000;
		int i = 0;
		while (num_messages_arrived < message_count && cooldown_timer < max_cooldown)
		{
			std::vector<Node*> node_list;
			for (auto& node : nodes_)
			{
				if (node->active_)
				{
					node_list.push_back(&(*node));
				}
			}
			algorithm_->on_tick(node_list, node_list.back()->destinations());

			for (auto& node : nodes_)
			{
				int prev_msg_recvd = node->recv_msg_count;
				bool sensed = (i + node->label()) % node->sensor_period_ == 0;
				sensed = sensed && node->has_sensor() && num_messages_created < message_count;
				node->tick(sensed);
				num_messages_created += sensed ? 1 : 0;
				if (prev_msg_recvd < node->recv_msg_count)
				{
					num_messages_arrived += node->recv_msg_count - prev_msg_recvd;
					cooldown_timer = 0;
				}
			}

			if (i % update_timeframe == 0)
				{
					//update_stats();
					//std::cout << "Time = " << i << ", Sent Message Total: " << num_messages_created << "; Arrived Message Total: " << num_messages_arrived << std::endl;
				}
			if (num_messages_created >= message_count)
			{
				//	This ensures that we don't loop infinitely if messages are lost
				//	Once the max number of messages are in play, there is a limit on how much longer the simulation can run
				cooldown_timer++; 
			}
			++i;
		}
		print_nodes();
		std::cout << "Sent Message Total: " << num_messages_created << "; Arrived Message Total: " << num_messages_arrived << std::endl;

		std::ofstream file{ file_name_ };

		//algorithm_->on_end(std::cout);
		algorithm_->on_end(file);
	}

	inline void Environment::print_layout()
	{
		const int node_count = static_cast<int>(nodes_.size());

		std::cout.width(2);
		std::cout << "    ";
		for(int i = 0; i < node_count; ++i)
		{
			std::cout << i+1 << " ";
		}
		std::cout << std::endl;

		for (auto& srcNode : nodes_)
		{
			int const label = srcNode->label();
			std::cout << label << " : ";
			for (auto& destNode : nodes_)
			{
				bool isNeighbor = false;
				for(auto& neighbor : srcNode->neighbors_)
				{
					if(neighbor == destNode.get())
					{
						isNeighbor = true;
						break;
					}
				}
				if (isNeighbor)
				{
					std::cout << srcNode->distance_to(*destNode) << " ";
				}
				else
				{
					std::cout << "  ";
				}
			}
			std::cout << std::endl;
		}
	}

	inline bool can_reach(Node* node, Node* destination)
	{
		//TODO: Edit this
		return true;
	}

	inline bool Environment::partitioned()
	{
		 for (auto& dest : destinations_)
		 {
			 for (auto& node: nodes_)
			 {
				 if (!can_reach(&( *node), dest))
				 {
					 return true;
				 }
			 }
		 }
		 return false;
	}

	inline void Environment::print_nodes()
	{
		for (auto& node : nodes_)
		{
			std::cout << "Node " << node->label() << ": sent messages = " << node->sent_msg_count << ", received messages = " << node->inbox_msg_count <<
				", generated messages = " << node->generated_msg_count_ << ", destination messages = " << node->recv_msg_count << std::endl;
		}
		// For each node, number of sent messages, number of received messages, and number of destination messages (messages that the node was the destination for)
		// Have the node return a list of all destination messages
			// For each message, the hop count and the number of timesteps it took to arrive (received_time - sent_time)
		// Calculations:
			// Using the sent and received message counts, calculate the number/percentage of lost messages
			// Calculate the average hop count and the average number of timesteps for the messages, then sort the messages by the times they were sent
	}
}
