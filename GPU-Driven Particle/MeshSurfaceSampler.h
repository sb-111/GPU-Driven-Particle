#pragma once
#include "VectorMath.h"
#include "Mesh.h"
#include <cstdint>
#include <vector>
namespace GP
{
	class MeshSurfaceSampler
	{
	public:
		// 메시 로컬 공간에서 표면 샘플 생성
		static std::vector<Math::Vector3> SampleSurfacePoints(
			const Mesh& mesh,
			uint32_t sampleCount,
			uint32_t seed);

	private:
		// 삼각형 면적 CDF 배열 반환
		static std::vector<float> BuildTriangleAreaCDF(
			const std::vector<Vertex>& vertices,
			const std::vector<uint32_t>& indices);
		/**
		* @brief CDF와 균일 난수를 이용해 면적 비례로 삼각형 하나를 선택한다.
		* @param triangleAreaCDF 정규화된 삼각형 누적 면적 CDF
		* @param randomValue [0, 1) 범위의 균일 난수
		* @return 선택된 삼각형의 인덱스
		*/
		static uint32_t SelectTriangleIndex(
			const std::vector<float>& triangleAreaCDF,
			float randomValue);
		/**
		* @brief 삼각형 내부에서 면적 기준으로 균일한 표면 점을 생성한다.
		* @param a 삼각형의 첫 번째 정점
		* @param b 삼각형의 두 번째 정점
		* @param c 삼각형의 세 번째 정점
		* @param u [0, 1) 범위의 균일 난수
		* @param v [0, 1) 범위의 균일 난수
		* @return 삼각형 로컬 공간의 표면 샘플 점
		*/
		static Math::Vector3 SamplePointInTriangle(
			const Math::Vector3& a,
			const Math::Vector3& b,
			const Math::Vector3& c,
			float u,
			float v);
	};
}
