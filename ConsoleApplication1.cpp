// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

Mat image; 

void mouseClick(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;

		Vec3b colour = image.at<Vec3b>(Point(x, y));

		cout << "Colour: " << (int)colour[0] << " " << (int)colour[1] << " " << (int)colour[2] << endl;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	image = imread("E:\\Pictures\\marine-field-sky.jpg");

	if (image.empty())
	{
		cout << "Couldn't load image" << endl;
		return -1;
	}

	namedWindow("Carrier", 1);
	imshow("Carrier", image);

	setMouseCallback("Carrier", mouseClick, NULL);

	waitKey(0);

	return 0;
}

