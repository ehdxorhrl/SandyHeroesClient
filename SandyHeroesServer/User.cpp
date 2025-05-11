#include "stdafx.h"
#include "Object.h"
#include "User.h"
#include "SessionManager.h"

Session::Session() {
	std::cout << "DEFAULT SESSION CONSTRUCTOR CALLED!!\n";
	exit(-1);
}
Session::Session(long long session_id, SOCKET s)
	: id_(session_id), c_socket_(s)
{
	remained_ = 0;
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

void Session::update(float elapsed_time)
{
	XMFLOAT3 velocity{ 0,0,0 };
	float speed = 10;
	XMFLOAT3 look = object_.look_vector();
	XMFLOAT3 right = object_.right_vector();
	look.y = 0.f;
	right.y = 0.f;
	look = xmath_util_float3::Normalize(look);
	right = xmath_util_float3::Normalize(right);

	if (is_key_down_['W']) velocity += look * speed;
	if (is_key_down_['S']) velocity -= look * speed;
	if (is_key_down_['A']) velocity -= right * speed;
	if (is_key_down_['D']) velocity += right * speed;

	object_.set_velocity(velocity);
	object_.set_position_vector(object_.position_vector() + (object_.velocity() * elapsed_time));
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
	sc_packet_user_info ip;
	ip.size = sizeof(ip);
	ip.type = S2C_P_USER_INFO;
	ip.id = id_;
	ip.x = object_.world_position_vector().x;
	ip.y = object_.world_position_vector().y;
	ip.z = object_.world_position_vector().z;
	do_send(&ip);
}

void Session::send_player_position()
{
	sc_packet_move mp;
	mp.size = sizeof(mp);
	mp.type = S2C_P_MOVE;
	mp.id = id_;
	XMFLOAT4X4 xf;
	XMFLOAT4X4 mat = object_.transform_matrix();
	XMStoreFloat4x4(&xf, XMLoadFloat4x4(&mat));
	memcpy(mp.matrix, &xf, sizeof(float) * 16);
	do_send(&mp);
}

void Session::process_packet(unsigned char* p, float elapsed_time)
{
	const unsigned char packet_type = p[1];
	static int _z = 0;
	switch (packet_type) {
	case C2S_P_LOGIN:
	{
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
		object_.set_position_vector(0, 10, _z);
		_z += 10;
		send_player_info_packet();
	
		sc_packet_enter ep;
		ep.size = sizeof(ep);
		ep.type = S2C_P_ENTER;
		ep.id = id_;
		strcpy_s(ep.name, name_.c_str());
		XMFLOAT4X4 xf;
		XMFLOAT4X4 mat = object_.transform_matrix();
		XMStoreFloat4x4(&xf, XMLoadFloat4x4(&mat));
		memcpy(ep.matrix, &xf, sizeof(float) * 16);
		
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
				XMFLOAT4X4 u_mat = u.second->object_.transform_matrix();
				XMStoreFloat4x4(&xf, XMLoadFloat4x4(&u_mat));
				memcpy(ep.matrix, &xf, sizeof(float) * 16);
				do_send(&ep);
			}
		}
		break;
	}
	case C2S_P_KEYBOARD_INPUT: {
		cs_packet_keyboard_input* packet = reinterpret_cast<cs_packet_keyboard_input*>(p);
		//std::cout << "key е╦ют" << packet->key << std::endl;
		//std::cout << packet->is_down << std::endl;

		is_key_down_[packet->key] = packet->is_down;
		
		//sc_packet_move mp;
		//mp.size = sizeof(mp);
		//mp.type = S2C_P_MOVE;
		//mp.id = id_;
		//XMFLOAT4X4 xf;
		//XMFLOAT4X4 mat = object_.transform_matrix();
		//XMStoreFloat4x4(&xf, XMLoadFloat4x4(&mat));
		//memcpy(mp.matrix, &xf, sizeof(float) * 16);
		//
		//const auto& users = SessionManager::getInstance().getAllSessions();
		//for (auto& u : users) {
		//	u.second->do_send(&mp);
		//}
		break;
	}

	case C2S_P_MOUSE_MOVE: {	
			cs_packet_mouse_move* packet = reinterpret_cast<cs_packet_mouse_move*>(p);
			object_.Rotate(0,
				static_cast<float>(packet->yaw) * 0.1,
				0.f);
		
			sc_packet_move mp;
			mp.size = sizeof(mp);
			mp.type = S2C_P_MOVE;
			mp.id = id_;
			XMFLOAT4X4 xf;
			XMFLOAT4X4 mat = object_.transform_matrix();
			XMStoreFloat4x4(&xf, XMLoadFloat4x4(&mat));
			memcpy(mp.matrix, &xf, sizeof(float) * 16);
		
			const auto& users = SessionManager::getInstance().getAllSessions();
			for (auto& u : users) {
				u.second->do_send(&mp);
			}
		break;
	}
	default:
		std::cout << "Error Invalid Packet Type\n";
		exit(-1);
	}
}