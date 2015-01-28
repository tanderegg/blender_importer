#include "BlenderFileBlock.h"

////////////////////////////////////
// BlenderFileBlock implementation
////////////////////////////////////
void BlenderFileBlock::Load(std::fstream *file, unsigned short pointer_size) {
	file->read(m_Header.code, 4);
	m_Header.code[4] = 0;

	file->read((char *)&(m_Header.size), 4);
	file->read((char *)&(m_Header.old_mem_address), pointer_size);
	file->read((char *)&(m_Header.sdna), 4);
	file->read((char *)&(m_Header.count), 4);

	InitBuffer(m_Header.size);
	file->read((char *)GetBuffer(), m_Header.size);
}

// First version retrieves a value when 'count' is known to be one
// Second version retrieves a value when iterating over many instances
// of an object.
void *BlenderFileBlock::GetPointer(unsigned int offset, unsigned int iteration, unsigned int structLength) {
	return (void*)&m_Buffer[offset + iteration * structLength];
}

char BlenderFileBlock::GetChar(unsigned int offset, unsigned int iteration, unsigned int structLength) {
	return *((char*)&m_Buffer[offset + iteration * structLength]);
}

int BlenderFileBlock::GetInt(char *name, StructureDNA *sdna) {
      int offset = GetMemberOffset(name, sdna);
	  return (offset == -1) ? -1 : *((int*)&m_Buffer[offset]);
}

int BlenderFileBlock::GetInt(unsigned int offset, unsigned int iteration, unsigned int structLength) {
      return *((int*)&m_Buffer[offset + iteration * structLength]);
}

short BlenderFileBlock::GetShort(char *name, StructureDNA *sdna) {
      int offset = GetMemberOffset(name, sdna);
	  return (offset == -1) ? -1 : *((short*)&m_Buffer[offset]);
}

short BlenderFileBlock::GetShort(unsigned int offset, unsigned int iteration, unsigned int structLength) {
      return *((short*)&m_Buffer[offset + iteration * structLength]);
}

float BlenderFileBlock::GetFloat(char *name, StructureDNA *sdna) {
      int offset = GetMemberOffset(name, sdna);
	  return (offset == -1) ? -1 : *((float*)&m_Buffer[offset]);
}

float BlenderFileBlock::GetFloat(unsigned int offset, unsigned int iteration, unsigned int structLength) {
      return *((float*)&m_Buffer[offset + iteration * structLength]);
}

const char* BlenderFileBlock::GetString(char *name, StructureDNA *sdna) {
      int offset = GetMemberOffset(name, sdna);      
	  return (offset == -1) ? "n/a" : (const char *)&m_Buffer[offset];
}

int BlenderFileBlock::GetMemberOffset(const char *name, StructureDNA *sdna) {
      std::string n(name);
      size_t pos = n.find('.');

	  // Split field name and calculate
	  // total offset if this is a combined field
      if (pos != std::string::npos) {
            std::string structname = n.substr(0, pos);
            std::string fieldname = n.substr(pos+1, n.size()-pos-1);
            
            int offset_struct = 0;
            int offset_field = 0;
            
			GetOffsets(&offset_struct, &offset_field, structname.c_str(), fieldname.c_str(), sdna);
            return offset_struct + offset_field;
      }
	  else {
		  // Otherwise, just load the appropriate structure,
		  // find the field, and return the offset
		  Structure structure = sdna->structures[m_Header.sdna];

		  for (unsigned int i = 0; i < structure.fields.size(); i++) {
			  if (strcmp(name, sdna->names[structure.fields[i].name_idx].c_str()) == 0) {
				  return structure.fields[i].offset;
			  }
		  }
		  return -1;
	  }
}

void BlenderFileBlock::GetOffsets(int *offsetStruct, int *offsetField, const char *structName, const char *fieldName, StructureDNA *sdna) {
	// First retrieve the structure
	Structure structure = sdna->structures[m_Header.sdna];

	// Run through the fields and find the right sub structure
	for (unsigned int i = 0; i < structure.fields.size(); i++) {
		if (strcmp(structName, sdna->names[structure.fields[i].name_idx].c_str()) == 0) {
			// we found the structure
			Field field = structure.fields[i];
			*offsetStruct = field.offset;

			// get info for substructure
			Structure *subStructure = sdna->GetStructureByTypeIndex(field.type_idx);

			//std::cout << "Structure Found: " << sdna->GetType(subStructure->type_idx).c_str() << "\n";
			//std::cout << "Num fields: " << subStructure->fields.size() << "\n";

			// find offset in substructure:
			for (unsigned int k = 0; k < subStructure->fields.size(); k++) {
				// PROBLEM IS HERE
				
				//std::cout << "Field: " << sdna->GetName(subStructure->fields[k].name_idx).c_str() << "\n";
				if (strcmp(fieldName, sdna->GetName(subStructure->fields[k].name_idx).c_str()) == 0) {
					// found field
					*offsetField = subStructure->fields[k].offset;
					return;
				}
			}
		}
	}
	assert(0 && "Sub structure field not found!");
}