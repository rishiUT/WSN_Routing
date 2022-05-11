// WSN_Routing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <strstream>

#include "environment.h"
#include "algorithm_test.h"
#include "algorithm.hpp"
#include "algorithm_raser.h"
#include "algorithm_pegasis_updated.h"


int main()
{
    DC::AlgorithmTest test;
    DC::Algorithm algo;
    DC::AlgorithmRaser raser;
    DC::AlgorithmPegasis pegasis;

    for (int sensor_period = 1500; sensor_period >= 100; sensor_period -= 100)
    {
        for (int high_load = 0; high_load < 2; ++high_load)
        {
			int high_load_sensor_period = high_load ? sensor_period / 4 : sensor_period;

            {
                std::string filename_algo = "results\\algo_" + std::to_string(high_load) + "_" + std::to_string(sensor_period) + ".tab";
                DC::Environment env_algo{ algo, 5, 40, 40, 4, 10 , sensor_period, high_load_sensor_period, filename_algo };
                //env_algo.run_timesteps(10000, 5000);
                env_algo.run_messages(10000, 15000);
            }
            {
                std::string filename_raser = "results\\raser_" + std::to_string(high_load) + "_" + std::to_string(sensor_period) + ".tab";
                DC::Environment env_raser{ raser, 5, 40, 40, 4, 10 , sensor_period, high_load_sensor_period, filename_raser };
                //env_raser.run_timesteps(10000, 5000);
                env_raser.run_messages(10000, 15000);
            }
            {
                /*std::string filename_pegasis = "results\\pegasis_" + std::to_string(high_load) + "_" + std::to_string(sensor_period) + ".tab";
                DC::Environment env_pegasis{ pegasis, 5, 40, 40, 1, 10 , sensor_period, high_load_sensor_period, filename_pegasis };
                env_pegasis.run_timesteps(10000, 5000);
                env_pegasis.run_messages(10000, 12000);*/
            }
        }
		
        
    }
    //env.print_layout();
    //env.run_messages(5, 200);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
