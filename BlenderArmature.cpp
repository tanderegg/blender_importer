#include "BlenderArmature.h"

bool BlenderArmature::LoadArmature(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	if(blocks.size() > 0) {
		BlenderFileBlock fBlock = blocks[0];

		m_Name = fBlock.GetString("id.name[66]", sdna);

		std::cout << "\nReading data for armature " << m_Name << "..." << std::endl;

		// Read in bone data
		m_Bones = new Bone[blocks.size()-1];
		for(unsigned int i=1; i < blocks.size()-1; i++) {
			BlenderFileBlock block = blocks[i];
			
			Structure *s = sdna->GetStructureFromBlock(&block);
			if(sdna->GetType(s->type_idx) != "Bone") {
				continue;
			}

			//unsigned int offset_name = block.GetMemberOffset("name[64]", sdna);
			std::cout << "Bone Name: " << block.GetString("name[64]", sdna) << std::endl;
			strcpy_s(m_Bones[i-1].name, block.GetString("name[64]", sdna));
			m_Bones[i-1].roll = block.GetFloat("roll", sdna);
		}

		return true;
	}
}