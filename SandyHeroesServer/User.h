#pragma once
#include "Object.h"

class EXP_OVER
{
public:
	EXP_OVER(IO_OP op) : io_op_(op)
	{
		ZeroMemory(&over_, sizeof(over_));

		wsabuf_[0].buf = reinterpret_cast<CHAR*>(buffer_);
		wsabuf_[0].len = sizeof(buffer_);
	}

	WSAOVERLAPPED	over_;
	IO_OP			io_op_;
	SOCKET			accept_socket_;
	unsigned char	buffer_[1024];
	WSABUF			wsabuf_[1];
};

class Session {
private:
	SOCKET			c_socket_;
	long long		id_;
	std::string		name_;

	EXP_OVER		recv_over_{ IO_RECV };

	Object			object_;
public:
	Session();
	Session(long long session_id, SOCKET s);
	~Session();

	void do_recv();
	void do_send(void* buff);
	void send_player_info_packet();
	void send_player_position();
	void process_packet(unsigned char* p);

public:
	unsigned char	remained_;
};