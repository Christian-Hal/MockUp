#pragma once

#include "Renderer.h"
#include "UI.h"
#include "Globals.h"        // need to remove this
#include "CanvasManager.h"
#include "BrushManager.h"
#include "DrawEngine.h"
#include "FrameRenderer.h"

#include "InputManager.h"   // should these two be part of controller?
#include "Window.h"
#include "CursorMode.h"

class AppState {
public:
    // initialize the app state and create instances for the different app components
    void init();

    // central component accessors used by AppController
    Window& getWindow();
    Renderer& getRenderer();
    UI& getUI();
    Globals& getGlobals();
    CanvasManager& getCanvasManager();
    BrushManager& getBrushManager();
    DrawEngine& getDrawEngine();
    FrameRenderer& getFrameRenderer();
    InputManager& getInputManager();

    // getter and setter methods for its variables
    CursorMode getCursorMode() const;
    void setCursorMode(CursorMode mode);

private:
    // Current cursor mode variable
    CursorMode currentCursorMode = CursorMode::Draw; // set the default mode

};