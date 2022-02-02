#include "model.h"
#include "mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <queue>
#include <iostream>

std::unique_ptr<Model> Model::load_fbx(Context &context, const std::string& path)
{
	auto model = std::make_unique<Model>();

	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("Error loading model" + std::string(import.GetErrorString()));
	}

	std::queue<aiNode*> nodes;
	nodes.push(scene->mRootNode);
	while (nodes.size() > 0) {
		aiNode* node = nodes.front();
		nodes.pop();
		
		std::cout << "mesh node " << node->mName.C_Str() << std::endl;
		for (int i = 0; i < node->mNumMeshes; i++)
		{
			auto amesh = scene->mMeshes[node->mMeshes[i]];
			Mesh mesh;
			for (size_t i = 0; i < amesh->mNumVertices; i++) {
				glm::vec3 pos{
					amesh->mVertices[i].x,
					amesh->mVertices[i].y,
					amesh->mVertices[i].z
				};
				glm::vec4 color{};
				if (amesh->HasVertexColors(0)) {
					color = {
						amesh->mColors[0][i].r,
						amesh->mColors[0][i].g,
						amesh->mColors[0][i].b,
						amesh->mColors[0][i].a
					};
				};
				glm::vec2 uv{};
				if (amesh->HasTextureCoords(0)) {
					uv = {
						amesh->mTextureCoords[0][i].x,
						amesh->mTextureCoords[0][i].y
					};
				};
				Vertex v{
					pos,
					color,
					uv
				};
				mesh.vertices.push_back(v);
			}

			for (size_t f = 0; f < amesh->mNumFaces; f++) {
				if (amesh->mFaces[f].mNumIndices != 3) {
					throw std::runtime_error("tried to import non-triangle mesh");
				}
				//amesh->mFaces[i].mIndices
				mesh.indices.push_back(static_cast<uint32_t>(amesh->mFaces[f].mIndices[0]));
				mesh.indices.push_back(static_cast<uint32_t>(amesh->mFaces[f].mIndices[1]));
				mesh.indices.push_back(static_cast<uint32_t>(amesh->mFaces[f].mIndices[2]));
			}
			
			model->meshes[amesh->mName.C_Str()] = mesh;
			std::cout << "mesh " << amesh->mName.C_Str() << std::endl;
		}
		for (uint64_t i = 0; i < node->mNumChildren; i++)
		{
			nodes.push(node->mChildren[i]);
		}

		if (scene->HasTextures()) {
			for (int i = 0; i < scene->mNumTextures; i++) {
				std::cout << "texture " << scene->mTextures[i]->mFilename.C_Str() << std::endl;
			}
		}

		if (scene->HasMaterials()) {
			for (int i = 0; i < scene->mNumMaterials; i++) {
				std::cout << "material " << scene->mMaterials[i]->GetName().C_Str() << std::endl;
			}
		}
	}


	return model;
}
