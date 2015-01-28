#include "BlenderFile.h"
#include "BlenderImporter.h"

///////////////////////////////
// BlenderFile implementation
///////////////////////////////
BlenderFile::BlenderFile(std::string filename, BlenderImporterConfig config) {
	m_Filename = filename;
	m_Config = config;
}

BlenderFile::~BlenderFile() {
}

void BlenderFile::Release() {
	m_Mesh.ReleaseMesh();
	//m_Armature.ReleaseArmature();
	ReleaseFileBlocks();
}

void BlenderFile::ReleaseFileBlocks() {
	for(unsigned int i=0; i < m_FileBlocks.size(); i++) {
		m_FileBlocks[i].ReleaseBuffer();
	}

	m_FileBlocks.clear();

	for(unsigned int i=0; i < m_MeshBlocks.size(); i++) {
		m_MeshBlocks[i].ReleaseBuffer();
	}

	m_MeshBlocks.clear();

	for(unsigned int i=0; i < m_ArmatureBlocks.size(); i++) {
		m_ArmatureBlocks[i].ReleaseBuffer();
	}

	m_ArmatureBlocks.clear();
}

std::string BlenderFile::GetFilename() {
	return m_Filename;
}

std::string BlenderFile::GetHeaderInfo() {
	char buffer[256];

	sprintf_s(buffer, "Identifier: %s\nPointer Size: %u\nLittle Endian: %s\nVersion: %s\n",
				m_FileHeader.identifier,
				m_FileHeader.pointer_size,
				(m_FileHeader.little_endian ? "True" : "False"),
				m_FileHeader.version);

	std::string output = buffer;
	return output;
}

void BlenderFile::Load() {
	std::fstream file;
	file.open(m_Filename.c_str(), std::fstream::in | std::fstream::binary);

	if(!file.is_open()) {
		assert(0 && "Failed to open file.");
	}

	/////////////////////////////////////////////////////////////
	// BLEND file header is 12 bytes, see BlendFileHeader struct
	/////////////////////////////////////////////////////////////
	char header[12];
	file.read(header, 12);

	for(int i=0; i < 7; i++) {
		m_FileHeader.identifier[i] = header[i];
	}
	
	m_FileHeader.identifier[7] = 0;

	if(strcmp("BLENDER", m_FileHeader.identifier) != 0) {
		assert(0 && "File Header Identifier is incorrect, this is either a compressed file or not a blend file.");
	}

	if(header[7] == '_') {
		m_FileHeader.pointer_size = 4;
	}
	else if(header[7] == '-') {
		m_FileHeader.pointer_size = 8;
	}
	else {
		assert(0 && "Invalid pointer size detected.");
	}

	if(header[8] == 'v') {
		m_FileHeader.little_endian = true;
	}
	else {
		m_FileHeader.little_endian = false;
	}

	for(int i=0; i < 3; i++) {
		m_FileHeader.version[i] = header[i+9];
	}
	m_FileHeader.version[3] = 0;

	std::cout << "\n\nHeader Info: \n\n" << GetHeaderInfo();

	///////////////////////
	// Load in file blocks
	///////////////////////
	
	BlenderFileBlock fileBlock;
	int headerSize = 16 + m_FileHeader.pointer_size;
	char *fbHeader = new char[headerSize];
	int counter = 0;
	int total_counter = 0;
	bool loadingMeshData = false;
	bool loadingArmatureData = false;

	std::cout << "Loading Fileblocks...\n";

	do {
		fileBlock.Load(&file, m_FileHeader.pointer_size);

		if(strcmp("DNA1", fileBlock.m_Header.code) == 0) {
			// Extract the Structure DNA and release the block
			bool result = ExtractSDNA(fileBlock);

			if(!result) {
				assert(0 && "There was an error loading SDNA data");
			}

			fileBlock.ReleaseBuffer();
		} else if(strcmp("ME", fileBlock.m_Header.code) == 0) {
			//std::cout << "\n\nMesh data sdna: " << fileBlock.m_Header.sdna << "\n\n";
			loadingMeshData = true;
			m_MeshBlocks.push_back(fileBlock);
		} else if(strcmp("AR", fileBlock.m_Header.code) == 0) {
			loadingArmatureData = true;
			m_ArmatureBlocks.push_back(fileBlock);
		}
		else {
			if(loadingMeshData && strcmp("DATA", fileBlock.m_Header.code) == 0) {
				m_MeshBlocks.push_back(fileBlock);
			} else if(loadingArmatureData && strcmp("DATA", fileBlock.m_Header.code) == 0) {
				m_ArmatureBlocks.push_back(fileBlock);
			}
			else {
				if(loadingMeshData) {
					loadingMeshData = false;
				}

				if(loadingArmatureData) {
					loadingArmatureData = false;
				}

				if(strcmp("DATA", fileBlock.m_Header.code) != 0) {
					std::cout << std::endl << fileBlock.m_Header.code;
					counter = 0;
				}
				else {
					counter += 1;
					if(counter%10 == 0) {
						std::cout << ".";
					}	
				}

				m_FileBlocks.push_back(fileBlock);
			}
		}

		total_counter += 1;

	} while (strcmp("ENDB", fileBlock.m_Header.code) != 0);

	std::cout << total_counter << " data blocks processed.\n";

	// find MDeformWeight block
	for(unsigned int i=0; i < m_FileBlocks.size(); i++) {
		Structure *s = m_SDNA.GetStructureFromBlock(&m_FileBlocks[i]);

		if(m_SDNA.GetType(s->type_idx) == "MDeformWeight") {
			std::cout << "MDeformWeight found!";
		}
	}

	delete[] fbHeader;
	file.close();

	m_Mesh.LoadMesh(&m_SDNA, m_MeshBlocks, m_Config.triangulate, m_Config.vertexUVs, m_Config.flipYZ);
	m_Armature.LoadArmature(&m_SDNA, m_ArmatureBlocks);

	std::cout << "\nMesh Data:\n" << m_Mesh.GetMeshInfo().c_str();

	/*for(int i=0; i < m_SDNA.structures.size(); i++) {
		std::cout << i << ": " << m_SDNA.types[m_SDNA.structures[i].type_idx] << ", " << m_SDNA.structures[i].fields.size() << " fields\n";
	}*/
}

bool BlenderFile::ExtractSDNA(BlenderFileBlock &block) {
	std::cout << "\n\nLoading SDNA\n\n";

	// Make sure there's nothing in here
	m_SDNA.names.clear();
	m_SDNA.lengths.clear();
	m_SDNA.types.clear();
	m_SDNA.structures.clear();

	// Get the buffer and initialize buffer pointer position
	unsigned char *buffer = block.GetBuffer();
	size_t pos = 0;

	// SDNA Identifier
	if(strncmp("SDNA", (char *)&buffer[pos], 4) != 0) {
		assert(0 && "SDNA File Block Invalid");
	}
	pos += 4;

	// NAMEs identifier
	if(strncmp("NAME", (char *)&buffer[pos], 4) != 0) {
		assert(0 && "SDNA File Block Invalid");
	}
	pos += 4;

	// Get names
	std::cout << "Loading name data...\n";
	unsigned int numNames = *((unsigned int *)&buffer[pos]);
	pos += 4;

	for(unsigned int i=0; i < numNames; i++) {
		m_SDNA.names.push_back(std::string((char *)&buffer[pos]));
		pos += (m_SDNA.names[i].size() + (size_t)1);
	}

	std::cout << "Number of names: " << m_SDNA.names.size() << "\n";

	// TYPEs identifier
	while((pos%4) != 0) pos++; // 4 byte alignment
	if(strncmp("TYPE", (char *)&buffer[pos], 4) != 0) {
		assert(0 && "SDNA File Block Invalid");
	}
	pos += 4;

	// Get types
	std::cout << "Loading type data...\n";
	unsigned int numTypes = *((unsigned int *)&buffer[pos]);
	pos += 4;

	for(unsigned int i=0; i < numTypes; i++) {
		m_SDNA.types.push_back(std::string((char *)&buffer[pos]));
		pos += (m_SDNA.types[i].size() + (size_t)1);
	}

	std::cout << "Number of types: " << m_SDNA.types.size() << "\n";

	// LEGNTHs identifier
	while((pos%4) != 0) pos++; // 4 byte alignment
	if(strncmp("TLEN", (char *)&buffer[pos], 4) != 0) {
		assert(0 && "SDNA File Block Invalid");
	}
	pos += 4;

	// get lengths
	std::cout << "Loading type length data...\n";
	for(unsigned int i=0; i < numTypes; i++) {
		m_SDNA.lengths.push_back(*(unsigned short *)&buffer[pos]);
		pos += 2;
	}

	std::cout << "Number of lengths: " << m_SDNA.lengths.size() << "\n";

	// STRUCTURES
	while((pos%4) != 0) pos++; // 4 byte alignment
	if(strncmp("STRC", (char *)&buffer[pos], 4) != 0) {
		assert(0 && "SDNA File Block Invalid");
	}
	pos += 4;

	// Get structures
	std::cout << "Loading structure data...\n";
	unsigned int numStructs = *((unsigned int *)&buffer[pos]);
	pos += 4;

	for(unsigned int i=0; i < numStructs; i++) {
		Structure structure;
		Field field;

		// Get the data type of this structure
		structure.type_idx = *((unsigned short *)&buffer[pos]);
		pos += 2;

		// Get each field
		size_t numFields = *((unsigned short *)&buffer[pos]);
		pos += 2;

		int offset = 0;
		for (unsigned int k=0; k < numFields; k++) {
			// Index into SDNA types array
			field.type_idx = *((unsigned short *)&buffer[pos]);
			pos += 2;
					
			// Index into SDNA names array
			field.name_idx = *((unsigned short *)&buffer[pos]);
			pos += 2;

			field.offset = offset;
			offset += BlenderImporter::ComputeFieldLength(m_SDNA.names[field.name_idx], m_SDNA.lengths[field.type_idx], m_FileHeader.pointer_size);

			structure.fields.push_back(field);
		}

		m_SDNA.structures.push_back(structure);
	}

	std::cout << "Number of structures: " << m_SDNA.structures.size() << "\n";

	return true;
}

int BlenderFile::GetNumFileBlocks() {
	return m_FileBlocks.size();
}

std::string BlenderFile::GetFileBlockInfo(int index) {
	char buffer[256];

	sprintf_s(buffer, "File Block Code: %s\nSize: %u\nOld Address: %p\nSDNA: %u\nCount: %u\n",
				m_FileBlocks[index].m_Header.code,
				m_FileBlocks[index].m_Header.size,
				m_FileBlocks[index].m_Header.old_mem_address,
				m_FileBlocks[index].m_Header.sdna,
				m_FileBlocks[index].m_Header.count);

	std::string output = buffer;
	return output;
}

std::string BlenderFile::GetSDNAInfo() {
	std::string output = "NAMES:\n\n";
	char buffer[256];

	for(unsigned int i=0; i < m_SDNA.structures.size(); i++) {
		sprintf_s(buffer, "%d: %s", m_SDNA.structures[i].type_idx, m_SDNA.GetType(m_SDNA.structures[i].type_idx).c_str());
		output += buffer;

		if(i%4 == 0) {
			output += "\n";
		}
		else {
			output += "\t";
		}
	}

	output += "\n";

	/*for(unsigned int i=0; i < m_SDNA.names.size(); i++) {
		sprintf_s(buffer, "%s, ", m_SDNA.names[i].c_str());
		output += buffer;
	}

	output += "TYPES:\n\n";

	for(unsigned int i=0; i < m_SDNA.types.size(); i++) {
		sprintf_s(buffer, "%s, ", m_SDNA.types[i].c_str());
		output += buffer;
	}

	output += "LENGTHS:\n\n";

	for(unsigned int i=0; i < m_SDNA.lengths.size(); i++) {
		sprintf_s(buffer, "%hu, ", m_SDNA.lengths[i]);
		output += buffer;
	}*/

	return output;
}