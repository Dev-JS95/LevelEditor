#pragma once


class World;

class Level {


public:
	void Tick();

	bool IsClear() { return bIsClear; }

private:
	World* mCurrentWorld;
	bool bIsClear;
};