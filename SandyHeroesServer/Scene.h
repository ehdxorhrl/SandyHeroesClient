#pragma once
#include "Object.h"

class Object;
class InputManager;
class InputControllerComponent;
class GameFramework;
class ColliderComponent;

class Scene
{
public:
	Scene() {}
	virtual ~Scene() {};

	virtual void Initialize(GameFramework* game_framework);
	virtual void BuildObject() = 0;

	void BuildScene(const std::string& scene_name);

	virtual bool CheckObjectByObjectCollisions() { return false; };

	void ReleaseMeshUploadBuffer();

	virtual bool ProcessInput(void* p) = 0;

	//반환 값: 월드 좌표계에서 피킹된 지점
	//설명: 스크린 x, y좌표를 받아 피킹 광선과 오브젝트들간 충돌검사를 시행
	//XMVECTOR GetPickingPointAtWorld(float sx, float sy, Object* picked_object);

	virtual void Update(float elapsed_time);

	void UpdateObjectWorldMatrix();

	Object* FindObject(const std::string& object_name);

protected:
	std::list<std::unique_ptr<Object>> object_list_;

	GameFramework* game_framework_{ nullptr };

	InputControllerComponent* main_input_controller_{ nullptr };

};

