// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <cstdint>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

struct Pos
{
	int x;
	int y;
};

#define noiseWidth 1600  
#define noiseHeight 900

double noise[noiseWidth][noiseHeight]; //the noise array

double smoothNoise(double x, double y);
double turbulence(double x, double y, double size);
void generateNoise();

const int MAXDISTANCE = 150;

int mode;
int lowestSky;
int distanceSky;
int distanceCloud;

struct Pos Pos1;
struct Pos Pos2;

Mat image, original;
Vec3b sky;
Vec3b cloud;

void generateNoise()
{
	for (int x = 0; x < noiseWidth; x++)
		for (int y = 0; y < noiseHeight; y++)
		{
			noise[x][y] = (rand() % 32768) / 32768.0;
		}
}

double smoothNoise(double x, double y)
{
	//get fractional part of x and y
	double fractX = x - int(x);
	double fractY = y - int(y);

	//wrap around
	int x1 = (int(x) + noiseWidth) % noiseWidth;
	int y1 = (int(y) + noiseHeight) % noiseHeight;

	//neighbor values
	int x2 = (x1 + noiseWidth - 1) % noiseWidth;
	int y2 = (y1 + noiseHeight - 1) % noiseHeight;

	//smooth the noise with bilinear interpolation
	double value = 0.0;
	value += fractX       * fractY       * noise[x1][y1];
	value += fractX       * (1 - fractY) * noise[x1][y2];
	value += (1 - fractX) * fractY       * noise[x2][y1];
	value += (1 - fractX) * (1 - fractY) * noise[x2][y2];

	return value;
}

double turbulence(double x, double y, double size)
{
	double value = 0.0, initialSize = size;

	while (size >= 1)
	{
		value += smoothNoise(x / size, y / size) * size;
		size /= 2.0;
	}

	return(128.0 * value / initialSize);
}

void mouseClick(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		Vec3b colour = image.at<Vec3b>(Point(x, y));

		if (mode == 0)
		{
			sky = colour;
			mode++;

			cout << "Colour (BGR): " << (int)colour[0] << " " << (int)colour[1] << " " << (int)colour[2] << endl;
		}

		else if (mode == 1)
		{
			lowestSky = y;
			mode++;

			cout << "Position (" << x << ", " << y << ")" << endl;
		}

		else if (mode == 2)
		{
			cloud = colour;
			mode++;

			cout << "Colour (BGR): " << (int)colour[0] << " " << (int)colour[1] << " " << (int)colour[2] << endl;
		}

		else if (mode == 3)
		{
			Pos1.x = x;
			Pos1.y = y;
			mode++;
		}

		else if (mode == 4)
		{
			Pos2.x = x;
			Pos2.y = y;
			mode++;
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	mode = 0;
	image = imread("E:\\Pictures\\marine-field-sky.jpg");

	if (image.empty())
	{
		cout << "Couldn't load image" << endl;
		return -1;
	}

	namedWindow("Carrier", 1);
	imshow("Carrier", image);

	setMouseCallback("Carrier", mouseClick, NULL);

	cout << "Please click on the sky." << endl;

	waitKey(0);

	for (int y = 0; y < lowestSky; y++)
	{
		cout << y << endl;
		for (int x = 0; x < image.cols; x++)
		{
			//cout << "Position (" << x << ", " << y << ")" << endl;
			Vec3b colour = image.at<Vec3b>(Point(x, y));

			distanceSky = abs(colour[0] - sky[0]) + abs(colour[1] - sky[1]) + abs(colour[2] - sky[2]);
			distanceCloud = abs(colour[0] - cloud[0]) + abs(colour[1] - cloud[1]) + abs(colour[2] - cloud[2]);
			distanceCloud /= 3;

			if ((distanceSky < distanceCloud) && (distanceSky < MAXDISTANCE))
			{
				image.at<Vec3b>(Point(x, y)) = sky;
			}

			else if ((distanceSky > distanceCloud) && (distanceCloud < MAXDISTANCE))
			{
				image.at<Vec3b>(Point(x, y)) = cloud;
			}

			else
			{
				colour[0] = 0;
				colour[1] = 0;
				colour[2] = 0;

				image.at<Vec3b>(Point(x, y)) = colour;
			}
		}
	}

	Vec3b colour;

	colour[0] = 0;
	colour[1] = 0;
	colour[2] = 0;

	for (int y = lowestSky; y < image.rows; y++)
	{
		cout << y << endl;
		for (int x = 0; x < image.cols; x++)
		{
			image.at<Vec3b>(Point(x, y)) = colour;
		}
	}

	imshow("Carrier", image);
	waitKey(0);

	/*for (int x = Pos1.x; x < Pos2.x; x++)
	{
	image.at<Vec3b>(Point(x, Pos1.y)) = colour;
	}

	for (int y = Pos1.y; y < Pos2.y; y++)
	{
	image.at<Vec3b>(Point(Pos2.x, y)) = colour;
	}

	for (int y = Pos1.y; y < Pos2.y; y++)
	{
	image.at<Vec3b>(Point(Pos1.x, y)) = colour;
	}

	for (int x = Pos1.x; x < Pos2.x; x++)
	{
	image.at<Vec3b>(Point(x, Pos2.y)) = colour;
	}

	imshow("Carrier", image);
	waitKey(0);
	*/
	generateNoise();
	Mat fullImageHSV;
	int distanceFromEdge;
	int finalDistance;
	int started = 0;
	int top, bottom;
	char **cloud = (char **)malloc((Pos2.x - Pos1.x) * (Pos2.y - Pos1.y));

	original = imread("E:\\Pictures\\marine-field-sky.jpg");

	cvtColor(original, fullImageHSV, CV_BGR2HLS);

	for (int x = Pos1.x; x < Pos2.x; x++)
	{
		for (int y = Pos1.y; y < Pos2.y; y++)
		{
			//cloud[x - Pos1.x][y - Pos1.y] = 0;

			finalDistance = 1000000;
			distanceFromEdge = y - Pos1.y;

			if (distanceFromEdge < finalDistance)
			{
				finalDistance = distanceFromEdge;
			}

			distanceFromEdge = Pos2.x - x;

			if (distanceFromEdge < finalDistance)
			{
				finalDistance = distanceFromEdge;
			}

			distanceFromEdge = Pos2.y - y;

			if (distanceFromEdge < finalDistance)
			{
				finalDistance = distanceFromEdge;
			}

			distanceFromEdge = x - Pos1.x;

			if (distanceFromEdge < finalDistance)
			{
				finalDistance = distanceFromEdge;
			}

			colour = fullImageHSV.at<Vec3b>(Point(x, y));

			if (finalDistance > 0)
			{
				colour[2] = 0;
				colour[1] = 200 + (turbulence(x, y, 64)) / 4;
			}


			//fullImageHSV.at<Vec3b>(Point(x, y)) = colour;
		}

		if (rand() % 2 && !started)
		{
			started = 1;
			int position = (rand() % (Pos2.y - Pos1.y)) + Pos1.y;
			fullImageHSV.at<Vec3b>(Point(x, position)) = colour;
			//cloud[x][position] = 1;
			top = position;
			bottom = position;
		}

		else if (started)
		{
			if (rand() % 2)
			{
				top++;
			}

			else
			{
				top--;
			}

			if (rand() % 2)
			{
				bottom++;
			}

			else
			{
				bottom--;
			}

			if (top + 20 >= bottom)
			{
				top--;
				bottom++;
			}

			cout << bottom - top << endl;

			for (int y = top; y < bottom; y++)
			{
				colour[2] = 0;
				colour[1] = 200 + (turbulence(x, y, 16)) / 4;
				fullImageHSV.at<Vec3b>(Point(x, y)) = colour;
			}
		}
	}

	cvtColor(fullImageHSV, image, CV_HLS2BGR);
	/*L = 192 + Uint8(turbulence(x, y, 64)) / 3;
	color = HSLtoRGB(ColorHSL(169, 255, L));

	pset(x, y, color);
	}*/

	imwrite("E:\\Pictures\\marine-field-sky-steg.jpg", image);
	imshow("Carrier", image);
	waitKey(0);

	return 0;
}
