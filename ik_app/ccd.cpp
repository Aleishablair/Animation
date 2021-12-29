#include "ccd.h"
#include <algorithm>
#include <system/debug_log.h>
#include <maths/math_utils.h>

bool CalculateCCD(
	gef::SkeletonPose& pose,
	const gef::SkinnedMeshInstance& animatedModel,
	const gef::Vector4& destPoint,
	const std::vector<int>& boneIndices)
{
	std::vector<gef::Matrix44> global_pose;
	std::vector<gef::Matrix44> local_pose;
	local_pose.resize(pose.skeleton()->joint_count());
	global_pose = pose.global_pose();


	for (int i = 0; i < pose.skeleton()->joint_count(); ++i)
		local_pose[i] = pose.local_pose()[i].GetMatrix();

	//obtain the inverse of the animated model's transform
	gef::Matrix44 worldToModelTransform;
	worldToModelTransform.Inverse(animatedModel.transform());

	//...and use it to move the destination point from world space to model space
	gef::Vector4 destPointModelSpace = destPoint.Transform(worldToModelTransform);

	// Get the end effectors position
	gef::Vector4 endEffectorPos = global_pose[boneIndices.back()].GetTranslation();

	// FIXME: REPLACE THE LINE BELOW
	//calculate the distance between the end effector's position and the destination position
	float distance = 0;

	float epsilon = 0.001f;
	//max number of iterations is used to stop CCD if there is no solution
	int maxIterations = 200;

	//perform the CCD algorithm if all the following conditions are valid
	/*
	- if the distance between the end effector point is greater than epsilon
	- we have not reached the maximum number of iterations
	*/
	while ((distance > epsilon) && (maxIterations > 0))
	{
		// FIXME: IMPLEMENT THE REVERSE ORDER FOR LOOP TO SOLVE THE IK CHAIN
		// USING THE ALGORITHM IN THE LECTURE NOTES

		//if a solution has not been reached, decrement the iterations counter
		--maxIterations;
	}


	//
	// This remain part of the function updates the gef::SkeletonPose with the newly calculate bone
	// transforms.
	// The gef::SkeletonPose interface wasn't originally designed to be updated in this way
	// You may wish to consider altering the interface to this class to remove redundant calculations

	// calculate new local pose of bones in IK chain
	for (size_t i = 0; i < boneIndices.size(); ++i)
	{
		int boneNum = boneIndices[i];

		const gef::Joint& joint = pose.skeleton()->joint(boneNum);
		if (joint.parent == -1)
		{
			local_pose[boneNum] = global_pose[boneNum];
		}
		else
		{
			gef::Matrix44 parentInv;
			parentInv.Inverse(global_pose[joint.parent]);
			local_pose[boneNum] = global_pose[boneNum] * parentInv;
		}
	}

	// recalculate global pose based on new local pose
	for (int i = 0; i < pose.skeleton()->joint_count(); ++i)
	{
		const gef::Joint& joint = pose.skeleton()->joint(i);
		if (joint.parent == -1)
			global_pose[i] = local_pose[i];
		else
			global_pose[i] = local_pose[i] * global_pose[joint.parent];
	}

	// update skeleton pose data structure
	pose.CalculateLocalPose(global_pose);
	pose.CalculateGlobalPose();


	if (maxIterations <= 0)
	{
		//if the iterations counter reaches 0 (or less), then a CCD solution was not found
		return false;
	}
	else
	{

		//if a solution was found, return true
		return true;
	}

}
