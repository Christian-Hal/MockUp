
#include "AppController.h"

#include "Window.h"
#include "Renderer.h"
#include "UI.h"
#include "Globals.h"
#include "CanvasManager.h"
#include "BrushManager.h"
#include "DrawEngine.h"
#include "FrameRenderer.h"

#include "InputManager.h"
#include "CanvasManipulation.h"
#include "ColorPicker.h"

static int SCR_WIDTH = 1280;
static int SCR_HEIGHT = 720;

// Temporary local utility object until CanvasManipulation is AppState-managed.
static CanvasManipulation canvasManipulation;


/*
Creates the window according to screen height and width values.

Initializes the UI.

@return true if window was successfully created.
*/
bool AppController::init() 
{
    // initialize the app state
    appState.init();

	auto& window = appState.getWindow();
	auto& renderer = appState.getRenderer();
	auto& ui = appState.getUI();
	auto& global = appState.getGlobals();
	auto& brushManager = appState.getBrushManager();
	auto& drawEngine = appState.getDrawEngine();
	auto& inputManager = appState.getInputManager();
	
	if (!window.create(SCR_WIDTH, SCR_HEIGHT, "Capstone", global)) {
		return false; 
	}

	if (!renderer.init(window.handle(), global)) {
		return false; 
	}

	drawEngine.init();
	brushManager.init();
	inputManager.init(window.handle());
	ui.init(window.handle(), renderer, global);

	// Route UI cursor mode interactions through AppController -> AppState.
	ui.bindCursorCallbacks(
		[this](CursorMode mode) { setCursorMode(mode); },
		[this]() { return getCursorMode(); }
	);

	// Route brush UI actions through controller-owned app state.
	ui.bindBrushCallbacks(
		[this]() -> const std::vector<BrushTool>& { return getBrushList(); },
		[this](int index) { setActiveBrush(index); },
		[this]() -> const BrushTool& { return getActiveBrush(); }
	);

	// Route hotkey label/rebind UI actions through controller-owned input flow.
	ui.bindHotkeyCallbacks(
		[this](InputAction action) { return getHotkeyString(action); },
		[this](InputAction action) { startRebind(action); },
		[this]() { return isWaitingForRebind(); },
		[this]() { return didRebindFail(); }
	);

	// Bind raw mouse input streams from InputManager into controller handlers.
	// From here on, controller chooses behavior based on AppState cursor mode.
    inputManager.bindMouseCallbacks(
        [this](const MouseState& m) { onMouseMove(m); },
        [this](const MouseState& m, int button, int action, int mods) { onMouseButton(m, button, action, mods); },
        [this](const MouseState& m, double sx, double sy) { onMouseScroll(m, sx, sy); }
    );

	// Bind keyboard action dispatch so controller owns keybind behavior decisions.
	inputManager.bindInputActionCallback(
		[this](InputAction action) { onInputAction(action); }
	);

	global.set_scr_width(SCR_WIDTH);
	global.set_scr_height(SCR_HEIGHT);

    return true;
}


/*
Runs the main program loop.
Begin frame, draw UI, end frame.
*/
void AppController::run() 
{
	auto& window = appState.getWindow();
	auto& renderer = appState.getRenderer();
	auto& ui = appState.getUI();
	auto& canvasManager = appState.getCanvasManager();
	auto& brushManager = appState.getBrushManager();
	auto& drawEngine = appState.getDrawEngine();
	auto& frameRenderer = appState.getFrameRenderer();
	auto& inputManager = appState.getInputManager();

	// RENDER LOOP 
	while (!window.shouldClose()) {
		window.pollEvents();

        // If there is an active canvas then let the draw engine grab and process the current mouse position
        if (canvasManager.hasActive()) {
            drawEngine.setCanvas(canvasManager.getActive());
            drawEngine.setColor(ui.getColor());

            if (brushManager.brushChange) { // if the brush has changed then update the draw engine's brush dab
                drawEngine.setBrushDab(brushManager.generateBrushDab(ui.brushSize));
                brushManager.brushChange = false;
            }

            if (drawEngine.isDrawing()) {
                double mouseX, mouseY;
                glfwGetCursorPos(window.handle(), &mouseX, &mouseY);
                drawEngine.processMousePos(mouseX, mouseY);
            }
		}

		// Let the draw system run a process
		drawEngine.update();

		// Render the canvas, the UI, and then clear stuff and swap buffers.
		// The order of these next four methods must not change
		renderer.beginFrame(canvasManager);
		ui.draw(canvasManager, frameRenderer);
		renderer.endFrame();
		inputManager.update();

		window.swapBuffers();
	}
}



/*
Shuts down the UI, renderer, and the window.
*/
void AppController::shutdown() {
	auto& frameRenderer = appState.getFrameRenderer();
	auto& ui = appState.getUI();
	auto& renderer = appState.getRenderer();
	auto& window = appState.getWindow();

	frameRenderer.shutdown();
	ui.shutdown();
	renderer.shutdown();
	window.destroy();
}


void AppController::setCursorMode(CursorMode mode)
{
    appState.setCursorMode(mode);
}

CursorMode AppController::getCursorMode() const
{
	return appState.getCursorMode();
}

const std::vector<BrushTool>& AppController::getBrushList()
{
	auto& brushManager = appState.getBrushManager();
	return brushManager.getLoadedBrushes();
}

const BrushTool& AppController::getActiveBrush()
{
	auto& brushManager = appState.getBrushManager();
	return brushManager.getActiveBrush();
}

void AppController::setActiveBrush(int index)
{
	auto& brushManager = appState.getBrushManager();
	brushManager.setActiveBrush(index);
}

std::string AppController::getHotkeyString(InputAction action)
{
	return InputManager::getHotkeyString(action);
}

void AppController::startRebind(InputAction action)
{
	InputManager::StartRebind(action);
}

bool AppController::isWaitingForRebind()
{
	return InputManager::IsWaitingForRebind();
}

bool AppController::didRebindFail()
{
	return InputManager::getRebindFail();
}


// mouse callback functions
// callback function for the mouse buttons
void AppController::onMouseButton(const MouseState& m, int button, int action, int mods)
{
	auto& canvasManager = appState.getCanvasManager();
	auto& drawEngine = appState.getDrawEngine();
	auto& window = appState.getWindow();

	// Handle start/stop actions that occur on press/release boundaries.
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !m.rightDown && canvasManager.hasActive())
	{
        Canvas& canvas = canvasManager.getActive();
		switch (getCursorMode())
		{
		case CursorMode::Rotate:
			canvasManipulation.startRotate(canvas, m.x, m.y);
			break;
		case CursorMode::ZoomIn:
			canvasManipulation.zooming(canvas, 1, 0.1f, m.x, m.y, window.handle());
			break;
		case CursorMode::ZoomOut:
			canvasManipulation.zooming(canvas, -1, 0.1f, m.x, m.y, window.handle());
			break;
		case CursorMode::Draw:
		case CursorMode::Erase:
			drawEngine.start();
			break;
		case CursorMode::ColorPick:
			ColorPicker::pickColor(m.x, m.y);
			break;
		default:
			break;
		}
	}

	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		ColorPicker::pickColor(m.x, m.y);
	}

	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT && drawEngine.isDrawing())
	{
		drawEngine.stop();
	}

}

// cursor x,y position callback, when left mouse button is pressed based on the mode, does the respective thing
// also consistenly updates the currX/currY with new x,y corrdinates when mouse moves
void AppController::onMouseMove(const MouseState& m)
{
    auto& canvasManager = appState.getCanvasManager();

	// Handle continuous drag actions while left button is held.
	if (m.leftDown && canvasManager.hasActive())
	{
		Canvas& canvas = canvasManager.getActive();
		switch (getCursorMode())
		{
		case CursorMode::Pan:
			canvasManipulation.panning(canvas, m.dx, m.dy);
			break;
		case CursorMode::Rotate:
			canvasManipulation.rotating(canvas, m.x, m.y);
			break;
		case CursorMode::ColorPick:
			ColorPicker::pickColor(m.x, m.y);
			break;
		default:
			break;
		}
	}
	else if (m.rightDown)
	{
		// Right-drag always samples color regardless of current tool.
		ColorPicker::pickColor(m.x, m.y);
	}
}

// scoll wheel call back, used for mouse wheel scrolling and rotating when holding r key
void AppController::onMouseScroll(const MouseState& m, double xoffset, double yoffset)
{
	auto& window = appState.getWindow();
    auto& canvasManager = appState.getCanvasManager();

	// Zoom around the current cursor position.
	if (canvasManager.hasActive()) {
		Canvas& canvas = canvasManager.getActive();
		canvasManipulation.zooming(canvas, yoffset, 0.1f, m.x, m.y, window.handle());
	}
}

void AppController::onInputAction(InputAction action)
{
    auto& drawEngine = appState.getDrawEngine();
    if (drawEngine.isDrawing()) {
        return; // Ignore input actions while drawing to prevent conflicts.
    }
    
	auto& canvasManager = appState.getCanvasManager();

	switch (action)
	{
	case InputAction::setRotate:
		setCursorMode(CursorMode::Rotate);
		break;
	case InputAction::setPan:
		setCursorMode(CursorMode::Pan);
		break;
	case InputAction::setDraw:
		setCursorMode(CursorMode::Draw);
		break;
	case InputAction::setErase:
		setCursorMode(CursorMode::Erase);
		break;
	case InputAction::setColor:
		setCursorMode(CursorMode::ColorPick);
		break;
	case InputAction::undo:
		if (canvasManager.hasActive()) {
			canvasManager.undo();
		}
		break;
	case InputAction::redo:
		if (canvasManager.hasActive()) {
			canvasManager.redo();
		}
		break;
	case InputAction::resetView:
		if (canvasManager.hasActive()) {
			Canvas& canvas = canvasManager.getActive();
			canvasManipulation.centerCamera(canvas);
		}
		break;
	default:
		break;
	}
}