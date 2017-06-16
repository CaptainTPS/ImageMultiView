#ifndef _CV_FUNCTIONS_H_
#define _CV_FUNCTIONS_H_

class cvFunctions
{
public:
	cvFunctions();
	~cvFunctions();

	static void filterDepth(unsigned char* d, int width, int height, int channel);
	static void seeImage(unsigned char* d, int width, int height, int channel);
private:

};

#endif