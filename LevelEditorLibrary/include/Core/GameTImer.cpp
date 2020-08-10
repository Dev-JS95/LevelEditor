#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer()
	: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;

}

//Reset() ȣ�� �� ���� total time����
float GameTimer::TotalTime()const
{

	//���� ���°� ��ž�� ���
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
	if (mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime)*mSecondsPerCount);
	}

	//���� ���� ���� ���¶��
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime

	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime)*mSecondsPerCount);
	}
}

//�ð� ���� ����
float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

//ī��Ʈ ����
void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

//Ÿ�̸� ����
void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	//���� ���¿��� Start�� �� �ÿ�
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     
	if (mStopped)
	{
		mPausedTime += (startTime - mStopTime);

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

//Ÿ���� ����
void GameTimer::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;
		mStopped = true;
	}
}

//�ð��� ������ ����Ѵ�
void GameTimer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	//(���� �ð� - ���� �ð�)*�ð��� ī��Ʈ �� = �ð� ����
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	//���� �������� ���� ���� �ð��� �����ð��� ����
	mPrevTime = mCurrTime;

	//������ ������� ����
	//�Ϻ� ���μ������� ������ �߻��� ������ �ֱ� ������
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}
