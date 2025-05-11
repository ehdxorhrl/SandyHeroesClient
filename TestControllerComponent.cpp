#include "stdafx.h"
#include "Object.h"
#include "TestControllerComponent.h"
#include "GameFramework.h"

TestControllerComponent::TestControllerComponent(Object* owner)
	: InputControllerComponent(owner)
{
	is_key_down_['W'] = false;
	is_key_down_['A'] = false;
	is_key_down_['S'] = false;
	is_key_down_['D'] = false;
	is_key_down_['Q'] = false;
	is_key_down_['E'] = false;
	is_key_down_[VK_LBUTTON] = false;

}

Component* TestControllerComponent::GetCopy()
{
	return new TestControllerComponent(*this);
}

bool TestControllerComponent::ProcessInput(UINT message_id, WPARAM w_param, LPARAM l_param, float message_time)
{
	switch (message_id)
	{
	case WM_LBUTTONDOWN:
		if (!is_key_down_[VK_LBUTTON])
		{
			is_key_down_[VK_LBUTTON] = true;
			mouse_xy_.x = LOWORD(l_param);
			mouse_xy_.y = HIWORD(l_param);
		}
		break;
	case WM_LBUTTONUP:
		if (is_key_down_[VK_LBUTTON])
		{
			is_key_down_[VK_LBUTTON] = false;
		}
		break;
	case WM_KEYDOWN:
		switch (w_param)
		{
		case 'W':
			if (!is_key_down_['W'])
			{
				is_key_down_['W'] = true;
					
			}
			break;
		case 'A':
			if (!is_key_down_['A'])
			{
				is_key_down_['A'] = true;
			}
			break;
		case 'S':
			if (!is_key_down_['S'])
			{
				is_key_down_['S'] = true;
			}
			break;
		case 'D':
			if (!is_key_down_['D'])
			{
				is_key_down_['D'] = true;
			}
			break;
		case 'Q':
			if (!is_key_down_['Q'])
			{
				is_key_down_['Q'] = true;
			}
			break;
		case 'E':
			if (!is_key_down_['E'])
			{
				is_key_down_['E'] = true;
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
			if (is_key_down_['W'])
			{
				is_key_down_['W'] = false;
			}
			break;
		case 'A':
			if (is_key_down_['A'])
			{
				is_key_down_['A'] = false;
			}
			break;
		case 'S':
			if (is_key_down_['S'])
			{
				is_key_down_['S'] = false;
			}
			break;
		case 'D':
			if (is_key_down_['D'])
			{
				is_key_down_['D'] = false;
			}
			break;
		case 'Q':
			if (is_key_down_['Q'])
			{
				is_key_down_['Q'] = false;
			}
			break;
		case 'E':
			if (is_key_down_['E'])
			{
				is_key_down_['E'] = false;
			}
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

void TestControllerComponent::Update(float elapsed_time)
{
	XMFLOAT3 velocity{ 0,0,0 };
	float speed = 10;
	XMFLOAT3 look = owner_->look_vector();
	XMFLOAT3 right = owner_->right_vector();
	XMFLOAT3 up{ 0,1,0 };
	if (is_key_down_['W']) velocity += look * speed;
	if (is_key_down_['S']) velocity -= look * speed;
	if (is_key_down_['A']) velocity -= right * speed;
	if (is_key_down_['D']) velocity += right * speed;
	if (is_key_down_['E']) velocity += up * speed;
	if (is_key_down_['Q']) velocity -= up * speed;
	owner_->set_velocity(velocity);
}
