#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector> 

namespace DC{
	struct MessageHopLogEntry
	{
		int srcNode = 0;
		int destNode = 0;
		int hopSource = 0;
		int hopDest = 0;
		int msgLabel = 0;
		int timestamp = 0;
		int hopCount = 0;
		int startTime = 0;
		int endTime = 0;
		int travelTime = 0;
		bool arrival_hop = false;

		void print(std::ostream& os)
		{
			os << srcNode << "\t";
			os << destNode << "\t";
			os << hopSource << "\t";
			os << hopDest << "\t";
			os << msgLabel << "\t";
			os << timestamp << "\t";
			os << hopCount << "\t";
			os << startTime << "\t";
			os << endTime << "\t";
			os << arrival_hop << "\t";
			os << travelTime << "\n";
		}
	};
	/*
	inline std::ostream& operator<<(std::ostream& os, MessageHopLogEntry const& entry)
	{
		os << entry.srcNode << "\t";
		os << entry.destNode << "\t";
		os << entry.msgLabel<< "\t";
		os << entry.timestamp << "\t";
		os << entry.hopCount << "\t";
		os << entry.startTime << "\t";
		os << entry.endTime << "\t";
		os << entry.travelTime << "\n";
		return os;
	}
	*/


	template<typename T>
	class Logger
	{
	public:

		void addEntry(T const& entry);
		void print(std::ostream& os, bool arrival_only = false);
		
	private:
		friend std::ostream& operator<<(std::ostream& os, Logger const& entry);
		
		std::vector<T> _entries;

	};
	
	template<typename T>
	inline void Logger<T>::addEntry(T const& entry)
	{
		_entries.push_back(entry);
	}

	template <typename T>
	void Logger<T>::print(std::ostream& os, bool arrival_only)
	{
		os << "srcNode" << "\t";
		os << "destNode" << "\t";
		os << "hopSource" << "\t";
		os << "hopDest" << "\t";
		os << "msgLabel" << "\t";
		os << "timestamp" << "\t";
		os << "hopCount" << "\t";
		os << "startTime" << "\t";
		os << "endTime" << "\t";
		os << "arrival_hop" << "\t";
		os << "travelTime" << "\n";
		for(auto& entry: _entries)
		{
			if(!arrival_only || entry.arrival_hop)
			{
				entry.print(os);
			}
		}
	}
	//template<typename T>
	//inline std::ostream& operator<<(std::ostream& os, Logger<T> const& logger)
	//{
	//	os << "srcNode" << "\t";
	//	os << "destNode" << "\t";
	//	os << "hopSource" << "\t";
	//	os << "hopDest" << "\t";
	//	os << "msgLabel" << "\t";
	//	os << "timestamp" << "\t";
	//	os << "hopCount" << "\t";
	//	os << "startTime" << "\t";
	//	os << "endTime" << "\t";
	//	os << "arrival_hop" << "\t";
	//	os << "travelTime" << "\n";
	//	for (auto& entry : logger._entries)
	//	{
	//		os << entry;
	//	}
	//	return os;
	//}

}
