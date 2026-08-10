#pragma once
#include "Texture.h"

#include <cstdint>
#include <fstream>
#include <vector>

namespace GP
{
	static bool LoadDDSTexture(Texture& tex, const char* path)
	{
		// binary: 바이트 그대로 읽어라, ate: at end - 열자마자 커서 파일 끝에
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return false;
		size_t size = (size_t)file.tellg(); // 지금 커서 위치 = 파일 크기
		std::vector<uint8_t> data(size); // 크기만큼 공간 확보
		file.seekg(0); // 커서를 처음으로 되감기. 이거 빼먹으면 끝에서부터 0바이트 읽음
		file.read((char*)data.data(), size); // 처음부터 size 바이트를 통으로 읽기
		return tex.CreateDDSFromMemory(data.data(), size, false);
	}
}
