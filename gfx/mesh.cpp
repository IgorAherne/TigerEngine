#include <fstream>
#include <sstream>

#include "mesh.h"
#include "material.h"
#include "gameObject.h"
#include "../gfxmath/vec3.h"


mesh *mesh::createTriangle() {
	mesh *triMesh = new mesh();
	
	std::vector<vec3> positions;
	positions.push_back(vec3(-0.5f, -0.5f, 0));//bot left
	positions.push_back(vec3(0.5f, -0.5f, 0));//bot right
	positions.push_back(vec3(0.f, 0.5f, 0)); //top center (counter-clockwise)
	triMesh->setPositions(positions, 3);

	std::vector<vec2> texCoords;
	texCoords.push_back(vec2(0,   0));
	texCoords.push_back(vec2(1,   0));
	texCoords.push_back(vec2(0.5f, 1));
	triMesh->setTexCoords(texCoords);

	//TODO tangents/normals

	return triMesh;
}



mesh *mesh::createQuad() {
	mesh *quadMesh = new mesh(GL_TRIANGLE_FAN);

	std::vector<vec3> positions;
	positions.push_back(vec3(-1, -1, 0)); //bot left
	positions.push_back(vec3(1, -1,  0)); //bot right
	positions.push_back(vec3(1,  1,  0));  //top right
	positions.push_back(vec3(-1, 1,  0)); //top left (counter clockwise)
	quadMesh->setPositions(positions);

	std::vector<vec2> texCoords;
	texCoords.push_back(vec2(0, 0));
	texCoords.push_back(vec2(1, 0));
	texCoords.push_back(vec2(1, 1));
	texCoords.push_back(vec2(0, 1));
	quadMesh->setTexCoords(texCoords);

	std::vector<vec3> normals;
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	quadMesh->setNormals(normals);

	//TODO tangents

	return quadMesh;
}


mesh *mesh::createQuarterQuad(int quadrant) {
	if (quadrant > 3)
		quadrant = 3;
	if (quadrant < 0)
		quadrant = 0;

	mesh *quarterQuad = new mesh(GL_TRIANGLE_FAN);

	std::vector<vec3> positions;
	positions.push_back(vec3(-1, -1, 0)); //bot left
	positions.push_back(vec3(0, -1, 0)); //bot right
	positions.push_back(vec3(0, 0, 0));  //top right
	positions.push_back(vec3(-1, 0, 0)); //top left (counter clockwise)

	std::vector<vec2> texCoords;
	texCoords.push_back(vec2(0, 0));
	texCoords.push_back(vec2(0.5, 0));
	texCoords.push_back(vec2(0.5, 0.5));
	texCoords.push_back(vec2(0, 0.5));
	
	vec3 offset;
	switch (quadrant) {
		case 0:{
			offset = vec3(1, 1, 0);
		}break;
		case 1:{
			offset = vec3(0, 1, 0);
		}break;
		case 2:{
			offset = vec3(0, 0, 0);
		}break;
		case 3:{
			offset = vec3(1, 0, 0);
		}break;
	}

	for (vec3 &pos : positions) {
		pos += offset;
	}
	for (vec2 &tex_coord : texCoords) {
		tex_coord += ( offset._vec2()*0.5f );
	}

	quarterQuad->setTexCoords(texCoords);
	quarterQuad->setPositions(positions);

	std::vector<vec3> normals;
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	normals.push_back(vec3(0, 0, 1));
	quarterQuad->setNormals(normals);

	return quarterQuad;
}



mesh::mesh(GLuint mesh_traversal_type/*=GL_TRIANGLES*/){
	glGenVertexArrays(1, &m_vao);

	this->mesh_traversal_type = mesh_traversal_type;

	m_vbo_ids = new GLuint[attributeType::MAX_SIZE]{}; //all entries have id of 0
}



//checks if provided arg is NOT nullptr, then deletes it as an ARRAY.
//only present in this .cpp  there is no declaration in the header.
void destroy(void *array_to_delete) {
	if (array_to_delete) {
		delete[] array_to_delete;
		array_to_delete = nullptr;
	}
}


mesh::~mesh() {
	//Texture coords, positions, colors are all destroyed correctly (they are vectors
	//of value types)

	//dispose of vao
	glDeleteVertexArrays(1, &m_vao);

	if (m_vbo_ids) { //dispose of vbos
		glDeleteBuffers(attributeType::MAX_SIZE, m_vbo_ids); //TODO some vbos in middle of array might be absent
	}
}


void mesh::draw_mesh() {
	glBindVertexArray(this->m_vao);

	//TODO: check if indices are present, then draw indices instead.
	//TODO: we are relying on the positions to be always present in a mesh, 
	//and using their count:    Perhaps don't rely on this
	glDrawArrays(this->mesh_traversal_type, 0, this->m_positions.size());

	glBindVertexArray(0); //unbind the VAO
}




void mesh::onGameObject_AddComponent() {
	//help the gameObject establish its quicklink to its mesh:
	m_gameObject->m_mesh = this; 
	

	material *_material = m_gameObject->getMaterial();
	//introduce this attached mesh to the gameObject's material (if it's there):
	if (_material) {
	   _material->m_mesh = this;
	}


	//force our gameObject to recompute the dimensions of the bounding shape.
	m_gameObject->reBoundingShape_fromTransform();

	//We've just updated the bounding shape.
	//Make sure we will avoid unnessesary recomputing of boundingShape_fromTransform
	//in the future.
	//Toggle it to false. Only mesh's vert coords (positions) are changed, it 
	//will toggle our mesh_positions_changed back to true:
	mesh_positions_changed = false;
}




void mesh::BufferData(bufferArgs &args){
	//vao was created in the constructor.
	glBindVertexArray(m_vao);

	if (args.resizeExistingVbos) {
		//generate all vertex buffer objects (since they all must have same size)
		//TODO iterate over each VBO and EXTEND the size. DON'T delete every 
		//VBO, re-creating it, since it will wipe all the data.
		if (m_vbo_ids[args.attribute_type]) { 
			glDeleteBuffers(1, &m_vbo_ids[args.attribute_type]);
		}
		glGenBuffers(1, &m_vbo_ids[args.attribute_type]);
	}//end if resize existing vbos == true

	else { //even if no resize was requested, 
		//check if the particular buffer needs to be generated:
		if (m_vbo_ids[args.attribute_type] == 0) {
			glGenBuffers(1, &m_vbo_ids[args.attribute_type]);
		}
	}


	glBindBuffer( GL_ARRAY_BUFFER, m_vbo_ids[args.attribute_type] );
	glBufferData( GL_ARRAY_BUFFER, args.numEntries*args.byteSizePerEntry, 
				  args.attributeArray, GL_STATIC_DRAW );

	size_t numItemsPerAttrib = 0;
	
	switch (args.attribSubType) {
		case attributeSubType::FLOAT: {
			numItemsPerAttrib = args.byteSizePerEntry / sizeof(float);
		}break;
		case attributeSubType::HALF_FLOAT: {
			numItemsPerAttrib = args.byteSizePerEntry / sizeof(float);
			numItemsPerAttrib *= 2;
		}break;
		case attributeSubType::INT: {
			numItemsPerAttrib = args.byteSizePerEntry / sizeof(int);
		}break;
		case attributeSubType::UNSIGNED_BYTE: {
			numItemsPerAttrib = args.byteSizePerEntry / sizeof(unsigned char);
		}break;
		case attributeSubType::BOOL: {
			numItemsPerAttrib = args.byteSizePerEntry / sizeof(bool);
		}break;
	}
	//notice, we are attaching the vbo to the cells of vao.
	//we are attaching them to the cells which match entries in attribute
	glVertexAttribPointer(args.attribute_type, numItemsPerAttrib, args.attribSubType, 
		                  args.attribSubType != attributeSubType::UNSIGNED_BYTE ?
						   GL_FALSE : GL_TRUE,  //normilized if it's a ubyte
							0, 0);
	glEnableVertexAttribArray(args.attribute_type); //enable vbo at this index.
}




void mesh::componentUpdate(float dt) {
	//TODO update the mesh component

	//update the bounding box, if the mesh's internal structure has changed:
	if (mesh_positions_changed) {
		m_gameObject->reBoundingShape_fromTransform();

		//toggle it back. If mesh's vert coords (positions) are changed, it 
		//will toggle our mesh_positions_changed back to true.
		mesh_positions_changed = false;
	}
}



//used in mesh generation from .obj file
struct objFile_mesh_data {
	std::vector<vec3> base_vert_cords;//accomulated "reference" data, which will then 
	std::vector<vec3> base_normals;//be queried by appropriate indices. Contents
	std::vector<vec2> base_tex_coords; //will be used to make the 3 vectors below:

	//"Expanded" data, which takes into the account indices.
	std::vector<vec3> final_vert_cords;//These 3 arrays are constructed by sampling
	std::vector<vec3> final_normals;//base vectors above, with the indices
	std::vector<vec2> final_tex_coords;//of obj faces
};



//forward declare functions used further in this cpp, but not in the header file:
void ProcessLine(std::string,  objFile_mesh_data &);
vec3 ExtractVertex(std::stringstream &);
vec3 ExtractNormal(std::stringstream &);
vec2 ExtractTexCoord(std::stringstream &);
vec2 ExtractTexCoord(std::stringstream &);
void ExtractFace(std::stringstream &, objFile_mesh_data&);



mesh *mesh::readObjMeshFile(std::string url_to_obj) {

	std::ifstream in_stream(url_to_obj.c_str(), std::ios::in);

	if (in_stream.fail()) {
		std::cout << "\n\nNo OBJ file was found at " << url_to_obj << "\n\n";
		return nullptr;
	}

	objFile_mesh_data objdata;

	std::string line;
	while (std::getline(in_stream, line)) {
		ProcessLine(line, objdata);
	}

	mesh *output_mesh = new mesh();
	output_mesh->setPositions(objdata.final_vert_cords); //supply the final elements,
	output_mesh->setTexCoords(objdata.final_tex_coords);//which were expanded from
	output_mesh->setNormals(objdata.final_normals);//base ones, using indices.

	//TODO  GenerateTangents();
	return output_mesh;
}




void ProcessLine(std::string line, objFile_mesh_data &objdata) {

	std::string token;
	std::stringstream line_ss(line);

	line_ss >> token;

	if (token != "") {

		if (token == "#") { //comment line
			//TODO if mesh has name:
			//line_ss >> token;
			//if (token == "object") {
			//	line_ss >> token;
			//	obj_mesh->name = token;
			//}
			return; //disregard this line and let's process the next one
		}
		//otherwise, pass whatever remains of the string stream 
		//(we removed the token already) as an argument
		else if (token == "v") { //vertex line
			objdata.base_vert_cords.push_back( ExtractVertex(line_ss) );
		}
		else if (token == "vt") { //texture coordinate
			objdata.base_tex_coords.push_back( ExtractTexCoord(line_ss) );
		}
		else if (token == "vn") { //face indexes
			objdata.base_normals.push_back( ExtractNormal(line_ss) );
		}
		else if (token == "f") { //face indexes
			ExtractFace(line_ss, objdata);
		}
		//TODO materials
		//else if (token == "mtllib" && !(*out_mat)) {//if material token & material
		//	line_ss >> token; // wasn't yet initialized this is the file to open 
		//	*out_mat = buildMaterial(token);//(it contains materials for this object)
		//}
	}//end if the index wasn't end of string
}



//works on the current line if it's the one for a vertex.
//constructs a Vector3 for position from it. 
vec3 ExtractVertex(std::stringstream &vert_ss) {
	vec3 vert;

	bool x_done = false;
	bool y_done = false;
	bool z_done = false;

	std::string process_string;

	while (vert_ss >> process_string) { //TODO get rid of process_string
		if (!x_done) { //why do u reconstruct a stringstream below?
			if (std::stringstream(process_string) >> vert.x) {
				x_done = true;
			}
		}
		else if (!y_done) {
			if (std::stringstream(process_string) >> vert.y) {
				y_done = true;
			}
		}
		else if (!z_done) {
			if (std::stringstream(process_string) >> vert.z) {
				z_done = true;
			}
		}
	}//end while
	return vert;
}



//works on the current line if it's the one for a vertex.
//constructs a Vector3 for a normal from it. 
vec3 ExtractNormal(std::stringstream &norm_ss) {
	vec3 norm; 

	bool x_done = false;
	bool y_done = false;
	bool z_done = false;

	std::string process_string;

	while (norm_ss >> process_string) {
		if (!x_done) {
			if (std::stringstream(process_string) >> norm.x) {
				x_done = true;
			}
		}
		else if (!y_done) {
			if (std::stringstream(process_string) >> norm.y) {
				y_done = true;
			}
		}
		else if (!z_done) {
			if (std::stringstream(process_string) >> norm.z) {
				z_done = true;
			}
		}
	}//end while
	return norm;
}



//extracts a texture coordinate from the supplied string
vec2 ExtractTexCoord(std::stringstream &tex_ss) {
	vec2 tex_coord;

	bool u_done = false;
	bool v_done = false;

	std::string process_string;

	while (tex_ss >> process_string) {
		if (!u_done) {
			if (std::stringstream(process_string) >> tex_coord.x) {
				u_done = true;
			}
		}
		else if (!v_done) {
			if (std::stringstream(process_string) >> tex_coord.y) {
				v_done = true;
			}
		}
	}
	return tex_coord;
}



void ExtractFace(std::stringstream &face_ss, objFile_mesh_data &objdata) {
	//Function, which "expands" vertex_coordinates, texture coordinates and normals
	//Into a longer number of elements. We use indices and the base elements, to 
	//create an "expanded" version of elements.

	//we didn't save indices because they are different for the all 
	//3 types of elements
	//VBOs can't  be indexed by 3 different orders.

	struct element_indices {
		element_indices() : vert_ix(0), tex_ix(0), norm_ix(0) {};

		unsigned int vert_ix;
		unsigned int tex_ix;
		unsigned int norm_ix;
	};

	element_indices idxs_1;
	element_indices idxs_2;
	element_indices idxs_3;

	bool one_done = false; //have we extracted info for the first vertex yet?
	bool two_done = false;
	bool three_done = false;

	std::string process_string;

	while (face_ss >> process_string) { //iterate through tokens. Each one looks like 112/2223/141 

		std::string token_val;

		if (!one_done) {
			std::stringstream token_string(process_string);

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_1.vert_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_1.tex_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_1.norm_ix;

			one_done = true;
		}
		else if (!two_done) {
			std::stringstream token_string(process_string);

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_2.vert_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_2.tex_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_2.norm_ix;

			two_done = true;
		}
		else if (!three_done) {
			std::stringstream token_string(process_string);

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_3.vert_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_3.tex_ix;

			std::getline(token_string, token_val, '/');
			std::stringstream(token_val) >> idxs_3.norm_ix;

			three_done = true;
		}
	}

	//store all the information in the final vectors of the argument struct. 
	objdata.final_vert_cords.push_back( objdata.base_vert_cords[idxs_1.vert_ix -1] );
	objdata.final_vert_cords.push_back( objdata.base_vert_cords[idxs_2.vert_ix -1] );
	objdata.final_vert_cords.push_back( objdata.base_vert_cords[idxs_3.vert_ix -1] );
	//-1 since obj counds from 1, not from 0
	objdata.final_tex_coords.push_back( objdata.base_tex_coords[idxs_1.tex_ix -1] );
	objdata.final_tex_coords.push_back( objdata.base_tex_coords[idxs_2.tex_ix -1] );
	objdata.final_tex_coords.push_back( objdata.base_tex_coords[idxs_3.tex_ix -1] );

	//3--> 2--> 1 because we need counter clock-wise winding (vertex traversal).
	objdata.final_normals.push_back( objdata.base_normals[idxs_1.norm_ix -1] );
	objdata.final_normals.push_back( objdata.base_normals[idxs_2.norm_ix -1] );
	objdata.final_normals.push_back( objdata.base_normals[idxs_3.norm_ix -1] );
}
