#include "Game.h"
#include "Level.h"
#include <exception>
using namespace std;

EndReason Game::Play()
{
    try {
        while (mCurrentLevel->IsClear()) {
            mCurrentLevel->Tick();
        }

        return EndReason::Exit;
    }
    catch (exception e) {
        return EndReason::Error;
    }
    return EndReason::Error;
}
