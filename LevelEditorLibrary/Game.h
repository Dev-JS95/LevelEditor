#pragma once


class Level;

enum class EndReason {
	Exit,
	Error
};

class Game {
	

public:
	EndReason Play();

public:
	Level* mCurrentLevel;
};