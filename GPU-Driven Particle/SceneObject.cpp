#include "SceneObject.h"
#include "CommandContext.h"
#include "Mesh.h"
void GP::SceneObject::Draw(GraphicsContext& gfx)
{
	gfx.SetVertexBuffer(0, m_Mesh->GetVertexBuffer().VertexBufferView());
	gfx.SetIndexBuffer(m_Mesh->GetIndexBuffer().IndexBufferView());
	gfx.DrawIndexed(m_Mesh->GetIndexCount(), 0, 0);
}
