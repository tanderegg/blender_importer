#pragma once

#include "BlenderCommon.h"
#include "BlenderFileBlock.h"
#include "BlenderMesh.h"
#include "BlenderArmature.h"

struct BlenderFileHeader {
	char identifier[8];
	unsigned short pointer_size;
	bool little_endian;
	char version[4];
};

class BlenderFile {
public:
	BlenderFile() {}
	BlenderFile(std::string filename, BlenderImporterConfig config);
	~BlenderFile();

	void Load();

	std::string GetFilename();
	std::string GetHeaderInfo();

	int GetNumFileBlocks();
	std::string GetFileBlockInfo(int index);

	bool ExtractSDNA(BlenderFileBlock &block);
	std::string GetSDNAInfo();
	StructureDNA *GetSDNA() { return &m_SDNA; }

	BlenderMesh *GetMesh() { return &m_Mesh; }
	BlenderArmature *GetArmature() { return &m_Armature; }

	void Release();
	void ReleaseFileBlocks();

private:
	std::string m_Filename;
	BlenderImporterConfig m_Config;

	BlenderFileHeader m_FileHeader;
	std::vector<BlenderFileBlock> m_FileBlocks;
	std::vector<BlenderFileBlock> m_MeshBlocks;
	std::vector<BlenderFileBlock> m_ArmatureBlocks;
	StructureDNA m_SDNA;

	BlenderMesh m_Mesh;
	BlenderArmature m_Armature;
};
