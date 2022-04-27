#pragma once
#include <string>
#include <vector>
#include <memory>

namespace DC
{
	class Node;

	class Message
	{
	    using string            = std::string;
	 public:
	    enum class MessageType{ msg, ack, heartbeat, protocol };
	    struct Signature;
	    using SignatureVect      = std::vector<Signature>;
	 
	    inline                  Message(Node* source, Node* destination, string& contents, int start_time, MessageType message_type = MessageType::msg);

	    MessageType             message_type() const                { return message_type_; }
	    Node*                   source() const                      { return source_; }
	    Node*                   destination() const                 { return destination_; }
	    Node*                   hop_source() const                  { return hop_source_; }
	    Node*                   hop_destination() const             { return hop_destination_; }
	    void                    set_hop_source(Node* new_source)    { hop_source_ = new_source; }
	    void                    set_hop_destination(Node* new_dst)  { hop_destination_ = new_dst; }
	    string                  contents() const                    { return contents_;}

	    void                    set_arrival_time(int arrival_time)  { arrival_time_ = arrival_time; }
	    int                     travel_time() const                 { return arrival_time_ - start_time_;}
		int                     arrival_time() const				{ return arrival_time_; }

	    void                    increment_hop()                     { ++hop_count_; }
	    int                     hop_count() const                   { return hop_count_;}

	    // Algorithm specific
		inline void set_ext_data(void* ptr) { ext_data_ = std::shared_ptr<void>(ptr); }
		template<typename T> inline std::shared_ptr<T> ext_data()	{ return std::static_pointer_cast<T>(ext_data_); }

	    bool                    arrived() const                     { return arrived_; }

	private:
	    MessageType             message_type_   = MessageType::msg;
	    Node*                   source_         = nullptr;
	    Node*                   destination_    = nullptr;
	    Node*                   hop_source_     = nullptr;
	    Node*                   hop_destination_= nullptr;
	    string                  contents_;
	    int                     hop_count_      = 0;
	    int                     start_time_     = 0;
	    int                     arrival_time_   = 0;
	 
	    bool                    arrived_        = false;
	    std::shared_ptr<void>   ext_data_;

	    //Messages should have an ID, too, in case of duplicate messages
	    //Can use the sender's ID and initial timestamp for that (in an actual system)
	    //In this system, I'll probably use the message's pointer (this)
	};

	struct Message::Signature{
	    Node*   sender_     = nullptr;
	    int     timestamp_  = 0;

	            Signature() = default;
	            Signature(Signature const& other) = default;
	            Signature& operator=(Signature const& other) = default;
	            Signature(Signature&& other) = default;
	            Signature& operator=(Signature&& other) = default;

	            Signature(Node* sender, int timestamp) : sender_{ sender }, timestamp_{ timestamp }{}
	};

	inline Message::Message(Node* _source, Node* _destination, string& _contents, int start_time, MessageType _message_type) :
	    message_type_{ _message_type }, source_{ _source }, destination_{ _destination }, contents_{ _contents }, start_time_{ start_time }
	{
	}
}
