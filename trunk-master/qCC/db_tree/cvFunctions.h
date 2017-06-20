#ifndef _CV_FUNCTIONS_H_
#define _CV_FUNCTIONS_H_

#include <vector>
#include <string>

using std::vector;
using std::string;

class cvFunctions
{
public:
	cvFunctions();
	~cvFunctions();

	static void filterDepth(unsigned char* d, int width, int height, int channel);
	static void seeImage(unsigned char* d, int width, int height, int channel);
	static void genVideo(vector<unsigned char*>& dataContainer, int width, int height, int channel, string path);
private:

};

#endif