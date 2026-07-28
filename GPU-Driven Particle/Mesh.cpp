#include "Mesh.h"
#include "RenderTypes.h"

void GP::Mesh::CreateSphere(float radius, uint32_t rings, uint32_t segments, const float color[4])
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;
	verts.reserve((rings + 1) * (segments + 1));

	for (uint32_t r = 0; r <= rings; ++r)
	{
		float phi = 3.14159265f * r / rings;
		float y = cosf(phi);
		float ringRadius = sinf(phi);
		for (uint32_t c = 0; c <= segments; ++c)
		{
			float theta = 2.0f * 3.14159265f * c / segments;

			Vertex v = {};
			v.position[0] = radius * ringRadius * cosf(theta);
			v.position[1] = radius * y;
			v.position[2] = radius * ringRadius * sinf(theta);

			v.color[0] = color[0];
			v.color[1] = color[1];
			v.color[2] = color[2];
			v.color[3] = color[3];
			verts.push_back(v);
		}
	}

	for (uint32_t r = 0; r < rings; ++r)
	{
		for (uint32_t c = 0; c < segments; ++c)
		{
			uint32_t a = r * (segments + 1) + c;
			uint32_t b = a + (segments + 1);

			if (r + 1 != rings) 
			{
				indices.push_back(a);
				indices.push_back(b);
				indices.push_back(b + 1);
			}
			if (r != 0)
			{
				indices.push_back(a);
				indices.push_back(b + 1);
				indices.push_back(a + 1);
			}
		}
	}

	m_IndexCount = (uint32_t)indices.size();
	m_VertexBuffer.Create(L"Mesh VB", (uint32_t)verts.size(), sizeof(Vertex), verts.data());
	m_IndexBuffer.Create(L"Mesh IB", m_IndexCount, sizeof(uint32_t), indices.data());

	m_BoundsMin = Math::Vector3(-radius);
	m_BoundsMax = Math::Vector3(radius);
}
