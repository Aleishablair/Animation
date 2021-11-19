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
#include <system/debug_log.h>
#include "ragdoll.h"

std::string model_name("xbot");


AnimatedMeshApp::AnimatedMeshApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	font_(NULL),
	mesh_(NULL),
	player_(NULL),
	renderer_3d_(NULL),
	model_scene_(NULL),
	walk_anim_(NULL),
	primitive_builder_(NULL),
	primitive_renderer_(NULL),
	dynamics_world_(NULL),
	solver_(NULL),
	overlapping_pair_cache_(NULL),
	dispatcher_(NULL),
	debug_drawer_(NULL),
	floor_mesh_(NULL),
	sphere_mesh_(NULL),
	sphere_rb_(NULL),
	ragdoll_(NULL)
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

		// animated model is scaled down to match the size of the physics ragdoll
		gef::Matrix44 player_transform;
		const float scale = 0.01f;
		player_transform.Scale(gef::Vector4(scale, scale, scale));
		player_->set_transform(player_transform);
	}


	// anims
	std::string walk_anim_name = model_name + "/" + model_name + "@walking_inplace.scn";
	std::string run_anim_name = model_name + "/" + model_name + "@running_inplace.scn";
	std::string idle_anim_name = model_name + "/" + model_name + "@idle.scn";


	walk_anim_ = LoadAnimation(walk_anim_name.c_str(), "");
	run_anim_ = LoadAnimation(run_anim_name.c_str(), "");
	idle_anim_ = LoadAnimation(idle_anim_name.c_str(), "");

	clip_player_.Init(player_->bind_pose());
	clip_player_.set_clip(idle_anim_);
	clip_player_.set_looping(true);
	clip_player_.set_anim_time(0.0f);

	primitive_builder_ = new PrimitiveBuilder(platform_);
	primitive_renderer_ = new PrimitiveRenderer(platform_);

	InitPhysicsWorld();
	CreateRigidBodies();

	InitRagdoll();
}

void AnimatedMeshApp::InitRagdoll()
{
	if (player_->bind_pose().skeleton())
	{
		ragdoll_ = new Ragdoll();
		ragdoll_->set_scale_factor(0.01f);

		std::string ragdoll_filename;
		ragdoll_filename = model_name + "/ragdoll.bullet";

		ragdoll_->Init(player_->bind_pose(), dynamics_world_, ragdoll_filename.c_str());
	}

	is_ragdoll_simulating_ = false;
}

void AnimatedMeshApp::CleanUp()
{
	CleanUpRagdoll();

	CleanUpRigidBodies();

	CleanUpPhysicsWorld();

	delete primitive_renderer_;
	primitive_renderer_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	CleanUpFont();

	delete player_;
	player_ = NULL;


	delete idle_anim_;
	idle_anim_ = NULL;

	delete walk_anim_;
	walk_anim_ = NULL;

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

void AnimatedMeshApp::CleanUpRagdoll()
{
	delete ragdoll_;
	ragdoll_ = NULL;
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
			if (keyboard->IsKeyPressed(gef::Keyboard::KC_SPACE))
			{
				is_ragdoll_simulating_ = !is_ragdoll_simulating_;
			}
		}
	}

	// update the current animation that is playing
	if (player_)
	{
		clip_player_.Update(frame_time, player_->bind_pose());
		player_->UpdateBoneMatrices(clip_player_.pose());
	}

	UpdatePhysicsWorld(frame_time);
	UpdateRigidBodies();



	if (player_ && ragdoll_)
	{
		if (is_ragdoll_simulating_)
		{
			ragdoll_->UpdatePoseFromRagdoll();
			player_->UpdateBoneMatrices(ragdoll_->pose());
		}
		else
		{
			ragdoll_->set_pose(clip_player_.pose());
			ragdoll_->UpdateRagdollFromPose();
		}
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
	if (player_)
	{
		renderer_3d_->DrawSkinnedMesh(*player_, player_->bone_matrices());
	}

	renderer_3d_->DrawMesh(floor_gfx_);
	renderer_3d_->DrawMesh(sphere_gfx_);

	if (dynamics_world_)
		dynamics_world_->debugDrawWorld();

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
	camera_eye_ = gef::Vector4(0.0f, 2.0f, 5.0f);
	camera_lookat_ = gef::Vector4(0.0f, 0.0f, 0.0f);
	camera_up_ = gef::Vector4(0.0f, 1.0f, 0.0f);
	camera_fov_ = gef::DegToRad(45.0f);
	near_plane_ = 0.01f;
	far_plane_ = 100.0f;
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

void AnimatedMeshApp::InitPhysicsWorld()
{
	/// collision configuration contains default setup for memory , collision setup . Advanced users can create their own configuration .
	btDefaultCollisionConfiguration * collision_configuration = new btDefaultCollisionConfiguration();

	/// use the default collision dispatcher . For parallel processing you can use a diffent dispatcher(see Extras / BulletMultiThreaded)
	dispatcher_ = new btCollisionDispatcher(collision_configuration);

	/// btDbvtBroadphase is a good general purpose broadphase . You can also try out btAxis3Sweep .
	overlapping_pair_cache_ = new btDbvtBroadphase();

	/// the default constraint solver . For parallel processing you can use a different solver (see Extras / BulletMultiThreaded)
	solver_ = new btSequentialImpulseConstraintSolver;

	dynamics_world_ = new btDiscreteDynamicsWorld(dispatcher_, overlapping_pair_cache_, solver_, collision_configuration);
	dynamics_world_->setGravity(btVector3(0, -9.8f, 0));


	debug_drawer_ = new GEFDebugDrawer(renderer_3d_);
	debug_drawer_->setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawFrames);
	dynamics_world_->setDebugDrawer(debug_drawer_);
}

void AnimatedMeshApp::CleanUpPhysicsWorld()
{
	delete debug_drawer_;
	debug_drawer_ = NULL;

	for (int i = dynamics_world_->getNumConstraints() - 1; i >= 0; i--)
	{
		btTypedConstraint* constraint = dynamics_world_->getConstraint(i);
		dynamics_world_->removeConstraint(constraint);
		delete constraint;
	}


	// remove the rigidbodies from the dynamics world and delete them
	for (int i = dynamics_world_->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject * obj = dynamics_world_->getCollisionObjectArray()[i];
		btRigidBody * body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		dynamics_world_->removeCollisionObject(obj);
		delete obj;
	}

	// delete collision shapes
	for (int j = 0; j< collision_shapes_.size(); j++)
	{
		btCollisionShape * shape = collision_shapes_[j];
		collision_shapes_[j] = 0;
		delete shape;
	}

	// delete dynamics world
	delete dynamics_world_;

	// delete solver
	delete solver_;

	// delete broadphase
	delete overlapping_pair_cache_;

	// delete dispatcher
	delete dispatcher_;

	dynamics_world_ = NULL;
	solver_ = NULL;
	overlapping_pair_cache_ = NULL;
	dispatcher_ = NULL;

	// next line is optional : it will be cleared by the destructor when the array goes out of scope
	collision_shapes_.clear();
}

void AnimatedMeshApp::UpdatePhysicsWorld(float delta_time)
{
	const btScalar simulation_time_step = 1.0f / 60.0f;
	const int max_sub_steps = 1;
	dynamics_world_->stepSimulation(simulation_time_step, max_sub_steps);
}

void AnimatedMeshApp::CreateRigidBodies()
{
	//the ground is a cube of side 100 at position y = 0.
	{
		btVector3 groundHalfExtents(btScalar(50.), btScalar(1.), btScalar(50.));
		btCollisionShape* groundShape = new btBoxShape(groundHalfExtents);

		collision_shapes_.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -groundHalfExtents.y(), 0));

		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		dynamics_world_->addRigidBody(body);

		floor_mesh_ = primitive_builder_->CreateBoxMesh(gef::Vector4(groundHalfExtents.x(), groundHalfExtents.y(), groundHalfExtents.z()));
		floor_gfx_.set_mesh(floor_mesh_);
		floor_gfx_.set_transform(btTransform2Matrix(groundTransform));
	}

//	if(0)
	{
		//create a dynamic rigidbody

		const btScalar  sphereRadius = 1.f;
		btCollisionShape* colShape = new btSphereShape(sphereRadius);

		collision_shapes_.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(1.f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		startTransform.setOrigin(btVector3(2, 10, 0));

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		dynamics_world_->addRigidBody(body);

		sphere_rb_ = body;
		sphere_mesh_ = primitive_builder_->CreateSphereMesh(sphereRadius, 20, 20);
		sphere_gfx_.set_mesh(sphere_mesh_);
		sphere_gfx_.set_transform(btTransform2Matrix(startTransform));
	}
}

void AnimatedMeshApp::CleanUpRigidBodies()
{
	delete sphere_mesh_;
	sphere_mesh_ = NULL;
	delete floor_mesh_;
	floor_mesh_ = NULL;
}

void AnimatedMeshApp::UpdateRigidBodies()
{
	if (sphere_rb_)
	{
		btTransform world_transform;
		sphere_rb_->getMotionState()->getWorldTransform(world_transform);
		sphere_gfx_.set_transform(btTransform2Matrix(world_transform));
	}
}
