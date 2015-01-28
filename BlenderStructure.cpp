#include "BlenderFileBlock.h"

///////////////////////////////
//
// StuctureDNA implementation
//
///////////////////////////////
Structure *StructureDNA::GetStructureByType(std::string type) {
	unsigned int type_idx;
	for(unsigned int i=0; i < types.size(); i++) {
		if(types[i] == type) {
			type_idx = i;
			break;
		}
	}

	return GetStructureByTypeIndex(type_idx);
}

Structure *StructureDNA::GetStructureByTypeIndex(unsigned int type_idx) {
	for(unsigned int i=0; i < structures.size(); i++) {
		if(structures[i].type_idx == type_idx) {
			return &structures[i];
		}
	}

	return 0;
}

Structure *StructureDNA::GetStructureFromBlock(BlenderFileBlock *fBlock) {
	return &structures[fBlock->m_Header.sdna];
}