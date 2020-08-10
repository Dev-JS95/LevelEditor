#pragma once
#include "ResourceType.h"


class GeometryGenerator {
	//�޼���
public:
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	//Method
	///<summary>
	/// �ڽ� ����
	/// ���� m �� n ��
	///</summary>
	StaticMeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Sphere ����
	/// slices�� stacks �Ķ���ͷ� �׼����̼� ���� ����
	///</summary>
	StaticMeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// geosphere ����
	///</summary>
	StaticMeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	///<summary>
	/// y-�࿡ ������ �Ǹ��� ����
	//  slices�� stacks �Ķ���ͷ� �׼����̼� ���� ����
	///</summary>
	StaticMeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	///  xz��鿡 m x n �׸��� ����
	///</summary>
	StaticMeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	///<summary>
	/// ��ũ���� ���ĵ� quad ����
	/// ����Ʈ���μ��̰� ȭ�� ȿ���� ���� ����
	///</summary>
	StaticMeshData CreateQuad(float x, float y, float w, float h, float depth);

private:
	//�޽� ����
	void Subdivide(StaticMeshData& meshData);
	//������ �߾��� ���
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	//�Ǹ��� ž ĸ ���
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, StaticMeshData& meshData);
	//�Ǹ��� ���� ĸ ���
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, StaticMeshData& meshData);
};