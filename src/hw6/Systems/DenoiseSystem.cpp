#include "DenoiseSystem.h"

#include "../Components/DenoiseData.h"

#include <_deps/imgui/imgui.h>

#include <spdlog/spdlog.h>

using namespace Ubpa;

rgbf ColorMap(float c) {
	float r = 0.8f, g = 1.f, b = 1.f;
	c = c < 0.f ? 0.f : (c > 1.f ? 1.f : c);

	if (c < 1.f / 8.f) {
		r = 0.f;
		g = 0.f;
		b = b * (0.5f + c / (1.f / 8.f) * 0.5f);
	}
	else if (c < 3.f / 8.f) {
		r = 0.f;
		g = g * (c - 1.f / 8.f) / (3.f / 8.f - 1.f / 8.f);
		b = b;
	}
	else if (c < 5.f / 8.f) {
		r = r * (c - 3.f / 8.f) / (5.f / 8.f - 3.f / 8.f);
		g = g;
		b = b - (c - 3.f / 8.f) / (5.f / 8.f - 3.f / 8.f);
	}
	else if (c < 7.f / 8.f) {
		r = r;
		g = g - (c - 5.f / 8.f) / (7.f / 8.f - 5.f / 8.f);
		b = 0.f;
	}
	else {
		r = r - (c - 7.f / 8.f) / (1.f - 7.f / 8.f) * 0.5f;
		g = 0.f;
		b = 0.f;
	}

	return rgbf{ r,g,b };
}

void DenoiseSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<DenoiseData>();
		if (!data)
			return;

		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		if (ImGui::Begin("Denoise")) {
			ImGui::Text("Mesh Data Structure Change");
			if (ImGui::Button("Mesh to HEMesh")) {
				data->heMesh->Clear();
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (data->mesh->GetSubMeshes().size() != 1) {
						spdlog::warn("number of submeshes isn't 1");
						return;
					}

					data->copy = *data->mesh;

					std::vector<size_t> indices(data->mesh->GetIndices().begin(), data->mesh->GetIndices().end());
					data->heMesh->Init(indices, 3);
					if (!data->heMesh->IsTriMesh())
						spdlog::warn("HEMesh init fail");
					
					for (size_t i = 0; i < data->mesh->GetPositions().size(); i++)
						data->heMesh->Vertices().at(i)->position = data->mesh->GetPositions().at(i);

					spdlog::info("Mesh to HEMesh success");
				}();
			}
			ImGui::SameLine();
			if (ImGui::Button("HEMesh to Mesh")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh->IsTriMesh() || data->heMesh->IsEmpty()) {
						spdlog::warn("HEMesh isn't triangle mesh or is empty");
						return;
					}

					data->mesh->SetToEditable();

					const size_t N = data->heMesh->Vertices().size();
					const size_t M = data->heMesh->Polygons().size();
					std::vector<Ubpa::pointf3> positions(N);
					std::vector<uint32_t> indices(M * 3);
					for (size_t i = 0; i < N; i++)
						positions[i] = data->heMesh->Vertices().at(i)->position;
					for (size_t i = 0; i < M; i++) {
						auto tri = data->heMesh->Indices(data->heMesh->Polygons().at(i));
						indices[3 * i + 0] = static_cast<uint32_t>(tri[0]);
						indices[3 * i + 1] = static_cast<uint32_t>(tri[1]);
						indices[3 * i + 2] = static_cast<uint32_t>(tri[2]);
					}
					data->mesh->SetColors({});
					data->mesh->SetUV({});
					data->mesh->SetPositions(std::move(positions));
					data->mesh->SetIndices(std::move(indices));
					data->mesh->SetSubMeshCount(1);
					data->mesh->SetSubMesh(0, { 0, M * 3 });
					data->mesh->GenUV();
					//data->mesh->GenNormals();
					//data->mesh->GenTangents();

					spdlog::info("HEMesh to Mesh success");
					}();
			}
			ImGui::SameLine();
			if (ImGui::Button("Recover Mesh")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}
					if (data->copy.GetPositions().empty()) {
						spdlog::warn("copied mesh is empty");
						return;
					}

					*data->mesh = data->copy;

					spdlog::info("recover success");
					}();
			}

			ImGui::Text("Data Handle");
			if (ImGui::Button("Add Noise")) {
				[&]() {
					if (!data->heMesh->IsTriMesh()) {
						spdlog::warn("HEMesh isn't triangle mesh");
						return;
					}

					for (auto* v : data->heMesh->Vertices()) {
						v->position += data->randomScale * (
							2.f * Ubpa::vecf3{ Ubpa::rand01<float>(),Ubpa::rand01<float>() ,Ubpa::rand01<float>() } - Ubpa::vecf3{ 1.f }
						);
					}

					spdlog::info("Add noise success");
				}();
			}

			ImGui::SameLine();
			if (ImGui::Button("Set Color Black")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					data->mesh->SetToEditable();
					const auto& normals = data->mesh->GetNormals();
					std::vector<rgbf> colors;
					for (const auto& n : normals)
						//colors.push_back((n.as<valf3>() + valf3{ 1.f }) / 2.f);
						colors.push_back(Ubpa::rgbf(0, 0, 0));
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Normal to Color Success");
				}();
			}

			ImGui::SameLine();
			if (ImGui::Button("Handle Redundant")) {
				[&data]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (data->mesh->GetSubMeshes().size() != 1) {
						spdlog::warn("number of submeshes isn't 1");
						return;
					}

					// To handle multiple same position vertex.
					std::vector<Ubpa::pointf3> allVertex = data->mesh->GetPositions();
					std::vector<size_t> sameVertexIx;
					std::vector<uint32_t> newIndices = data->mesh->GetIndices();

					for (int i = 0; i < allVertex.size(); ++i) {
						for (int j = i + 1; j < allVertex.size(); ++j) {
							// 有位置相同的顶点
							if (allVertex[i] == allVertex[j]) {
								bool isHandle = false;

								// 相同的顶点之前是否已经处理过了
								for (int k = 0; k < sameVertexIx.size(); ++k) {
									if (sameVertexIx[k] == i) {
										isHandle = true;
										break;
									}
								}

								if (!isHandle) {
									sameVertexIx.push_back(j);

									// 处理多余的相同顶点的索引值
									for (int l = 0; l < newIndices.size(); ++l) {
										if (j == newIndices[l]) {
											newIndices[l] = i;
										}
									}
								}
							}
						}
					}

					// Delete redundant same vertex.
					int i = 0;
					bool hasDelete;
					for (auto iter = allVertex.begin(); iter < allVertex.end();) {
						hasDelete = false;
						for (int j = 0; j < sameVertexIx.size(); ++j) {
							// 如果顶点有其它相同位置的顶点
							if (i == sameVertexIx[j]) {
								iter = allVertex.erase(iter);
								hasDelete = true;
								break;
							}
						}
						i++;
						if (!hasDelete) iter++;
					}

					// Update
					//data->mesh->SetColors({});
					//data->mesh->SetUV({});
					data->mesh->SetPositions(std::move(allVertex));
					data->mesh->SetIndices(std::move(newIndices));
					//data->mesh->SetSubMeshCount(1);
					//data->mesh->SetSubMesh(0, { 0, newIndices.size() * 3 });
					//data->mesh->GenUV();
					//data->mesh->GenNormals();
					//data->mesh->GenTangents();

					spdlog::info("Handle Same Position Success!");

					}();
			}
			ImGui::SameLine();
			if (ImGui::Button("If Has Same")) {
				[&data]() {
					const std::vector<Vertex*> allVertex = data->heMesh->Vertices();
					for (int i = 0; i < allVertex.size(); ++i) {
						for (int j = i + 1; j < allVertex.size(); ++j) {
							Ubpa::pointf3& p0 = allVertex[i]->position;

							if (*(allVertex[i]) == *(allVertex[j])) {
								spdlog::info("Mesh has Same Vertex Position:");
								return;
							}
						}
					}

					spdlog::info("Mesh All Vertex Position is Different");
					}();
			}

			ImGui::Text("Smoothing");

			ImGui::RadioButton("One Side", &data->BoundaryCalcMode, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Drop Side", &data->BoundaryCalcMode, 1);

			ImGui::RadioButton("No Constraint", &data->SLMmode, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Hard Constraint", &data->SLMmode, 1);
			ImGui::SameLine();
			ImGui::RadioButton("Soft Constraint", &data->SLMmode, 2);


			if (ImGui::Button("Local Laplace Smooth")) {
				// 匿名函数的作用是减少return的作用域,以确保调用End()
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh->IsTriMesh() || data->heMesh->IsEmpty()) {
						spdlog::warn("HEMesh isn't triangle mesh or is empty");
						return;
					}

					data->heMesh->UpdateVertexsPos(data->num_iterations, data->lambda);
					spdlog::info("update success");
					}();
			}
			ImGui::SameLine();
			if (ImGui::Button("Global Laplace Smooth")) {
				// 匿名函数的作用是减少return的作用域,以确保调用End()
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh->IsTriMesh() || data->heMesh->IsEmpty()) {
						spdlog::warn("HEMesh isn't triangle mesh or is empty");
						return;
					}

					data->heMesh->GLS(data->SLMmode, (BoundaryWeightCalcMode)data->BoundaryCalcMode);
					spdlog::info("Global laplace smooth update success");
					}();
			}
			ImGui::SameLine();
			if (ImGui::Button("Generate UV")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh->IsTriMesh() || data->heMesh->IsEmpty()) {
						spdlog::warn("HEMesh isn't triangle mesh or is empty");
						return;
					}

					data->heMesh->BoundaryMap(data->convexShape, data->edgeLen);
					spdlog::info("Generate UV Success");
					}();
			}

			ImGui::Text("Visibility");
			if (ImGui::Button("Normal")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					data->mesh->SetToEditable();
					const auto& normals = data->mesh->GetNormals();
					std::vector<rgbf> colors;
					for (const auto& n : normals) {
						//spdlog::info(pow2(n.at(0)) + pow2(n.at(1)) + pow2(n.at(2))); //1.0
						colors.push_back((n.as<valf3>() + valf3{ 1.f }) / 2.f);
					}
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Normal to Color Success");
					}();
			}
			ImGui::SameLine();
			if (ImGui::Button("Mean Curvature")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh->IsTriMesh() || data->heMesh->IsEmpty()) {
						spdlog::warn("HEMesh isn't triangle mesh or is empty");
						return;
					}

					data->mesh->SetToEditable();
					std::vector<float> cs;
					for (auto* v : data->heMesh->Vertices())
						cs.push_back(v->GetMeanCurvatureOperator().norm() / 2.f);
					std::vector<rgbf> colors;
					for (auto c : cs)
						colors.push_back(ColorMap(c));
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Mean Curvature to Color Success");
					}();
			}
		}
		ImGui::End();
	});
}
