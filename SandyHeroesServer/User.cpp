#include "stdafx.h"
#include "User.h"
#include "SessionManager.h"

Session::Session() {
	std::cout << "DEFAULT SESSION CONSTRUCTOR CALLED!!\n";
	exit(-1);
}
Session::Session(long long session_id, SOCKET s) : id_(session_id), c_socket_(s)
{
	remained_ = 0;
	do_recv();
}
Session::~Session()
{
	sc_packet_leave lp;
	lp.size = sizeof(lp);
	lp.type = S2C_P_LEAVE;
	lp.id = id_;
	const auto& users = SessionManager::getInstance().getAllSessions();
	for (auto& g : users) {
		if (id_ != g.first) {
			g.second->do_send(&lp);
		}
	}
	closesocket(c_socket_);
}

void Session::do_recv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&recv_over_.over_, sizeof(recv_over_.over_));
	recv_over_.wsabuf_[0].buf = reinterpret_cast<CHAR*>(recv_over_.buffer_ + remained_);
	recv_over_.wsabuf_[0].len = sizeof(recv_over_.buffer_) - remained_;
	
	auto ret = WSARecv(c_socket_, recv_over_.wsabuf_, 1, NULL,
		&recv_flag, &recv_over_.over_, NULL);
	if (0 != ret) {
		auto err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) {
			//print_error_message(err_no);
			exit(-1);
		}
	}
}
void Session::do_send(void* buff)
{
	EXP_OVER* o = new EXP_OVER(IO_SEND);
	const unsigned char packet_size = reinterpret_cast<unsigned char*>(buff)[0];
	memcpy(o->buffer_, buff, packet_size);
	o->wsabuf_[0].len = packet_size;
	DWORD size_sent;
	WSASend(c_socket_, o->wsabuf_, 1, &size_sent, 0, &(o->over_), NULL);
}

void Session::send_player_info_packet()
{
	sc_packet_user_info p;
	p.size = sizeof(p);
	p.type = S2C_P_USER_INFO;
	p.id = id_;
	p.x = x_;
	p.y = y_;
	do_send(&p);
}

void Session::send_player_position()
{
	sc_packet_move p;
	p.size = sizeof(p);
	p.type = S2C_P_MOVE;
	p.id = id_;
	p.x = x_;
	p.y = y_;
	do_send(&p);
}

void Session::process_packet(unsigned char* p)
{
	const unsigned char packet_type = p[1];
	switch (packet_type) {
	case C2S_P_LOGIN:
	{
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
		name_ = packet->name;
		x_ = 4;
		y_ = 4;
		z_ = 0;
		send_player_info_packet();
	
		sc_packet_enter ep;
		ep.size = sizeof(ep);
		ep.type = S2C_P_ENTER;
		ep.id = id_;
		strcpy_s(ep.name, name_.c_str());
		ep.o_type = 0;
		ep.x = x_;
		ep.y = y_;
		ep.z = z_;
		
		const auto& users = SessionManager::getInstance().getAllSessions();
		for (auto& u : users) {
			if (u.first != id_)
				u.second->do_send(&ep);
		}
	
		for (auto& u : users) {
			if (u.first != id_) {
				sc_packet_enter ep;
				ep.size = sizeof(ep);
				ep.type = S2C_P_ENTER;
				ep.id = u.first;
				strcpy_s(ep.name, u.second->name_.c_str());
				ep.o_type = 0;
				ep.x = u.second->x_;
				ep.y = u.second->y_;
				ep.z = u.second->z_;
				do_send(&ep);
			}
		}
		break;
	}
	case C2S_P_KEYBOARD_INPUT: {
		//cs_packet_keyboard_input* packet = reinterpret_cast<cs_packet_keyboard_input*>(p);
		//switch (packet->direction) {
		//case MOVE_UP: if (_y > 0) _y = _y - 1; break;
		//case MOVE_DOWN: if (_y < (MAP_HEIGHT - 1)) _y = _y + 1; break;
		//case MOVE_LEFT: if (_x > 0) _x = _x - 1; break;
		//case MOVE_RIGHT: if (_x < (MAP_WIDTH - 1)) _x = _x + 1; break;
		//}
		//
		//sc_packet_move mp;
		//mp.size = sizeof(mp);
		//mp.type = S2C_P_MOVE;
		//mp.id = _id;
		//mp.x = _x;
		//mp.y = _y;
		//for (auto& u : g_users) {
		//	u.second.do_send(&mp);
		//}
		//break;
	}

	//case C2S_P_MOUSE_MOVE: {
	//	cs_packet_mouse_move* packet = reinterpret_cast<cs_packet_mouse_move*>(p);
	//	switch (packet->direction) {
	//	case MOVE_UP: if (_y > 0) _y = _y - 1; break;
	//	case MOVE_DOWN: if (_y < (MAP_HEIGHT - 1)) _y = _y + 1; break;
	//	case MOVE_LEFT: if (_x > 0) _x = _x - 1; break;
	//	case MOVE_RIGHT: if (_x < (MAP_WIDTH - 1)) _x = _x + 1; break;
	//	}
	//
	//	sc_packet_move mp;
	//	mp.size = sizeof(mp);
	//	mp.type = S2C_P_MOVE;
	//	mp.id = _id;
	//	mp.x = _x;
	//	mp.y = _y;
	//	for (auto& u : g_users) {
	//		u.second.do_send(&mp);
	//	}
	//	break;
	//}
	default:
		std::cout << "Error Invalid Packet Type\n";
		exit(-1);
	}
}