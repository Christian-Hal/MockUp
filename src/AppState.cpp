#include "AppState.h"

// Centralized application components.
// These remain externally visible for existing modules still using extern declarations.
Window window;
Renderer renderer;
UI ui;
Globals global;
CanvasManager canvasManager;
BrushManager brushManager;
DrawEngine drawEngine;
FrameRenderer frameRenderer;
InputManager inputManager;

void AppState::init()
{
    
}

Window& AppState::getWindow() { return window; }
Renderer& AppState::getRenderer() { return renderer; }
UI& AppState::getUI() { return ui; }
Globals& AppState::getGlobals() { return global; }
CanvasManager& AppState::getCanvasManager() { return canvasManager; }
BrushManager& AppState::getBrushManager() { return brushManager; }
DrawEngine& AppState::getDrawEngine() { return drawEngine; }
FrameRenderer& AppState::getFrameRenderer() { return frameRenderer; }
InputManager& AppState::getInputManager() { return inputManager; }

CursorMode AppState::getCursorMode() const
{
    return currentCursorMode;
}

void AppState::setCursorMode(CursorMode mode)
{
    currentCursorMode = mode;
}