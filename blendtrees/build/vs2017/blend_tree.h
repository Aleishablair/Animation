#pragma once
#include <animation/skeleton.h>
#include <vector>
#include "motion_clip_player.h"
#include <map>
#include <string>

class BlendTree;
class BlendNode;

struct BlendNodeInput
{
	BlendNode* node;
	//bool valid

	BlendNodeInput() :
		node(nullptr)
	{

	}

};

class BlendNode
{
public:
	BlendNode(BlendTree* _tree);
	std::vector<BlendNodeInput> inputs_;
	std::vector<std::string> variables_;
	gef::SkeletonPose output_pose_;
	BlendTree* tree_;

	bool Update(float delta_time);
	void Start();
	virtual bool Process(float delta_time) = 0;
	virtual void StartInternal() {}
	void SetInput(int input_num, BlendNode* node);
	void SetVariable(int variable_num, const std::string& variable);

};

class OutputNode : public BlendNode
{
public:
	OutputNode(BlendTree* _tree);
	bool Process(float delta_time);
};

class ClipNode : public BlendNode
{
public:
	ClipNode(BlendTree* _tree);
	void SetClip(const gef::Animation* anim);
	void StartInternal() override;
	bool Process(float delta_time) override;

	MotionClipPlayer clip_player_;
};

class Linear2BlendNode : public BlendNode
{
public:
	Linear2BlendNode(BlendTree* _tree);
	bool Process(float delta_time) override;
};

class BlendTree
{
public:
	BlendTree();
	void Init(const gef::SkeletonPose& bind_pose);
	void CleanUp();
	void Start();
	void Update(float delta_time);

	OutputNode output_;
	gef::SkeletonPose bind_pose_;

	std::map<std::string, float> variables_;

};

