#pragma once
#include <vector>
#include <string>
#include <GLEW\glew.h>
#include "component.h"
#include "../gfxmath/vec2.h"
#include "../gfxmath/vec3.h"
#include "../gfxmath/vec4.h"
#include "../gfxmath/color.h"
class material;


//used to index appropriate entry in vbo_ids array
//These ARE NOT IDs of vbo, just indices in vb_ids array
enum attributeType{
	POSITION = 0,
	COLORS,
	NORMALS,
	TANGENTS,
	TEXCOORDS,
	INDICES,
	ADD_COLORS, 
	MAX_SIZE
};


class mesh : public component{
	friend class gameObject;
	friend class material;

protected:
	
	//each attribute is made of several entries. for example, vec3 is made of floats.
	enum attributeSubType {
		FLOAT = GL_FLOAT,
		HALF_FLOAT = GL_HALF_FLOAT,
		INT = GL_INT,
		UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
		BOOL = GL_BOOL
	};

	//buffer args stores all the information required to be buffered
	//into OpenGL side. 
	struct bufferArgs {
		//constructor
		bufferArgs( void *attributeArray = nullptr, size_t numEntries = 0,
					size_t byteSizePerEntry = sizeof(vec2),
					attributeType attribute_type = attributeType::TEXCOORDS,
					attributeSubType attribSubType = attributeSubType::FLOAT,
					bool resizeExistingVbos = false ){

			this->attributeArray = attributeArray;
			this->numEntries = numEntries;
			this->byteSizePerEntry = byteSizePerEntry;
			this->attribute_type = attribute_type;
			this->attribSubType = attribSubType;
			this->resizeExistingVbos = resizeExistingVbos;
		}

		//pointer to your array with actual data. Default = nullptr.
		//This is NOT VAO. It's an array of data, like array of vec4 etc.
		void *attributeArray;

		//TEXCOORDS, VERTICES, NORMALS, etc. Default = attributes::TEXCOORDS 
		attributeType attribute_type; 

		//each attribute is made of several entries.
		//For example, vec3 is made of floats. Default = FLOAT
		attributeSubType attribSubType;

		//how many entries in array.  Default = 0
		size_t numEntries;

		//how big is each entry.  Default = sizeof(vec2)
		size_t byteSizePerEntry;  

		//use old GPU memory or delete and alocate new?  Default = false
		bool resizeExistingVbos;
	};


	void BufferData(bufferArgs &args);


public:
	//creates a mesh, generating VAO and assuming the  mesh->mesh_traversal_type  
	//as provided in the argument.
	mesh(GLuint mesh_traversal_type =GL_TRIANGLES);

	//given a path towards a file (for example, "../myMeshes/myMesh.obj")
	//generates a mesh based on the descriptive contents in the file.
	//Returns nullptr if file was corrupted or had incorrect syntaxis
	static mesh *readObjMeshFile(std::string url_to_obj);
	~mesh();

	
	//returns a pointer to a triangle mesh. You must delete it when done
	static mesh *createTriangle();

	//returns a pointer to a quad mesh. You must delete it when done
	static mesh *createQuad();

	//create a quarter of a quad. 
	//quadrant0 = top right, 
	//quadrant1 = top left, 
	//quadrant2 = bot left
	//quadrant3 = bot right;  (goes counter-clockwise)
	static mesh *createQuarterQuad(int quadrant);


	//TODO chekc if setters change the sizes of their arrays. for instance m_verticesNum.
	//watch out, since bufferData will need to know if the new data is larger


	//using things like GL_POINTS, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_LINES, etc.
	//Allows to modify the way this mesh's vertices get processed. 
	// (Separatelly, in groups of 3, consecutively, in pairs,  etc)
	void set_MeshTraversal_type(GLuint type) {
		this->mesh_traversal_type = type;
	}

	//provide texture coordinates to this mesh.
	//It automatically BuffersData to GPU
	inline void setTexCoords(const std::vector<vec2> &coords, 
														bool highPrecision = true) {

		size_t old_size = m_texCoords.size();
		m_texCoords = std::vector<vec2>(coords); //copy by VALUE!!

		attributeSubType subtype = highPrecision ? attributeSubType::FLOAT :
													attributeSubType::HALF_FLOAT;

		bufferArgs args(&m_texCoords[0], m_texCoords.size(), sizeof(vec2),
						attributeType::TEXCOORDS, subtype,
						old_size >= m_texCoords.size() ? false : true);

		BufferData(args);
	}
	 


	//provide positions to this mesh.
	//It automatically BuffersData to GPU
	inline void setPositions(const std::vector<vec3> &pos, bool highPrecision=true){
		//make sure this mesh's positions array is filled 
		//with client data
		size_t old_size = m_positions.size();
		m_positions = std::vector<vec3>(pos); //copy by VALUE!!

		attributeSubType subtype = highPrecision ? attributeSubType::FLOAT :
												attributeSubType::HALF_FLOAT;

		bufferArgs args(&m_positions[0], m_positions.size(), sizeof(vec3),
						attributeType::POSITION, subtype,
						old_size >= m_positions.size() ? false : true);

		BufferData(args);

		// following variable will trigger update of the gameObject's bounding box,
		// during mesh::componentUpdate()
		mesh_positions_changed = true;
	}



	//provide texture coordinates to this mesh.
	//It automatically BuffersData to GPU
	inline void setIndices(const std::vector<unsigned int> &indices) {

		size_t old_size = m_indices.size();
		m_indices = std::vector<unsigned int>(indices);//copy by VALUE!!

		bufferArgs args(&m_indices[0], m_indices.size(), sizeof(size_t),
						attributeType::INDICES, attributeSubType::INT,
						old_size >= m_indices.size() ? false : true);

		BufferData(args);
	}


	//provide normal vectors to this mesh.
	//It automatically BuffersData to GPU
	inline void setNormals(	const std::vector<vec3> &normals, 
							bool highPrecision=true	){

		size_t old_size = m_normals.size();
		m_normals = std::vector<vec3>(normals);

		attributeSubType subtype = highPrecision ? attributeSubType::FLOAT :
													attributeSubType::HALF_FLOAT;

		bufferArgs args(&m_normals[0], m_normals.size(), sizeof(vec3),
						attributeType::NORMALS, subtype,
						old_size >= m_normals.size() ? false : true);

		BufferData(args);
	}


	//provide tangent vectors to this mesh.
	//w component signifies the handedness of the coordinate system.
	//Can be 1 or -1
	//It automatically BuffersData to GPU
	inline void setTangents( const std::vector<vec4> &tangents, 
														 bool highPrecision=true ){

		size_t old_size = m_tangents.size();
		m_tangents = std::vector<vec4>(tangents);

		attributeSubType subtype = highPrecision ? attributeSubType::FLOAT :
													attributeSubType::HALF_FLOAT;

		bufferArgs args(&m_tangents[0], m_tangents.size(), sizeof(vec4),
						attributeType::TANGENTS, subtype,
						old_size >= m_tangents.size() ? false : true);

		BufferData(args);
	}



	//provide color vectors to this mesh.
	//It automatically BuffersData to GPU
	inline void setColors(const std::vector<color> &colors) {
		size_t old_size = m_colors.size();
		m_colors = std::vector<color>(colors);

		attributeSubType subtype = attributeSubType::UNSIGNED_BYTE;
			
		bufferArgs args(&m_colors[0], m_colors.size(), sizeof(color),
						attributeType::COLORS, subtype,
						old_size >= m_colors.size() ? false : true);

		BufferData(args);
	}


	//provide additional color vectors to this mesh.
	//It automatically BuffersData to GPU
	inline void setAdditionalColors(const std::vector<vec4> &additional_colors, 
											bool highPrecision = true){

		size_t old_size = m_additional_colors.size();
		m_additional_colors = std::vector<vec4>(additional_colors);

		attributeSubType subtype = highPrecision ? attributeSubType::FLOAT :
												   attributeSubType::HALF_FLOAT;

		bufferArgs args(&m_additional_colors[0], m_additional_colors.size(),
						sizeof(vec4),
						attributeType::COLORS, subtype,
						old_size >= m_additional_colors.size() ? false : true);

		BufferData(args);
	}


	//returns things like GL_POINTS, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_LINES, etc.
	//Allows to find out the way this mesh's vertices get processed. 
	// (Separatelly, in groups of 3, consecutively, in pairs,  etc)
	inline GLuint get_MeshTraversal_type() {
		return this->mesh_traversal_type;
	}




	//provides a reference to texture coordinates of this mesh as const vec2 array. 
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<vec2> &getTexCoords(){
		return m_texCoords;
	}
	//provides a reference to vertex coordinates of this mesh as const vec3 array. 
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<vec3> &getPositions() {
		return m_positions;
	}

	//provides a reference to normals of this mesh as const vec3 array. 
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<vec3> &getNormals() {
		return m_normals;
	}
	//provides a reference to tangents of this mesh as const vec4 array. 
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<vec4> &getTangents() {
		return m_tangents;
	}
	//provides a reference to colors of this mesh as const color array. 
	//recall, color is a structure of 4 unsigned bytes.
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<color> &getColors() {
		return m_colors;
	}
	//provides a reference to ADDITIONAL colors of this mesh as  const vec4 array 
	//Length of outputted array can be found by calling getAttributeSizes()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<vec4> &getAdditionalColors() {
		return m_additional_colors;
	}
	//provides a reference to the indices of this mesh as  const size_t array.
	//Length of outputted array can be found by calling getIndicesSize()
	//If the user wishes, he could make a copy of the array himself.
	inline const std::vector<unsigned int> &getIndices() {
		return m_indices;
	}


	//uses the attributes of mesh's VAO to draw the mesh.
	//Uses the PREVIOUSLY bound shader. 
	//For example, can be envoked by material or light.
	//material::draw();
	void draw_mesh();



protected:
	void componentUpdate(float dt) override;
	void onGameObject_AddComponent() override;



private:
	std::vector<vec4> m_tangents; //vector4!
	std::vector<color> m_colors; //4-byte long structures
	//if someone needs additional vector 4 in shaders
	std::vector<vec4> m_additional_colors; 

	std::vector<vec2> m_texCoords;
	std::vector<vec3> m_normals;
	std::vector<vec3> m_positions;
	bool mesh_positions_changed;

	GLuint mesh_traversal_type; //can be GL_TRAINGLES, GL_TRIANGLE_STRIP, etc

	//has different size than other attributes, m_indices_size
	std::vector<unsigned int> m_indices;

	//vertex attribute object:
	GLuint m_vao;
	//array with ID for each appropriate vbo. Use attributeType enum to get 
	//needed vbo.
	GLuint *m_vbo_ids;

};

