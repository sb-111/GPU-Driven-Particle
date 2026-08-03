#pragma once

#include <string>

namespace GP
{
	// 사용자가 취소하면 false를 반환하고 outPath는 변경 X
	bool OpenSceneFileDialog(std::string& outPath);
}
