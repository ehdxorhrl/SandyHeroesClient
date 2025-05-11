#pragma once
#include "stdafx.h"
//version 0.1   버전은 항상 바꿔줄 것

// 서버 주소와 포트는 나중에 입력받는식으로 수정
constexpr short SERVER_PORT = 9000;
constexpr char SERVER_ADDR[] = "127.0.0.1";

// SC는 서버에서 클라이언트로, CS는 클라에서 서버로 보내는 패킷
constexpr char S2C_P_USER_INFO = 1;
constexpr char S2C_P_MOVE = 2;
constexpr char S2C_P_ENTER = 3;
constexpr char S2C_P_LEAVE = 4;

constexpr char C2S_P_LOGIN = 101;
constexpr char C2S_P_KEYBOARD_INPUT = 102;
constexpr char C2S_P_MOUSE_MOVE = 103;

constexpr char MAX_ID_LENGTH = 20;

constexpr char PRESS_OFF = 0;
constexpr char PRESS_ON = 1;

#pragma pack (push,1)

struct sc_packet_user_info { // 서버에서 돌린 루프로 추출한 object에 대한 정보를 담은 패킷
	unsigned char size;
	char type;
	long long  id;
	float x, y, z;
};

struct sc_packet_move {
	unsigned char size;
	char type;
	long long id;
	float matrix[16];
};

struct sc_packet_enter {
	unsigned char size;
	char type;
	long long  id;
	char name[MAX_ID_LENGTH];
	float x, y, z;
};

struct sc_packet_leave {
	unsigned char size;
	char type;
	long long  id;
};

struct cs_packet_login {
	unsigned char  size;
	char  type;
};

struct cs_packet_keyboard_input {
	unsigned char  size;
	char  type;
	unsigned char key;
	unsigned char pressed;
};

struct cs_packet_mouse_move {
	unsigned char  size;
	char  type;
	float yaw;
};


#pragma pack (pop)