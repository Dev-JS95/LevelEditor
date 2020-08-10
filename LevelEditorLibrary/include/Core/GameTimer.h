#pragma once

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; 
	float DeltaTime()const; 

	void Reset(); // Ÿ�̸Ӹ� ����
	void Start(); // Ÿ�̸Ӹ� ���߰� �ٽ� ���� �� �� mPausedTime ���� �� �簳�� ���� �� ����
	void Stop();  // Ÿ�̾ ���� �� ������ ���� �� ����
	void Tick();  // �ð��� ������ ����

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;	//���� �ð�
	__int64 mPausedTime;	//�����ߴٰ� ���� �ð���
	__int64 mStopTime;	//Stop�� �ð�
	__int64 mPrevTime;	//ƽ ��� �� �����ð�
	__int64 mCurrTime;	//���� �ð�

	bool mStopped;
};
