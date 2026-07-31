#pragma once
#include "pch.h"
#include "MeshLoader.h"
namespace GP
{
	class Mesh;

	/*	Asset Cache */
	class MeshLibrary
	{
	public:
		MeshLibrary();
		~MeshLibrary();
		Mesh* Get(const char* path, ENormalMode mode = ENormalMode::Smooth);
		const std::vector<std::unique_ptr<Mesh>>& GetAll() const {return m_Meshes;}
		void Clear();
	private:
		std::vector<std::unique_ptr<Mesh>> m_Meshes;
		std::unordered_map<std::string, Mesh*> m_MeshTable;
	};

}
