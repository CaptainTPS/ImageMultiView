#include <stdio.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

class PatchInpaint
{
public:

	void mainLoop(std::string colorPath, std::string maskPath, std::string depthPath);
	//color is C3, mask and depth are C1
	void mainLoop(char* colorMat, char* maskMat, char* depthMat, char* outColor, int width, int height);

private:

};