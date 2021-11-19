#ifndef _RAGDOLL_H
#define _RAGDOLL_H

#include "btBulletDynamicsCommon.h"
#include "animation/skeleton.h"

class Ragdoll
{
public:
	Ragdoll();
	void Init(const gef::SkeletonPose& bind_pose, btDiscreteDynamicsWorld* dynamics_world, const char* physics_filename);
	~Ragdoll();
	void UpdatePoseFromRagdoll();
	void UpdateRagdollFromPose();


	inline gef::SkeletonPose& pose() { return pose_; }
	inline void set_pose(const gef::SkeletonPose& pose) {
		pose_ = pose;
	}
	inline void set_scale_factor(float scale_factor) { scale_factor_ = scale_factor; }
	inline float scale_factor() const { return scale_factor_;  }
	inline std::vector<gef::Matrix44>& bone_world_matrices() { return bone_world_matrices_; }

private:
	gef::SkeletonPose bind_pose_;
	gef::SkeletonPose pose_;
	std::vector<gef::Matrix44> bone_rb_offset_matrices_;
	std::vector<btRigidBody*> bone_rbs_;
	std::vector<gef::Matrix44> bone_world_matrices_;
	float scale_factor_;
};

gef::Matrix44 btTransform2Matrix(const btTransform& transform);
btTransform Matrix2btTransform(const gef::Matrix44& mtx);



#endif // !_RAGDOLL_H
