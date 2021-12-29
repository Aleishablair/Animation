#include "ccd.h"
#include <algorithm>
#include <system/debug_log.h>
#include <maths/math_utils.h>

bool CalculateCCD(
	gef::SkeletonPose& pose,
	const gef::SkinnedMeshInstance& animatedModel,
	const gef::Vector4& destPoint,
	const std::vector<int>& boneIndices,
	std::vector<std::pair<float, float>> constraints,
	std::vector<int> priorityBones)
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
	//float distance = 0;
	float distance = (endEffectorPos - destPointModelSpace).Length();

	//float minDistance = 0.001f;
	//min distance
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

		for (size_t priority = 0; priority < priorityBones.size(); priority++)
		{
			int index = priorityBones[priority];
			gef::Vector4 currVec, destVec, crossVec;
			//obtain vector from end effector point to current joint
			//EB
			currVec = endEffectorPos - global_pose[boneIndices[index]].GetTranslation();
			//obtain vector from the destination point to current joinT
			//DB
			destVec = destPointModelSpace - global_pose[boneIndices[index]].GetTranslation();

			//normalize our two vectors
			if (currVec.Length() > epsilon)
			{
				currVec.Normalise();
			}
			if (destVec.Length() > epsilon)
			{
				destVec.Normalise();
			}

			//calculate angle between two vectors
			float vecDot = currVec.DotProduct(destVec);
			float angle = 0.0f;
			if (fabsf(vecDot))
			{
				if (vecDot > 1.0f)
				{
					vecDot = 1.0f;
				}
				else if (vecDot < -1.0f)
				{
					vecDot = -1.0f;
				}
				angle = acosf(vecDot);
			}

			//enforce contraint of joint so that joint does not rotate past an angle
			angle = std::max(angle, constraints[index].first);
			angle = std::min(angle, constraints[index].second);

			//calculate cross product axis of our two vectors
			crossVec = currVec.CrossProduct(destVec);

			//if the angle is 0 or if the cross prod axis is 0,0,0 then dont bother rotating the joint and move on to next one
			if (angle == 0)
			{
				continue;
			}

			//save transform of joint youre rotating before you rotate it
			gef::Matrix44 oldTransform = global_pose[boneIndices[index]];

			//rotate the quaternion of the joint using a quaternion created from the angle and crossVector axis you calculated earlier
			gef::Quaternion boneRot;
			boneRot.SetFromMatrix(oldTransform);
			float sinHalfAngle = sinf(angle * 0.5f);
			float cosHalfAngle = cosf(angle * 0.5f);
			gef::Quaternion rot(crossVec.x() * sinHalfAngle, crossVec.y() * sinHalfAngle, crossVec.z() * sinHalfAngle, cosHalfAngle);
			boneRot = boneRot * rot;
			boneRot.Normalise();
			
			global_pose[boneIndices[index]].Rotation(boneRot);
			global_pose[boneIndices[index]].SetTranslation(oldTransform.GetTranslation());

			//modify child joints via iterative process. vector of joints is saves such that the parent joint will always be before the child joint
			for (size_t i = index + 1; i < boneIndices.size(); i++)
			{
				//save transform of child joint before modifying
				gef::Matrix44 tempOld = global_pose[boneIndices[i]];

				//since a child joints transform is formed from its parent joints transform youll have to undo the parents old transform since it has been modified. Done by concatenating the inverse of the parents old transform to the child transform
				gef::Matrix44 oldTransformInv;
				oldTransformInv.Inverse(oldTransform);

				gef::Matrix44 local_transform = global_pose[boneIndices[i]] * oldTransformInv;
				global_pose[boneIndices[i]] = local_transform * global_pose[boneIndices[i - 1]];

				//save the old transform of the child transform before it was modified into oldTransform which will be used to modify its child transform in the next iteration
				oldTransform = tempOld;

				//get the transform of the end effector since it has most likely been modified
				endEffectorPos = global_pose[boneIndices.back()].GetTranslation();
				distance = (endEffectorPos - destPointModelSpace).Length();

				//if distance is less than minDistance then end effector is close enough to destination location so break out of CCD algorithm
				if (distance < epsilon)
				{
					break;
				}
			}

			--index;

		}

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
