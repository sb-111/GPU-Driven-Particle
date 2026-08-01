#pragma once

#include "Scene.h"
#include "SceneObject.h"
#include "imgui/imgui.h"

#include <string>

namespace GP
{
	inline void DrawSceneObjectPanel(Scene& scene, SceneObject*& selectedObject)
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
			selectedObject = objects.front().get();

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
					selectedObject = object;
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

		ImGui::End();
	}
}
