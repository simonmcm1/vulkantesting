
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON 1
#define TINYGLTF_NO_INCLUDE_STB_IMAGE 1
#define TINYGLTF_NO_STB_IMAGE_WRITE 1

#include "model.h"
#include "mesh.h"
#include <tiny_gltf.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <queue>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

std::unique_ptr<Model> Model::load_fbx(Context& context, const std::string& path)
{
	auto model = std::make_unique<Model>();

	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

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
				glm::vec3 normal{};
				if (amesh->HasNormals()) {
					normal = {
						amesh->mNormals[i].x,
						amesh->mNormals[i].y,
						amesh->mNormals[i].z
					};
				}
				glm::vec3 tangent{};
				if (amesh->HasTangentsAndBitangents()) {
					tangent = {
						amesh->mTangents[i].x,
						amesh->mTangents[i].y,
						amesh->mTangents[i].z
					};
				}

				Vertex v{
					pos,
					color,
					uv,
					normal,
					tangent
				};
				mesh.vertices.push_back(v);
			}

			for (size_t f = 0; f < amesh->mNumFaces; f++) {
				if (amesh->mFaces[f].mNumIndices != 3) {
					throw std::runtime_error("tried to import non-triangle mesh");
				}

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

glm::vec2 vec2_from_buffer(uint8_t* buffer_pointer) {
	float* x = (float*)buffer_pointer;
	float* y = (float*)(buffer_pointer + sizeof(float));
	return glm::vec2(*x, *y);
}

glm::vec3 vec3_from_buffer(uint8_t* buffer_pointer) {
	float* x = (float*)buffer_pointer;
	float* y = (float*)(buffer_pointer + sizeof(float));
	float* z = (float*)(buffer_pointer + sizeof(float) * 2);
	return glm::vec3(*x, *y, *z);
}

glm::vec4 vec4_from_buffer(uint8_t* buffer_pointer) {
	float* x = (float*)buffer_pointer;
	float* y = (float*)(buffer_pointer + sizeof(float));
	float* z = (float*)(buffer_pointer + sizeof(float) * 2);
	float* w = (float*)(buffer_pointer + sizeof(float) * 3);
	return glm::vec4(*x, *y, *z, *w);
}

void load_meshes(Model* model, tinygltf::Model& gltf) {
	for (const auto& mesh : gltf.meshes) {
		//std::cout << "mesh " << mesh.name << std::endl;
		uint32_t prim_index = 0;
		for (const auto& prim : mesh.primitives) {
			Mesh m;
			uint32_t vertex_count = gltf.accessors[prim.attributes.find("POSITION")->second].count;
			//lets build a vertex array that we can assign attributes on
			for (int i = 0; i < vertex_count; i++) {
				m.vertices.push_back(Vertex());
			}
			for (const auto& attr : prim.attributes) {
				auto& accessor = gltf.accessors[attr.second];
				auto& view = gltf.bufferViews[accessor.bufferView];
				if (attr.first == "POSITION") {
					for (uint32_t i = 0; i < vertex_count; i++) {
						uint32_t index = accessor.byteOffset + view.byteOffset + accessor.ByteStride(view) * i;
						glm::vec3 pos = vec3_from_buffer(&gltf.buffers[view.buffer].data[index]);
						m.vertices[i].pos = pos;
					}

					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
					assert(accessor.type == TINYGLTF_TYPE_VEC3);
				}
				else if (attr.first == "NORMAL") {
					for (uint32_t i = 0; i < vertex_count; i++) {
						uint32_t index = accessor.byteOffset + view.byteOffset + accessor.ByteStride(view) * i;
						m.vertices[i].normal = vec3_from_buffer(&gltf.buffers[view.buffer].data[index]);
					}
					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
					assert(accessor.type == TINYGLTF_TYPE_VEC3);
				}
				else if (attr.first == "TEXCOORD_0") {
					for (uint32_t i = 0; i < vertex_count; i++) {
						uint32_t index = accessor.byteOffset + view.byteOffset + accessor.ByteStride(view) * i;
						m.vertices[i].uv = vec2_from_buffer(&gltf.buffers[view.buffer].data[index]);
					}
					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
					assert(accessor.type == TINYGLTF_TYPE_VEC2);
				}
			}

			//indices
			auto& accessor = gltf.accessors[prim.indices];
			auto& view = gltf.bufferViews[accessor.bufferView];
			for (uint32_t i = 0; i < accessor.count; i++) {
				uint32_t index = accessor.byteOffset + view.byteOffset + accessor.ByteStride(view) * i;
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					uint16_t* idx = (uint16_t*)&gltf.buffers[view.buffer].data[index];
					m.indices.push_back(static_cast<uint32_t>(*idx));
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					uint32_t* idx = (uint32_t*)&gltf.buffers[view.buffer].data[index];
					m.indices.push_back(*idx);
				}
				else {
					throw std::runtime_error("unhandled GLTX indices component type");
				}
			}
			assert(accessor.type == TINYGLTF_TYPE_SCALAR);

			RecalculateTangents(m);

			model->meshes[mesh.name + "_" + std::to_string(prim_index++)] = m;
		}
	}
}

SceneNode* load_node(Model* model, tinygltf::Model& gltf, uint32_t index, const glm::mat4 &parent_matrix) {
	const auto& node = gltf.nodes.at(index);
	
	SceneNode scene_node;
	if (node.mesh >= 0) {
		scene_node.mesh = gltf.meshes[node.mesh].name + "_0";
		scene_node.name = node.name;
		//TODO: handle multiple primitives per mesh. scene_node.mesh should be a vector maybe?
	}
	
	if (node.matrix.empty()) {
		//pos,rot,scale
		if(node.translation.size() > 0)
			scene_node.transform.position = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
		if(node.rotation.size() > 0)
			scene_node.transform.rotation = glm::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
		if(node.scale.size() > 0)
			scene_node.transform.scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
	}
	else {
		//matrix
		scene_node.transform.set_matrix(glm::make_mat4(node.matrix.data()));
	}

	//transform by the parents matrix
	glm::mat4 global_matrix = scene_node.transform.matrix() * parent_matrix;
	scene_node.transform.set_matrix(global_matrix);

	for (uint32_t node_index : node.children) {
		scene_node.children.push_back(load_node(model, gltf, node_index, global_matrix));
	}

	model->nodes[node.name] = scene_node;
	return &model->nodes[node.name];
}

void load_nodes(Model* model, tinygltf::Model &gltf) {
	auto scene = gltf.scenes.at(gltf.defaultScene);

	for (uint32_t node_index : scene.nodes) {
		model->scene_root.children.push_back(load_node(model, gltf, node_index, model->scene_root.transform.matrix()));
	}
}

std::unique_ptr<Model> Model::load_gltf(Context& context, const std::string& path)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf;
	std::string err;
	std::string warn;
	loader.LoadASCIIFromFile(&gltf, &err, &warn, path);

	if (!err.empty()) {
		throw std::runtime_error("error loading gltf model " + err);
	}

	if (!warn.empty()) {
		std::cout << "warning while loading model:" << warn << std::endl;
	}

	auto model = std::make_unique<Model>();

	load_meshes(model.get(), gltf);
	load_nodes(model.get(), gltf);


	return model;
}