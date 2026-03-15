
#pragma once 

class ResolutionIndependence {

public: 
	// maybe? 
	//public static float getScaleFactor(int windowWidth, int windowHeight, int baseWidth, int baseHeight); 

	// 
	float desiredBrushSize; 
	float sourceWidth; 
	float scale = desiredBrushSize / sourceWidth; 

	// drawWidth = sourceWidth * scale; 
	// drawHeight = sourceHeight * scale; 

private: 

};