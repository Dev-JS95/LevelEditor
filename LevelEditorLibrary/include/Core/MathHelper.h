#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

class MathHelper
{
public:

	// float [0, 1) �� ���� 
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// float [a, b) �� ����
	static float RandF(float a, float b)
	{
		return a + RandF() * (b - a);
	}


	// int int [a, b) �� ����
	static int Rand(int a, int b)
	{
		return a + rand() % ((b - a) + 1);
	}

	//Ÿ�� T���� a�� b�� ���� �� ����
	template<typename T>
	static T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	//Ÿ�� T���� a�� b�� ū �� ����
	template<typename T>
	static T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	//Ÿ�� T���� a���� b���� t������ ����
	template<typename T>
	static T Lerp(const T& a, const T& b, float t)
	{
		return a + (b - a) * t;
	}

	//Ÿ�� T���� x�� low�� high���� ������ �� ����
	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x);
	}

	// Returns the polar angle of the point (x,y) in [0, 2*PI).
	static float AngleFromXY(float x, float y);

	//���� ��ǥ�� ������ǥ�� ��ȯ
	static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
	{
		return DirectX::XMVectorSet(
			radius * sinf(phi) * cosf(theta),
			radius * cosf(phi),
			radius * sinf(phi) * sinf(theta),
			1.0f);
	}

	
	//�����
	static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M)
	{
		DirectX::XMMATRIX A = M;
		A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
		return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
	}

	//�׵����
	static DirectX::XMFLOAT4X4 Identity4x4()
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		
		return I;
	}

	//���� Vector3�� ����
	static DirectX::XMVECTOR RandUnitVec3();

	//���� �ݱ����� Vector3�� ����
	static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

	//Float_Infinity
	static const float Infinity;

	//Pi��
	static const float Pi;

};
