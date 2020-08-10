#pragma once

#include "../Core/MathHelper.h"
#include <vector>
#include <unordered_map>

//�ִϸ��̼� Ű������
struct Keyframe {
	Keyframe();
	~Keyframe();

	float TimePos;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT3 Scale;
	DirectX::XMFLOAT4 RotationQuat;
};

//�� ������ Ű������ ���� �迭 
struct BoneAnimation {
	std::vector<Keyframe> Keyframes;

	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float t, DirectX::XMFLOAT4X4& M)const;
};

//����Ʈ�� ����ü
struct Joint {
	int mParentIndex;	//�θ� �ε���
	std::string mName;	//����Ʈ �̸�
	DirectX::XMFLOAT4X4 mGlobalBindposeInverse; //�� ������(SRT)
};

//���̷����� ����ü
struct Skeleton {
	std::vector<Joint> mJoints;
	bool IsAnimationable = false;
};

//���� �ִϸ��̼��� ���� �迭
struct AnimationClip {
	std::vector<BoneAnimation> BoneAnimations;
	Skeleton* Skeleton;	//����� ����

	float GetClipStartTime() const;
	float GetClipEndTime() const;

	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms) const;
};

//���̷��� - �ִϸ��̼� ����
class SkinnedData {

private:
	Skeleton* mSkeleton;	//Joint�� ��������
	std::unordered_map<std::string, AnimationClip> mAnimations;	//�ִϸ��̼� Ŭ�� ����

public:
	UINT BoneCount() const;

	float GetClipStartTime(const std::string& clipName) const;	//Ŭ�� ���۽ð�
	float GetClipEndTime(const std::string& clipName) const;	//Ŭ�� �� �ð�

	void Set(Skeleton* skeleton, std::unordered_map<std::string, AnimationClip>& animations);	//���̷���� �ִϸ��̼� Ŭ�� ����

	void GetFinalTransforms(const std::string& clipName, float timePos, std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const;	//���� ��ȯ ��

};
