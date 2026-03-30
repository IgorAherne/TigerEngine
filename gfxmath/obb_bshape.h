#pragma once
#include "vec3.h"
#include "bshape.h"
#include "mat3.h"


//Object-aligned-bounding-box Bounding Shape
class obb_bshape : public bshape{
	
public:
	//set default 0.5 length in each direction from the centre of the box.
	//
	//scaleRot_matrix brings object-oriented shape into world coordinates.
	//should be built from the  world rotation and local scale.
	obb_bshape(vec3 center_pos_world, mat3 scaleRot_matrix);

	//sets desired WORLD position and local half dimensions.
	//
	//scaleRot_matrix brings object-oriented shape into world coordinates.
	//should be built from the world rotation and local scale.
	obb_bshape(vec3 center_pos_world, vec3 half_dimensions_local, 
															mat3 scaleRot_matrix);

	//vert_coords_local is the local-space vertex coordinates array.
	//
	//scaleRot_matrix brings object-oriented shape into world coordinates.
	//should be built from the world rotation and local scale.
	obb_bshape( vec3 center_pos_world, const std::vector<vec3> &vert_coords_local,
				mat3 scaleRot_matrix);

	~obb_bshape();
	
	//returns minimum and maximum value when this shape is projected on a unit-length
	//direction. This allows to see how far along any arbitrary axis the shape 
	//extends.
	//center_position of this box is taken into the account, and is in world space
	virtual vec2 project_on_normal_world(vec3 unit_length_dir_world) const override;

	//relativelly heavy computation,
	//uses information from the array of vertex positions 
	//(expects to see coordinates in local space), to tweek the dimensions of
	//the bounding shape. That way it will sit tighter arround the vertices.
	//This does not update modelMatrix. 
	//
	//Use this function only if mesh was distorted or re-specified.
	//Any world transformations like scale, positioning, rotating don't requre you 
	//to call re-compute.
	virtual void recompute(	const std::vector<vec3> &vert_coords_local) override;

	//scaleRot_matrix brings object-oriented shape into world coordinates.
	//should be built from the  world rotation and local scale.
	//Gets typically updated by transform of our gameObject.
	//
	//Don't forget to separately  setCenterPos_world() !
	inline void setScaleRot_matrix(mat3 scaleRotMatrix) {
		this->scaleRot_matrix = scaleRotMatrix;
	}
	inline mat3 getScaleRot_matrix() {
		return this->scaleRot_matrix;
	}


	//even though there is a  center_pos_world  coordinate, it only tracks the 
	//position of the gameObject. However, if the mesh is offsetted from the 
	//origin coordinate, then this offset is generated.
	//returns an offset  which is already in the world space.
	//True center of this object-oriented bounding box can be obtained via
	//   center_pos_world  +  getOffsetToMeshCenter_world()
	inline vec3 getOffsetToMeshCenter_world() {
		return scaleRot_matrix*average_mesh_coord_local;
	}

protected:
	//parent class has  vec3 center_pos declared.
	//howver, we will need to know the average position of the mesh as well, to 
	//wrap the box tighter arround the mesh.
	//it's in local space (in the space of the mesh)
	vec3 average_mesh_coord_local;

	vec3 half_dimensions_local; //value of the extents of the box in local space.

	//brings object-oriented shape into world coordinates.
	//Usually is updated by the transform component of the gameObject.
	mat3 scaleRot_matrix;
};

