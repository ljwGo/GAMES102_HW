#include "DenoiseSystem.h"

#include "../Components/DenoiseData.h"

#include <_deps/imgui/imgui.h>

#include <spdlog/spdlog.h>

#include "../Common.h"

/* quadric error metric 二次度量误差曲面简化 */

using namespace Ubpa;

void DenoiseSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<DenoiseData>();
		if (!data)
			return;

		if (ImGui::Begin("Denoise")) {
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
						data->heMesh->Vertices().at(i)->position = castVector3f(data->mesh->GetPositions().at(i));

					data->heMesh->qem->SetHEMesh(data->heMesh.get());
					data->heMesh->qem->Init();

					spdlog::info("Mesh to HEMesh success");
				}();
			}

			if (ImGui::Button("Add Noise")) {
				[&]() {
					if (!data->heMesh->IsTriMesh()) {
						spdlog::warn("HEMesh isn't triangle mesh");
						return;
					}

					for (auto* v : data->heMesh->Vertices()) {
						v->position += castVector3f(data->randomScale * (
							2.f * Ubpa::vecf3{ Ubpa::rand01<float>(),Ubpa::rand01<float>() ,Ubpa::rand01<float>() } - Ubpa::vecf3{ 1.f }
						));
					}

					spdlog::info("Add noise success");
				}();
			}

			if (ImGui::Button("Set Color Black")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					data->mesh->SetToEditable();
					std::vector<rgbf> colors;
					for (const auto& v : data->mesh->GetPositions())
						//colors.push_back((n.as<valf3>() + valf3{ 1.f }) / 2.f);
						colors.push_back(Ubpa::rgbf(0, 0, 0));
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Normal to Color Success");
					}();
			}

			if (ImGui::Button("Contract")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh) {
						spdlog::warn("heMesh is nullptr");
						return;
					}

					data->heMesh->qem->Contract(data->iterCount);

					spdlog::info("Contract Success");

					}();
			}

			if (ImGui::Button("Set Normal to Color")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					data->mesh->SetToEditable();
					const auto& normals = data->mesh->GetNormals();
					std::vector<rgbf> colors;
					for (const auto& n : normals)
						colors.push_back((n.as<valf3>() + valf3{ 1.f }) / 2.f);
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Normal to Color Success");
				}();
			}

			if (ImGui::Button("Set Root Color")) {
				[&]() {
					if (!data->mesh) {
						spdlog::warn("mesh is nullptr");
						return;
					}

					if (!data->heMesh) {
						spdlog::warn("heMesh is nullptr");
						return;
					}

					data->mesh->SetToEditable();
					std::vector<rgbf> colors;

					auto& heap = data->heMesh->qem->GetHeap();
					MyPair* pair = heap.Root();
					assert(pair != nullptr);
					for (const auto& v : data->heMesh->Vertices()){
						if (v == pair->pair.first || v == pair->pair.second) {
							colors.push_back(Ubpa::rgbf(255, 0, 0));
							continue;
						}
						colors.push_back(Ubpa::rgbf(0, 0, 0));
					}
					data->mesh->SetColors(std::move(colors));

					spdlog::info("Set Normal to Color Success");
					}();
			}

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

					for (size_t i = 0; i < N; i++) {
						positions[i] = castPointf3(data->heMesh->Vertices().at(i)->position);
					}

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
					data->mesh->GenNormals();
					data->mesh->GenTangents();

					spdlog::info("HEMesh to Mesh success");
				}();
			}

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
		
			if (ImGui::Button("Test")) {
				CalcPlaneExpression(Vector3f(), Vector3f(), Vector3f());
			}
}
		ImGui::End();
	});
}
