#include <iostream>
#include <cstring>
#include "utilities/OBJLoader.hpp"
#include "utilities/lodepng.h"
#include "rasteriser.hpp"

#include <mpi.h>

void build_MPI_Float4(float4 *float4ToTransfer, MPI_Datatype *float4_MPI){
	MPI_Datatype typeFloat4[4] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
	int blockLengthFloat4[4] = { 1, 1, 1, 1 };
	MPI_Aint displacementFloat4[4];

	MPI_Get_address(&float4ToTransfer->x, &displacementFloat4[0]);
	MPI_Get_address(&float4ToTransfer->y, &displacementFloat4[1]);
	MPI_Get_address(&float4ToTransfer->z, &displacementFloat4[2]);
	MPI_Get_address(&float4ToTransfer->w, &displacementFloat4[3]);

	displacementFloat4[0] = 0;
  displacementFloat4[1] = sizeof(float);
  displacementFloat4[2] =2*sizeof(float);
	displacementFloat4[3] = 3*sizeof(float);;

	MPI_Type_create_struct(4, blockLengthFloat4, displacementFloat4, typeFloat4, float4_MPI);
	MPI_Type_commit(float4_MPI);
}
void build_MPI_Float3(float3 *float3ToTransfer, MPI_Datatype *float3_MPI){
	MPI_Datatype typeFloat3[3] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT };
	int blockLengthFloat3[3] = { 1, 1, 1 };
	MPI_Aint displacementFloat3[3];

	MPI_Get_address(&float3ToTransfer->x, &displacementFloat3[0]);
	MPI_Get_address(&float3ToTransfer->y, &displacementFloat3[1]);
	MPI_Get_address(&float3ToTransfer->z, &displacementFloat3[2]);

	displacementFloat3[0] = 0;
  displacementFloat3[1] = 1*sizeof(float);
  displacementFloat3[2] = 2*sizeof(float);

	MPI_Type_create_struct(3, blockLengthFloat3, displacementFloat3, typeFloat3, float3_MPI);
	MPI_Type_commit(float3_MPI);
}

int main(int argc, char **argv) {
	MPI_Init(NULL, NULL);
	std::string input("../input/sphere.obj");
	std::string output("../output/sphere.png");
	unsigned int width = 1920;
	unsigned int height = 1080;
	unsigned int depth = 3;

	for (int i = 1; i < argc; i++) {
		if (i < argc -1) {
			if (std::strcmp("-i", argv[i]) == 0) {
				input = argv[i+1];
			} else if (std::strcmp("-o", argv[i]) == 0) {
				output = argv[i+1];
			} else if (std::strcmp("-w", argv[i]) == 0) {
				width = (unsigned int) std::stoul(argv[i+1]);
			} else if (std::strcmp("-h", argv[i]) == 0) {
				height = (unsigned int) std::stoul(argv[i+1]);
			} else if (std::strcmp("-d", argv[i]) == 0) {
				depth = (int) std::stoul(argv[i+1]);
			}
		}
	}
	std::cout << "Loading '" << input << "' file... " << std::endl;

	std::vector<Mesh> meshs = loadWavefront(input, false);
	std::vector<Mesh> meshsOrigin = meshs;
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	if(world_rank!=0){
		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).vertices.size(); j++){
				meshs.at(i).vertices.at(j).x = 0;
				meshs.at(i).vertices.at(j).y = 0;
				meshs.at(i).vertices.at(j).z = 0;
				meshs.at(i).vertices.at(j).w = 0;
			}

			for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
				meshs.at(i).textures.at(j).x = 0;
				meshs.at(i).textures.at(j).y = 0;
				meshs.at(i).textures.at(j).z = 0;
			}

			for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
				meshs.at(i).normals.at(j).x = 0;
				meshs.at(i).normals.at(j).y = 0;
				meshs.at(i).normals.at(j).z = 0;
			}
		}
	}

	MPI_Datatype float4_MPI;
  MPI_Datatype float3_MPI;

	build_MPI_Float4(&meshs.at(0).vertices.at(0), &float4_MPI);
  build_MPI_Float3(&meshs.at(0).textures.at(0), &float3_MPI);

		for(unsigned i=0; i<meshs.size(); i++){

      MPI_Bcast(&(meshs.at(i).vertices.at(0)), meshs.at(i).vertices.size(), float4_MPI, 0 ,MPI_COMM_WORLD);
      MPI_Bcast(&(meshs.at(i).textures.at(0)), meshs.at(i).textures.size(), float3_MPI, 0 ,MPI_COMM_WORLD);
      MPI_Bcast(&(meshs.at(i).normals.at(0)), meshs.at(i).normals.size(), float3_MPI, 0 ,MPI_COMM_WORLD);

		}


	std::cout << "Process:" << world_rank << " " << meshsOrigin[0].vertices[0].x << " " << meshsOrigin[0].vertices[0].y << " " << meshsOrigin[0].vertices[0].z <<
	" " << meshsOrigin[0].vertices[0].w << " Comp " << meshs[0].vertices[0].x << " " << meshs[0].vertices[0].y << " " << meshs[0].vertices[0].z <<
	" " << meshs[0].vertices[0].w << std::endl;

	std::vector<unsigned char> frameBuffer = rasterise(meshs, width, height, depth);

	std::string substring = output.substr(0, output.find('.'));
	output = substring + std::to_string(world_rank) + ".png";

	std::cout << "Writing image to '" << output << "'..." << std::endl;

	unsigned error = lodepng::encode(output, frameBuffer, width, height);

	if(error)
	{
		std::cout << "An error occurred while writing the image file: " << error << ": " << lodepng_error_text(error) << std::endl;
	}
	MPI_Finalize();
	return 0;
}
