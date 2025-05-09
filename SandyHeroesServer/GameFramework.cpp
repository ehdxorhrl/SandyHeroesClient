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
    for (auto& w : workers_)
        w.join();
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

    //씬 생성 및 초기화
    //scene_ = std::make_unique<BaseScene>();
    //scene_->Initialize(this);
    
    server_timer_.reset(new Timer);
    server_timer_->Reset();

    auto num_core = std::thread::hardware_concurrency();

    for (unsigned int i = 0; i < num_core; ++i)
        workers_.emplace_back([this] {
        this->worker();
            });
}

void GameFramework::FrameAdvance()
{
    server_timer_->Tick();

    //인풋 처리
    ProcessInput();

    //충돌처리
    //scene_->CheckObjectByObjectCollisions();

    //업데이트
    //scene_->Update(server_timer_->ElapsedTime());
    //scene_->UpdateObjectWorldMatrix();
}

void GameFramework::do_accept(SOCKET s_socket, EXP_OVER* accept_over)
{
    SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    accept_over->accept_socket_ = c_socket;
    accept_over->io_op_ = IO_ACCEPT;
    AcceptEx(s_socket, c_socket, accept_over->buffer_, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
        NULL, &accept_over->over_);
}

void GameFramework::ProcessInput()
{
    //먼저 Scene에서 인풋을 처리하는지 확인한다
    //if (scene_)
    //{
    //    if (scene_->ProcessInput(id, w_param, l_param, time))
    //        return;
    //}
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

            // 소켓을 IOCP에 등록
            CreateIoCompletionPort(reinterpret_cast<HANDLE>(eo->accept_socket_),
                hIOCP_, new_id, 0);

            // ❗ AcceptEx로 받은 소켓에 대해 컨텍스트 업데이트
            setsockopt(eo->accept_socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                (char*)&socket_, sizeof(socket_));

            // 세션 생성 및 등록
            auto session = std::make_shared<Session>(new_id, eo->accept_socket_);
            SessionManager::getInstance().add(new_id, session);

            // 이제 안전하게 수신 시작 가능
            session->do_recv();

            // 다음 클라이언트 수신 대기
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
            session->do_recv();
        }
        break;
		}
	}
}


