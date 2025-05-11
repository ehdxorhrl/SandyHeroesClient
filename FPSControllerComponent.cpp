#include "stdafx.h"
#include "FPSControllerComponent.h"
#include "Object.h"
#include "AnimatorComponent.h"
#include "PlayerAnimationState.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "Scene.h"
#include "Mesh.h"
#include "Shader.h"
#include "ModelInfo.h"
#include "Material.h"
#include "AnimationSet.h"
#include "GunComponent.h"

FPSControllerComponent::FPSControllerComponent(Object* owner) : InputControllerComponent(owner)
{
	is_key_down_['W'] = false;
	is_key_down_['A'] = false;
	is_key_down_['S'] = false;
	is_key_down_['D'] = false;

	mouse_xy_.x = kDefaultFrameBufferWidth / 2;
	mouse_xy_.y = kDefaultFrameBufferHeight / 2;
}

Component* FPSControllerComponent::GetCopy()
{
    return new FPSControllerComponent(*this);
}

bool FPSControllerComponent::ProcessInput(UINT message_id, WPARAM w_param, LPARAM l_param, float message_time)
{
	switch (message_id)
	{
	case WM_LBUTTONDOWN:
		is_firekey_down_ = true;
		mouse_xy_.x = LOWORD(l_param);
		mouse_xy_.y = HIWORD(l_param);
		{		
			GunComponent* gun = Object::GetComponentInChildren<GunComponent>(owner_);
		if (!gun)
		{
			break;
		}
			// 1. 총구 위치
			XMFLOAT3 gun_shoting_point{ gun->owner()->world_position_vector() };

			// 2. 피킹 지점(월드 좌표계)
			int sx = mouse_xy_.x;
			int sy = mouse_xy_.y;
			Object* picked_object = nullptr;
			XMVECTOR picking_point_w = scene_->GetPickingPointAtWorld(sx, sy, picked_object);

			// 3. 1번에서 2번을 향하는 총알 발사
			XMFLOAT3 bullet_dir{};
			XMStoreFloat3(&bullet_dir, XMVector3Normalize(picking_point_w - XMLoadFloat3(&gun_shoting_point)));
			Mesh* bullet_mesh = Scene::FindMesh("green_cube", scene_->meshes());
			gun->FireBullet(bullet_dir, bullet_mesh);
		}
		break;
	case WM_LBUTTONUP:
		is_firekey_down_ = false;
		break;
	case WM_KEYDOWN:
		switch (w_param)
		{
		case 'W':
			is_key_down_['W'] = true;
			break;
		case 'A':
			is_key_down_['A'] = true;
			break;
		case 'S':
			is_key_down_['S'] = true;
			break;
		case 'D':
			is_key_down_['D'] = true;
			break;
		case VK_SPACE:
		{
			constexpr float kFallCheckVelocity = -0.5;
			if (owner_->velocity().y > kFallCheckVelocity)
			{
				is_jumpkey_pressed_ = true;
			}
		}
			break; 
		case VK_SHIFT:
			if (dash_cool_delta_time_ <= 0)
			{
				is_dash_pressed_ = true;
				dash_cool_delta_time_ = dash_cool_time_;
				dash_velocity_ = { 0,0,0 };
				dash_before_position_ = owner_->position_vector();
				XMFLOAT3 look = owner_->look_vector();
				XMFLOAT3 right = owner_->right_vector();
				look.y = 0.f;
				right.y = 0.f;
				if (is_key_down_['W'])
				{
					dash_velocity_ += look;
				}
				if (is_key_down_['S'])
				{
					dash_velocity_ -= look;
				}
				if (is_key_down_['D'])
				{
					dash_velocity_ += right;
				}
				if (is_key_down_['A'])
				{
					dash_velocity_ -= right;
				}
				if (xmath_util_float3::Length(dash_velocity_) == 0)
				{
					dash_velocity_ += look;
				}
			}
			break;
		default:
			return false;
			break;
		}
		break;
	case WM_KEYUP:
		switch (w_param)
		{
		case 'W':
			is_key_down_['W'] = false;
			break;
		case 'A':
			is_key_down_['A'] = false;
			break;
		case 'S':
			is_key_down_['S'] = false;
			break;
		case 'D':
			is_key_down_['D'] = false;
			break;

		default:
			return false;
			break;
		}
		break;
	default:
		return false;
		break;
	}
	return true;
}

void FPSControllerComponent::Update(float elapsed_time)
{
	XMFLOAT3 velocity{ 0,0,0 };
	float speed = 10;
	XMFLOAT3 look = owner_->look_vector();
	XMFLOAT3 right = owner_->right_vector();
	look.y = 0.f; // xz 평면을 따라 이동
	right.y = 0.f; 
	look = xmath_util_float3::Normalize(look);
	right = xmath_util_float3::Normalize(right);

	if (is_key_down_['W']) velocity += look * speed;
	if (is_key_down_['S']) velocity -= look * speed;
	if (is_key_down_['A']) velocity -= right * speed;
	if (is_key_down_['D']) velocity += right * speed;

	y_axis_velocity_ = 0.f;

	//if (owner_->is_ground())
	//{
	//	y_axis_velocity_ = 0.f;
	//}
	//else
	//{
	//	y_axis_velocity_ -= gravity_ * elapsed_time;
	//}

	//if (is_jumpkey_pressed_)
	//{
	//	y_axis_velocity_ = jump_speed_;
	//	is_jumpkey_pressed_ = false;
	//}

	
	//XMFLOAT3 position = owner_->position_vector();
	//if (position.y < 0)
	//{
	//	owner_->set_position_vector(position.x, 0, position.z);
	//}
	//velocity.y += y_axis_velocity_;

	// 대쉬
	//constexpr float kDashSpeed = 70.f;
	//constexpr float kDashLength = 10.f;
	//if (is_dash_pressed_)
	//{
	//	is_dash_pressed_ = false;
	//	dash_velocity_ = (xmath_util_float3::Normalize(dash_velocity_) * kDashSpeed);
	//}
	//if (xmath_util_float3::Length(owner_->position_vector() - dash_before_position_) >= kDashLength)
	//{
	//	dash_velocity_ = { 0,0,0 };
	//	AnimatorComponent* animator = Object::GetComponent<AnimatorComponent>(owner_);
	//	if (animator)
	//	{
	//		animator->animation_state()->set_animation_track((int)PlayerAnimationTrack::kIdle);
	//	}
	//}
	//dash_cool_delta_time_ -= elapsed_time;
	//velocity += dash_velocity_;

	//owner_->set_velocity(velocity);

	//if (is_firekey_down_)
	//{
	//	//총알이 총구에서 피킹 지점으로 발사되게 구현
	//	//2가지 정보 필요 1. 총구 위치 2. 피킹 지점
	//	GunComponent* gun = Object::GetComponentInChildren<GunComponent>(owner_);
	//	if (gun && (gun->fire_type() == GunFireType::kAuto))
	//	{
	//		// 1. 총구 위치
	//		XMFLOAT3 gun_shoting_point{ gun->owner()->world_position_vector() };
	//
	//		// 2. 피킹 지점(월드 좌표계)
	//		int sx = mouse_xy_.x;
	//		int sy = mouse_xy_.y;
	//		Object* picked_object = nullptr;
	//		XMVECTOR picking_point_w = scene_->GetPickingPointAtWorld(sx, sy, picked_object);
	//
	//		// 3. 1번에서 2번을 향하는 총알 발사
	//		XMFLOAT3 bullet_dir{};
	//		XMStoreFloat3(&bullet_dir, XMVector3Normalize(picking_point_w - XMLoadFloat3(&gun_shoting_point)));
	//		Mesh* bullet_mesh = Scene::FindMesh("green_cube", scene_->meshes());
	//		gun->FireBullet(bullet_dir, bullet_mesh);
	//	}
	//}

}

void FPSControllerComponent::set_camera_object(Object* value)
{
	camera_object_ = value;
}

void FPSControllerComponent::set_scene(Scene* value)
{
	scene_ = value;
}

