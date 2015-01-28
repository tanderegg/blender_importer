#pragma once

#include <fstream>
#include <cassert>

#include "BlenderStructure.h"

struct BlenderFileBlockHeader {
	char code[5];
	unsigned int size;
	void *old_mem_address;
	unsigned int sdna;
	unsigned int count;
};

class BlenderFileBlock {
public:
	BlenderFileBlock() { m_Buffer = 0; }
	~BlenderFileBlock() {}

	void InitBuffer(size_t size) { m_Buffer = new unsigned char[size]; }
	unsigned char *GetBuffer() { return m_Buffer; }
	void ReleaseBuffer() { if(m_Buffer) delete[] m_Buffer; }

	int GetMemberOffset(const char *name, StructureDNA *sdna);
	void GetOffsets(int *offsetStruct, int *offsetField, const char *structName, const char *fieldName, StructureDNA *sdna);
	void *GetPointer(unsigned int offset, unsigned int iteration, unsigned int structLength);
	char GetChar(unsigned int offset, unsigned int iteration, unsigned int structLength);
	int GetInt(char *name, StructureDNA *sdna);
	int GetInt(unsigned int offset, unsigned int iteration, unsigned int structLength);
	short GetShort(char *name, StructureDNA *sdna);
	short GetShort(unsigned int offset, unsigned int iteration, unsigned int structLength);
	float GetFloat(char *name, StructureDNA *sdna);
	float GetFloat(unsigned int offset, unsigned int iteration, unsigned int structLength);
	const char *GetString(char *name, StructureDNA *sdna);

	void Load(std::fstream *file, unsigned short pointer_size);

	BlenderFileBlockHeader m_Header;

private:
	unsigned char *m_Buffer;
};