#pragma once

#include "Editor.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>

#include "Material.h"

class MaterialEditor : public Editor {
	Material &m_material;

   public:
	MaterialEditor(Material &material)
		: Editor("Material"), m_material(material) {}

	void renderUI() override {
		ImGui::ColorEdit3("Albedo", &m_material.albedo().x);
		ImGui::SliderFloat("Roughness", &m_material.roughness(), 0.0f, 1.0f);
		ImGui::SliderFloat("Metalness", &m_material.metalness(), 0.0f, 1.0f);
		ImGui::ColorEdit3("F0", &m_material.F0().x);
	}
};