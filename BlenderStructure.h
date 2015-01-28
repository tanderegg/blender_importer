#pragma once

#include <string>
#include <vector>
#include <iostream>

struct Field {
	unsigned short type_idx;
	unsigned short name_idx;
	unsigned int offset;
};

struct Structure {
	unsigned short type_idx;
	std::vector<Field> fields;
};

class BlenderFileBlock;
struct StructureDNA {
	std::vector<std::string> names;
	std::vector<std::string> types;
	std::vector<unsigned short> lengths;
	std::vector<Structure> structures;

	Structure *GetStructureByType(std::string type);
	Structure *GetStructureByTypeIndex(unsigned int type_idx);
	Structure *GetStructureFromBlock(BlenderFileBlock *fBlock);
	std::string GetType(unsigned short type_idx) { return types[type_idx]; }
	std::string GetName(unsigned short name_idx) { return names[name_idx]; }
};
