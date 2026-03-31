#include "repositories.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include <iostream>

#define DEF_ASSETS_DIR L"./default_assets/*"


//instantiate static variables:
repositories *repositories::repository = nullptr;




repositories::repositories(){
	//upon the creation of any repository, its maps will be filled with the
	//contents from corresponding folders.
	//we generate and load shaders, textures and meshes.
	fill_repositories_from_Windows_folders(DEF_ASSETS_DIR);
}


repositories::~repositories(){
	//TODO introduce binary shaders. Make sure they are deleted if nessesary
	for (auto kvp : textures) {
		delete kvp.second;
	}
	for (auto kvp : meshes) {
		delete kvp.second;
	}
}



void repositories::fill_repositories_from_Windows_folders(const std::wstring &folder){
	//get all the names of the contents of the "folder".
	//They could be a subfolder, or some file, like .jpg .obj .glsl  etc.
	wstr_vector content_names = get_all_file_names_in_windows_dir(folder);

	for (const std::wstring &content : content_names) {
		//will be zero if '.'  IS found in name:
		if (content.find_last_of(L'.') != content.npos){//means its a file, not a sub-directory
			//attempt to make sense of the file, if its extension'
			//is supported:  (and remove the * from the end of the string)
			processFile(folder.substr(0, folder.find(L'*')), content);
		}
		else { //otherwise, its a sub-directory.
			//construct a new, deeper path. Remove preious *, and add it on the end
			//of a new string. Make sure it's accompanied with /
			fill_repositories_from_Windows_folders(folder.substr(0, folder.find('*')) + content + L"/*");
		}
	}//end foreach content
}




wstr_vector  repositories::get_all_file_names_in_windows_dir(const std::wstring &folder){

	//structure containing all info about the obtained file.
	//For instance, creation time, size of the file, last access time etc.
	WIN32_FIND_DATA search_data; 
	memset(&search_data, 0, sizeof(WIN32_FIND_DATA)); //initialize all members to 0.

	//HADLE is a void pointer
	HANDLE handle = FindFirstFile(folder.c_str(), &search_data);

	std::vector<std::wstring> output;
	
	while (handle != INVALID_HANDLE_VALUE)	{
		if ( std::wstring(search_data.cFileName) != L"." 
				&& std::wstring(search_data.cFileName) != L"..") {
					//will output any name of the internal folder OR a file,
					//with its extension
					output.push_back(search_data.cFileName);
		}

		//check the next file in the handle
		if (FindNextFile(handle, &search_data) == FALSE)
			break; //end while if there are no more files or folders
	}

	//Close the handle after use or memory/resource leak will occur
	FindClose(handle);
	return output;
}



bool repositories::processFile( const std::wstring &file_directory, 
								const std::wstring &file_name ){
	
	if (tryMake_shader(file_directory, file_name))
		return true;
	//TODO try to make texture and mesh
	if (tryMake_texture(file_directory, file_name))
		return true;

	if (tryMake_mesh(file_directory, file_name))
		return true;

	return false;
}



void repositories::plugSourceInShader( std::wstring file_directory, 
									   std::wstring file_name, 
									   GLuint shader_type, 
									   size_t type_pos) {

	std::wstring clean_name = file_name.substr(0, type_pos);
	//create or access an existing shader:

	//to query the map of shaders, we need a normal string to be 
	//created from our wide-string:
	shader **s = &shaders[std::string(	clean_name.begin(),	clean_name.end()  )];
	//s is pointer to a pointer to shader.
	//Now we are pointing to the location where the shader is stored in the map.

	//check if there was a shader already, or the map simply zero-initialized the 
	//pointer (meaning it points to null):
	if (!*s) //if it zero-initialized its value, we have to make the shader object
		*s = new shader();//ourselves, at the map's location of shader value.

	//first get the concatenated url, then convert it into a usual std::string:
	std::wstring full_url = file_directory + file_name;
	std::string c_string_url(full_url.begin(), full_url.end());

	//use the usual string to access the shader's function, loading the source code.
	(*s)->loadSource(c_string_url, shader_type);

	//check if all of the source code is ready to compile this shader:
	if ((*s)->isReadyForCompile())
		(*s)->compileShader();
}




bool repositories::tryMake_shader( const std::wstring &file_directory,
								   const std::wstring &file_name ) {
	//get the extension of the file:
	size_t dot_pos = file_name.find('.');
	std::wstring file_extension = file_name.substr(dot_pos + 1); //don't include dot.

	//see if extension is known:
	if (file_extension == L"glsl") {
		size_t type_pos = 0;

		if ((type_pos = file_name.find(L"Vert"))  !=  file_name.npos) {
			plugSourceInShader(	file_directory, file_name, 
									GL_VERTEX_SHADER, type_pos );
			return true;
		}
		else if ((type_pos = file_name.find(L"Frag")) != file_name.npos) {
			plugSourceInShader(	file_directory, file_name,
									GL_FRAGMENT_SHADER, type_pos );
			return true;
		}
		else if ((type_pos = file_name.find(L"Geom")) != file_name.npos) {
			plugSourceInShader(	file_directory, file_name, 
									GL_GEOMETRY_SHADER, type_pos );
			return true;
		}
		else if ((type_pos = file_name.find(L"Compute")) != file_name.npos) {
			plugSourceInShader( file_directory, file_name,
								GL_COMPUTE_SHADER, type_pos );
			return true;
		}
	}//end if extension was L"glsl"

	//TODO add .hlsl, .cg etc

	//if we didn't manage to return by now - this means we couldn't process the file.
	return false;
}



bool repositories::tryMake_texture( const std::wstring &file_directory,
								 const std::wstring &file_name) {
	//get the extension of the file:
	size_t dot_pos = file_name.find('.');
	std::wstring file_extension = file_name.substr(dot_pos + 1); //don't include dot.
	std::wstring clean_name = file_name.substr(0, dot_pos);

	//see if extension is known:
	if (file_extension == L"jpg" 
		|| file_extension == L"png" 
		|| file_extension == L"tga" ) {

		//to query the map of textures, we need a normal string to be 
		//created from our wide-string:
		texture **t = &textures[std::string(clean_name.begin(), clean_name.end())];
		//s is pointer to a pointer to texture.
		//Now we are pointing to the location where the texture is stored in the map.

		//first get the concatenated url, then convert it into a usual std::string:
		std::wstring full_url = file_directory + file_name;
		std::string c_string_url(full_url.begin(), full_url.end());
		
		bool isFourChannel = false;
		if (full_url.find(L"four_channel") != std::wstring::npos) {
			isFourChannel = true;
		}
		//check if there was a texture already, or the map simply zero-initialized 
		//the pointer (meaning it points to null):
		if (!*t) { //if it zero-initialized its value, we have to make the texture
				   //ourselves, at the map's location of mesh value.
			*t = new texture(c_string_url, false, isFourChannel);
		}
		return true;
	}
	//TODO introduce 16 and 32 bit texture support

	//if we didn't manage to return by now - this means we couldn't process the file.
	return false;
}




bool repositories::tryMake_mesh( const std::wstring &file_directory,
								 const std::wstring &file_name) {
	//get the extension of the file:
	size_t dot_pos = file_name.find('.');
	std::wstring file_extension = file_name.substr(dot_pos + 1); //don't include dot.
	std::wstring clean_name = file_name.substr(0, dot_pos);

	//see if extension is known:
	if (file_extension == L"obj") {
		//to query the map of meshes, we need a normal string to be 
		//created from our wide-string:
		mesh **m = &meshes[  std::string(clean_name.begin(),  clean_name.end()) ];
		//s is pointer to a pointer to mesh.
		//Now we are pointing to the location where the mesh is stored in the map.

		//first get the concatenated url, then convert it into a usual std::string:
		std::wstring full_url = file_directory + file_name;
		std::string c_string_url(full_url.begin(), full_url.end());

		//check if there was a mesh already, or the map simply zero-initialized the 
		//pointer (meaning it points to null):
		if (!*m) { //if it zero-initialized its value, we have to make the mesh
				 //ourselves, at the map's location of mesh value.
			*m = mesh::readObjMeshFile(c_string_url);
		}
		return true;
	}
	//TODO introduce  .fbx

	//if we didn't manage to return by now - this means we couldn't process the file.
	return false;
}

