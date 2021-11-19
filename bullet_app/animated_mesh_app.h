#ifndef _ANIMATED_MESH_APP_H
#define _ANIMATED_MESH_APP_H

#include <system/application.h>
#include <graphics/sprite.h>
#include <maths/vector2.h>
#include <maths/vector4.h>
#include <maths/matrix44.h>
#include <vector>
#include <graphics/skinned_mesh_instance.h>
#include "motion_clip_player.h"
#include "primitive_builder.h"
#include "primitive_renderer.h"
#include "gef_debug_drawer.h"

#include "btBulletDynamicsCommon.h"
#include "ragdoll.h"


// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Font;
	class Renderer3D;
	class Mesh;
	class Scene;
	class Skeleton;
	class InputManager;
	class Animation;
}

class AnimatedMeshApp : public gef::Application
{
public:
	AnimatedMeshApp(gef::Platform& platform);
	void Init();

	void InitRagdoll();

	gef::Skeleton* GetFirstSkeleton(gef::Scene* scene);

	gef::Mesh* GetFirstMesh(gef::Scene* scene);

	void CleanUp();
	void CleanUpRagdoll();
	bool Update(float frame_time);
	void Render();
private:
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
	void SetupLights();
	void SetupCamera();
	gef::Animation* LoadAnimation(const char* anim_scene_filename, const char* anim_name);

	void InitPhysicsWorld();
	void CleanUpPhysicsWorld();
	void UpdatePhysicsWorld(float delta_time);

	void CreateRigidBodies();
	void CleanUpRigidBodies();
	void UpdateRigidBodies();

	gef::SpriteRenderer* sprite_renderer_;
	gef::Renderer3D* renderer_3d_;
	gef::InputManager* input_manager_;
	gef::Font* font_;

	float fps_;

	class gef::Mesh* mesh_;
	gef::SkinnedMeshInstance* player_;

	gef::Scene* model_scene_;

	gef::Vector4 camera_eye_;
	gef::Vector4 camera_lookat_;
	gef::Vector4 camera_up_;
	float camera_fov_;
	float near_plane_;
	float far_plane_;

	gef::Animation* idle_anim_;
	gef::Animation* walk_anim_;
	gef::Animation* run_anim_;

	MotionClipPlayer clip_player_;

	PrimitiveBuilder* primitive_builder_;
	PrimitiveRenderer* primitive_renderer_;

	btDiscreteDynamicsWorld* dynamics_world_;
	btSequentialImpulseConstraintSolver* solver_;
	btBroadphaseInterface* overlapping_pair_cache_;
	btCollisionDispatcher* dispatcher_;
	btAlignedObjectArray<btCollisionShape*> collision_shapes_;
	GEFDebugDrawer* debug_drawer_;

	gef::Mesh* floor_mesh_;
	gef::MeshInstance floor_gfx_;

	btRigidBody* sphere_rb_;
	gef::Mesh* sphere_mesh_;
	gef::MeshInstance sphere_gfx_;

	Ragdoll* ragdoll_;

	bool is_ragdoll_simulating_;

};

#endif // _ANIMATED_MESH_APP_H
