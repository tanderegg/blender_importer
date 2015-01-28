#include "BlenderMesh.h"

////////////////////////////////////////
// BlenderMesh implementation
//
// An object to store mesh data
////////////////////////////////////////
BlenderMesh::BlenderMesh() {
	m_Vertices = 0;
	m_Loops = 0;
	m_LoopUVs = 0;
	m_Polygons = 0;
	m_TexPolygons = 0;

	m_Faces = 0;
	m_TexFaces = 0;
	m_DeformVerts = 0;
	m_DeformWeights = 0;
}

std::string BlenderMesh::GetMeshInfo() {
	char buffer[256];

	sprintf_s(buffer, "Name: %s\nTotal Vertices: %d\nTotal Edges: %d\nTotal Faces: %d\nTotal Loops: %d\nTotal Polygons: %d\n",
				m_Name.c_str(),
				m_TotalVerts,
				m_TotalEdges,
				m_TotalFaces,
				m_TotalLoops,
				m_TotalPolygons);

	return std::string(buffer);
}

bool BlenderMesh::LoadMesh(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks, bool triangulate, bool vertexUVs, bool flipYZ) {
	BlenderFileBlock fBlock = blocks[0];

	m_TotalVerts = fBlock.GetInt("totvert", sdna);
	m_TotalEdges = fBlock.GetInt("totedge", sdna);
	m_TotalFaces = fBlock.GetInt("totface", sdna);
	m_TotalLoops = fBlock.GetInt("totloop", sdna);
	m_TotalPolygons = fBlock.GetInt("totpoly", sdna);

	m_Name = fBlock.GetString("id.name[66]", sdna);

	m_Vertices		= ExtractVertices(sdna, blocks, flipYZ);
	m_Faces			= ExtractFaces(sdna, blocks);
	//m_TexFaces	= ExtractTexFaces(sdna, blocks);
	m_DeformVerts	= ExtractDeformVerts(sdna, blocks);
	m_DeformWeights = ExtractDeformWeights(sdna, blocks);

	// If there are no faces, than this is a
	// newer blend file which uses MPolys and MLoops
	if(m_Faces == 0) {
		m_Loops		= ExtractLoops(sdna, blocks);
		m_LoopUVs	= ExtractLoopUVs(sdna, blocks);
		m_Polygons	= ExtractPolys(sdna, blocks);
		m_TexPolygons = ExtractTexPolys(sdna, blocks);

		ConvertPolysToFaces();
	}

	if(triangulate)
		Triangulate();

	if(vertexUVs)
		UVsToVerts();

	return true;
}

void BlenderMesh::ReleaseMesh() {
	if(m_Vertices) {
		delete[] m_Vertices;
		m_Vertices=0;
	}

	if(m_Loops) {
		delete[] m_Loops;
		m_Loops=0;
	}

	if(m_LoopUVs) {
		delete[] m_LoopUVs;
		m_LoopUVs = 0;
	}

	if(m_Polygons) {
		delete[] m_Polygons;
		m_Polygons = 0;
	}

	if(m_TexPolygons) {
		delete[] m_TexPolygons;
		m_TexPolygons = 0;
	}

	if(m_Faces) {
		delete[] m_Faces;
		m_Faces=0;
	}

	if(m_TexFaces) {
		delete[] m_TexFaces;
		m_TexFaces=0;
	}

	if(m_DeformVerts) {
		delete[] m_DeformVerts;
		m_DeformVerts = 0;
	}

	if(m_DeformWeights) {
		delete[] m_DeformWeights;
		m_DeformWeights = 0;
	}
}

MVert *BlenderMesh::ExtractVertices(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks, bool flipYZ) {
	Structure *mVert = sdna->GetStructureByType("MVert");
	unsigned int length = sdna->lengths[mVert->type_idx];

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		// Get block structure
		unsigned int type_idx = sdna->GetStructureFromBlock(&fBlock)->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		// Check if we have found the vertices block
		if (strcmp("MVert", blockStructureName.c_str()) == 0) {
			std::cout << "MVert Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MVert *vertices = new MVert[count];
			
			// Retrieve the member offsets for this structure
			// for the fields we are interested in
			int offset_co = blocks[i].GetMemberOffset("co[3]", sdna);
			int offset_no = blocks[i].GetMemberOffset("no[3]", sdna);
			
			// Iterate and retreive the data
			for (unsigned int k=0; k < count; k++) {
				vertices[k].co[0] = blocks[i].GetFloat(offset_co, k, length);

				if(flipYZ) {
					vertices[k].co[2] = -blocks[i].GetFloat(offset_co+4, k, length);
					vertices[k].co[1] = blocks[i].GetFloat(offset_co+8, k, length);
				} else {
					vertices[k].co[1] = blocks[i].GetFloat(offset_co+4, k, length);
					vertices[k].co[2] = blocks[i].GetFloat(offset_co+8, k, length);
				}

				vertices[k].no[0] = blocks[i].GetShort(offset_no, k, length);

				if(flipYZ) {
					vertices[k].no[2] = -blocks[i].GetShort(offset_no+2, k, length);
					vertices[k].no[1] = blocks[i].GetShort(offset_no+4, k, length);
				}
				else {
					vertices[k].no[1] = blocks[i].GetShort(offset_no+2, k, length);
					vertices[k].no[2] = blocks[i].GetShort(offset_no+4, k, length);
				}

				vertices[k].isUVSet = false;
				vertices[k].nextSupplVert = -1;
				vertices[k].uv[0] = 0.0f;
				vertices[k].uv[1] = 0.0f;
			}

			return vertices;
		}
	}

	std::cout << "No MVert Block Found!\n";
	return 0;
}

MFace *BlenderMesh::ExtractFaces(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mFace = sdna->GetStructureByType("MFace");
	unsigned int length = sdna->lengths[mFace->type_idx];

	std::cout << "\nFace structure: " << sdna->GetType(mFace->type_idx) << "\n\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);

		if(type_idx != 95) {
			std::cout << "Block Structure ID: " << type_idx << ", Name: " << blockStructureName << "\n";
			for(unsigned int j=0; j < s->fields.size(); j++) {
				std::string name = sdna->GetName(s->fields[j].name_idx);
				std::string type = sdna->GetType(s->fields[j].type_idx);
				std::cout << "\t" << s->fields[j].offset << ": " << name << "(" << type << ")\n";
			}

			std::cout << "\n\tCount: " << fBlock.m_Header.count << "\n\n";
		}
		
		if (strcmp("MFace", blockStructureName.c_str()) == 0) {
			std::cout << "MFace Block Found!\n";

			// NOT IMPLEMENTED FOR NOW
			// MFace used in old versions of blender
		}
	}

	std::cout << "No MFace Block Found!\n";
	return 0;
}

MLoop *BlenderMesh::ExtractLoops(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mLoop = sdna->GetStructureByType("MLoop");
	unsigned int length = sdna->lengths[mLoop->type_idx];

	std::cout << "\nLoop structure: " << sdna->GetType(mLoop->type_idx) << "\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		if (strcmp("MLoop", blockStructureName.c_str()) == 0) {
			std::cout << "MLoop Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MLoop *loops = new MLoop[count];
			int offset_v = blocks[i].GetMemberOffset("v", sdna);
			int offset_e = blocks[i].GetMemberOffset("e", sdna);

			for (unsigned int k=0; k < count; k++) {
				loops[k].v = blocks[i].GetInt(offset_v, k, length);
				loops[k].e = blocks[i].GetInt(offset_e, k, length);
			}

			return loops;
		}
	}

	std::cout << "No MLoop Block Found!\n";
	return 0;
}

MLoopUV *BlenderMesh::ExtractLoopUVs(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mLoopUV = sdna->GetStructureByType("MLoopUV");
	unsigned int length = sdna->lengths[mLoopUV->type_idx];

	std::cout << "\nLoopUV structure: " << sdna->GetType(mLoopUV->type_idx) << "\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		if (strcmp("MLoopUV", blockStructureName.c_str()) == 0) {
			std::cout << "MLoopUV Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MLoopUV *loops = new MLoopUV[count];
			int offset_uv = blocks[i].GetMemberOffset("uv[2]", sdna);

			for (unsigned int k=0; k < count; k++) {
				loops[k].uv[0] = blocks[i].GetFloat(offset_uv, k, length);
				loops[k].uv[1] = blocks[i].GetFloat(offset_uv+4, k, length);
			}

			return loops;
		}
	}

	std::cout << "No MLoopUV Block Found!\n";
	return 0;
}

MPoly *BlenderMesh::ExtractPolys(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mPoly = sdna->GetStructureByType("MPoly");
	unsigned int length = sdna->lengths[mPoly->type_idx];

	std::cout << "\nPolygon structure: " << sdna->GetType(mPoly->type_idx) << "\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		if (strcmp("MPoly", blockStructureName.c_str()) == 0) {
			std::cout << "MPoly Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MPoly *polys = new MPoly[count];
			int offset_loopstart = blocks[i].GetMemberOffset("loopstart", sdna);
			int offset_totloop = blocks[i].GetMemberOffset("totloop", sdna);
			int offset_mat_nr = blocks[i].GetMemberOffset("mat_nr", sdna);

			for (unsigned int k=0; k < count; k++) {
				polys[k].loopstart = blocks[i].GetInt(offset_loopstart, k, length);
				polys[k].totloop = blocks[i].GetInt(offset_totloop, k, length);
				polys[k].mat_nr = blocks[i].GetShort(offset_mat_nr, k, length);
			}

			return polys;
		}
	}

	std::cout << "No MPoly Block Found!\n";
	return 0;
}

MTexPoly *BlenderMesh::ExtractTexPolys(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mTexPoly = sdna->GetStructureByType("MTexPoly");
	unsigned int length = sdna->lengths[mTexPoly->type_idx];

	std::cout << "\nTexPolygon structure: " << sdna->GetType(mTexPoly->type_idx) << "\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		if (strcmp("MTexPoly", blockStructureName.c_str()) == 0) {
			std::cout << "MTexPoly Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MTexPoly *texPolys = new MTexPoly[count];

			int offset_tpage	= blocks[i].GetMemberOffset("*tpage", sdna);
			int offset_flag		= blocks[i].GetMemberOffset("flag", sdna);
			int offset_transp	= blocks[i].GetMemberOffset("transp", sdna);
			int offset_mode		= blocks[i].GetMemberOffset("mode", sdna);
			int offset_tile		= blocks[i].GetMemberOffset("tile", sdna);

			for (unsigned int k=0; k < count; k++) {
				texPolys[k].tpage	= blocks[i].GetPointer(offset_tpage, k, length);
				texPolys[k].flag	= blocks[i].GetChar(offset_flag, k, length);
				texPolys[k].transp	= blocks[i].GetChar(offset_transp, k, length);
				texPolys[k].mode	= blocks[i].GetShort(offset_mode, k, length);
				texPolys[k].tile	= blocks[i].GetShort(offset_tile, k, length);
				texPolys[k].pad		= 0;
			}

			return texPolys;
		}
	}

	std::cout << "No MPoly Block Found!\n";
	return 0;
}

MDeformVert	*BlenderMesh::ExtractDeformVerts(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mDeformVert = sdna->GetStructureByType("MDeformVert");
	unsigned int length = sdna->lengths[mDeformVert->type_idx];

	std::cout << "\nMDeformVert structure: " << sdna->GetType(mDeformVert->type_idx) << "\n";

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);
		
		if (strcmp("MDeformVert", blockStructureName.c_str()) == 0) {
			std::cout << "MDeformVert Block Found!\n";

			unsigned int count = blocks[i].m_Header.count;
			MDeformVert *deformVerts = new MDeformVert[count];

			int offset_totWeight = blocks[i].GetMemberOffset("totweight", sdna);
			int offset_flag = blocks[i].GetMemberOffset("flag", sdna);

			for (unsigned int k=0; k < count; k++) {
				deformVerts[k].totWeight	= blocks[i].GetInt(offset_totWeight, k, length);
				deformVerts[k].flag			= blocks[i].GetInt(offset_flag, k, length);
			}

			return deformVerts;
		}
	}

	std::cout << "No MDeformVert Block Found!\n";
	return 0;
}

MDeformWeight *BlenderMesh::ExtractDeformWeights(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks) {
	Structure *mDeformWeight = sdna->GetStructureByType("MDeformWeight");
	unsigned int length = sdna->lengths[mDeformWeight->type_idx];

	std::cout << "\nMDeformWeight structure: " << sdna->GetType(mDeformWeight->type_idx) << "\n";

	// First get total count
	unsigned int total_count=0;

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);

		if (strcmp("MDeformWeight", blockStructureName.c_str()) == 0) {			
			unsigned int count = blocks[i].m_Header.count;
			total_count += count;
		}
	}

	if(total_count > 0)
		std::cout << "MDeformWeight Block Found!\n";
	else {
		std::cout << "No MDeformWeight Block Found!\n";
		return 0;
	}

	MDeformWeight *deformWeights = new MDeformWeight[total_count];

	for (unsigned int i=0; i < blocks.size(); i++) {
		BlenderFileBlock fBlock = blocks[i];

		Structure *s = sdna->GetStructureFromBlock(&fBlock);
		unsigned int type_idx = s->type_idx;
		std::string blockStructureName = sdna->GetType(type_idx);

		unsigned int count = blocks[i].m_Header.count;

		if (strcmp("MDeformWeight", blockStructureName.c_str()) == 0) {
			int offset_def_nr = blocks[i].GetMemberOffset("def_nr", sdna);
			int offset_weight = blocks[i].GetMemberOffset("weight", sdna);

			for (unsigned int k=0; k < count; k++) {
				deformWeights[k].def_nr	= blocks[i].GetInt(offset_def_nr, k, length);
				deformWeights[k].weight = blocks[i].GetFloat(offset_weight, k, length);
				//std::cout << "DefNR: " << deformWeights[k].def_nr << ", Weight: " << deformWeights[k].weight << std::endl;
			}
		}
	}

	return deformWeights;
}

void BlenderMesh::ConvertPolysToFaces() {
	if(m_Faces !=0 || m_Polygons == 0 || m_Loops == 0 || m_LoopUVs == 0) {
		assert(0 && "Faces must be null, and polygon data must exist");
	}

	m_Faces = new MFace[m_TotalPolygons];
	m_TexFaces = new MTFace[m_TotalPolygons];
	for(int i=0; i < m_TotalPolygons; i++) {
		MPoly polygon		= m_Polygons[i];
		MTexPoly texPolygon = m_TexPolygons[i];
		MFace face;
		MTFace texFace;

		if(polygon.totloop > 4) {
			assert(0 && "Polygon with more than 4 vertices detected.");
			continue;
		}

		face.mat_nr = polygon.mat_nr;
		face.isQuad = true;

		//std::cout << "Face: " << face.v1 << " " << face.v2 << " " << face.v3 << " " << face.v4 << "\n";

		for(int j=0; j < 4; j++) {
			if(polygon.totloop > j) 
				face.v[j] = m_Loops[polygon.loopstart+j].v;
			else
				face.v[j] = 0;

			texFace.uv[j][0] = m_LoopUVs[polygon.loopstart+j].uv[0];
			texFace.uv[j][1] = m_LoopUVs[polygon.loopstart+j].uv[1];
		}

		texFace.flag	= texPolygon.flag;
		texFace.mode	= texPolygon.mode;
		texFace.tile	= texPolygon.tile;
		texFace.transp	= texPolygon.transp;
		texFace.tpage	= texPolygon.tpage;
		texFace.unwrap	= false;
		
		m_Faces[i] = face;
		m_TexFaces[i] = texFace;
		m_TotalFaces += 1;
	}

	if(m_LoopUVs) {
		delete[] m_LoopUVs;
		m_LoopUVs = 0;
	}
	if(m_Loops) {
		delete[] m_Loops;
		m_Loops = 0;
	}
	m_TotalLoops = 0;

	if(m_Polygons) {
		delete[] m_Polygons;
		m_Polygons = 0;
	}
	m_TotalPolygons = 0;
}

// NOTE: This assumes that all faces are quads!
void BlenderMesh::Triangulate() {
	MFace *triFaces = new MFace[m_TotalFaces*2];
	MTFace *triTexFaces = new MTFace[m_TotalFaces*2];

	unsigned int outFace = 0;
	for(int i=0; i < m_TotalFaces; i++) {
		MFace *face = &m_Faces[i];
		MTFace *texFace = &m_TexFaces[i];

		triFaces[outFace].supplV1 = false;
		triFaces[outFace].supplV2 = false;
		triFaces[outFace].supplV3 = false;
		triFaces[outFace].isQuad = false;
		triFaces[outFace].mat_nr = face->mat_nr;
		triFaces[outFace].v[0] = face->v[0];
		triFaces[outFace].v[1] = face->v[1];
		triFaces[outFace].v[2] = face->v[2];

		triTexFaces[outFace].flag = texFace->flag;
		triTexFaces[outFace].mode = texFace->mode;
		triTexFaces[outFace].tile = texFace->tile;
		triTexFaces[outFace].tpage = texFace->tpage;
		triTexFaces[outFace].transp = texFace->transp;
		triTexFaces[outFace].unwrap = texFace->unwrap;
		triTexFaces[outFace].uv[0][0] = texFace->uv[0][0];
		triTexFaces[outFace].uv[0][1] = texFace->uv[0][1];
		triTexFaces[outFace].uv[1][0] = texFace->uv[1][0];
		triTexFaces[outFace].uv[1][1] = texFace->uv[1][1];
		triTexFaces[outFace].uv[2][0] = texFace->uv[2][0];
		triTexFaces[outFace].uv[2][1] = texFace->uv[2][1];

		outFace++;

		// If this is a quad, split
		if(face->v[4] != 0) {
			triFaces[outFace-1].isQuad = true;
			triFaces[outFace].isQuad = true;
			triFaces[outFace].mat_nr = face->mat_nr;
			triFaces[outFace].supplV1 = false;
			triFaces[outFace].supplV2 = false;
			triFaces[outFace].supplV3 = false;
			triFaces[outFace].v[0] = face->v[0];
			triFaces[outFace].v[1] = face->v[2];
			triFaces[outFace].v[2] = face->v[3];

			triTexFaces[outFace].flag = texFace->flag;
			triTexFaces[outFace].mode = texFace->mode;
			triTexFaces[outFace].tile = texFace->tile;
			triTexFaces[outFace].tpage = texFace->tpage;
			triTexFaces[outFace].transp = texFace->transp;
			triTexFaces[outFace].unwrap = texFace->unwrap;
			triTexFaces[outFace].uv[0][0] = texFace->uv[0][0];
			triTexFaces[outFace].uv[0][1] = texFace->uv[0][1];
			triTexFaces[outFace].uv[1][0] = texFace->uv[2][0];
			triTexFaces[outFace].uv[1][1] = texFace->uv[2][1];
			triTexFaces[outFace].uv[2][0] = texFace->uv[3][0];
			triTexFaces[outFace].uv[2][1] = texFace->uv[3][1];

			outFace++;
		}
	}

	m_TotalFaces = outFace;
	delete[] m_TexFaces;
	m_TexFaces = triTexFaces;

	delete[] m_Faces;
	m_Faces = triFaces;
}

// NOTE: This assumes that faces have already been triangulated
void BlenderMesh::UVsToVerts() {
	std::vector<MVert> newVertices;
	unsigned int newVertCount = 0;

	for(int i=0; i < m_TotalFaces; i++) {
		MFace *face = &m_Faces[i];
		MTFace *texFace = &m_TexFaces[i];
		MVert newVert;

		for(unsigned int j=0; j < 3; j++) {
			if(!m_Vertices[face->v[j]].isUVSet) {
				m_Vertices[face->v[j]].uv[0] = texFace->uv[j][0];
				m_Vertices[face->v[j]].uv[1] = texFace->uv[j][1];
				m_Vertices[face->v[j]].isUVSet = true;
			}
			else if(m_Vertices[face->v[j]].uv[0] != texFace->uv[j][0] ||
					m_Vertices[face->v[j]].uv[1] != texFace->uv[j][1]) {

				//newVert = findExistingDup(&newVertices, m_Vertices[face->v[j]].co, texFace->uv[j]);

				// Duplicate vertex and assign new uv
				newVert = m_Vertices[face->v[j]];
				newVert.uv[0] = texFace->uv[j][0];
				newVert.uv[1] = texFace->uv[j][1];
				newVertices.push_back(newVert);
				face->v[j] = m_TotalVerts + newVertCount;
				newVertCount++;
			}
		}
	}

	MVert *finalVertices = new MVert[m_TotalVerts + newVertCount];

	for(int i=0; i < m_TotalVerts; i++) {
		finalVertices[i] = m_Vertices[i];
	}

	for(unsigned int i=0; i < newVertCount; i++) {
		finalVertices[m_TotalVerts+i] = newVertices[i];
	}

	newVertices.clear();
	delete[] m_Vertices;
	m_Vertices = finalVertices;
	m_TotalVerts += newVertCount;
}