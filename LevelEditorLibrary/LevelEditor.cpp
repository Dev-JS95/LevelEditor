#include "include/LevelEditor.h"

LevelEditor::LevelEditor(HWND handle) :
	hViewport(handle),
	mRenderer(handle),
	mTimer(),
	mBrowser()
{
}


void LevelEditor::Tick()
{
	Update();
	Draw();
}

void LevelEditor::Update()
{
	mTimer.Tick();
	//mRenderer.CalculateFrameStatus(timer);
	mRenderer.Update(mTimer);
}

void LevelEditor::Draw()
{
	mRenderer.Render();
}
