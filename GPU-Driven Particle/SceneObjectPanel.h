#pragma once

#include "Scene.h"
#include "SceneObject.h"
#include "SDFDebugSettings.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <string>

namespace GP
{
	inline void DrawSceneObjectPanel(
		Scene& scene,
		SceneObject*& selectedObject,
		SDFDebugSettings& sdfDebugSettings)
	{
		const auto& objects = scene.GetObjects();

		if (!ImGui::Begin("Scene Object"))
		{
			ImGui::End();
			return;
		}

		if (objects.empty())
		{
			selectedObject = nullptr;
			ImGui::TextDisabled("No scene objects");
			ImGui::End();
			return;
		}

		bool selectedStillInScene = false;
		for (const auto& object : objects)
		{
			if (object.get() == selectedObject)
			{
				selectedStillInScene = true;
				break;
			}
		}
		if (!selectedStillInScene)
		{
			selectedObject = objects.front().get();
			sdfDebugSettings.resetSliceToCenter = true;
		}

		const char* previewName = selectedObject->GetName().empty()
			? "Unnamed Object"
			: selectedObject->GetName().c_str();

		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::BeginCombo("Object", previewName))
		{
			for (size_t index = 0; index < objects.size(); ++index)
			{
				SceneObject* object = objects[index].get();
				const std::string fallbackName = "Object " + std::to_string(index);
				const char* name = object->GetName().empty() ? fallbackName.c_str() : object->GetName().c_str();
				const bool isSelected = object == selectedObject;

				ImGui::PushID(static_cast<int>(index));
				if (ImGui::Selectable(name, isSelected))
				{
					selectedObject = object;
					sdfDebugSettings.resetSliceToCenter = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		bool visible = selectedObject->IsVisible();
		if (ImGui::Checkbox("Visible", &visible))
			selectedObject->SetVisible(visible);
		ImGui::Text("SDF Collider: %s", selectedObject->IsSDFCollider() ? "Yes" : "No");

		Math::Vector3 position = selectedObject->GetTransform().GetTranslation();
		float positionValues[3] = { position.GetX(), position.GetY(), position.GetZ() };
		if (ImGui::DragFloat3("Position", positionValues, 0.05f))
			selectedObject->GetTransform().SetTranslation(Math::Vector3(positionValues[0], positionValues[1], positionValues[2]));

		float scale = selectedObject->GetTransform().GetScale();
		if (ImGui::DragFloat("Uniform Scale", &scale, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			selectedObject->GetTransform().SetScale(scale);

		ImGui::ColorEdit4("Material Color", selectedObject->GetMaterial().baseColor);

		MeshSDF* sdf = selectedObject->GetMesh()
			? selectedObject->GetMesh()->GetSDF()
			: nullptr;
		const bool hasValidSDF =
			selectedObject->IsSDFCollider() &&
			sdf &&
			sdf->grid.resolution[0] > 0 &&
			sdf->grid.resolution[1] > 0 &&
			sdf->grid.resolution[2] > 0;
		if (hasValidSDF && ImGui::CollapsingHeader("SDF Debug", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Show SDF Debug", &sdfDebugSettings.showDebug);

			const char* axisNames[] = { "X", "Y", "Z" };
			int axisIndex = static_cast<int>(sdfDebugSettings.axis);
			if (ImGui::Combo("Slice Axis", &axisIndex, axisNames, IM_ARRAYSIZE(axisNames)))
			{
				sdfDebugSettings.axis = static_cast<ESDFSliceAxis>(axisIndex);
				sdfDebugSettings.sliceIndex =
					sdf->grid.resolution[axisIndex] / 2;
			}

			const uint32_t selectedAxis =
				static_cast<uint32_t>(sdfDebugSettings.axis);
			if (sdfDebugSettings.resetSliceToCenter)
			{
				sdfDebugSettings.sliceIndex =
					sdf->grid.resolution[selectedAxis] / 2;
				sdfDebugSettings.resetSliceToCenter = false;
			}
			const uint32_t maxSliceIndex =
				sdf->grid.resolution[selectedAxis] - 1;
			sdfDebugSettings.sliceIndex = std::min(
				sdfDebugSettings.sliceIndex,
				maxSliceIndex);

			int sliceIndex = static_cast<int>(sdfDebugSettings.sliceIndex);
			if (ImGui::SliderInt(
				"Slice Index",
				&sliceIndex,
				0,
				static_cast<int>(maxSliceIndex)))
			{
				sdfDebugSettings.sliceIndex = static_cast<uint32_t>(sliceIndex);
			}

			ImGui::Text(
				"Grid: %u x %u x %u",
				sdf->grid.resolution[0],
				sdf->grid.resolution[1],
				sdf->grid.resolution[2]);
		}

		ImGui::End();
	}
}
