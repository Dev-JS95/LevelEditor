#include "AnimationHelper.h"


using namespace DirectX;

Keyframe::Keyframe()
	: TimePos(0.0f),
	Translation(0.0f, 0.0f, 0.0f),
	Scale(1.0f, 1.0f, 1.0f),
	RotationQuat(0.0f, 0.0f, 0.0f, 1.0f)
{
}

Keyframe::~Keyframe()
{
}

float BoneAnimation::GetStartTime()const
{
	return Keyframes.front().TimePos;
}

float BoneAnimation::GetEndTime()const
{
	float f = Keyframes.back().TimePos;

	return f;
}

void BoneAnimation::Interpolate(float t, XMFLOAT4X4& M)const
{
	if (t <= Keyframes.front().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.front().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.front().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (t >= Keyframes.back().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&Keyframes.back().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.back().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.back().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		for (UINT i = 0; i < Keyframes.size() - 1; ++i)
		{
			if (t >= Keyframes[i].TimePos && t <= Keyframes[i + 1].TimePos)
			{
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);

				XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				XMVECTOR s1 = XMLoadFloat3(&Keyframes[i + 1].Scale);

				XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Translation);
				XMVECTOR p1 = XMLoadFloat3(&Keyframes[i + 1].Translation);

				XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].RotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&Keyframes[i + 1].RotationQuat);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}



float AnimationClip::GetClipStartTime()const
{
	float t = MathHelper::Infinity;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		if (BoneAnimations[i].Keyframes.size() == 0)
			continue;
		t = MathHelper::Min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime()const
{
	float t = 0.0f;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		if (BoneAnimations[i].Keyframes.size() == 0)
			continue;
		t = MathHelper::Max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms)const
{
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		if (BoneAnimations[i].Keyframes.size() == 0) {
			boneTransforms[i] = MathHelper::Identity4x4();
		}
		else {
			BoneAnimations[i].Interpolate(t, boneTransforms[i]);
		}

	}
}

float SkinnedData::GetClipStartTime(const std::string& clipName)const
{
	auto clip = mAnimations.find(clipName);
	float startTime;
	if (clip->second.BoneAnimations.size() != 0)
		startTime = clip->second.GetClipStartTime();
	else
		startTime = 0.0f;
	return startTime;
}

float SkinnedData::GetClipEndTime(const std::string& clipName)const
{
	auto clip = mAnimations.find(clipName);
	float endTime;
	if (clip->second.BoneAnimations.size() != 0)
		endTime = clip->second.GetClipEndTime();
	else
		endTime = 0.0f;

	return endTime;
}

UINT SkinnedData::BoneCount()const
{
	return (UINT)mSkeleton->mJoints.size();
}


void SkinnedData::Set(Skeleton* skeleton, std::unordered_map<std::string, AnimationClip>& animations)
{
	mSkeleton = skeleton;
	mAnimations = animations;
}

void SkinnedData::GetFinalTransforms(const std::string& clipName, float timePos, std::vector<XMFLOAT4X4>& finalTransforms)const
{
	auto& joint = mSkeleton->mJoints;
	UINT numBones = (UINT)joint.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);

	// Ŭ���� ���� �ð��� �°� �����Ѵ�
	auto clip = mAnimations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	//
	// ��Ʈ�� ��ȯ ���ϱ�
	//

	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// ��Ʈ���� �θ��尡 ���� ������ �̴� �ڽ��� ���� ����� ����
	toRootTransforms[0] = toParentTransforms[0];

	// �ڽĵ��� ��Ʈ�� ��ȯ�� ���Ѵ�
	for (UINT i = 1; i < numBones; ++i)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

		int parentIndex = joint[i].mParentIndex;
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// �����º�ȯ�� ��Ʈ�� ��ȯ�� ���Ͽ� ������ȯ�� ��´�
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&joint[i].mGlobalBindposeInverse);	//����->joint���� ��ȯ ���
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);	//joint->root���� ��ȯ ���
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		
		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
}