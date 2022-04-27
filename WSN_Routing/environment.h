#pragma once
#include<vector>
#include<iostream>
#include "node.hpp"
//#include "message.hpp"
#include "algorithm_base.h"

namespace DC
{
	class Environment
	{
		static constexpr int comm_range = 15;
		using NodeUnqPtr		= std::unique_ptr<Node>;
		using NodeVector		= std::vector<NodeUnqPtr>;
	public:
		inline						Environment(AlgorithmBase& algorithm, int node_count, int actuator_count);
		void					run(int loop_count, int message_count);
		void					print_layout();
	private:
		AlgorithmBase*			algorithm_;
		NodeVector				nodes_;
	};

	inline Environment::Environment(AlgorithmBase& algorithm, int node_count, int actuator_count):
		algorithm_ { &algorithm }
	{
		std::srand(10);  // NOLINT(cert-msc51-cpp)

		for (int i = 0; i < node_count; ++i)
		{
			static constexpr int maxXY = 20;
			int x = std::rand() % maxXY;	// NOLINT(concurrency-mt-unsafe)
			int y = std::rand() % maxXY;	// NOLINT(concurrency-mt-unsafe)

			bool has_sensor = (i >= actuator_count);
			bool is_active = true;

			NodeUnqPtr node = std::make_unique<Node>(i + 1, x, y, has_sensor, is_active, *algorithm_);
			nodes_.push_back(std::move(node));

			algorithm_->on_node_init(nodes_[i].get());
			if(has_sensor)
			{
				for(int act_ndx = 0; act_ndx < actuator_count; ++act_ndx)
				{
					nodes_[i]->add_destination(*nodes_[act_ndx]);
				}
			}
		}
		for (int i = 0; i < actuator_count; ++i)
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
					if(srcNode->distance_to(*destNode) < comm_range)
					{
						srcNode->add_neighbor(*destNode);
					}
				}
			}
		}

	}

	inline void Environment::run(int loop_count, int message_count)
	{
		for (int i = 0; i < loop_count; i++)
		{
			for (auto& node : nodes_)
			{
				int val = std::rand();
				bool sensed = ((val % 10) == 0);
				sensed = sensed && node->has_sensor();
				node->tick(sensed);
			}
		}
		for (auto& node : nodes_)
		{
			std::cout << "Node " << node->label() << ": sent messages = " << node->sent_msg_count << ", received messages = " << node->recv_msg_count <<"\n";
		}

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
}
