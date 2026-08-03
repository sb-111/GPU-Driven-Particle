#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include "pch.h"
#include "SceneFileDialog.h"

#pragma comment(lib, "Comdlg32.lib")

namespace GameCore { extern HWND g_hWnd; }

bool GP::OpenSceneFileDialog(std::string& outPath)
{
	char path[MAX_PATH] = {};
	OPENFILENAMEA dialog = {};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = GameCore::g_hWnd;
	dialog.lpstrFilter = "Scene JSON (*.json)\0*.json\0All Files\0*.*\0";
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.lpstrInitialDir = "Scenes";
	dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (!GetOpenFileNameA(&dialog))
		return false;

	outPath = path;
	return true;
}
