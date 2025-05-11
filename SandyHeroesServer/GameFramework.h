#pragma once
#include "User.h"
class Timer;
//class Scene;
//class InputManager;

class GameFramework
{
public:
	GameFramework();
	~GameFramework();

	void Initialize();

	void ProcessInput();
	//void ProcessInput(UINT id, WPARAM w_param, LPARAM l_param, float time);

	void FrameAdvance();

	void do_accept(SOCKET s_socket, EXP_OVER* accept_over); //비동기 accept(임시)
	void worker();  //스레드 함수
private:
	static GameFramework* kGameFramework;
	HANDLE hIOCP_;
	SOCKET socket_;
	std::unique_ptr<Timer> server_timer_;
	std::atomic<int> new_id_ = 0;
	EXP_OVER accept_over_{ IO_ACCEPT };
	std::vector <std::thread> workers_;
	std::chrono::steady_clock::time_point last_send_time_;
	const std::chrono::milliseconds send_interval_{ 100 };
	//std::unique_ptr<Scene> scene_ = nullptr;

};