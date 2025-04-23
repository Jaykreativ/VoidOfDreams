#pragma once

#include "Zap/Zap.h"
#include "Zap/Rendering/Window.h"
#include "Zap/Rendering/Renderer.h"
#include "Zap/Rendering/PBRenderer.h"
#include "Zap/Rendering/Gui.h"

struct RenderData {
	Zap::Window* window;
	Zap::Renderer* renderer;
	Zap::PBRenderer* pbRender;
	Zap::Gui* pGui;
};