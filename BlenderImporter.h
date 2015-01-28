#pragma once

#include <sstream>

#include "BlenderCommon.h"
#include "BlenderFile.h"

//////////////////////////////
// Encapsulation classes
//////////////////////////////
class BlenderImporter {
public:
	BlenderImporter() {}
	~BlenderImporter() {}

	static BlenderFile LoadBlendFile(std::string filename, BlenderImporterConfig config);
	static unsigned int ComputeFieldLength(std::string field_name, unsigned short length, size_t pointer_size);
};