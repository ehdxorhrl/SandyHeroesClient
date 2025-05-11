#include "stdafx.h"
#include "AITestScene.h"
#include "Object.h"
#include "ColorShader.h"
#include "FrameResourceManager.h"
#include "DescriptorManager.h"
#include "Mesh.h"
#include "CubeMesh.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "InputControllerComponent.h"
#include "TestControllerComponent.h"
#include "InputManager.h"
#include "ModelInfo.h"
#include "SkinnedMeshShader.h"
#include "SkinnedMesh.h"
#include "Material.h"
#include "StandardMeshShader.h"
#include "StandardSkinnedMeshShader.h"
#include "AnimationSet.h"
#include "FPSControllerComponent.h"
#include "AnimatorComponent.h"
#include "PlayerAnimationState.h"
#include "AnimatorComponent.h"
#include "GameFramework.h"
#include "ColliderComponent.h"
#include "GunComponent.h"
#include "AIComponent.h"

void AITestScene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* command_list,
	ID3D12RootSignature* root_signature, GameFramework* game_framework)
{
	game_framework_ = game_framework;

	BuildShader(device, root_signature);
	BuildMesh(device, command_list);
	BuildMaterial(device, command_list);
	BuildObject(device, command_list);
	BuildFrameResources(device);
	BuildDescriptorHeap(device);
	BuildConstantBufferViews(device);
	BuildShaderResourceViews(device);
}

void AITestScene::BuildShader(ID3D12Device* device, ID3D12RootSignature* root_signature)
{
	//씬에서 사용하는 쉐이더 개수만큼 예약
	int shader_count = 2;
	shaders_.reserve(shader_count);
	shaders_.push_back(std::make_unique<StandardMeshShader>());
	shaders_.push_back(std::make_unique<StandardSkinnedMeshShader>());

	for (int i = 0; i < shader_count; ++i)
	{
		shaders_[i]->CreateShader(device, root_signature);
	}

}

void AITestScene::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list)
{
	meshes_.reserve(2);
	meshes_.push_back(std::make_unique<CubeMesh>());
	Material* material = new Material{};
	material->set_albedo_color(0, 1, 0, 1);
	meshes_[0].get()->AddMaterial(material);
	meshes_[0].get()->set_name("green_cube");
	materials_.emplace_back();
	materials_.back().reset(material);

	model_infos_.reserve(3);
	model_infos_.push_back(std::make_unique<ModelInfo>("./Resource/Model/Dog00.bin", meshes_, materials_));

	BuildScene();

	for (const std::unique_ptr<Mesh>& mesh : meshes_)
	{
		mesh->CreateShaderVariables(device, command_list);
	}
}

void AITestScene::BuildObject(ID3D12Device* device, ID3D12GraphicsCommandList* command_list)
{
	//TODO: 각 메쉬의 컴포넌트 연결 개수를 파악하면 아래 수치를 디테일하게 설정할 수 있을것 같다..
	cb_object_capacity_ = 10000;
	cb_skinned_mesh_object_capacity_ = 10000;

	//모델 오브젝트 배치
	Object* player = model_infos_[0]->GetInstance();
	player_ = player;
	player->set_position_vector(XMFLOAT3{ 0, 15, 0 });
	AnimatorComponent* animator = Object::GetComponent<AnimatorComponent>(player);
	animator->set_animation_state(new PlayerAnimationState);

	//FPS 조작용 컨트롤러 설정
	FPSControllerComponent* fps_controller = new FPSControllerComponent(player);
	player->AddComponent(fps_controller);
	fps_controller->set_client_wnd(game_framework_->main_wnd());
	fps_controller->set_scene(this);

	//메인 컨트롤러로 설정
	main_input_controller_ = fps_controller;

	//모든 총기 정보 로드
	GunComponent::LoadGunInfosFromFile("./Resource/GunInfos.txt");

	//플레이어 총기 장착
	//TODO: 총기 메쉬 장착 구현
	Object* player_gun_frame = player->FindFrame("WeaponR_locator");
	player_gun_frame->AddChild(new Object());
	player_gun_frame = player_gun_frame->child();
	GunComponent* player_gun = new GunComponent(player_gun_frame);
	player_gun->LoadGunInfo("classic");
	player_gun_frame->AddComponent(player_gun);
	player_gun_frame->AddComponent(new MeshComponent(player_gun_frame, Scene::FindMesh("green_cube", meshes_)));
	player_gun_frame->Scale(0.3);

	//카메라 설정
	Object* camera_object = new Object();
	player->AddChild(camera_object);
	fps_controller->set_camera_object(camera_object);
	camera_object->set_position_vector(0, 0.5, 0); // 플레이어 캐릭터의 키가 150인것을 고려하여 머리위치에 배치
	camera_object->set_name("CAMERA_1");
	CameraComponent* camera_component =
		new CameraComponent(camera_object, 0.3, 10000,
			(float)kDefaultFrameBufferWidth / (float)kDefaultFrameBufferHeight, 58);
	camera_object->AddComponent(camera_component);
	main_camera_ = camera_component;

	//충돌체 추가
	//BoundingOrientedBox* obb = new BoundingOrientedBox;
	//{
	//	//TODO: OBB 초기값 설정
	//	obb->Center = player->position_vector();
	//	obb->Extents = { 1,1,1 };
	//
	//}
	//ColliderComponent* collider = new ColliderComponent(player, obb);
	//player->AddComponent(collider);

	//씬 리스트에 추가
	object_list_.emplace_back();
	object_list_.back().reset(player);

	// AI 테스트용 몬스터


	camera_object = new Object;
	camera_object->set_name("CAMERA_2");
	//MeshComponent* cube = new MeshComponent(camera_object, Scene::FindMesh("green_cube", meshes_));
	//camera_object->AddComponent(cube);
	camera_component =
		new CameraComponent(camera_object, 0.3, 10000,
			(float)kDefaultFrameBufferWidth / (float)kDefaultFrameBufferHeight, 58);
	TestControllerComponent* controller = new TestControllerComponent(camera_object);
	camera_object->AddComponent(camera_component);
	camera_object->AddComponent(controller);

	object_list_.emplace_back();
	object_list_.back().reset(camera_object);

}

void AITestScene::BuildConstantBufferViews(ID3D12Device* device)
{
	//25.02.23 수정
	//루트 디스크립터 테이블에서 루트 CBV로 변경함에 따라 
	//더이상 이 함수를 통해 CBV를 생성하지 않음

	//UINT object_cb_size = d3d_util::CalculateConstantBufferSize(sizeof(CBObject));

	//UINT bone_transform_cb_size = d3d_util::CalculateConstantBufferSize(sizeof(CBBoneTransform));
	//UINT bone_transform_offset = descriptor_manager_->cbv_bone_transform_offset();

	//UINT pass_cb_size = d3d_util::CalculateConstantBufferSize(sizeof(CBPass));

	////각 프레임리소스별로 cbv가 필요하다
	//for (int frame_index = 0; frame_index < FrameResourceManager::kFrameCount; ++frame_index)
	//{
	//	//프레임리소스에서 원하는 상수버퍼의 리소스를 가져온다.
	//	ID3D12Resource* object_cb =
	//		frame_resource_manager_->GetResource(frame_index)->cb_object->Resource();

	//	D3D12_GPU_VIRTUAL_ADDRESS cb_adress = object_cb->GetGPUVirtualAddress();
	//	//모든 오브젝트에 대해 cbv가 필요하다.
	//	for (UINT i = 0; i < cb_object_capacity_; ++i)
	//	{
	//		// 디스크립터 힙 오프셋
	//		int heap_index = cb_object_capacity_ * frame_index + i;
	//		D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptor_manager_->GetCpuHandle(heap_index);

	//		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
	//		cbv_desc.BufferLocation = cb_adress;
	//		cbv_desc.SizeInBytes = object_cb_size;

	//		device->CreateConstantBufferView(&cbv_desc, handle);

	//		// 다음 오브젝트의 상수버퍼 어드레스
	//		cb_adress += object_cb_size;
	//	}

	//	// 위 오브젝트와 동일한 방법으로 만들어준다.
	//	ID3D12Resource* bone_transform_cb =
	//		frame_resource_manager_->GetResource(frame_index)->cb_bone_transform->Resource();

	//	cb_adress = bone_transform_cb->GetGPUVirtualAddress();
	//	for (UINT i = 0; i < cb_skinned_mesh_object_capacity_; ++i)
	//	{
	//		int heap_index = descriptor_manager_->cbv_bone_transform_offset() +
	//			cb_skinned_mesh_object_capacity_ * frame_index + i;
	//		D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptor_manager_->GetCpuHandle(heap_index);

	//		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
	//		cbv_desc.BufferLocation = cb_adress;
	//		cbv_desc.SizeInBytes = bone_transform_cb_size;

	//		device->CreateConstantBufferView(&cbv_desc, handle);

	//		cb_adress += bone_transform_cb_size;
	//	}

	//	ID3D12Resource* pass_cb =
	//		frame_resource_manager_->GetResource(frame_index)->cb_pass->Resource();

	//	cb_adress = pass_cb->GetGPUVirtualAddress();

	//	int heap_index = descriptor_manager_->cbv_pass_offset() + frame_index;
	//	D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptor_manager_->GetCpuHandle(heap_index);

	//	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
	//	cbv_desc.BufferLocation = cb_adress;
	//	cbv_desc.SizeInBytes = pass_cb_size;

	//	device->CreateConstantBufferView(&cbv_desc, handle);

	//}

}

using namespace file_load_util;
void AITestScene::BuildScene()
{
	std::ifstream scene_file{ "./Resource/Model/Base.bin", std::ios::binary };

	int root_object_count = ReadFromFile<int>(scene_file);

	std::string load_token;

	for (int i = 0; i < root_object_count; ++i)
	{
		ReadStringFromFile(scene_file, load_token);
		if (load_token[0] == '@')
		{
			load_token.erase(0, 1);
			object_list_.emplace_back();
			object_list_.back().reset(FindModelInfo(load_token)->GetInstance());

			ReadStringFromFile(scene_file, load_token);
			XMFLOAT4X4 transfrom = ReadFromFile<XMFLOAT4X4>(scene_file);
			object_list_.back()->set_transform_matrix(transfrom);
		}
		else
		{
			std::string object_name = load_token;

			ReadStringFromFile(scene_file, load_token); // <Transfrom>
			XMFLOAT4X4 transfrom = ReadFromFile<XMFLOAT4X4>(scene_file);

			model_infos_.push_back(std::make_unique<ModelInfo>("./Resource/Model/" + object_name + ".bin", meshes_, materials_));

			object_list_.emplace_back();
			object_list_.back().reset(model_infos_.back()->GetInstance());

			object_list_.back()->set_transform_matrix(transfrom);

		}
	}

}

bool AITestScene::CheckObjectByObjectCollisions()
{
	auto player_collider = Object::GetComponent<ColliderComponent>(player_);
	
	XMFLOAT3 rayOrigin = player_->world_position_vector();
	rayOrigin.y -= player_collider->extent().y * 0.9f; // 발밑에서 Ray 시작
	XMFLOAT3 rayDir = { 0.0f, -1.0f, 0.0f }; // 아래 방향
	float maxDistance = 5.0f; // 최대 체크 거리
	
	for (auto& object : object_list_)
	{
		ColliderComponent* collider = Object::GetComponent<ColliderComponent>(object.get());
		if (nullptr != collider)
		{
			// 1차: OBB 충돌 검사
			if (player_collider->Intersects(collider))
			{
				MeshComponent* mesh = Object::GetComponentInChildren<MeshComponent>(object.get());
				if (nullptr != mesh)
				{
					if (player_collider->CheckOBBMeshCollision(mesh, player_->world_matrix(), object->world_matrix()))
					{
						// 2차: 발밑으로 RayCast 체크
						if (collider->CheckRayGroundCollision(mesh, rayOrigin, rayDir, maxDistance, object->world_matrix()))
						{
							player_->set_is_ground(true);
							player_->set_velocity({ player_->velocity().x, 0.0f, player_->velocity().z }); // y 속도 정지
							return true;
						}
						else
						{
							// 바닥이 아니면 위치 복구 및 속도 제한
							//ResolveCollision(m_pPlayer, pGameObject);
							return true;
						}
					}
				}
			}
		}
	}
	player_->set_is_ground(false); // 아무 것도 닿지 않으면 공중
	return false;
}

void AITestScene::Render(ID3D12GraphicsCommandList* command_list)
{
	main_camera_->UpdateCameraInfo();

	CBPass cb_pass{};
	cb_pass.view_matrix = xmath_util_float4x4::TransPose(main_camera_->view_matrix());
	cb_pass.proj_matrix = xmath_util_float4x4::TransPose(main_camera_->projection_matrix());
	cb_pass.camera_position = main_camera_->world_position();

	//TODO: 조명 관련 클래스를 생성후 그것을 사용하여 아래 정보 업데이트(현재는 테스트용 하드코딩)
	cb_pass.ambient_light = XMFLOAT4{ 0.1,0.1,0.1, 1 };
	cb_pass.lights[0].strength = XMFLOAT3{ 1,1,1 };
	cb_pass.lights[0].direction = XMFLOAT3{ 1,-1, -0.5 };
	cb_pass.lights[0].enable = true;
	cb_pass.lights[0].type = 0;

	for (int i = 1; i < 16; ++i)
		cb_pass.lights[i].enable = false;

	FrameResourceManager* frame_resource_manager = game_framework_->frame_resource_manager();
	frame_resource_manager->curr_frame_resource()->cb_pass.get()->CopyData(0, cb_pass);

	//25.02.23 수정
	//기존 루트 디스크립터 테이블에서 루트 CBV로 변경
	D3D12_GPU_VIRTUAL_ADDRESS cb_pass_address =
		frame_resource_manager->curr_frame_resource()->cb_pass.get()->Resource()->GetGPUVirtualAddress();

	command_list->SetGraphicsRootConstantBufferView((int)RootParameterIndex::kRenderPass, cb_pass_address);

	Mesh::ResetCBObjectCurrentIndex();
	SkinnedMesh::ResetCBSkinnedMeshObjectCurrentIndex();

	// 단순한 배치 처리 
	// 씬에서 사용하는 쉐이더가 n개이면 SetPipelineState가 n번 호출된다
	for (const std::unique_ptr<Shader>& shader : shaders_)
	{
		command_list->SetPipelineState(shader->GetPipelineState());

		for (const std::unique_ptr<Mesh>& mesh : meshes_)
		{
			if (mesh->shader_type() == (int)shader->shader_type())
			{
				mesh->Render(command_list, frame_resource_manager, game_framework_->descriptor_manager());
			}
		}

	}


}

bool AITestScene::ProcessInput(UINT id, WPARAM w_param, LPARAM l_param, float time)
{
	if (main_input_controller_)
	{
		if (main_input_controller_->ProcessInput(id, w_param, l_param, time))
			return true;
	}
	switch (id)
	{
	case WM_KEYDOWN:
		// 카메라 전환 테스트
		if (w_param == 'K')
		{
			//바꿀 카메라 오브젝트를 찾고
			Object* camera = FindObject("CAMERA_2");

			//그 오브젝트의 카메라와 컨트롤러를 씬으로 가져온다
			CameraComponent* new_camera = Object::GetComponent<CameraComponent>(camera);
			if (new_camera) // nullptr 방지
			{
				main_camera_ = new_camera;
			}
			main_input_controller_ = Object::GetComponent<InputControllerComponent>(camera);
		}
		if (w_param == 'L')
		{
			//바꿀 카메라 오브젝트를 찾고
			Object* camera = FindObject("Dog00");
			//그 오브젝트의 카메라와 컨트롤러를 씬으로 가져온다
			CameraComponent* new_camera = Object::GetComponentInChildren<CameraComponent>(camera);
			if (new_camera) // nullptr 방지
			{
				main_camera_ = new_camera;
			}
			main_input_controller_ = Object::GetComponent<FPSControllerComponent>(camera);
		}
		break;

	default:
		return false;
		break;
	}
}

void AITestScene::AddRemotePlayer(int id, const std::string& name, const XMFLOAT3& pos)
{
	Object* remote = model_infos_[0]->GetInstance();

	remote->set_position_vector(pos);
	AnimatorComponent* m_animator = Object::GetComponent<AnimatorComponent>(remote);
	m_animator->set_animation_state(new PlayerAnimationState);

	Object* remote_gun_frame = remote->FindFrame("WeaponR_locator");
	remote_gun_frame->AddChild(new Object());
	remote_gun_frame = remote_gun_frame->child();
	GunComponent* remote_gun = new GunComponent(remote_gun_frame);
	remote_gun->LoadGunInfo("classic");
	remote_gun_frame->AddComponent(remote_gun);
	remote_gun_frame->AddComponent(new MeshComponent(remote_gun_frame, Scene::FindMesh("green_cube", meshes_)));
	remote_gun_frame->Scale(0.1);

	remote->set_id(id);
	remote->set_name(name);

	object_list_.emplace_back();
	object_list_.back().reset(remote);

}