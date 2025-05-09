#pragma once
#include "Scene.h"
#include "ColliderComponent.h"

class MeshColliderComponent;

class BaseScene :
    public Scene
{
public:
	virtual void BuildShader(ID3D12Device* device, ID3D12RootSignature* root_signature) override;
	virtual void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list) override;
	virtual void BuildObject(ID3D12Device* device, ID3D12GraphicsCommandList* command_list) override;

	virtual void Render(ID3D12GraphicsCommandList* command_list) override;

	virtual bool ProcessInput(UINT id, WPARAM w_param, LPARAM l_param, float time) override;

	virtual void Update(float elapsed_time) override;

	//TODO: 단일 플레이어 뿐만 아니라 적, 다른 플레이어에 대한 ground 체크도 필요
	void CheckPlayerIsGround();

	void PrepareGroundChecking();	//맵 바닥체크를 위한 사전 작업

	Object* player() const { return player_; }
private:
	//TODO: Player 객체 구현
	Object* player_ = nullptr;

	static constexpr int kStageMaxCount{ 8 };	// 게임 스테이지 총 개수
	bool is_prepare_ground_checking_ = false;
	std::array<std::list<MeshColliderComponent*>, kStageMaxCount> checking_maps_mesh_collider_list_;	//맵 바닥체크를 위한 메쉬 콜라이더 리스트 배열
	int stage_clear_num_{ 0 };	// 플레이어의 스테이지 진행도

	bool is_render_debug_mesh_ = false;	//디버그용 와이어프레임 obb를 렌더하는지 여부

};

