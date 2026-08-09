#include "Mesh.h"
#include "RenderTypes.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

void GP::Mesh::Create(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices)
{
	ASSERT(!verts.empty() && !indices.empty(), "Mesh::Create - 빈 메시");

	// 원본 보관
	m_CPUVertices = verts;
	m_CPUIndices = indices;

	m_VertexCount = (uint32_t)verts.size();
	m_IndexCount = (uint32_t)indices.size();
	m_VertexBuffer.Create(L"Mesh VB", m_VertexCount, sizeof(Vertex), verts.data());
	m_IndexBuffer.Create(L"Mesh IB", m_IndexCount, sizeof(uint32_t), indices.data());

	// 정점에서 AABB 직접 구하기
	float mn[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	float mx[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const Vertex& v : verts)
	{
		for (int i = 0; i < 3; ++i)
		{
			mn[i] = std::min(mn[i], v.position[i]);
			mx[i] = std::max(mx[i], v.position[i]);
		}
	}
	m_BoundsMin = Math::Vector3(mn[0], mn[1], mn[2]);
	m_BoundsMax = Math::Vector3(mx[0], mx[1], mx[2]);
}

void GP::Mesh::CreateSphere(float radius, uint32_t rings, uint32_t segments)
{
	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;
	verts.reserve((size_t)(rings + 1) * (segments + 1));

	for (uint32_t r = 0; r <= rings; ++r)
	{
		float phi = 3.14159265f * r / rings;
		float y = cosf(phi);
		float ringRadius = sinf(phi);
		for (uint32_t c = 0; c <= segments; ++c)
		{
			float theta = 2.0f * 3.14159265f * c / segments;

			Vertex v = {};
			// 원점 중심 구여서 단위 위치가 곧 법선
			v.normal[0] = ringRadius * cosf(theta);
			v.normal[1] = y;
			v.normal[2] = ringRadius * sinf(theta);

			v.position[0] = radius * v.normal[0];
			v.position[1] = radius * v.normal[1];
			v.position[2] = radius * v.normal[2];
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

	Create(verts, indices);
}
