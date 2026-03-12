
#pragma once

#include <glfw/glfw3.h>
#include "Canvas.h"

class CanvasManipulation {
public:
	void zooming(Canvas& canvas, double yoffset, float zoomSpeed, double xpos, double ypos, GLFWwindow* window);
	//void zoomDragging(Canvas& canvas, double yoffset, float zoomSpeed, double xpos, double ypos);
	void panning(Canvas& canvas, double deltaX, double deltaY);
	void startRotate(Canvas& canvas, double xpos, double ypos);
	void rotating(Canvas& canvas, double xpos, double ypos);
	void centerCamera(Canvas& canvas);

private:
	void viewMatrix(Canvas& canvas, double x, double y, float oldZoom);
};
