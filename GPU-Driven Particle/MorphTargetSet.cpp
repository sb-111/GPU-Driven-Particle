#include "MorphTargetSet.h"
#include "ParticleShared.h"

void GP::MorphTargetSet::Create(const std::string& name, const std::vector<Math::Vector3>& localPoints)
{
	m_Name = name;
	m_TargetCount = static_cast<uint32_t>(localPoints.size());

	if (localPoints.empty())
	{
		return;
	}

	std::vector<MorphTargetPoint> gpuPoints;
	gpuPoints.reserve(m_TargetCount);

	for (const Math::Vector3& localPoint : localPoints)
	{
		MorphTargetPoint gpuPoint = {};
		gpuPoint.position.x = static_cast<float>(localPoint.GetX());
		gpuPoint.position.y = static_cast<float>(localPoint.GetY());
		gpuPoint.position.z = static_cast<float>(localPoint.GetZ());

		gpuPoints.push_back(gpuPoint);
	}

	m_TargetBuffer.Create(L"Morph Target Set", m_TargetCount, sizeof(MorphTargetPoint), gpuPoints.data());
}

bool GP::MorphTargetSet::IsValid() const
{
	return m_TargetCount >= 1 && m_TargetBuffer.GetResource() != nullptr;
}

const std::string& GP::MorphTargetSet::GetName() const
{
	return m_Name;
}

uint32_t GP::MorphTargetSet::GetTargetCount() const
{
	return m_TargetCount;
}

StructuredBuffer& GP::MorphTargetSet::GetTargetBuffer()
{
	return m_TargetBuffer;
}

const StructuredBuffer& GP::MorphTargetSet::GetTargetBuffer() const
{
	return m_TargetBuffer;
}
