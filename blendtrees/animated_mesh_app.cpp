#include "animated_mesh_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/texture.h>
#include <graphics/mesh.h>
#include <graphics/primitive.h>
#include <assets/png_loader.h>
#include <graphics/image_data.h>
#include <graphics/font.h>
#include <maths/vector2.h>
#include <input/input_manager.h>
#include <input/sony_controller_input_manager.h>
#include <input/keyboard.h>
#include <maths/math_utils.h>
#include <graphics/renderer_3d.h>
#include <graphics/scene.h>
#include <animation/skeleton.h>
#include <animation/animation.h>

AnimatedMeshApp::AnimatedMeshApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	font_(NULL),
	mesh_(NULL),
	player_(NULL),
	renderer_3d_(NULL),
	model_scene_(NULL),
	walk_anim(NULL),
	run_anim(NULL),
	idle_anim(NULL),
	jump_anim(NULL)
{
}

void AnimatedMeshApp::Init()
{
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	renderer_3d_ = gef::Renderer3D::Create(platform_);
	input_manager_ = gef::InputManager::Create(platform_);

	InitFont();
	SetupCamera();
	SetupLights();

	// create a new scene object and read in the data from the file
	// no meshes or materials are created yet
	// we're not making any assumptions about what the data may be loaded in for
	model_scene_ = new gef::Scene();
	model_scene_->ReadSceneFromFile(platform_, "xbot/xbot.scn");

	// we do want to render the data stored in the scene file so lets create the materials from the material data present in the scene file
	model_scene_->CreateMaterials(platform_);

	// if there is mesh data in the scene, create a mesh to draw from the first mesh
	mesh_ = GetFirstMesh(model_scene_);

	// get the first skeleton in the scene
	gef::Skeleton* skeleton = GetFirstSkeleton(model_scene_);

	if (skeleton)
	{
		player_ = new gef::SkinnedMeshInstance(*skeleton);
		player_->set_mesh(mesh_);
	}

	// anims
	walk_anim = LoadAnimation("xbot/xbot@walking_inplace.scn", "");
	jump_anim = LoadAnimation("xbot/xbot@jump.scn", "");
	idle_anim = LoadAnimation("xbot/xbot@idle.scn", "");

	InitBlendTree();
}


void AnimatedMeshApp::CleanUp()
{
	CleanUpFont();

	delete player_;
	player_ = NULL;

	delete walk_anim;
	walk_anim = NULL;

	delete mesh_;
	mesh_ = NULL;

	delete model_scene_;
	model_scene_ = NULL;

	delete input_manager_;
	input_manager_ = NULL;

	delete sprite_renderer_;
	sprite_renderer_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;
}

bool AnimatedMeshApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;

	// read input devices
	if (input_manager_)
	{
		input_manager_->Update();

		// controller input
		gef::SonyControllerInputManager* controller_manager = input_manager_->controller_input();
		if (controller_manager)
		{
		}

		// keyboard input
		gef::Keyboard* keyboard = input_manager_->keyboard();
		if (keyboard)
		{
			if (keyboard->IsKeyDown(keyboard->KC_2))
			{
				speed = speed >= 1.0f ? 1.0f : speed + 0.01f;
			}
			if (keyboard->IsKeyDown(keyboard->KC_1))
			{
				speed = speed <= 0 ? 0 : speed - 0.01f;
			}

			if (keyboard->IsKeyDown(keyboard->KC_NUMPADSTAR))
			{
				jumpBlend = jumpBlend >= 1.0f ? 1.0f : jumpBlend + 0.01f;
			}
			else if (keyboard->IsKeyDown(keyboard->KC_NUMPADSLASH))
			{
				jumpBlend = jumpBlend <= 0.0f ? 0.0f : jumpBlend - 0.01f;
			}
		}

		
	}

	if(player_)
	{
		blend_tree.variables_["idle_walk_blend"] = speed;
		blend_tree.variables_["jump_blend"] = jumpBlend;
		blend_tree.Update(frame_time);
		blended_pose = blend_tree.output_.output_pose_;

		player_->UpdateBoneMatrices(blended_pose);
		
	}

	// build a transformation matrix that will position the character
	// use this to move the player around, scale it, etc.
	if (player_)
	{
		gef::Matrix44 player_transform;
		gef::Matrix44 player_scale;
		gef::Matrix44 player_rotate;
		gef::Matrix44 player_translate;

		player_transform.SetIdentity();
		player_scale.SetIdentity();
		player_rotate.SetIdentity();
		player_translate.SetIdentity();

		player_scale.Scale(gef::Vector4(0.3f, 0.3f, 0.3f));
		player_rotate.RotationY(gef::DegToRad(45.f));
		player_translate.SetTranslation(gef::Vector4(25.f, -25.f, -100.f, 1.f));

		player_transform = player_scale * player_rotate * player_translate;

		player_->set_transform(player_transform);
	}

	return true;
}

void AnimatedMeshApp::Render()
{
	// setup view and projection matrices
	gef::Matrix44 projection_matrix;
	gef::Matrix44 view_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(camera_fov_, (float)platform_.width() / (float)platform_.height(), near_plane_, far_plane_);
	view_matrix.LookAt(camera_eye_, camera_lookat_, camera_up_);
	renderer_3d_->set_projection_matrix(projection_matrix);
	renderer_3d_->set_view_matrix(view_matrix);

	// draw meshes here
	renderer_3d_->Begin();

	// draw the player, the pose is defined by the bone matrices
	if(player_)
		renderer_3d_->DrawSkinnedMesh(*player_, player_->bone_matrices());

	renderer_3d_->End();

	// setup the sprite renderer, but don't clear the frame buffer
	// draw 2D sprites here
	sprite_renderer_->Begin(false);
	DrawHUD();
	sprite_renderer_->End();
}
void AnimatedMeshApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void AnimatedMeshApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void AnimatedMeshApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(600.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "Speed: %.1f", speed);
		font_->RenderText(sprite_renderer_, gef::Vector4(600.0f, 400.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "Blend: %.1f", blend_speed);
	}
}

void AnimatedMeshApp::SetupLights()
{
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-300.0f, -500.0f, 100.0f));

	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();
	default_shader_data.set_ambient_light_colour(gef::Colour(0.5f, 0.5f, 0.5f, 1.0f));
	default_shader_data.AddPointLight(default_point_light);
}

void AnimatedMeshApp::SetupCamera()
{
	// initialise the camera settings
	camera_eye_ = gef::Vector4(-1.0f, 1.0f, 4.0f);
	camera_lookat_ = gef::Vector4(0.0f, 1.0f, 0.0f);
	camera_up_ = gef::Vector4(0.0f, 1.0f, 0.0f);
	camera_fov_ = gef::DegToRad(45.0f);
	near_plane_ = 0.01f;
	far_plane_ = 1000.f;
}

gef::Skeleton* AnimatedMeshApp::GetFirstSkeleton(gef::Scene* scene)
{
	gef::Skeleton* skeleton = NULL;
	if (scene)
	{
		// check to see if there is a skeleton in the the scene file
		// if so, pull out the bind pose and create an array of matrices
		// that wil be used to store the bone transformations
		if (scene->skeletons.size() > 0)
			skeleton = scene->skeletons.front();
	}

	return skeleton;
}

gef::Mesh* AnimatedMeshApp::GetFirstMesh(gef::Scene* scene)
{
	gef::Mesh* mesh = NULL;

	if (scene)
	{
		// now check to see if there is any mesh data in the file, if so lets create a mesh from it
		if (scene->mesh_data.size() > 0)
			mesh = model_scene_->CreateMesh(platform_, scene->mesh_data.front());
	}

	return mesh;
}

gef::Animation* AnimatedMeshApp::LoadAnimation(const char* anim_scene_filename, const char* anim_name)
{
	gef::Animation* anim = NULL;

	gef::Scene anim_scene;
	if (anim_scene.ReadSceneFromFile(platform_, anim_scene_filename))
	{
		// if the animation name is specified then try and find the named anim
		// otherwise return the first animation if there is one
		std::map<gef::StringId, gef::Animation*>::const_iterator anim_node_iter;
		if (anim_name)
			anim_node_iter = anim_scene.animations.find(gef::GetStringId(anim_name));
		else
			anim_node_iter = anim_scene.animations.begin();

		if (anim_node_iter != anim_scene.animations.end())
			anim = new gef::Animation(*anim_node_iter->second);
	}

	return anim;
}

void AnimatedMeshApp::InitBlendTree()
{
	if (player_ && player_->bind_pose().skeleton())
	{
		blend_tree.Init(player_->bind_pose());

		//create clip node for idle
		ClipNode* idle_clip_node = new ClipNode(&blend_tree);
		idle_clip_node->SetClip(idle_anim); 

		ClipNode* walk_clip_node = new ClipNode(&blend_tree);
		walk_clip_node->SetClip(walk_anim);

		ClipNode* jump_clip_node = new ClipNode(&blend_tree);
		jump_clip_node->SetClip(jump_anim);

		//create linear2blend node
		Linear2BlendNode* l2b_node_idle_walk = new Linear2BlendNode(&blend_tree);
		Linear2BlendNode* l2b_node_move_jump = new Linear2BlendNode(&blend_tree);

		//set variables
		blend_tree.variables_["idle_walk_blend"] = speed;
		l2b_node_idle_walk->SetVariable(0, "idle_walk_blend");

		blend_tree.variables_["jump_blend"] = jumpBlend;
		l2b_node_move_jump->SetVariable(0, "jump_blend");

		//connect nodes
		
		l2b_node_idle_walk->SetInput(0, idle_clip_node);
		l2b_node_idle_walk->SetInput(1, walk_clip_node);
		
		l2b_node_move_jump->SetInput(0, l2b_node_idle_walk);
		l2b_node_move_jump->SetInput(1, jump_clip_node);

		blend_tree.output_.SetInput(0, l2b_node_move_jump);

		blend_tree.Start();
	}
}