#pragma once
#include "BlenderFileBlock.h"

///////////////////////////////
// Blender Object Structures
///////////////////////////////
struct MVert
{
	float co[3];
	short no[3];
	char flag;
	char mat_nr;
	char bweight;
	char pad[3];

	// Extended fields
	float uv[2];			// Blender stores UV in faces, but we need them at the vertex level
	bool isUVSet;			// set to true for first texture coord pair set (to prevent overwriting in later faces)
	long nextSupplVert;	// used in duplicteVertex mode to to point to next additional vertex
};

struct MLoop {
	unsigned int v;
	unsigned int e;
};

struct MLoopUV {
	float uv[2];
	int flag;
};

struct MPoly {
	int loopstart;
	int totloop;
	short mat_nr;
	char flag, pad;
};

struct MTexPoly {
	void *tpage;
	char flag;
	char transp;
	short mode;
	short tile;
	short pad;
};

struct MFace {
	/*int v1;
	int v2;
	int v3;
	int v4; // note: D3D only uses triangles*/
	int v[4];

	char pad[1];
	short mat_nr;
	char edcode;
	char flag;

	// Extended fields
	bool isQuad;
	bool supplV1;  // true if v1 is index from duplicated vertices
	bool supplV2;  // true if v2 is index from duplicated vertices
	bool supplV3;  // true if v3 is index from duplicated vertices
};
 
struct MTFace {
	float uv[4][2];
	void *tpage;
	char flag;
	char transp;
	short mode;
	short tile;
	short unwrap;
};

struct MDeformVert {
	//void *dw;
	int totWeight;
	int flag;
};

struct MDeformWeight {
	int def_nr;
	float weight;
};

class BlenderMesh {
public:
	BlenderMesh();
	~BlenderMesh() {}

	int m_TotalVerts;
	int m_TotalEdges;
	int m_TotalLoops;
	int m_TotalPolygons;

	// Old versions
	int m_TotalFaces;

	std::string m_Name;

	MVert			*m_Vertices;
	MLoop			*m_Loops;
	MLoopUV			*m_LoopUVs;
	MPoly			*m_Polygons;
	MTexPoly		*m_TexPolygons;
	MDeformVert		*m_DeformVerts;
	MDeformWeight	*m_DeformWeights;

	// Old files
	MFace	*m_Faces;
	MTFace	*m_TexFaces;

	std::string GetMeshInfo();
	int GetTotalFaces()		{ return m_TotalFaces; }
	int GetTotalVertices()	{ return m_TotalVerts; }
	MVert *GetVertices()	{ return m_Vertices; }
	MFace *GetFaces()		{ return m_Faces; }

	bool LoadMesh(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks, bool triangulate, bool vertexUVs, bool flipYZ);
	void ReleaseMesh();
	
private:
	MVert			*ExtractVertices(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks, bool flipYZ);
	MFace			*ExtractFaces(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MLoop			*ExtractLoops(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MLoopUV			*ExtractLoopUVs(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MPoly			*ExtractPolys(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MTexPoly		*ExtractTexPolys(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MDeformVert		*ExtractDeformVerts(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);
	MDeformWeight	*ExtractDeformWeights(StructureDNA *sdna, std::vector<BlenderFileBlock> &blocks);

	// Convert blender's MPoly format
	// to the older MFace format
	// for ease of use
	void ConvertPolysToFaces();

	// Triangulate quads
	void Triangulate();

	// Extract UVs from faces and assign
	// to vertices, duplicating as necessary
	void UVsToVerts();
};