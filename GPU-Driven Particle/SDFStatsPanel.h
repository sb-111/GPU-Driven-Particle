#pragma once

#include "MeshLibrary.h"
#include "Mesh.h"
#include "GpuStats.h"
#include "imgui/imgui.h"

namespace GP
{
	// MeshLibrary가 소유한 메시들의 SDF 베이크 시간/VRAM을 표로 표시
	inline void DrawSDFStatsPanel(const MeshLibrary& meshLibrary)
	{
		if (!ImGui::Begin("SDF Stats"))
		{
			ImGui::End();
			return;
		}

		double totalBakeMs = 0.0;
		uint64_t totalAllocated = 0;
		uint64_t totalVoxels = 0;

		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("SDFStatsTable", 5, tableFlags))
		{
			ImGui::TableSetupColumn("Mesh");
			ImGui::TableSetupColumn("Resolution");
			ImGui::TableSetupColumn("Tris");
			ImGui::TableSetupColumn("Bake (ms)");
			ImGui::TableSetupColumn("Alloc (MB)");
			ImGui::TableHeadersRow();

			for (const auto& mesh : meshLibrary.GetAll())
			{
				const MeshSDF* sdf = mesh->GetSDF();
				if (!sdf)
					continue;

				// 경로에서 파일명만
				const std::string& path = mesh->GetSourcePath();
				const size_t slash = path.find_last_of("/\\");
				const char* name = path.empty()
					? "(procedural)"
					: path.c_str() + (slash == std::string::npos ? 0 : slash + 1);

				const uint64_t voxels =
					(uint64_t)sdf->grid.resolution[0] * sdf->grid.resolution[1] * sdf->grid.resolution[2];

				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
				ImGui::TableNextColumn(); ImGui::Text("%ux%ux%u",
					sdf->grid.resolution[0], sdf->grid.resolution[1], sdf->grid.resolution[2]);
				ImGui::TableNextColumn(); ImGui::Text("%u", mesh->GetIndexCount() / 3);
				ImGui::TableNextColumn(); ImGui::Text("%.1f", sdf->bakeMs);
				ImGui::TableNextColumn(); ImGui::Text("%.2f", sdf->allocatedBytes / 1048576.0);

				totalBakeMs += sdf->bakeMs;
				totalAllocated += sdf->allocatedBytes;
				totalVoxels += voxels;
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted("Total");
			ImGui::TableNextColumn(); ImGui::Text("%llu voxels", (unsigned long long)totalVoxels);
			ImGui::TableNextColumn();
			ImGui::TableNextColumn(); ImGui::Text("%.1f", totalBakeMs);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", totalAllocated / 1048576.0);

			ImGui::EndTable();
		}

		const VideoMemoryInfo vram = QueryVideoMemory();
		if (vram.valid)
		{
			ImGui::Text("GPU VRAM: %.0f / %.0f MB",
				vram.currentUsage / 1048576.0, vram.budget / 1048576.0);
			if (vram.currentUsage > 0)
				ImGui::Text("SDF share: %.2f MB (%.2f%% of usage)",
					totalAllocated / 1048576.0,
					100.0 * (double)totalAllocated / (double)vram.currentUsage);
		}
		else
			ImGui::TextDisabled("GPU VRAM: query failed");

		ImGui::End();
	}
}
