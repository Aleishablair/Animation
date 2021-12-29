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
#include <input/touch_input_manager.h>
#include <maths/math_utils.h>
#include <graphics/renderer_3d.h>
#include <graphics/scene.h>
#include <animation/skeleton.h>
#include <animation/animation.h>
#include <system/debug_log.h>
#include <maths/math_utils.h>

#include "picking.h"
#include "ccd.h"

std::string model_name("xbot");


AnimatedMeshApp::AnimatedMeshApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	font_(NULL),
	mesh_(NULL),
	player_(NULL),
	renderer_3d_(NULL),
	model_scene_(NULL),
	primitive_builder_(NULL),
	primitive_renderer_(NULL),
	effector_position_(gef::Vector4::kZero)
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
	std::string model_scene_name = model_name + "/" + model_name + ".scn";


	model_scene_ = new gef::Scene();
	model_scene_->ReadSceneFromFile(platform_, model_scene_name.c_str());

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
		player_->UpdateBoneMatrices(player_->bind_pose());

		// output skeleton joint names
		for (int joint_num = 0; joint_num < skeleton->joints().size(); ++joint_num)
		{
			std::string bone_name;
			model_scene_->string_id_table.Find(skeleton->joint(joint_num).name_id, bone_name);
			gef::DebugOut("%d: %s\n", joint_num, bone_name.c_str());
		}

		ik_pose_ = player_->bind_pose();
	}

	primitive_builder_ = new PrimitiveBuilder(platform_);
	primitive_renderer_ = new PrimitiveRenderer(platform_);
}


void AnimatedMeshApp::CleanUp()
{
	delete primitive_renderer_;
	primitive_renderer_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	CleanUpFont();

	delete player_;
	player_ = NULL;

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

	bool mb_down = false;
	gef::Vector2 mouse_position(gef::Vector2::kZero);

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
			if (keyboard->IsKeyPressed(gef::Keyboard::KC_SPACE))
			{
				ik_pose_ = player_->bind_pose();
				player_->UpdateBoneMatrices(ik_pose_);
			}
		}

		// mouse
		mouse_position = input_manager_->touch_manager()->mouse_position();
		mb_down = input_manager_->touch_manager()->is_button_down(1);
	}


	if (mb_down)
	{
		gef::Vector4 mouse_ray_start_point, mouse_ray_direction;
		//mouse_position = gef::Vector2(600, 300);
		GetScreenPosRay(mouse_position, renderer_3d_->projection_matrix(), renderer_3d_->view_matrix(), mouse_ray_start_point, mouse_ray_direction, (float)platform_.width(), (float)platform_.height(), ndc_zmin_);

		if (RayPlaneIntersect(mouse_ray_start_point, mouse_ray_direction, gef::Vector4(0.0f, 0.0f, 0.0f), gef::Vector4(0.0f, 0.0f, 1.0f), effector_position_))
		{
			std::vector<int> bone_indices;
			bone_indices.push_back(16); // left shoulder
			bone_indices.push_back(17); // left elbow
			bone_indices.push_back(18); // left wrist

			CalculateCCD(ik_pose_, *player_, effector_position_, bone_indices);

			player_->UpdateBoneMatrices(ik_pose_);
		}
	}

	if (player_)
	{

	}

	return true;
}

void AnimatedMeshApp::Render()
{
	SetCameraMatrices();

	// draw meshes here
	renderer_3d_->Begin();

	// draw the player, the pose is defined by the bone matrices
	if (player_)
	{
		renderer_3d_->DrawSkinnedMesh(*player_, player_->bone_matrices());
	}

	primitive_renderer_->Reset();

	RenderEndEffector();

	primitive_renderer_->Render(*renderer_3d_);


	renderer_3d_->End();

	// setup the sprite renderer, but don't clear the frame buffer
	// draw 2D sprites here
	sprite_renderer_->Begin(false);
	DrawHUD();
	sprite_renderer_->End();
}
void AnimatedMeshApp::SetCameraMatrices()
{
	// setup view and projection matrices
	gef::Matrix44 projection_matrix;
	gef::Matrix44 view_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(camera_fov_, (float)platform_.width() / (float)platform_.height(), near_plane_, far_plane_);
	view_matrix.LookAt(camera_eye_, camera_lookat_, camera_up_);

	if (renderer_3d_)
	{
		renderer_3d_->set_projection_matrix(projection_matrix);
		renderer_3d_->set_view_matrix(view_matrix);
	}
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
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
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
	camera_eye_ = gef::Vector4(0.0f, 100.0f, 500.0f);
	camera_lookat_ = gef::Vector4(0.0f, 100.0f, 0.0f);
	camera_up_ = gef::Vector4(0.0f, 1.0f, 0.0f);
	camera_fov_ = gef::DegToRad(45.0f);
	near_plane_ = 0.01f;
	far_plane_ = 1000.0f;

	SetCameraMatrices();
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

void AnimatedMeshApp::RenderEndEffector()
{
	const float effector_half_line_length = 20.0f;
	const gef::Colour effector_colour(0.0f, 1.0f, 0.0f);

	primitive_renderer_->AddLine(effector_position_ - gef::Vector4(-effector_half_line_length, 0.0f, 0.0f), effector_position_ + gef::Vector4(-effector_half_line_length, 0.0f, 0.0f), effector_colour);
	primitive_renderer_->AddLine(effector_position_ - gef::Vector4(0.0f, -effector_half_line_length, 0.0f), effector_position_ + gef::Vector4(0.0f, -effector_half_line_length, 0.0f), effector_colour);
	primitive_renderer_->AddLine(effector_position_ - gef::Vector4(0.0f, 0.0f, -effector_half_line_length), effector_position_ + gef::Vector4(0.0f, 0.0f, -effector_half_line_length), effector_colour);
}
