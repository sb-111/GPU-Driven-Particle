#pragma once
namespace GP
{
	struct Vertex
	{
		float position[3];	// [0, 12]
		float color[4];		// [12, 28]
	};
	__declspec(align(16)) struct ComputeCB
	{
		float deltaTime;
		uint32_t particleCount;
	};
}
