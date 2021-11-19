#include "blend_tree.h"

BlendNode::BlendNode(BlendTree* _tree) :
	tree_(_tree)
{
	if (tree_)
	{
		output_pose_ = tree_->bind_pose_;
	}
}

bool BlendNode::Update(float delta_time)
{
	bool all_inputs_valid = true;
	if (inputs_.size() > 0)
	{
		for (int input_num = 0; input_num < inputs_.size(); ++input_num)
		{
			bool inputs_valid = false;

			if(inputs_[input_num].node)
			{
				inputs_valid = inputs_[input_num].node->Update(delta_time);
			}
			
			if (!inputs_valid && all_inputs_valid)
			{
				all_inputs_valid = false;
			}			
		}
	}

	bool all_variables_valid = true;
	if (variables_.size() > 0)
	{
		for (int variable_num = 0; variable_num < variables_.size(); ++variable_num)
		{
			const std::string& variable = variables_[variable_num];
			bool variable_valid = tree_->variables_.find(variable) != tree_->variables_.end();

			if (!variable_valid && all_variables_valid)
				all_variables_valid = false;
		}
	}


	bool output_valid = false;
	if (all_inputs_valid && all_variables_valid)
		output_valid = Process(delta_time);

	return output_valid;
}

void BlendNode::Start()
{
	for (int input_num = 0; input_num < inputs_.size(); ++input_num)
	{
		BlendNodeInput& input = inputs_[input_num];
		if (input.node)
			input.node->Start();
	}
	StartInternal();
}


void BlendNode::SetInput(int input_num, BlendNode* node)
{
	if (node && input_num < inputs_.size())
	{
		inputs_[input_num].node = node;
	}
}


void BlendNode::SetVariable(int variable_num, const std::string& variable)
{
	if (variable.size() > 0 && variable_num < variables_.size())
	{
		variables_[variable_num] = variable;
	}
}

BlendTree::BlendTree() :
	output_(this)
{

}

void BlendTree::Init(const gef::SkeletonPose& bind_pose)
{
	bind_pose_ = bind_pose;
	output_.output_pose_ = bind_pose_;
}


void BlendTree::CleanUp()
{

}

void BlendTree::Start()
{
	output_.Start();
}

void BlendTree::Update(float delta_time)
{
	bool valid = output_.Update(delta_time);
}

ClipNode::ClipNode(BlendTree* _tree) :
	BlendNode(_tree)
{
	if (tree_)
		clip_player_.Init(tree_->bind_pose_);
	clip_player_.set_looping(true);
}

void ClipNode::SetClip(const gef::Animation* anim)
{
	clip_player_.set_clip(anim);
}

void ClipNode::StartInternal()
{
	clip_player_.set_anim_time(0.0f);
}

bool ClipNode::Process(float delta_Time)
{
	bool valid = false;

	if (clip_player_.clip())
	{
		clip_player_.Update(delta_Time, tree_->bind_pose_);
		output_pose_ = clip_player_.pose();
		valid = true;
	}

	return valid;
}


OutputNode::OutputNode(BlendTree* _tree) :
	BlendNode(_tree)
{
	inputs_.resize(1);
}


bool OutputNode::Process(float delta_time)
{
	output_pose_ = inputs_[0].node->output_pose_;
	return true;
}


Linear2BlendNode::Linear2BlendNode(BlendTree* _tree) :
	BlendNode(_tree)
{
	inputs_.resize(2);
	variables_.resize(1);
}


bool Linear2BlendNode::Process(float delta_time)
{
	float blend_value = tree_->variables_[variables_[0]];
	output_pose_.Linear2PoseBlend(inputs_[0].node->output_pose_, inputs_[1].node->output_pose_, blend_value);
	return true;
}