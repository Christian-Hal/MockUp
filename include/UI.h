
#pragma once 

#include <functional>
#include <string>
#include <vector>

#include "Renderer.h"
#include "CanvasManager.h"
#include "Canvas.h"
#include "Globals.h"
#include "FrameRenderer.h"
#include "BrushTool.h"
#include "InputManager.h"

#include <GLFW/glfw3.h>

#include "CursorMode.h"

#include "ImSequencer.h"


// enum for keeping track of default / modular UI state
enum class UIMode {
	Default,
	Modular
};

// enum for all of the different UI elements
enum class UIElement
{
	colorWheel,
	brushSizeSlider,
	brushSelection,
	cursorModeButtons,
	animationTimeline,
	layers,
};

// this creates a list of the elements that we can interate through
constexpr UIElement elements[] = {
	UIElement::colorWheel,
	UIElement::brushSizeSlider,
	UIElement::brushSelection,
	UIElement::cursorModeButtons,
	UIElement::animationTimeline,
	UIElement::layers,
};

// delegate for the ImSequencer animation timeline 
//struct AnimationDelegate : public ImSequencer::SequenceInterface
//{
//	// fixed demo data (guaranteed visible)
//	int frameMin = 0;
//	int frameMax = 100;
//
//	static const int ITEM_COUNT = 5;
//
//	int starts[ITEM_COUNT] = { 0, 10, 20, 30, 40 };
//	int ends[ITEM_COUNT] = { 10, 25, 50, 60, 90 };
//
//	// timeline range
//	int GetFrameMin() const override { return frameMin; }
//	int GetFrameMax() const override { return frameMax; }
//
//	int GetItemCount() const override { return ITEM_COUNT; }
//
//	int GetItemTypeCount() const override { return 1; }
//	const char* GetItemTypeName(int) const override { return "Track"; }
//
//	const char* GetItemLabel(int index) const override {
//		static char buf[32];
//		sprintf_s(buf, "Track %d", index);
//		return buf;
//	}
//
//	void Get(int index, int** start, int** end, int* type, unsigned int* color) override
//	{
//		if (start) *start = &starts[index];
//		if (end)   *end = &ends[index];
//		if (type)  *type = 0;
//		if (color) *color = IM_COL32(100, 180, 255, 255);
//	}
//
//	void Add(int) override {}
//	void Del(int) override {}
//	void Duplicate(int) override {}
//
//	void BeginEdit(int) override {}
//	void EndEdit() override {}
//
//	void DoubleClick(int) override {}
//};

struct AnimationDelegate : public ImSequencer::SequenceInterface
{
	CanvasManager* manager = nullptr;

	//  tracks = layers (NOT frames)
	int GetItemCount() const override
	{
		if (!manager || !manager->hasActive())
			return 0;

		return manager->getActive().getNumLayers();
	}

	int GetFrameMin() const override { return 1; }
	int GetFrameMax() const override { return FrameRenderer::getNumFrames(); }

	int GetItemTypeCount() const override { return 1; }
	const char* GetItemTypeName(int) const override { return "Layer"; }

	const char* GetItemLabel(int index) const override
	{
		static char buf[32];
		sprintf_s(buf, "Layer %d", index);
		return buf;
	}

	void Get(int index, int** start, int** end, int* type, unsigned int* color) override
	{
		static std::vector<int> starts;
		static std::vector<int> ends;

		int frames = FrameRenderer::getNumFrames();

		starts.resize(frames);
		ends.resize(frames);

		for (int i = 0; i < frames; i++)
		{
			starts[i] = i + 1;
			ends[i] = i + 2;
		}

		if (start) *start = &starts[FrameRenderer::getCurFrame() - 1];
		if (end)   *end = &ends[FrameRenderer::getCurFrame() - 1];
		if (type)  *type = 0;

		if (color)
		{
			*color = (index == manager->getActive().getCurLayer())
				? IM_COL32(255, 200, 100, 255)
				: IM_COL32(100, 180, 255, 255);
		}
	}

	void DoubleClick(int index) override
	{
		if (!manager) return;

		Canvas& c = manager->getActive();

		int targetFrame = index + 1;
		int delta = targetFrame - FrameRenderer::getCurFrame();

		FrameRenderer::selectFrame(c, delta);
	}

	void Add(int) override
	{
		if (!manager) return;

		Canvas& c = manager->getActive();

		FrameRenderer::createFrame(c);

		int newFrame = FrameRenderer::getCurFrame();
		FrameRenderer::selectFrame(c, 0); // sync refresh (important)
	}

	void Del(int) override
	{
		if (!manager) return;
		FrameRenderer::removeFrame(manager->getActive());
	}

	void Duplicate(int) override {}
	void BeginEdit(int) override {}
	void EndEdit() override {}
};

class UI {

public:
	using SetCursorModeCallback = std::function<void(CursorMode)>;
	using GetCursorModeCallback = std::function<CursorMode()>;
	using GetBrushListCallback = std::function<const std::vector<BrushTool>& ()>;
	using SetActiveBrushCallback = std::function<void(int)>;
	using GetActiveBrushCallback = std::function<const BrushTool& ()>;
	using LoadBrushCallback = std::function<void(const std::string&)>;
	using GenerateBrushDabCallback = std::function<std::vector<float>(int)>;
	using GetHotkeyLabelCallback = std::function<std::string(InputAction)>;
	using StartRebindCallback = std::function<void(InputAction)>;
	using BoolCallback = std::function<bool()>;
	using ResetCanvasPositionCallback = std::function<void()>;

	using saveToRecentActivityCallback = std::function<void(const std::string&)>;
	using getRecentActivityCallback = std::function<const std::vector<std::string>& ()>;
	using getDefaultFolderPathCallback = std::function<std::string()>;
	using setDefaultFolderPathCallback = std::function<void(const std::string&)>;

	// Lets the controller provide cursor state read/write hooks.
	// UI emits intent through these callbacks instead of owning app state.
	void bindCursorCallbacks(SetCursorModeCallback setCb, GetCursorModeCallback getCb);
	void bindBrushCallbacks(GetBrushListCallback getListCb, SetActiveBrushCallback setActiveCb, GetActiveBrushCallback getActiveCb, LoadBrushCallback loadBrushCb, GenerateBrushDabCallback genDabCb);
	void bindHotkeyCallbacks(GetHotkeyLabelCallback getLabelCb, StartRebindCallback startCb, BoolCallback isWaitingCb, BoolCallback didFailCb);
	void bindCanvasCallbacks(ResetCanvasPositionCallback resetPositionCb);

	void bindRecentActivityCallbacks(saveToRecentActivityCallback saveCb, getRecentActivityCallback getCb);
	void bindDefaultFolderPathCallback(getDefaultFolderPathCallback getDefaultFolderPathCb, setDefaultFolderPathCallback setDefaultFolderPathCb);

	// ui drawing functions
	void drawUI(CanvasManager& canvasManager, FrameRenderer frameRenderer);

	// other functions
	void init(GLFWwindow* window, Renderer& renderer, Globals& g_inst);
	void shutdown();
	Color getColor();
	void setColor(Color pixelColor);
	int brushSize = 1; // default brush size

	CursorMode getCursorMode() const;
	void setCursorMode(CursorMode);

	// ui state stuff
	enum class UIState {
		start_menu,
		main_screen
	};
	static bool showNewCanvasPopup;

	void closeCanvasPopup(int index);
	void requestCloseCanvas(int index, CanvasManager& canvasManager);

private:
	SetCursorModeCallback setCursorModeCb;
	GetCursorModeCallback getCursorModeCb;

	GetBrushListCallback getBrushListCb;
	SetActiveBrushCallback setActiveBrushCb;
	GetActiveBrushCallback getActiveBrushCb;
	LoadBrushCallback loadBrushFromFileCb;
	GenerateBrushDabCallback generateDabCb;

	GetHotkeyLabelCallback getHotkeyLabelCb;
	StartRebindCallback startRebindCb;
	BoolCallback isWaitingForRebindCb;
	BoolCallback didRebindFailCb;

	ResetCanvasPositionCallback resetCanvasPositionCb;

	// keeps track of the elements and their visibility
	std::unordered_map<UIElement, bool> elementVisibility;
	UIMode uiMode = UIMode::Default;

	saveToRecentActivityCallback saveToRecentActivityCb;
	getRecentActivityCallback getRecentActivityCb;

	getDefaultFolderPathCallback getDefaultFolderPathCb;
	setDefaultFolderPathCallback setDefaultFolderPathCb;

	// main UI drawing functions for each state
	void drawStartScreen(CanvasManager& canvasManager);
	void drawMainScreen(CanvasManager& canvasManager, FrameRenderer frameRenderer);

	// helper functions for drawing different panels / popups
	void drawCustomCursor(CanvasManager& canvasManager);
	void drawLeftPanel(CanvasManager& canvasManager);
	void drawRightPanel(CanvasManager& canvasManager);
	void drawBottomPanel(CanvasManager& canvasManager, FrameRenderer frameRenderer);
	void drawBlockPanel(CanvasManager& canvasManager);
	void drawCanvasTabs(CanvasManager& canvasManager);
	void drawMainMenu(CanvasManager& canvasManager);
	void drawColorWindow(CanvasManager& canvasManager);
	void drawBrushSizeWindow(CanvasManager& canvasManager);
	void drawLayersWindow(CanvasManager& canvasManager);
	void drawBrushesWindow(CanvasManager& canvasManager);
	void drawCursorModesWindow(CanvasManager& canvasManager);
	void drawTimelineWindow(CanvasManager& canvasManager);

	void drawNewCanvasPopup(CanvasManager& canvasManager);
	void drawSettingsPopup(CanvasManager& canvasManager);

	void drawTopPanel(CanvasManager& canvasManager);
	// keeps track of the UI's current state so we know what stuff to draw
	UIState curState = UIState::start_menu;

	// tracks the window display size for the ui panel sizing calculations
	float displayWidth;
	float displayHeight;

	// boolean variables for which popups/panels to show
	bool showPanels = true;
	bool showSettingsPopup = false;
	bool showCloseConfirm = false;
	int pendingCloseIndex = -1;
};