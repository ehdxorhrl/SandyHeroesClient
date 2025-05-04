#include "stdafx.h"
#include "GameFramework.h"
#include "Packet.h"
#include "Timer.h"
#include "Object.h"
#include "Shader.h"
#include "TestScene.h"
#include "FrameResourceManager.h"
#include "DescriptorManager.h"
#include "Mesh.h"
#include "InputManager.h"
#include "InputControllerComponent.h"
#include "ModelInfo.h"
#include "Material.h"
#include "AnimationSet.h"
#include "BaseScene.h"

GameFramework* GameFramework::kGameFramework = nullptr;


GameFramework::GameFramework() 
{
	assert(kGameFramework == nullptr);
	kGameFramework = this;
}

GameFramework::~GameFramework()
{
    if (d3d_device_)
    {
        FlushCommandQueue();
    }
}

void GameFramework::Initialize(HINSTANCE hinstance, HWND hwnd)
{
    app_instance_ = hinstance;
    main_wnd_ = hwnd;

    InitDirect3D();

    OnResize();

    d3d_command_list_->Reset(d3d_command_allocator_.Get(), nullptr);

    BuildRootSignature();

    frame_resource_manager_ = std::make_unique<FrameResourceManager>();
    descriptor_manager_ = std::make_unique<DescriptorManager>();
    input_manager_ = std::make_unique<InputManager>();

    //씬 생성 및 초기화
    scene_ = std::make_unique<BaseScene>();
    scene_->Initialize(d3d_device_.Get(), d3d_command_list_.Get(), d3d_root_signature_.Get(), 
        this);

    d3d_command_list_->Close();
    ID3D12CommandList* cmd_list[] = { d3d_command_list_.Get() };
    d3d_command_queue_->ExecuteCommandLists(1, cmd_list);

    FlushCommandQueue();

    scene_->ReleaseMeshUploadBuffer();

    client_timer_.reset(new Timer);
    client_timer_->Reset();
}

void GameFramework::InitDirect3D()
{
    UINT dxgi_farctory_flags = 0;

#if defined(DEBUG) | defined(_DEBUG)
    // D3D12 디버그 레이어를 활성화 합니다.
    {
        ComPtr<ID3D12Debug> debeg_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debeg_controller))))
        {
            debeg_controller->EnableDebugLayer();

            dxgi_farctory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    CreateDXGIFactory2(dxgi_farctory_flags, IID_PPV_ARGS(&dxgi_factory_));

    // 하드웨어 디바이스의 생성을 시도합니다.
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, // 기본 어뎁터 사용합니다.
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&d3d_device_));

    // 실패할 경우 WRAP 디바이스를 사용합니다.
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWrapAdapter;
        dxgi_factory_->EnumWarpAdapter(IID_PPV_ARGS(&pWrapAdapter));

        D3D12CreateDevice(
            pWrapAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&d3d_device_));
    }

    if (!d3d_device_)
    {
        MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
        ::PostQuitMessage(0);
        return;
    }

    //펜스 생성
    d3d_device_->CreateFence(current_fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d_fence_));

    rtv_descriptor_size_ = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsv_descriptor_size_ = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbv_srv_uav_descriptor_size_ = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    DescriptorManager::kCbvSrvUavDescriptorSize = cbv_srv_uav_descriptor_size_;

    //4x msaa 검사
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS quality_levels = {};
    quality_levels.Format = back_buffer_format_;
    quality_levels.SampleCount = 4;
    quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    quality_levels.NumQualityLevels = 0;
    d3d_device_->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &quality_levels,
        sizeof(quality_levels));

    msaa_quality_ = quality_levels.NumQualityLevels;
    assert(msaa_quality_ > 0 && "Unexpected MSAA quality level.");

    CreateCommandObject();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

}

void GameFramework::CreateCommandObject()
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HRESULT hresult = d3d_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&d3d_command_queue_));

    d3d_device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        IID_PPV_ARGS(&d3d_command_allocator_));

    d3d_device_->CreateCommandList(
        0, 
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        d3d_command_allocator_.Get(),
        nullptr, 
        IID_PPV_ARGS(&d3d_command_list_));

    // 렌더링 하기에 앞서서 커맨드 리스트를 리셋하고 커맨드를 기록하는데,
    // 리셋을 하기 위해선 커맨드 리스트가 닫혀있는 상태여야 합니다.
    d3d_command_list_->Close();
}

void GameFramework::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc.Width = client_width_;
    swap_chain_desc.BufferDesc.Height = client_height_;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = client_refresh_rate_;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferDesc.Format = back_buffer_format_;
    swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_desc.SampleDesc.Count = msaa_state_ ? 4 : 1;
    swap_chain_desc.SampleDesc.Quality = msaa_state_ ? (msaa_quality_ - 1) : 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = kSwapChainBufferCount;
    swap_chain_desc.OutputWindow = main_wnd_;
    swap_chain_desc.Windowed = true;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    HRESULT hresult = dxgi_factory_->CreateSwapChain(
        d3d_command_queue_.Get(),
        &swap_chain_desc,
        (IDXGISwapChain**)dxgi_swap_chain_.GetAddressOf());

    dxgi_factory_->MakeWindowAssociation(main_wnd_, DXGI_MWA_NO_ALT_ENTER);

    int i = 0;
    ++i;
}

void GameFramework::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.NumDescriptors = kSwapChainBufferCount;
    rtv_heap_desc.NodeMask = 0;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    d3d_device_->CreateDescriptorHeap(&rtv_heap_desc, 
        IID_PPV_ARGS(d3d_rtv_heap_.GetAddressOf()));

    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_desc.NumDescriptors = kDepthStencilBufferCount;
    dsv_heap_desc.NodeMask = 0;
    dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    d3d_device_->CreateDescriptorHeap(&dsv_heap_desc,
        IID_PPV_ARGS(d3d_dsv_heap_.GetAddressOf()));

}

void GameFramework::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE descriptor_range[6];
    descriptor_range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //albedo
    descriptor_range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //spec gloss
    descriptor_range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); //metal gloss
    descriptor_range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); //emission
    descriptor_range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); //normal
    descriptor_range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5); //cube

    constexpr int root_parameter_size{ 11 };
    CD3DX12_ROOT_PARAMETER root_parameter[root_parameter_size];

    //25.02.23 수정
    //기존 루트 디스크립터 테이블에서 루트 CBV사용으로 변경
    root_parameter[0].InitAsConstantBufferView(0); // world matrix
    root_parameter[1].InitAsConstantBufferView(1); // bone transform
    root_parameter[2].InitAsConstantBufferView(2); // bone offset (default buffer)
    root_parameter[3].InitAsConstantBufferView(3); // render pass
    root_parameter[4].InitAsConstantBufferView(4); // material
    root_parameter[5].InitAsDescriptorTable(1, &descriptor_range[0], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameter[6].InitAsDescriptorTable(1, &descriptor_range[1], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameter[7].InitAsDescriptorTable(1, &descriptor_range[2], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameter[8].InitAsDescriptorTable(1, &descriptor_range[3], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameter[9].InitAsDescriptorTable(1, &descriptor_range[4], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameter[10].InitAsDescriptorTable(1, &descriptor_range[5], D3D12_SHADER_VISIBILITY_PIXEL);

    
    CD3DX12_STATIC_SAMPLER_DESC aniso_warp{ 0 };    //비등방 필터링 warp 모드 샘플러
    CD3DX12_STATIC_SAMPLER_DESC linear_warp{ 1, D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT };

    constexpr int static_sampler_size = 2;
    D3D12_STATIC_SAMPLER_DESC static_sampler[static_sampler_size]{ aniso_warp , linear_warp };

    CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc(root_parameter_size, root_parameter, static_sampler_size, static_sampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serialized_root_sig = nullptr;
    ComPtr<ID3DBlob> error_blob = nullptr;
    D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, 
        serialized_root_sig.GetAddressOf(), error_blob.GetAddressOf());

    if (error_blob)
    {
        OutputDebugStringA((char*)error_blob->GetBufferPointer());
    }

    d3d_device_->CreateRootSignature(
        0,
        serialized_root_sig->GetBufferPointer(),
        serialized_root_sig->GetBufferSize(),
        IID_PPV_ARGS(d3d_root_signature_.GetAddressOf()));

}


void GameFramework::OnResize()
{
    FlushCommandQueue();

    d3d_command_list_->Reset(d3d_command_allocator_.Get(), nullptr);

    //후면버퍼를 다시 만들기 위해 기존 버퍼 삭제
    for (ComPtr<ID3D12Resource>& swap_chain_buffer : d3d_swap_chain_buffers_)
        swap_chain_buffer.Reset();

    d3d_depth_stencil_buffer_.Reset();

    //후면버퍼 크기 변경
    dxgi_swap_chain_->ResizeBuffers(
        kSwapChainBufferCount, 
        client_width_, client_height_, 
        back_buffer_format_, 
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    current_back_buffer_ = 0;

    //Rtv 생성
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle (
        d3d_rtv_heap_->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < kSwapChainBufferCount; ++i)
    {
        //버퍼 가져오기
        dxgi_swap_chain_->GetBuffer(i, 
            IID_PPV_ARGS(d3d_swap_chain_buffers_[i].GetAddressOf()));
        //뷰 생성
        d3d_device_->CreateRenderTargetView(
            d3d_swap_chain_buffers_[i].Get(), nullptr, rtv_heap_handle);

        //힙 오프셋 이동
        rtv_heap_handle.Offset(1, rtv_descriptor_size_);
    }


    //Depth-Stencil 버퍼 생성
    D3D12_RESOURCE_DESC ds_buffer_desc = {};
    ds_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_buffer_desc.Alignment = 0;
    ds_buffer_desc.Width = client_width_;
    ds_buffer_desc.Height = client_height_;
    ds_buffer_desc.DepthOrArraySize = 1;
    ds_buffer_desc.MipLevels = 1;
    ds_buffer_desc.Format = depth_stencil_buffer_format_;
    ds_buffer_desc.SampleDesc.Count = msaa_state_ ? 4 : 1; //msaa를 사용하면 4 아니면 1
    ds_buffer_desc.SampleDesc.Quality = msaa_state_ ? (msaa_quality_ - 1) : 0; 
    ds_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ds_buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ds_clear_value = {};
    ds_clear_value.Format = depth_stencil_buffer_format_;
    ds_clear_value.DepthStencil.Depth = 1.f;
    ds_clear_value.DepthStencil.Stencil = 0;

    d3d_device_->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &ds_buffer_desc,
        D3D12_RESOURCE_STATE_COMMON,
        &ds_clear_value,
        IID_PPV_ARGS(d3d_depth_stencil_buffer_.GetAddressOf()));

    //Dsv 생성
    // 향후 ds 버퍼를 구성할 때 포멧이 바뀌거나 버퍼에 대한 
    // 설명이 desc에 충분하지 않다면 아래 주석을 사용해서 dsv를 생성해야함
    /*D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;*/
    d3d_device_->CreateDepthStencilView(
        d3d_depth_stencil_buffer_.Get(), 
        nullptr,                
        DepthStencilView());

    //뎁스버퍼를 사용 가능 상태로 변경
    d3d_command_list_->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            d3d_depth_stencil_buffer_.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE));

    //명령 실행
    d3d_command_list_->Close();
    ID3D12CommandList* command_list[] = { d3d_command_list_.Get() };
    d3d_command_queue_->ExecuteCommandLists(_countof(command_list), command_list);

    FlushCommandQueue();

    //뷰포트 업데이트
    client_viewport_.TopLeftX = 0;
    client_viewport_.TopLeftY = 0;
    client_viewport_.Width = client_width_;
    client_viewport_.Height = client_height_;
    client_viewport_.MaxDepth = 1.f;
    client_viewport_.MinDepth = 0;

    scissor_rect_ = { 0, 0, client_width_, client_height_ };
}

void GameFramework::ProcessInput()
{
    while (!input_manager_->IsEmpty())
    {
        InputMessage message = input_manager_->DeQueueInputMessage(client_timer_->PlayTime());
        ProcessInput(message.id, message.w_param, message.l_param, message.time);
    }
}

void GameFramework::ProcessInput(UINT id, WPARAM w_param, LPARAM l_param, float time)
{
    //먼저 Scene에서 인풋을 처리하는지 확인한다
    if (scene_)
    {
        if (scene_->ProcessInput(id, w_param, l_param, time))
            return;
    }
    switch (id)
    {
    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        if (w_param == 'P')
        {
            FlushCommandQueue();

            d3d_command_list_->Reset(d3d_command_allocator_.Get(), nullptr);

            scene_ = std::make_unique<BaseScene>();
            scene_->Initialize(d3d_device_.Get(), d3d_command_list_.Get(), d3d_root_signature_.Get(),
                this);

            d3d_command_list_->Close();
            ID3D12CommandList* cmd_list[] = { d3d_command_list_.Get() };
            d3d_command_queue_->ExecuteCommandLists(1, cmd_list);

            FlushCommandQueue();

            scene_->ReleaseMeshUploadBuffer();

        }
        break;
    default:
        break;
    }
}

void GameFramework::FrameAdvance()
{
    client_timer_->Tick();

    //인풋 처리
    ProcessInput();

    //충돌처리
    //scene_->CheckObjectByObjectCollisions();

    //업데이트
    scene_->Update(client_timer_->ElapsedTime());
    scene_->UpdateObjectWorldMatrix();


    //렌더
    auto& command_allocator = frame_resource_manager_->curr_frame_resource()->d3d_allocator;

    command_allocator->Reset();

    d3d_command_list_->Reset(command_allocator.Get(), nullptr);

    d3d_command_list_->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            d3d_swap_chain_buffers_[current_back_buffer_].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    d3d_command_list_->RSSetViewports(1, &client_viewport_);
    d3d_command_list_->RSSetScissorRects(1, &scissor_rect_);

    d3d_command_list_->ClearRenderTargetView(
        CurrentBackBufferView(), DirectX::Colors::Purple, 0, nullptr);
    d3d_command_list_->ClearDepthStencilView(
        DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

    const UINT kNumRenderTargetDescriptors = 1; // 렌더대상의 개수
    d3d_command_list_->OMSetRenderTargets(
        kNumRenderTargetDescriptors,
        &CurrentBackBufferView(),
        true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptor_heaps[] = { descriptor_manager_->GetDescriptorHeap() };
    d3d_command_list_->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);

    d3d_command_list_->SetGraphicsRootSignature(d3d_root_signature_.Get());

    scene_->Render(d3d_command_list_.Get());

    d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        d3d_swap_chain_buffers_[current_back_buffer_].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT));

    d3d_command_list_->Close();
    ID3D12CommandList* command_lists[] = {d3d_command_list_.Get()};
    d3d_command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists);

    dxgi_swap_chain_->Present(0, 0);
    current_back_buffer_ = (current_back_buffer_ + 1) % kSwapChainBufferCount;

    frame_resource_manager_->curr_frame_resource()->fence = ++current_fence_value_;

    d3d_command_queue_->Signal(d3d_fence_.Get(), current_fence_value_);

    frame_resource_manager_->CirculateFrameResource(d3d_fence_.Get());

    std::wstring fps{ L"SandyHeroes(" };
    fps += std::to_wstring(client_timer_->Fps()) + L"fps)";
    SetWindowText(main_wnd_, fps.c_str());
}

void GameFramework::FlushCommandQueue()
{
    ++current_fence_value_;

    d3d_command_queue_->Signal(d3d_fence_.Get(), current_fence_value_);

    if (d3d_fence_->GetCompletedValue() < current_fence_value_)
    {
        HANDLE event_handle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

        d3d_fence_->SetEventOnCompletion(current_fence_value_, event_handle);

        WaitForSingleObject(event_handle, INFINITE);
        CloseHandle(event_handle);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GameFramework::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        d3d_rtv_heap_->GetCPUDescriptorHandleForHeapStart(), 
        current_back_buffer_,
        rtv_descriptor_size_);
}

D3D12_CPU_DESCRIPTOR_HANDLE GameFramework::DepthStencilView() const
{
    return d3d_dsv_heap_->GetCPUDescriptorHandleForHeapStart();
}

LRESULT GameFramework::ProcessWindowMessage(HWND h_wnd, UINT message_id, WPARAM w_param, LPARAM l_param)
{
    switch (message_id)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
        input_manager_->EnQueueInputMessage(message_id, w_param, l_param, client_timer_->PlayTime());
        break;
    default:
        break;
    }
    return 0;
}

FrameResourceManager* GameFramework::frame_resource_manager() const
{
    return frame_resource_manager_.get();
}

DescriptorManager* GameFramework::descriptor_manager() const
{
    return descriptor_manager_.get();
}

HWND GameFramework::main_wnd() const
{
    return main_wnd_;
}

void GameFramework::ConnectServer()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);

    socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDR, &addr.sin_addr);
    int ref = WSAConnect(socket_, reinterpret_cast<sockaddr*>(&addr),
        sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL);
    if(ref == SOCKET_ERROR)
        PostQuitMessage(0);
}
