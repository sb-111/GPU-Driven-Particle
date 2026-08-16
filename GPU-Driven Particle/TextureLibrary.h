#pragma once

#include "Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace GP
{
	// 파티클 스프라이트 DDS를 경로 기준으로 캐시
	class TextureLibrary
	{
	public:
		bool LoadFromDirectories(const std::vector<std::string>& directories);

		const std::vector<std::string>& GetAssetPaths() const { return m_AssetPaths; }
		const Texture* Find(const std::string& path) const;
		const Texture& GetFallback() const;

	private:
		std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
		std::vector<std::string> m_AssetPaths;
	};
}
