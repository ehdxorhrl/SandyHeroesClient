#include "stdafx.h"
#include "BaseScene.h"
#include "GameFramework.h"

bool BaseScene::ProcessInput(void* p)
{
	//if (main_input_controller_)
	//{
	//	if (main_input_controller_->ProcessInput(id, w_param, l_param, time))
	//		return true;
	//}
}

void BaseScene::Update(float elapsed_time)
{
	Scene::Update(elapsed_time);

	UpdateObjectWorldMatrix();

	CheckPlayerIsGround();
}

void BaseScene::CheckPlayerIsGround()
{
	//if (!is_prepare_ground_checking_)
	//{
	//	PrepareGroundChecking();
	//}
	//
	//XMFLOAT3 position = player_->world_position_vector();
	//constexpr float kGroundYOffset = 1.5f;
	//position.y += kGroundYOffset;
	//XMVECTOR ray_origin = XMLoadFloat3(&position);
	//position.y -= kGroundYOffset;
	//XMVECTOR ray_direction = XMVectorSet(0, -1, 0, 0);
	//
	//bool is_collide = false;
	//float distance{ std::numeric_limits<float>::max() };
	//for (auto& mesh_collider : checking_maps_mesh_collider_list_[stage_clear_num_])
	//{
	//	float t{};
	//	if (mesh_collider->CollisionCheckByRay(ray_origin, ray_direction, t))
	//	{
	//		is_collide = true;
	//		if (t < distance)
	//		{
	//			distance = t;
	//		}
	//	}
	//}
	//if (stage_clear_num_ - 1 >= 0)
	//{
	//	for (auto& mesh_collider : checking_maps_mesh_collider_list_[stage_clear_num_ - 1])
	//	{
	//		float t{};
	//		if (mesh_collider->CollisionCheckByRay(ray_origin, ray_direction, t))
	//		{
	//			is_collide = true;
	//			if (t < distance)
	//			{
	//				distance = t;
	//			}
	//		}
	//	}
	//}
	//if (is_collide)
	//{
	//	float distance_on_ground = distance - kGroundYOffset; //지면까지의 거리
	//	if (distance_on_ground > 0.005f)
	//	{
	//		player_->set_is_ground(false);
	//		return;
	//	}
	//	position.y -= distance_on_ground;
	//	player_->set_is_ground(true);
	//	player_->set_position_vector(position);
	//	return;
	//}
	//
	//player_->set_is_ground(false);
}

void BaseScene::PrepareGroundChecking()
{
	static const std::array<std::string, kStageMaxCount>
		stage_names{ "BASE", "STAGE1", "STAGE2", "STAGE3", "STAGE4", "STAGE5", "STAGE6", "STAGE7", };
	for (int i = 0; i < stage_names.size(); ++i)
	{
		Object* object = Scene::FindObject(stage_names[i]);
		//checking_maps_mesh_collider_list_[i] = Object::GetComponentsInChildren<MeshColliderComponent>(object);
	}
	is_prepare_ground_checking_ = true;
}

