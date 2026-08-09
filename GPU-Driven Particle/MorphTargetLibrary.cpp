#include "MorphTargetLibrary.h"

#include "Mesh.h"
#include "MeshSurfaceSampler.h"
#include "MorphTargetSet.h"

GP::MorphTargetLibrary::~MorphTargetLibrary() = default;

GP::MorphTargetSet& GP::MorphTargetLibrary::GetOrCreateSurfaceTarget(
	const std::string& name,
	const Mesh& mesh,
	uint32_t sampleCount,
	uint32_t seed)
{
	ASSERT(sampleCount > 0, "MorphTargetLibrary: sample count must be greater than zero");

	ASSERT(!mesh.GetCPUVertices().empty() && !mesh.GetCPUIndices().empty(),
		"MorphTargetLibrary: source mesh has no CPU geometry");

	const SurfaceTargetKey key =
	{
		&mesh,
		sampleCount,
		seed
	};

	// 있으면 기존 거 반환
	const auto found = m_SurfaceTargets.find(key);
	if (found != m_SurfaceTargets.end())
	{
		return *found->second;
	}
	// 없으면 생성
	std::vector<Math::Vector3> localPoints = MeshSurfaceSampler::SampleSurfacePoints(mesh, sampleCount, seed);

	ASSERT(!localPoints.empty(), "MorphTargetLibrary: failed to generate surface samples");

	auto newTargetSet = std::make_unique<MorphTargetSet>();
	newTargetSet->Create(name, localPoints);

	const auto inserted = m_SurfaceTargets.emplace(key, std::move(newTargetSet));

	return *inserted.first->second;
}
void GP::MorphTargetLibrary::Clear()
{
	m_SurfaceTargets.clear();
}
