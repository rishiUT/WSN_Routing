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
	    int                     travel_time() const                 { return arrival_time_ ? arrival_time_ - start_time_ : 0;}
		int                     arrival_time() const				{ return arrival_time_; }

	    void                    increment_hop()                     { ++hop_count_; }
	    int                     hop_count() const                   { return hop_count_;}

		void					set_priority(bool priority)			{ priority_ = priority; }
		bool					priority() const					{ return priority_; }

		int						get_id() const						{ return id_; }

		int						label() const						{ return label_; }
		int						envelope_label() const				{ return envelope_label_; }
		int						start_time() const					{ return start_time_; }

		void					set_hop_timestamp(int hop_timestamp) { hop_timestamp_ = hop_timestamp; }
		int						hop_timestamp()						{ return hop_timestamp_; }

	    // Algorithm specific
		inline void set_ext_data(void* ptr) { ext_data_ = std::shared_ptr<void>(ptr); }
		template<typename T> inline std::shared_ptr<T> ext_data()	{ return std::static_pointer_cast<T>(ext_data_); }


	private:
	    MessageType             message_type_   = MessageType::msg;
	    Node*                   source_         = nullptr;
	    Node*                   destination_    = nullptr;
	    Node*                   hop_source_     = nullptr;
	    Node*                   hop_destination_= nullptr;
	    string                  contents_;
		int						id_ = 0;
	    int                     hop_count_      = 0;
	    int                     start_time_     = 0;
	    int                     arrival_time_   = 0;
		int						hop_timestamp_	= 0;
		static int				ID_COUNTER;
		int						label_			= 0;
		int						envelope_label_ = 0;
	 
		bool					priority_		= false;
	    std::shared_ptr<void>   ext_data_;

	    //Messages should have an ID, too, in case of duplicate messages
	    //Can use the sender's ID and initial timestamp for that (in an actual system)
	    //In this system, I'll probably use the message's pointer (this)
	};

	using MessagePtr = std::shared_ptr<Message>;

	int Message::ID_COUNTER = 0;

	inline Message::Message(Node* _source, Node* _destination, string& _contents, int start_time, MessageType _message_type) :
	    message_type_{ _message_type }, source_{ _source }, destination_{ _destination }, contents_{ _contents }, start_time_{ start_time }
	{
		id_ = (int)(this);
		label_ = ID_COUNTER++;
		envelope_label_ = label_;
	}
}
