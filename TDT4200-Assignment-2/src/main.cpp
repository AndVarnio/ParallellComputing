#include <iostream>
#include <cstring>
#include "utilities/OBJLoader.hpp"
#include "utilities/lodepng.h"
#include "rasteriser.hpp"

#include <mpi.h>

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

	int nElementsTot = 0;
	int nVerticesTot = 0;
	int nTexturesTot = 0;
	int nNormalsTot = 0;

	for(unsigned i=0; i<meshs.size(); i++){
		nVerticesTot = nVerticesTot + 4*meshs.at(i).vertices.size();
		nTexturesTot = nTexturesTot + 3*meshs.at(i).textures.size();
		nNormalsTot = nNormalsTot + 3*meshs.at(i).normals.size();
	}

	nElementsTot = nVerticesTot + nTexturesTot + nNormalsTot;
	float* VTNArray = new float[nElementsTot];

	if(world_rank==0){

		int arrayIndex = 0;

		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).vertices.size(); j++){
				VTNArray[arrayIndex] = meshs.at(i).vertices.at(j).x;
				VTNArray[arrayIndex+1] = meshs.at(i).vertices.at(j).y;
				VTNArray[arrayIndex+2] = meshs.at(i).vertices.at(j).z;
				VTNArray[arrayIndex+3] = meshs.at(i).vertices.at(j).w;
				arrayIndex = arrayIndex + 4;
			}
		}
		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
				VTNArray[arrayIndex] = meshs.at(i).textures.at(j).x;
				VTNArray[arrayIndex+1] = meshs.at(i).textures.at(j).y;
				VTNArray[arrayIndex+2] = meshs.at(i).textures.at(j).z;
				arrayIndex = arrayIndex + 3;
			}
		}

		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).normals.size(); j++){
				VTNArray[arrayIndex] = meshs.at(i).normals.at(j).x;
				VTNArray[arrayIndex+1] = meshs.at(i).normals.at(j).y;
				VTNArray[arrayIndex+2] = meshs.at(i).normals.at(j).z;
				arrayIndex = arrayIndex + 3;
			}
		}

		MPI_Bcast(VTNArray, nElementsTot, MPI_FLOAT, 0, MPI_COMM_WORLD);

	}
	else{

		MPI_Bcast(VTNArray, nElementsTot, MPI_FLOAT, 0, MPI_COMM_WORLD);

		int arrayIndex = 0;

		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).vertices.size(); j++){
				meshs.at(i).vertices.at(j).x = VTNArray[arrayIndex];
				meshs.at(i).vertices.at(j).y = VTNArray[arrayIndex+1];
				meshs.at(i).vertices.at(j).z = VTNArray[arrayIndex+2];
				meshs.at(i).vertices.at(j).w = VTNArray[arrayIndex+3];
				arrayIndex = arrayIndex + 4;
			}
		}
		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).textures.size(); j++){
				meshs.at(i).textures.at(j).x = VTNArray[arrayIndex];
				meshs.at(i).textures.at(j).y = VTNArray[arrayIndex+1];
				meshs.at(i).textures.at(j).z = VTNArray[arrayIndex+2];
				arrayIndex = arrayIndex + 3;
			}
		}

		for(unsigned i=0; i<meshs.size(); i++){
			for(unsigned j=0; j<meshs.at(i).normals.size(); j++){
				meshs.at(i).normals.at(j).x = VTNArray[arrayIndex];
				meshs.at(i).normals.at(j).y = VTNArray[arrayIndex+1];
				meshs.at(i).normals.at(j).z = VTNArray[arrayIndex+2];
				arrayIndex = arrayIndex + 3;
			}
		}

	}

	delete VTNArray;
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
