#include <iostream>
#include <cstring>
#include "utilities/OBJLoader.hpp"
#include "utilities/lodepng.h"
#include "rasteriser.hpp"

#include <mpi.h>

struct float4Transfer
{
    float x;
		float y;
		float z;
		float w;
};

struct float3Transfer
{
    float x;
		float y;
		float z;
};

void build_MPI_Float4(float4Transfer *float4ToTransfer, MPI_Datatype *float4_MPI){
	MPI_Datatype typeFloat4[4] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
	int blockLengthFloat4[4] = { 1, 1, 1, 1 };
	MPI_Aint displacementFloat4[4];

	MPI_Get_address(&float4ToTransfer[0].x, &displacementFloat4[0]);
	MPI_Get_address(&float4ToTransfer[0].y, &displacementFloat4[1]);
	MPI_Get_address(&float4ToTransfer[0].z, &displacementFloat4[2]);
	MPI_Get_address(&float4ToTransfer[0].w, &displacementFloat4[3]);

	displacementFloat4[0] = 0;
  displacementFloat4[1] = &float4ToTransfer[0].y - &float4ToTransfer[0].x;
  displacementFloat4[2] = &float4ToTransfer[0].z - &float4ToTransfer[0].x;
	displacementFloat4[3] = &float4ToTransfer[0].w - &float4ToTransfer[0].x;

	MPI_Type_create_struct(4, blockLengthFloat4, displacementFloat4, typeFloat4, float4_MPI);
	MPI_Type_commit(float4_MPI);
}
void build_MPI_Float3(float3Transfer *float3ToTransfer, MPI_Datatype *float3_MPI){
	MPI_Datatype typeFloat3[3] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT };
	int blockLengthFloat3[3] = { 1, 1, 1 };
	MPI_Aint displacementFloat3[3];

	MPI_Get_address(&float3ToTransfer[0].x, &displacementFloat3[0]);
	MPI_Get_address(&float3ToTransfer[0].y, &displacementFloat3[1]);
	MPI_Get_address(&float3ToTransfer[0].z, &displacementFloat3[2]);

	displacementFloat3[0] = 0;
  displacementFloat3[1] = &float3ToTransfer[0].y - &float3ToTransfer[0].x;
  displacementFloat3[2] = &float3ToTransfer[0].z - &float3ToTransfer[0].x;

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

	///////////////////Float 4 MPI type//////////////////////////
	// struct float4Transfer* float4ToTransfer = new float4Transfer[meshs.size()];
	// MPI_Datatype float4_MPI;

	// build_MPI_Float3(float4ToTransfer, &float4_MPI);
	///////////////////Float 3 MPI type//////////////////////////
	struct float3Transfer* float3ToTransfer = new float3Transfer[meshs.size()];
	MPI_Datatype float3_MPI;

	build_MPI_Float3(float3ToTransfer, &float3_MPI);

	//if master
	//For each meshs
	// 	create float to transfer array
	//	for each vertices
	//		fill float to transfer array with verticeelements from mesh
	//	build_MPI_FLOAT
	//	broadcast arra
	// repeat for textures and normals

	//if slave
	//For each meshs
	// 	create float to transfer array
	// 	broadcast array
	//	for each vertices
	//		set vertice element equal to array element
	//	repeat fo textures an normals
	MPI_Datatype float4_MPI;
	struct float4Transfer exampleStruct = {1.0, 1.0, 1.0, 1.0};
	build_MPI_Float4(&exampleStruct, &float4_MPI);
	if(world_rank==0){
		for(unsigned i=0; i<meshs.size(); i++){

			{
			struct float4Transfer* float4ToTransfer = new float4Transfer[meshs[i].vertices.size()];
			// MPI_Datatype float4_MPI;
			// build_MPI_Float4(float4ToTransfer, &float4_MPI);
			for(unsigned j=0; j<meshs[i].vertices.size(); j++){
				float4ToTransfer[j].x = meshs[i].vertices[j].x;
				float4ToTransfer[j].y = meshs[i].vertices[j].y;
				float4ToTransfer[j].z = meshs[i].vertices[j].z;
				float4ToTransfer[j].w = meshs[i].vertices[j].w;
			}
			if(i==0){


			std::cout << float4ToTransfer[0].x << " " << float4ToTransfer[0].y << " " << float4ToTransfer[0].z << " " << float4ToTransfer[0].w << " \n";
		}
			MPI_Bcast(float4ToTransfer, meshs[i].vertices.size(), float4_MPI, 0, MPI_COMM_WORLD);
			delete float4ToTransfer;
			}

			{
			struct float3Transfer* float3ToTransfer = new float3Transfer[meshs[i].textures.size()];
			MPI_Datatype float3_MPI;
			build_MPI_Float3(float3ToTransfer, &float3_MPI);
			for(unsigned j=0; j<meshs[i].textures.size(); j++){
				float3ToTransfer[j].x = meshs[i].textures[j].x;
				float3ToTransfer[j].y = meshs[i].textures[j].y;
				float3ToTransfer[j].z = meshs[i].textures[j].z;
			}
			MPI_Bcast(float3ToTransfer, meshs[i].textures.size(), float3_MPI, 0, MPI_COMM_WORLD);
			delete float3ToTransfer;
			}

			{
			struct float3Transfer* float3ToTransfer = new float3Transfer[meshs[i].normals.size()];
			MPI_Datatype float3_MPI;
			build_MPI_Float3(float3ToTransfer, &float3_MPI);
			for(unsigned j=0; j<meshs[i].normals.size(); j++){
				float3ToTransfer[j].x = meshs[i].normals[j].x;
				float3ToTransfer[j].y = meshs[i].normals[j].y;
				float3ToTransfer[j].z = meshs[i].normals[j].z;
			}
			MPI_Bcast(float3ToTransfer, meshs[i].normals.size(), float3_MPI, 0, MPI_COMM_WORLD);
			delete float3ToTransfer;
			}
		}
	}
	else{
		for(unsigned i=0; i<meshs.size(); i++){
			{
			struct float4Transfer* float4ToTransfer = new float4Transfer[meshs[i].vertices.size()];

			MPI_Bcast(float4ToTransfer, meshs[i].vertices.size(), float4_MPI, 0, MPI_COMM_WORLD);

			for(unsigned j=0; j<meshs[i].vertices.size(); j++){
				meshs[i].vertices[j].x = float4ToTransfer[j].x;
				meshs[i].vertices[j].y = float4ToTransfer[j].y;
				meshs[i].vertices[j].z = float4ToTransfer[j].z;
				meshs[i].vertices[j].w = float4ToTransfer[j].w;
			}
			// std::cout << float4ToTransfer[0].x << " " << float4ToTransfer[0].y << " " << float4ToTransfer[0].z << " " << float4ToTransfer[0].w << " \n";
			delete float4ToTransfer;
			}

			{
			struct float3Transfer* float3ToTransfer = new float3Transfer[meshs[i].textures.size()];
			MPI_Datatype float3_MPI;
			build_MPI_Float3(float3ToTransfer, &float3_MPI);
			MPI_Bcast(float3ToTransfer, meshs[i].textures.size(), float3_MPI, 0, MPI_COMM_WORLD);

			for(unsigned j=0; j<meshs[i].textures.size(); j++){
				meshs[i].textures[j].x = float3ToTransfer[j].x;
				meshs[i].textures[j].y = float3ToTransfer[j].y;
				meshs[i].textures[j].z = float3ToTransfer[j].z;
			}
			delete float3ToTransfer;
			}

			{
			struct float3Transfer* float3ToTransfer = new float3Transfer[meshs[i].normals.size()];
			MPI_Datatype float3_MPI;
			build_MPI_Float3(float3ToTransfer, &float3_MPI);
			MPI_Bcast(float3ToTransfer, meshs[i].normals.size(), float3_MPI, 0, MPI_COMM_WORLD);

			for(unsigned j=0; j<meshs[i].normals.size(); j++){
				meshs[i].normals[j].x = float3ToTransfer[j].x;
				meshs[i].normals[j].y = float3ToTransfer[j].y;
				meshs[i].normals[j].z = float3ToTransfer[j].z;
			}
			delete float3ToTransfer;
			}

		}
	}

	std::cout << "Process:" << world_rank << " " << meshsOrigin[0].vertices[0].x << " " << meshsOrigin[0].vertices[0].y << " " << meshsOrigin[0].vertices[0].z <<
	" " << meshsOrigin[0].vertices[0].w << " Comp " << meshs[0].vertices[0].x << " " << meshs[0].vertices[0].y << " " << meshs[0].vertices[0].z <<
	" " << meshs[0].vertices[0].w << std::endl;
	// 	// fill it with
	// int nElementsTot = 0;
	// int nVerticesTot = 0;
	// int nTexturesTot = 0;
	// int nNormalsTot = 0;
	//
	// for(unsigned i=0; i<meshs.size(); i++){
	// 	nVerticesTot = nVerticesTot + 4*meshs.at(i).vertices.size();
	// 	nTexturesTot = nTexturesTot + 3*meshs.at(i).textures.size();
	// 	nNormalsTot = nNormalsTot + 3*meshs.at(i).normals.size();
	// }
	//
	// nElementsTot = nVerticesTot + nTexturesTot + nNormalsTot;
	// float* VTNArray = new float[nElementsTot];
	//
	// if(world_rank==0){
	//
	// 	int arrayIndex = 0;
	//
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).vertices.size(); j++){
	// 			VTNArray[arrayIndex] = meshs.at(i).vertices.at(j).x;
	// 			VTNArray[arrayIndex+1] = meshs.at(i).vertices.at(j).y;
	// 			VTNArray[arrayIndex+2] = meshs.at(i).vertices.at(j).z;
	// 			VTNArray[arrayIndex+3] = meshs.at(i).vertices.at(j).w;
	// 			arrayIndex = arrayIndex + 4;
	// 		}
	// 	}
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
	// 			VTNArray[arrayIndex] = meshs.at(i).textures.at(j).x;
	// 			VTNArray[arrayIndex+1] = meshs.at(i).textures.at(j).y;
	// 			VTNArray[arrayIndex+2] = meshs.at(i).textures.at(j).z;
	// 			arrayIndex = arrayIndex + 3;
	// 		}
	// 	}
	//
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).normals.size(); j++){
	// 			VTNArray[arrayIndex] = meshs.at(i).normals.at(j).x;
	// 			VTNArray[arrayIndex+1] = meshs.at(i).normals.at(j).y;
	// 			VTNArray[arrayIndex+2] = meshs.at(i).normals.at(j).z;
	// 			arrayIndex = arrayIndex + 3;
	// 		}
	// 	}
	//
	// 	MPI_Bcast(VTNArray, nElementsTot, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//
	// }
	// else{
	//
	// 	MPI_Bcast(VTNArray, nElementsTot, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//
	// 	int arrayIndex = 0;
	//
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).vertices.size(); j++){
	// 			meshs.at(i).vertices.at(j).x = VTNArray[arrayIndex];
	// 			meshs.at(i).vertices.at(j).y = VTNArray[arrayIndex+1];
	// 			meshs.at(i).vertices.at(j).z = VTNArray[arrayIndex+2];
	// 			meshs.at(i).vertices.at(j).w = VTNArray[arrayIndex+3];
	// 			arrayIndex = arrayIndex + 4;
	// 		}
	// 	}
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
	// 			meshs.at(i).textures.at(j).x = VTNArray[arrayIndex];
	// 			meshs.at(i).textures.at(j).y = VTNArray[arrayIndex+1];
	// 			meshs.at(i).textures.at(j).z = VTNArray[arrayIndex+2];
	// 			arrayIndex = arrayIndex + 3;
	// 		}
	// 	}
	//
	// 	for(unsigned i=0; i<meshs.size(); i++){
	// 		for(unsigned j=0; j<meshs.at(i).normals.size(); j++){
	// 			meshs.at(i).normals.at(j).x = VTNArray[arrayIndex];
	// 			meshs.at(i).normals.at(j).y = VTNArray[arrayIndex+1];
	// 			meshs.at(i).normals.at(j).z = VTNArray[arrayIndex+2];
	// 			arrayIndex = arrayIndex + 3;
	// 		}
	// 	}
	//
	// }
	//
	// delete VTNArray;
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
