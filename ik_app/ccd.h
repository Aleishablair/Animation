#ifndef _CCD_H

#include <animation/skeleton.h>
#include <graphics/skinned_mesh_instance.h>

bool CalculateCCD(
	gef::SkeletonPose& pose,
	const gef::SkinnedMeshInstance& animatedModel,
	const gef::Vector4& destPoint,
	const std::vector<int>& boneIndices,
	std::vector<std::pair<float, float>> constraints,
	std::vector<int> priority_bones);

#endif // !_CCD_H
