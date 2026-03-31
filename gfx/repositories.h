#pragma once
#include <map>
#include <string>
#include <vector>
#include <Windows.h> //TODO compatibility. Surround with defines



class shader;
class texture;
class mesh;



typedef std::vector<std::wstring> wstr_vector;
typedef unsigned int GLuint;


//singleton-like class, which stores contents used by the engine
class repositories {
public:
	
	//If the file has url  "./default_assets/myShaderVert.glsl",  supply to the 
	//function a string "myShader"   
	//"Vert" is considered a type, .glsl is extension
	inline static shader *getShader(std::string name_no_extension_no_type) {
		if (!repository) {
			repository = new repositories(); //singleton-like pattern
		}
		auto& shaders_lnk = repository->shaders;

		auto iterator = shaders_lnk.find(name_no_extension_no_type);
		if (iterator != shaders_lnk.end()) {
			return iterator->second;
		}
		return nullptr;
	}

	//If the file has url  "./default_assets/myTexture.jpg",  supply to the 
	//function a string "myTexture" 
	//Anything after the dot is considered an extension
	inline static texture *getTexture(std::string name_no_extension) {
		if (!repository) {
			repository = new repositories();
		}
		auto& textures_lnk = repository->textures;

		auto iterator = textures_lnk.find(name_no_extension);
		if (iterator != textures_lnk.end()) {
			return iterator->second;
		}
		return nullptr;
	}

	//If the file has url  "./default_assets/myMesh.obj",  supply to the 
	//function a string "myMesh" 
	//Anything after the dot is considered an extension
	inline static mesh *getMesh(std::string name_no_extension) {
		if (!repository) {
			repository = new repositories();
		}
		auto& meshes_lnk = repository->meshes;

		auto iterator = meshes_lnk.find(name_no_extension);
		if (iterator != meshes_lnk.end()){ 
			return iterator->second;
		}
		return nullptr;
	}



private:
	repositories(); //don't ever make a public constructor.
	~repositories();//this class is a singleton

	//scans all the designated folders, picks up objects and populates the 
	//corresponding repository maps with those additional, new contents.
	//Any internal directories will also be scanned.
	void fill_repositories_from_Windows_folders(const std::wstring &folder);

	//returns a vector with names of files contained in a folder.
	//for example,   myFile.txt, myMesh.obj, myTexture.jpg
	//works for windows
	//To pass any directory as argument, use  L as a prefix, for a long string. 
	//Example:  L"./default_assets/"
	//TODO add another signature, surrounded with #defines, which accepts different
	//argument, and works with other systems.
	wstr_vector get_all_file_names_in_windows_dir(const std::wstring &folder);

	//attempts to gather data from a file, if its extension is supported.
	//if supported, will process it as either shader, texture, mesh, etc.
	//Returns false otherwsie.
	//Should only be called upon a file, not a subdirectory. 
	bool processFile(const std::wstring &file_directory, 
													const std::wstring &file_name);
	

	//attempts to make a shader from a given file. 
	//Will NOT create shader if:
	//-name doesn't contain Vert / Frag / Geom in the name (note the capitalization!)
	//-name doesn't end with .glsl  or an appropriate shader extension.
	bool tryMake_shader(const std::wstring &file_directory,
						const std::wstring &file_name);

	//TODO wstring is windows string. Add more compatibility
	//attempts to make a texture from a given file. 
	bool tryMake_texture(const std::wstring &file_directory,
						const std::wstring &file_name );


	bool tryMake_mesh( const std::wstring &file_directory, 
					   const std::wstring &file_name);
	

	//aids code-reusability. Gets called by tryMake_shader().
	//file directory should be simmilar to "./default_assets/default_shaders/"
	//file_name should be simmilar to "myShaderVert.glsl", where "Vert" is later
	//reffered to as 'type'.
	//shader_type can be simmilar to GL_VERTEX_SHADER
	//type_pos is the index in the file_name string, at which type is found. Type in
	//this case is meant to be either "Vert", "Frag" or "Geom". 
	//In the case of "myShaderVert.glsl"  it would have a value of 8.
	void plugSourceInShader( std::wstring file_directory,
							 std::wstring file_name,
							 GLuint shader_type,
							 size_t type_pos );



	static repositories *repository;

	//TODO introduce binary shaders. Make sure they are deleted if nessesary

	//query shaders with the name, which is free from "Vert", "Frag", "Geom", and 
	//any extension like .glsl etc. A plane name like "myShader"  will suffice.
	//as opposed to "myShaderFrag.glsl".
	std::map<std::string, shader*> shaders;
	std::map<std::string, texture*> textures; 
	std::map<std::string, mesh*> meshes;

	//later on - a map of scripts
};

