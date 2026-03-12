#pragma once

#include "AppState.h"
#include <string>

class AppController {
public:
    bool init();        // initialize the app controller and the app state
    void run();         // run the main loop of the application
    void shutdown();    // clean up resources and shut down the application

    // -- Callback methods used for interaction with the app state from components -- //
    // cursor mode
    void setCursorMode(CursorMode mode);
    CursorMode getCursorMode() const;

    // brush info
    const std::vector<BrushTool>& getBrushList();
    const BrushTool& getActiveBrush();
    void setActiveBrush(int index);

    // hotkey actions
    std::string getHotkeyString(InputAction action);
    void startRebind(InputAction action);
    bool isWaitingForRebind();
    bool didRebindFail();

private:
    // app state instance
    AppState appState; 

    // mouse callback functions
    void onMouseMove(const MouseState& m);
    void onMouseButton(const MouseState& m, int button, int action, int mods);
    void onMouseScroll(const MouseState& m, double xoffset, double yoffset);
    void onInputAction(InputAction action);
};