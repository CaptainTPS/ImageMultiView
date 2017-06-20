#include <stdio.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

class PatchInpaint
{
public:

	void mainLoop(std::string colorPath, std::string maskPath, std::string depthPath, bool isLeft);
	//color is C3, mask and depth are C1
	void mainLoop(unsigned char* colorMat, unsigned char* maskMat, unsigned char* depthMat, unsigned char* outColor, int width, int height, bool isLeft);

private:

};