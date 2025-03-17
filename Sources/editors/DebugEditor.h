#pragma once

#include "Editor.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>

#include "Material.h"
#include "Transform.h"

class DebugEditor : public Editor {
	float &m_deltaTime;

   public:
	DebugEditor(float &deltaTime)
		: Editor("Performances"), m_deltaTime(deltaTime) {}

	void renderUI() override { ImGui::Text("FPS: %.1f", 1.0f / m_deltaTime); }
};