#include "MeshSurfaceSampler.h"
#include "Mesh.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

std::vector<Math::Vector3> GP::MeshSurfaceSampler::SampleSurfacePoints(const Mesh& mesh, uint32_t sampleCount, uint32_t seed)
{
	std::vector<Math::Vector3> samples;
	const std::vector<Vertex>& vertices = mesh.GetCPUVertices();
	const std::vector<uint32_t>& indices = mesh.GetCPUIndices();

	if (vertices.empty() || indices.empty())
	{
		return samples;
	}

	// 삼각형 면적에 대한 CDF 생성
	std::vector<float> triangleAreaCDF = BuildTriangleAreaCDF(vertices, indices);

	if (triangleAreaCDF.empty() || triangleAreaCDF.back() == 0.0f)
	{
		return samples;
	}
	samples.reserve(sampleCount);

	std::mt19937 rand(seed);
	std::uniform_real_distribution<float> random01(
		0.0f,
		1.0f);

	for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
	{
		const float randomArea = random01(rand);

		// 삼각형 인덱스
		const uint32_t triangleIndex = SelectTriangleIndex(triangleAreaCDF, randomArea);

		const uint32_t indexOffset = triangleIndex * 3;

		const uint32_t i0 = indices[indexOffset + 0];
		const uint32_t i1 = indices[indexOffset + 1];
		const uint32_t i2 = indices[indexOffset + 2];

		const Math::Vector3 a(
			vertices[i0].position[0],
			vertices[i0].position[1],
			vertices[i0].position[2]);

		const Math::Vector3 b(
			vertices[i1].position[0],
			vertices[i1].position[1],
			vertices[i1].position[2]);

		const Math::Vector3 c(
			vertices[i2].position[0],
			vertices[i2].position[1],
			vertices[i2].position[2]);

		const float u = random01(rand);
		const float v = random01(rand);

		const Math::Vector3 samplePoint = SamplePointInTriangle(a, b, c, u, v);

		samples.push_back(samplePoint);
	}
	return samples;
}

std::vector<float> GP::MeshSurfaceSampler::BuildTriangleAreaCDF(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	float cumulativeArea = 0.0f;
	const uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);

	std::vector<float> triangleAreaCDF;
	triangleAreaCDF.reserve(triangleCount);

	for (uint32_t triIndex = 0; triIndex < triangleCount; ++triIndex)
	{
		const uint32_t indexOffset = triIndex * 3;

		uint32_t i0 = indices[indexOffset + 0];
		uint32_t i1 = indices[indexOffset + 1];
		uint32_t i2 = indices[indexOffset + 2];

		const Math::Vector3 v0(
			vertices[i0].position[0],
			vertices[i0].position[1],
			vertices[i0].position[2]);

		const Math::Vector3 v1(
			vertices[i1].position[0],
			vertices[i1].position[1],
			vertices[i1].position[2]);

		const Math::Vector3 v2(
			vertices[i2].position[0],
			vertices[i2].position[1],
			vertices[i2].position[2]);

		const float area =
			0.5f * Math::Length(Math::Cross(v1 - v0, v2 - v0));
		cumulativeArea += area;
		triangleAreaCDF.push_back(cumulativeArea);
	}
	// CDF 정규화 [0,1]범위
	if (cumulativeArea > 0.0f)
	{
		for (float& cdfValue : triangleAreaCDF)
		{
			cdfValue /= cumulativeArea;
		}
	}
	return triangleAreaCDF;
}

uint32_t GP::MeshSurfaceSampler::SelectTriangleIndex(const std::vector<float>& triangleAreaCDF, float randomValue)
{
	int32_t lowerBound = 0;
	int32_t upperBound = static_cast<int32_t>(triangleAreaCDF.size()) - 1;

	while (lowerBound <= upperBound)
	{
		const int32_t midPoint = lowerBound + (upperBound - lowerBound) / 2;
		if (triangleAreaCDF[midPoint] < randomValue)
		{
			lowerBound = midPoint + 1;
		}
		else
		{
			upperBound = midPoint - 1;
		}
	}
	return static_cast<uint32_t>(lowerBound); // randomValue 이상인 첫 번째 CDF 값의 인덱스
}

Math::Vector3 GP::MeshSurfaceSampler::SamplePointInTriangle(const Math::Vector3& a, const Math::Vector3& b, const Math::Vector3& c, float u, float v)
{
	const float s = sqrtf(u);

	// 세 정점의 barycentric 가중치 (합은 항상 1)
	const float wA = 1.0f - s;
	const float wB = s * (1.0f - v);
	const float wC = s * v;

	return wA * a + wB * b + wC * c;
}
