// WSN_Routing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "environment.h"
#include "algorithm_test.h"
#include "algorithm.hpp"
#include "algorithm_raser.h"
#include "algorithm_pegasis.h"


int main()
{
    DC::AlgorithmTest test;
    DC::Algorithm algo;
    DC::AlgorithmRaser raser;
    DC::AlgorithmPegasis pegasis;
    DC::Environment env{ pegasis, 5, 40, 40, 4, 10 , 200, "output.tab"};
    env.print_layout();
    env.run_messages(5, 200);
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
