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
#include "blend_tree.h"


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
}

class AnimatedMeshApp : public gef::Application
{
public:
	AnimatedMeshApp(gef::Platform& platform);
	void Init();

	gef::Skeleton* GetFirstSkeleton(gef::Scene* scene);

	gef::Mesh* GetFirstMesh(gef::Scene* scene);

	void CleanUp();
	bool Update(float frame_time);
	void Render();
private:
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
	void SetupLights();
	void SetupCamera();
	void InitBlendTree();
	gef::Animation* LoadAnimation(const char* anim_scene_filename, const char* anim_name);


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

	MotionClipPlayer anim_player_walk;
	gef::Animation* walk_anim;
	MotionClipPlayer anim_player_run;
	gef::Animation* run_anim;
	MotionClipPlayer anim_player_idle;
	gef::Animation* idle_anim;
	MotionClipPlayer anim_player_jump;
	gef::Animation* jump_anim;
	gef::SkeletonPose blended_pose;

	BlendTree blend_tree;

	float speed = 0;
	float jumpBlend = 0;
	float blend_speed = 0.f;
	float walk_speed = 1.0f;
	float walk_speed_min = 0;
	float walk_speed_max = 0;
	float run_speed = 3.0f;
	float run_speed_min = 0;
	float run_speed_max = 0;

	
};

#endif // _ANIMATED_MESH_APP_H
