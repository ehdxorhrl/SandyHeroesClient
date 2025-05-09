#include "stdafx.h"
#include "Scene.h"
//#include "InputManager.h"
//#include "InputControllerComponent.h"
#include "GameFramework.h"


//XMVECTOR Scene::GetPickingPointAtWorld(float sx, float sy, Object* picked_object)
//{
//	//picking ray 계산
//	Object* camera_object = main_camera_->owner();
//
//	XMVECTOR picking_point_w = XMLoadFloat3(&(camera_object->world_position_vector() + (camera_object->world_look_vector() * 100.f)));
//	float picking_length_min = std::numeric_limits<float>::max();
//
//	XMFLOAT4X4 proj = main_camera_->projection_matrix();
//
//	float vx = (2.f * sx / kDefaultFrameBufferWidth - 1.f) / proj(0, 0);
//	float vy = (-2.f * sy / kDefaultFrameBufferHeight + 1.f) / proj(1, 1);
//
//	XMMATRIX view = XMLoadFloat4x4(&main_camera_->view_matrix());
//	XMMATRIX inverse_view = XMMatrixInverse(&XMMatrixDeterminant(view), view);
//
//	//월드좌표계에서 피킹광선
//	XMVECTOR ray_origin{ XMVectorSet(0, 0, 0, 1.f) };
//	XMVECTOR ray_direction{ XMVectorSet(vx, vy, 1.f, 0) };
//
//	ray_origin = XMVector3TransformCoord(ray_origin, inverse_view);
//	ray_direction = XMVector3Normalize(XMVector3Transform(ray_direction, inverse_view));
//
//	//TODO: 피킹 처리 리펙토링
//	for (const auto& mesh : meshes_)
//	{
//		if (mesh->name() == "Debug_Mesh")
//			continue;
//		const auto& mesh_component_list = mesh->mesh_component_list();
//		for (const auto& mesh_component : mesh_component_list)
//		{
//			XMMATRIX world = XMLoadFloat4x4(&mesh_component->owner()->world_matrix());
//			XMMATRIX inverse_world = XMMatrixInverse(&XMMatrixDeterminant(world), world);
//			XMMATRIX to_local = inverse_view * inverse_world;
//
//			// 메쉬 로컬좌표의 피킹반직선
//			XMVECTOR ray_origin_local = XMVector3TransformCoord(ray_origin, to_local);
//			XMVECTOR ray_direction_local = XMVector3Normalize(XMVector3Transform(ray_direction, to_local));
//
//			float t_min{ 0 }; // 반직선 교점 매개변수 최저값, 즉 가장 가까운 교점의 매개변수
//			if (mesh->bounds().Intersects(ray_origin_local, ray_direction_local, t_min))
//			{
//				auto& positions = mesh->positions();
//				auto& indices_array = mesh->indices_array();
//
//				t_min = std::numeric_limits<float>::max();
//				for (auto& indices : indices_array)
//				{
//					//TODO: mesh가 triangle list 인지 strip 인지 판정해야함
//					float t{ 0 };
//					for (int i = 0; i < indices.size(); i += 3)
//					{
//						UINT i0 = indices[i + 0];
//						UINT i1 = indices[i + 1];
//						UINT i2 = indices[i + 2];
//
//						XMVECTOR v0 = XMLoadFloat3(&positions[i0]);
//						XMVECTOR v1 = XMLoadFloat3(&positions[i1]);
//						XMVECTOR v2 = XMLoadFloat3(&positions[i2]);
//
//						if (TriangleTests::Intersects(ray_origin_local, ray_direction_local, v0, v1, v2, t))
//						{
//							if (t < t_min) // 가장 반직선에 가까운 삼각형과의 교점 매개변수
//							{
//								t_min = t;
//							}
//						}
//					}
//				}	//forend indices_array
//				if (t_min < std::numeric_limits<float>::max()) // t_min 유효한지 실제 메쉬와 부딪힌건지 체크
//				{
//					// 피킹지점을 월드 좌표계와 비교
//					XMVECTOR picking_point = ray_origin_local + (ray_direction_local * t_min);
//					picking_point = XMVector3TransformCoord(picking_point, world);
//					float length = XMVectorGetX(XMVector3Length(picking_point - ray_origin));
//					if (length < picking_length_min)
//					{
//						picking_length_min = length;
//						picking_point_w = picking_point;
//						picked_object = mesh_component->owner();
//					}
//				}
//			}	//ifend mesh->bounds().Intersects
//		}
//	}
//
//	return picking_point_w;
//}

void Scene::Update(float elapsed_time)
{
	for (const std::unique_ptr<Object>& object : object_list_)
	{
		object->Update(elapsed_time);
	}
}

void Scene::UpdateObjectWorldMatrix()
{
	for (const std::unique_ptr<Object>& object : object_list_)
	{
		object->UpdateWorldMatrix(nullptr);
	}
}

//Object* Scene::FindObject(const std::string& object_name)
//{
//	auto it = std::find_if(object_list_.begin(), object_list_.end(), [&object_name](const std::unique_ptr<Object>& object) {
//		return object.get()->name() == object_name;
//		});
//
//	if (it != object_list_.end())
//	{
//		return (*it).get();
//	}
//
//	return nullptr;
//}

//ModelInfo* Scene::FindModelInfo(const std::string& name)
//{
//	auto it = std::find_if(model_infos_.begin(), model_infos_.end(), [&name](const std::unique_ptr<ModelInfo>& object) {
//		return object.get()->model_name() == name;
//		});
//
//	if (it != model_infos_.end())
//	{
//		return (*it).get();
//	}
//	return nullptr;
//}

void Scene::Initialize(GameFramework* game_framework)
{
	game_framework_ = game_framework;
	//
	//BuildShader(device, root_signature);
	//BuildMesh(device, command_list);
	//BuildMaterial(device, command_list);
	//BuildObject(device, command_list);
	//BuildFrameResources(device);
	//BuildDescriptorHeap(device);
	//BuildConstantBufferViews(device);
	//BuildShaderResourceViews(device);
}


using namespace file_load_util;
void Scene::BuildScene(const std::string& scene_name)
{
	//std::ifstream scene_file{ "./Resource/Model/" + scene_name + ".bin", std::ios::binary };
	//
	//int root_object_count = ReadFromFile<int>(scene_file);
	//
	//std::string load_token;
	//
	//for (int i = 0; i < root_object_count; ++i)
	//{
	//	ReadStringFromFile(scene_file, load_token);
	//	if (load_token[0] == '@')
	//	{
	//		load_token.erase(0, 1);
	//		object_list_.emplace_back();
	//		object_list_.back().reset(FindModelInfo(load_token)->GetInstance());
	//
	//		ReadStringFromFile(scene_file, load_token);
	//		XMFLOAT4X4 transfrom = ReadFromFile<XMFLOAT4X4>(scene_file);
	//		object_list_.back()->set_transform_matrix(transfrom);
	//	}
	//	else
	//	{
	//		std::string object_name = load_token;
	//
	//		ReadStringFromFile(scene_file, load_token); // <Transfrom>
	//		XMFLOAT4X4 transfrom = ReadFromFile<XMFLOAT4X4>(scene_file);
	//
	//		model_infos_.push_back(std::make_unique<ModelInfo>("./Resource/Model/" + object_name + ".bin", meshes_, materials_));
	//
	//		object_list_.emplace_back();
	//		object_list_.back().reset(model_infos_.back()->GetInstance());
	//
	//		object_list_.back()->set_transform_matrix(transfrom);
	//
	//	}
	//}
}
