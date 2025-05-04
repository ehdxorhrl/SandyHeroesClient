#include "stdafx.h"
#include "User.h"
#include "GameFramework.h"
#include "SessionManager.h"
#include "Packet.h"
#include "Timer.h"

//#include "Object.h"
//#include "TestScene.h"


GameFramework* GameFramework::kGameFramework = nullptr;


GameFramework::GameFramework()
{
    assert(kGameFramework == nullptr);
    kGameFramework = this;
}

GameFramework::~GameFramework()
{
    closesocket(socket_);
    WSACleanup();
}

void GameFramework::Initialize()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);

    socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (socket_ <= 0) std::cout << "ERRPR" << "원인";
    else std::cout << "Socket Created.\n";

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN));
    listen(socket_, SOMAXCONN);

    hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket_), hIOCP_, -1, 0);

    do_accept(socket_, &accept_over_);

    //do_accept(socket_, &accept_over_);

    //씬 생성 및 초기화
    //scene_ = std::make_unique<BaseScene>();
    //scene_->Initialize(d3d_device_.Get(), d3d_command_list_.Get(), d3d_root_signature_.Get(),
    //    this);

    client_timer_.reset(new Timer);
    client_timer_->Reset();
}

void GameFramework::ProcessInput()
{
    //while (!input_manager_->IsEmpty())
    //{
    //    InputMessage message = input_manager_->DeQueueInputMessage(client_timer_->PlayTime());
    //    ProcessInput(message.id, message.w_param, message.l_param, message.time);
    //}
}

//void GameFramework::ProcessInput()
//{
//    //먼저 Scene에서 인풋을 처리하는지 확인한다
//    //if (scene_)
//    //{
//    //    if (scene_->ProcessInput(id, w_param, l_param, time))
//    //        return;
//    //}
//}

void GameFramework::FrameAdvance()
{
    client_timer_->Tick();

    auto num_core = std::thread::hardware_concurrency();

    std::vector <std::thread> workers;

    for (unsigned int i = 0; i < num_core; ++i)
        workers.emplace_back([this] {
        this->worker();
            });
    for (auto& w : workers)
        w.join();
    //인풋 처리
    //ProcessInput();

    //충돌처리
    //scene_->CheckObjectByObjectCollisions();

    //업데이트
    //scene_->Update(client_timer_->ElapsedTime());
    //scene_->UpdateObjectWorldMatrix();

}


void GameFramework::do_accept(SOCKET s_socket, EXP_OVER* accept_over)
{
    SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    accept_over->accept_socket_ = c_socket;
    AcceptEx(s_socket, c_socket, accept_over->buffer_, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
        NULL, &accept_over->over_);
}


void GameFramework::worker()
{
	while (true) {
		DWORD io_size;
		WSAOVERLAPPED* o;
		ULONG_PTR key;
		BOOL ret = GetQueuedCompletionStatus(hIOCP_, &io_size, &key, &o, INFINITE);
		EXP_OVER* eo = reinterpret_cast<EXP_OVER*>(o);
		if (FALSE == ret) {
			auto err_no = WSAGetLastError();
			//print_error_message(err_no);
			const auto& users = SessionManager::getInstance().getAllSessions();
			if (users.count(key) != 0)
				SessionManager::getInstance().remove(static_cast<int>(key));
			continue;
		}
		if ((eo->io_op_ == IO_RECV || eo->io_op_ == IO_SEND) && (0 == io_size)) {
			const auto& users = SessionManager::getInstance().getAllSessions();
			if (users.count(key) != 0)
				SessionManager::getInstance().remove(static_cast<int>(key));
			continue;
		}
		switch (eo->io_op_) {
		case IO_ACCEPT:
		{
			int new_id = new_id_++;
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(eo->accept_socket_),
				hIOCP_, new_id, 0);

			SessionManager::getInstance().add(new_id, std::make_shared<Session>(new_id, eo->accept_socket_));

			do_accept(socket_, &accept_over_);
		}
		break;
		case IO_SEND:
			delete eo;
			break;
        case IO_RECV:
        {
            auto session = SessionManager::getInstance().get(static_cast<int>(key));
            if (!session) {
                // 세션이 없으면 그냥 무시
                delete eo;  // EXP_OVER 해제 (필요하면)
                break;
            }

            unsigned char* p = eo->buffer_;
            int data_size = io_size + session->remained_;

            while (p < eo->buffer_ + data_size) {
                unsigned char packet_size = *p;
                if (p + packet_size > eo->buffer_ + data_size)
                    break;
                session->process_packet(p);
                p = p + packet_size;
            }

            if (p < eo->buffer_ + data_size) {
                session->remained_ = static_cast<unsigned char>(eo->buffer_ + data_size - p);
                memcpy(p, eo->buffer_, session->remained_);
            }
            else {
                session->remained_ = 0;
            }

            delete eo;  // EXP_OVER 해제
            session->do_recv();
        }
        break;
		}
	}
}


