#include "TextureLibrary.h"

#include "TextureLoader.h"
#include "Utility.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
	bool HasDDSExtension(const std::filesystem::path& path)
	{
		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return extension == ".dds";
	}

	bool IsSkyboxTexture(const std::filesystem::path& root, const std::filesystem::path& path)
	{
		const std::filesystem::path relativePath = path.lexically_relative(root);
		return !relativePath.empty() && *relativePath.begin() == "Skybox";
	}
}

bool GP::TextureLibrary::LoadFromDirectories(const std::vector<std::string>& directories)
{
	m_Textures.clear();
	m_AssetPaths.clear();

	namespace fs = std::filesystem;
	std::vector<fs::path> texturePaths;
	for (const std::string& directory : directories)
	{
		const fs::path root(directory);
		std::error_code error;
		if (!fs::exists(root, error))
			continue;

		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root, error))
		{
			if (error)
				break;
			if (!entry.is_regular_file(error) || !HasDDSExtension(entry.path()))
				continue;
			if (IsSkyboxTexture(root, entry.path()))
				continue;

			texturePaths.push_back(entry.path());
		}
	}

	std::sort(texturePaths.begin(), texturePaths.end());
	for (const fs::path& texturePath : texturePaths)
	{
		const std::string assetPath = texturePath.generic_string();
		auto texture = std::make_unique<Texture>();
		if (!LoadDDSTexture(*texture, assetPath.c_str()))
		{
			Utility::Printf("[Texture] %s 로드 실패\n", assetPath.c_str());
			continue;
		}

		m_AssetPaths.push_back(assetPath);
		m_Textures.emplace(assetPath, std::move(texture));
	}

	return !m_AssetPaths.empty();
}

const Texture* GP::TextureLibrary::Find(const std::string& path) const
{
	const auto it = m_Textures.find(path);
	return it != m_Textures.end() ? it->second.get() : nullptr;
}

const Texture& GP::TextureLibrary::GetFallback() const
{
	ASSERT(!m_AssetPaths.empty(), "파티클 텍스처가 하나도 없습니다.");
	return *m_Textures.at(m_AssetPaths.front());
}
