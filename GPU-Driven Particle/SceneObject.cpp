#include "SceneObject.h"
#include "CommandContext.h"
#include "Mesh.h"

const GP::MaterialCB& GP::SceneObject::ResolveMaterial(const SubMesh& section) const
{
	// Override = mtl 사용 X
	if (m_ColorSource == EColorSource::Override)
		return m_Material;

	// mtl 색 사용
	// mtl 없는 메시는 오브젝트 색으로 fallback
	const MaterialCB* material = m_Mesh->FindMaterial(section.materialSlot);
	return material ? *material : m_Material;
}

void GP::SceneObject::Draw(GraphicsContext& gfx, uint32_t materialRootIndex)
{
	gfx.SetVertexBuffer(0, m_Mesh->GetVertexBuffer().VertexBufferView());
	gfx.SetIndexBuffer(m_Mesh->GetIndexBuffer().IndexBufferView());

	// 섹션별로 머티리얼 다르게 Draw
	for (const SubMesh& section : m_Mesh->GetSections())
	{
		const MaterialCB& material = ResolveMaterial(section);
		gfx.SetDynamicConstantBufferView(materialRootIndex, sizeof(MaterialCB), &material);
		gfx.DrawIndexed(section.indexCount, section.indexOffset, 0);
	}
}
