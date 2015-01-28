#pragma once
#include "BlenderFileBlock.h"

struct Bone {
	Bone *next;
	Bone *prev;
	void *prop;
	Bone *parent;
	char name[64];
	float roll;
	float head[3];
	float tail[3];
	float bone_mat[3][3];
	int flag;
	float arm_head[3];
	float arm_tail[3];
	float arm_mat[3];
	float arm_roll;
	float dist;
	float weight;
	float xwidth;
	float length;
	float zwidth;
	float ease1;
	float ease2;
	float rad_head;
	float rad_tail;
	float size[3];
	int layer;
	short segments;
	short pad[1];
};

class BlenderArmature {
public:
	BlenderArmature() {}
	~BlenderArmature() {}

	bool LoadArmature(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);

private:
	std::string m_Name;
	Bone *m_Bones;
};