#pragma once

#include "targetver.h"
#include "Packet.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#define NOMINMAX						// min, max 메크로를 사용하지 않음
// Windows 헤더 파일
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

// C++ 헤더 파일
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <concurrent_unordered_map.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <numeric>
#include <thread>
#include <atomic>
#include <mutex>

// DirectX 관련 헤더 파일 및 선언문
//#include <wrl.h>
//#include <shellapi.h>

//#include <d3d12.h>
//#include <dxgi1_4.h>
//#include <D3Dcompiler.h>
#include <DirectXMath.h>
//#include <DirectXPackedVector.h>
//#include <DirectXColors.h>
//#include <DirectXCollision.h>

//#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "d3d12.lib")
//#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "dxguid.lib")

//#include "d3dx12.h"

//using Microsoft::WRL::ComPtr;
using namespace DirectX;

// 상수값
//constexpr int kDefaultFrameBufferWidth = 1366;
//constexpr int kDefaultFrameBufferHeight = 768;
//constexpr UINT kDefaultRefreshRate = 60;
//constexpr int kMaxBoneCount = 128; //skinned mesh의 본 최대 개수
//constexpr int kMaxLights = 16;		// 조명처리의 최대 개수

//enum class RootParameterIndex {
//	kWorldMatrix = 0, kBoneTransform, kBoneOffset, kRenderPass,
//	kMaterial, kAlbedoMap, kSpecGlosMap, kMetalGlosMap, kEmissionMap, kNormalMap, kCubeMap
//};

enum IO_OP { IO_RECV, IO_SEND, IO_ACCEPT };

// 오류 검출 함수


// 유틸 함수

inline bool IsZero(float value, float epsilon = std::numeric_limits<float>::epsilon() * 100)
{
	return std::fabs(value) < epsilon;
}

//namespace d3d_util
//{
//	// 업로드 버퍼를 이용하여 디폴트버퍼 생성
//	// 주의: 이 함수가 실행되고 gpu 명령이 전부 실행된 뒤 upload_buffer를 해제해야한다.
//	ComPtr<ID3D12Resource> CreateDefaultBuffer(
//		ID3D12Device* device,
//		ID3D12GraphicsCommandList* command_list,
//		const void* init_data,
//		UINT64 byte_size,
//		ComPtr<ID3D12Resource>& upload_buffer);
//
//	// 상수 버퍼는 항상 256바이트의 배수여야 한다.
//	// 이 함수는 버퍼의 사이즈를 256의 배수로 계산해주는 함수다.
//	UINT CalculateConstantBufferSize(UINT byte_size);
//}

namespace xmath_util_float3
{
	inline XMFLOAT3 Add(const XMFLOAT3& vector1, const XMFLOAT3& vector2)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMLoadFloat3(&vector1) + XMLoadFloat3(&vector2));
		return r_value;
	}

	inline XMFLOAT3 Subtract(const XMFLOAT3& vector1, const XMFLOAT3& vector2)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMLoadFloat3(&vector1) - XMLoadFloat3(&vector2));
		return r_value;
	}

	inline XMFLOAT3 ScalarProduct(const XMFLOAT3& vector, float scalar)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMLoadFloat3(&vector) * scalar);
		return r_value;
	}

	inline float DotProduct(const XMFLOAT3& vector1, const XMFLOAT3& vector2)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMVector3Dot(XMLoadFloat3(&vector1), XMLoadFloat3(&vector2)));
		return r_value.x;
	}

	inline XMFLOAT3 CrossProduct(const XMFLOAT3& vector1, const XMFLOAT3& vector2)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMVector3Cross(XMLoadFloat3(&vector1), XMLoadFloat3(&vector2)));
		return r_value;
	}

	inline XMFLOAT3 Normalize(const XMFLOAT3& vector)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMVector3Normalize(XMLoadFloat3(&vector)));
		return r_value;
	}

	inline float Length(const XMFLOAT3& vector)
	{
		XMFLOAT3 r_value;
		XMStoreFloat3(&r_value, XMVector3Length(XMLoadFloat3(&vector)));
		return r_value.x;
	}

}

namespace xmath_util_float4
{
	inline XMFLOAT4 Add(const XMFLOAT4& vector1, const XMFLOAT4& vector2)
	{
		XMFLOAT4 r_value;
		XMStoreFloat4(&r_value, XMLoadFloat4(&vector1) + XMLoadFloat4(&vector2));
		return r_value;
	}

	inline XMFLOAT4 Subtract(const XMFLOAT4& vector1, const XMFLOAT4& vector2)
	{
		XMFLOAT4 r_value;
		XMStoreFloat4(&r_value, XMLoadFloat4(&vector1) - XMLoadFloat4(&vector2));
		return r_value;
	}

	inline XMFLOAT4 ScalarProduct(const XMFLOAT4& vector, float scalar)
	{
		XMFLOAT4 r_value;
		XMStoreFloat4(&r_value, XMLoadFloat4(&vector) * scalar);
		return r_value;
	}

}

namespace xmath_util_float4x4
{
	inline XMFLOAT4X4 Identity()
	{
		XMFLOAT4X4 r_value;
		XMStoreFloat4x4(&r_value, XMMatrixIdentity());
		return r_value;
	}

	inline XMFLOAT4X4 PerspectiveFovLH(float fov_angle_y, float aspect_ratio, float near_z, float far_z)
	{
		XMFLOAT4X4 r_value;
		XMStoreFloat4x4(&r_value, XMMatrixPerspectiveFovLH(fov_angle_y, aspect_ratio, near_z, far_z));
		return(r_value);
	}

	inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& matrix1, const XMFLOAT4X4& matrix2)
	{
		XMFLOAT4X4 r_value;
		XMStoreFloat4x4(&r_value, XMLoadFloat4x4(&matrix1) * XMLoadFloat4x4(&matrix2));
		return r_value;
	}

	inline XMFLOAT4X4 TransPose(const XMFLOAT4X4& matrix)
	{
		XMFLOAT4X4 r_value;
		XMStoreFloat4x4(&r_value, XMMatrixTranspose(XMLoadFloat4x4(&matrix)));
		return r_value;
	}

	inline XMFLOAT4X4 ScalarProduct(const XMFLOAT4X4& matrix, float scalar)
	{
		XMFLOAT4X4 r_value;
		XMStoreFloat4x4(&r_value, XMLoadFloat4x4(&matrix) * scalar);
		return r_value;
	}

	inline XMFLOAT4X4 Interpolate(const XMFLOAT4X4& matrix1, const XMFLOAT4X4& matrix2, float t)
	{
		XMFLOAT4X4 r_value;
		XMVECTOR s1, r1, t1;
		XMVECTOR s2, r2, t2;
		XMMatrixDecompose(&s1, &r1, &t1, XMLoadFloat4x4(&matrix1));
		XMMatrixDecompose(&s2, &r2, &t2, XMLoadFloat4x4(&matrix2));
		XMVECTOR S = XMVectorLerp(s1, s2, t);
		XMVECTOR R = XMQuaternionSlerp(r1, r2, t);
		XMVECTOR T = XMVectorLerp(t1, t2, t);

		XMStoreFloat4x4(&r_value, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
		return r_value;
	}


}

// xmf 관련 연산자 오버로딩
inline XMFLOAT3 operator+(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return xmath_util_float3::Add(lhs, rhs); }
inline void operator+=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { lhs = xmath_util_float3::Add(lhs, rhs); }
inline XMFLOAT3 operator-(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return xmath_util_float3::Subtract(lhs, rhs); }
inline void operator-=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { lhs = xmath_util_float3::Subtract(lhs, rhs); }
inline XMFLOAT3 operator*(const XMFLOAT3& lhs, const float& rhs) { return xmath_util_float3::ScalarProduct(lhs, rhs); }
inline XMFLOAT4X4 operator*(const XMFLOAT4X4& lhs, const XMFLOAT4X4& rhs) { return xmath_util_float4x4::Multiply(lhs, rhs); }

namespace file_load_util
{
#ifdef _DEBUG
	// 로드 토큰이 잘못되었으면 디버그용 문장을 출력해주는 함수
	inline void PrintDebugStringLoadTokenError(const std::string& file_name,
		const std::string& load_token, const std::string& compare_token)
	{
		if (load_token != compare_token)
		{
			std::string temp = file_name + "을 로드하는 과정에서 " + load_token + "부분에 문제가 생겼습니다.";
			std::wstring debug_str;
			debug_str.assign(temp.begin(), temp.end());
			OutputDebugString(debug_str.c_str());
			throw;
		}

	}
#endif // DEBUG

	inline BYTE ReadStringFromFile(std::ifstream& file, std::string& str)
	{
		BYTE strLength = 0;
		file.read((char*)&strLength, sizeof(BYTE));
		str.resize(strLength);
		file.read(&str[0], strLength);
		return strLength;
	}

	template <class T>
	inline T ReadFromFile(std::ifstream& file)
	{
		T rvalue;
		file.read((char*)&rvalue, sizeof(T));
		return rvalue;
	}

	template <class T>
	inline void ReadFromFile(std::ifstream& file, T* buffer, const int& count)
	{
		file.read((char*)buffer, sizeof(T) * count);
	}
}