#pragma once

#include "Editor.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <vector>

#include "Material.h"
#include "Transform.h"
#include "Light.h"

class LightsEditor : public Editor {
	std::vector<std::shared_ptr<AbstractLight>>& m_lights;

   public:
	LightsEditor(std::vector<std::shared_ptr<AbstractLight>>& lights)
		: Editor("Lights"), m_lights(lights) {}

	void renderUI() override {
		const char* items[] = {"Directional", "Point"};

		for (int i = 0; i < m_lights.size(); i++) {
			AbstractLight& light = *m_lights[i];
			if (ImGui::CollapsingHeader(
					std::string("Light " + std::to_string(i)).c_str())) {
				ImGui::Indent(10.0f);
				const char* comboLabel = items[light.getType()];

				if (ImGui::BeginCombo(
						("Type##" + std::to_string(i)).c_str(),
						comboLabel)) {	// Combo box for type selection
					for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
						const bool is_selected = (comboLabel == items[n]);
						if (ImGui::Selectable(items[n], is_selected)) {
							if (n != light.getType()) {
								std::shared_ptr<AbstractLight> new_light;
								switch (n) {
									case 0:
										new_light =
											std::make_shared<DirectionalLight>(
												light.color(),
												light.baseIntensity(),
												glm::vec3(0.0, -1.0, 0.0));
										break;
									case 1:
										new_light =
											std::make_shared<PointLight>(
												light.color(),
												light.baseIntensity(),
												glm::vec3(0.0), 1.0, 0.0, 0.0);
								}
								m_lights[i] = new_light;
								light = *new_light;
							};
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				// Common properties: color and intensity
				glm::vec3 color = light.color();
				float intensity = light.baseIntensity();
				ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(),
								  &color.x);
				ImGui::SliderFloat(("Intensity##" + std::to_string(i)).c_str(),
								   &intensity, 0.0f, 10.0f);
				light.color() = color;
				light.baseIntensity() = intensity;

				if (light.getType() == 0) {	 // Directional light
					std::shared_ptr<DirectionalLight> dirLight =
						std::dynamic_pointer_cast<DirectionalLight>(
							m_lights[i]);
					glm::vec3 dir = dirLight->getDirection();
					ImGui::SliderFloat3(
						("Direction##" + std::to_string(i)).c_str(), &dir.x,
						-1.0f, 1.0f);
					dirLight->setDirection(dir);
				} else if (light.getType() == 1) {	// Point light
					std::shared_ptr<PointLight> pointLight =
						std::dynamic_pointer_cast<PointLight>(m_lights[i]);
					glm::vec3 pos = pointLight->getTranslation();
					ImGui::SliderFloat3(
						("Position##" + std::to_string(i)).c_str(), &pos.x,
						-10.0f, 10.0f);
					pointLight->setTranslation(pos);
					ImGui::SliderFloat(
						("Attenuation constant##" + std::to_string(i)).c_str(),
						&pointLight->attenuationConstant(), 0.0f, 1.0f);
					ImGui::SliderFloat(
						("Attenuation linear##" + std::to_string(i)).c_str(),
						&pointLight->attenuationLinear(), 0.0f, 1.0f);
					ImGui::SliderFloat(
						("Attenuation quadratic##" + std::to_string(i)).c_str(),
						&pointLight->attenuationQuadratic(), 0.0f, 1.0f);
				}
				ImGui::Indent(-10.0f);
			}
		}
		if (m_lights.size() < 10) {
			if (ImGui::Button("Add light")) {
				m_lights.push_back(std::make_shared<PointLight>(
					glm::vec3(1.0f), 1.0f, glm::vec3(0.0f), 1.0f, 0.0f, 0.0f));
			}
			ImGui::SameLine();
		}
		if (m_lights.size() > 0 && ImGui::Button("Remove light")) {
			m_lights.erase(m_lights.begin() + m_lights.size() - 1);
		}
	}
};