#pragma once

#include "Mesh.h"
#include "Scene.h"
#include "SceneObject.h"
#include "imgui/imgui.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace GP
{
	struct SDFStatsSummary
	{
		double bakeMs = 0.0;
		uint64_t allocatedBytes = 0;
		uint64_t voxels = 0;
	};

	inline SDFStatsSummary CalculateSDFStats(const std::vector<const Mesh*>& meshes)
	{
		SDFStatsSummary summary;
		for (const Mesh* mesh : meshes)
		{
			const MeshSDF* sdf = mesh->GetSDF();
			if (sdf == nullptr)
				continue;

			summary.voxels +=
				static_cast<uint64_t>(sdf->grid.resolution[0]) *
				sdf->grid.resolution[1] *
				sdf->grid.resolution[2];
			summary.bakeMs += sdf->bakeMs;
			summary.allocatedBytes += sdf->allocatedBytes;
		}
		return summary;
	}

	inline std::vector<const Mesh*> CollectSceneSDFMeshes(const Scene& scene)
	{
		std::vector<const Mesh*> meshes;
		std::unordered_set<const Mesh*> visited;

		for (const auto& object : scene.GetObjects())
		{
			const Mesh* mesh = object->IsSDFCollider() ? object->GetMesh() : nullptr;
			if (mesh != nullptr && mesh->GetSDF() != nullptr && visited.insert(mesh).second)
				meshes.push_back(mesh);
		}
		return meshes;
	}

	inline void DrawSDFStatsTable(const char* tableId, const std::vector<const Mesh*>& meshes)
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
		if (!ImGui::BeginTable(tableId, 6, tableFlags))
			return;

		ImGui::TableSetupColumn("Mesh");
		ImGui::TableSetupColumn("Resolution");
		ImGui::TableSetupColumn("Voxel (cm)");
		ImGui::TableSetupColumn("Tris");
		ImGui::TableSetupColumn("Bake (ms)");
		ImGui::TableSetupColumn("Alloc (MB)");
		ImGui::TableHeadersRow();

		for (const Mesh* mesh : meshes)
		{
			const MeshSDF* sdf = mesh->GetSDF();
			if (sdf == nullptr)
				continue;

			const std::string& path = mesh->GetSourcePath();
			const size_t slash = path.find_last_of("/\\");
			const char* name = path.empty()
				? "(procedural)"
				: path.c_str() + (slash == std::string::npos ? 0 : slash + 1);

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
			ImGui::TableNextColumn(); ImGui::Text("%ux%ux%u",
				sdf->grid.resolution[0], sdf->grid.resolution[1], sdf->grid.resolution[2]);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", sdf->grid.voxelSize * 100.0f);
			ImGui::TableNextColumn(); ImGui::Text("%u", mesh->GetIndexCount() / 3);
			ImGui::TableNextColumn(); ImGui::Text("%.1f", sdf->bakeMs);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", sdf->allocatedBytes / 1048576.0);
		}

		const SDFStatsSummary summary = CalculateSDFStats(meshes);
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted("Total");
		ImGui::TableNextColumn(); ImGui::Text("%llu voxels", static_cast<unsigned long long>(summary.voxels));
		ImGui::TableNextColumn();
		ImGui::TableNextColumn(); ImGui::Text("%.1f", summary.bakeMs);
		ImGui::TableNextColumn(); ImGui::Text("%.2f", summary.allocatedBytes / 1048576.0);

		ImGui::EndTable();
	}

	inline void DrawSDFStatsPanel(const Scene& scene)
	{
		if (!ImGui::Begin("SDF Stats"))
		{
			ImGui::End();
			return;
		}

		const std::vector<const Mesh*> sceneMeshes = CollectSceneSDFMeshes(scene);

		if (ImGui::CollapsingHeader("Scene SDF Assets", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (sceneMeshes.empty())
				ImGui::TextDisabled("No SDF colliders in the current scene");
			else
				DrawSDFStatsTable("SceneSDFStatsTable", sceneMeshes);
		}


		ImGui::End();
	}
}
