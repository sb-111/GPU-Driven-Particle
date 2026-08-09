#pragma once

#include "GpuBuffer.h"
#include "VectorMath.h"

#include <cstdint>
#include <string>
#include <vector>

namespace GP
{
	class MorphTargetSet
	{
	public:
		// 로컬 표면 점 목록을 GPU 버퍼로 업로드
		void Create(const std::string& name, const std::vector<Math::Vector3>& localPoints);

		bool IsValid() const;

		const std::string& GetName() const;
		uint32_t GetTargetCount() const;
		StructuredBuffer& GetTargetBuffer();
		const StructuredBuffer& GetTargetBuffer() const;
	private:
		std::string m_Name;
		uint32_t m_TargetCount = 0;
		StructuredBuffer m_TargetBuffer;
	};

}
