// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <cstdint>
#include <math.h>
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

struct node
{
	int value;
	struct node *next;
};

#define noiseWidth 10000  
#define noiseHeight 10000

double noise[noiseWidth][noiseHeight];

double smoothNoise(double x, double y);
double turbulence(double x, double y, double size);
void generateNoise();

const int MAXDISTANCE = 150;
const int MINBORDER = 20;
const int MAXBLURDISTANCE = 11;
const int MAXVERTICALCORRECTION = 4;
const int BASECOLOUR = 165;

char *stegFile;

int mode;
int temp;
int lowestSky;
int distanceSky;
int distanceCloud;
int currentByte;
int bitsLeft = 0;
int bitsLeftBackup = 0;
int previousBitsLeft = 0;
int stegPosition = 0;
int stegPositionBackup = 0;
int previousStegPosition = 0;
int fileSize = 0;
int distanceFromEdge;
int finalDistance;
int top, bottom;

char **cloudLayer;

struct Pos Pos1;
struct Pos Pos2;

Mat image;
Mat original;
Mat fullImageHSV;
Mat previousFullImageHSV;

Vec3b sky;
Vec3b cloud;
Vec3b colourTop;
Vec3b colourBottom;
Vec3b previousColourTop;
Vec3b previousColourBottom;

//Function code taken from: http://lodev.org/cgtutor/randomnoise.html
void generateNoise()
{
	for (int x = 0; x < noiseWidth; x++)
		for (int y = 0; y < noiseHeight; y++)
		{
			noise[x][y] = (rand() % 32768) / 32768.0;
		}
}

//Function code taken from: http://lodev.org/cgtutor/randomnoise.html
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

//Function code taken from: http://lodev.org/cgtutor/randomnoise.html
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

void reloadByte()
{
	if (stegPosition > 0)
	{
		currentByte = stegFile[stegPosition - 1];
	}

	else
	{
		currentByte = stegFile[stegPosition];
	}
}

void removeLastCloud()
{
	bitsLeft = previousBitsLeft;
	stegPosition = previousStegPosition;
	bitsLeftBackup = bitsLeft;
	stegPositionBackup = stegPosition;
	reloadByte();

	fullImageHSV = previousFullImageHSV.clone();
	cvtColor(fullImageHSV, fullImageHSV, CV_HLS2BGR);
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
			cout << "Please click on the lowest part of the sky." << endl;
		}

		else if (mode == 1)
		{
			lowestSky = y;
			mode++;

			cout << "Position (" << x << ", " << y << ")" << endl;
			cout << "Please click on a cloud." << endl;
		}

		else if (mode == 2)
		{
			cloud = colour;
			mode++;

			cout << "Colour (BGR): " << (int)colour[0] << " " << (int)colour[1] << " " << (int)colour[2] << endl;
		}

		else if (mode % 2 == 1)
		{
			Pos1.x = x;
			Pos1.y = y;
			cout << "Position (" << x << ", " << y << ")" << endl;
			mode++;
		}

		else if (mode % 2 == 0)
		{
			Pos2.x = x;
			Pos2.y = y;
			cout << "Position (" << x << ", " << y << ")" << endl;
			mode++;
		}
	}

	else if (event == EVENT_RBUTTONDOWN)
	{
		removeLastCloud();
	}
}

int getBit()
{
	if (stegPosition >= fileSize)
	{
		return rand() % 2;
	}

	if (bitsLeft <= 0)
	{
		currentByte = stegFile[stegPosition];
		stegPosition++;
		bitsLeft = 8;
	}

	bitsLeft--;

	return currentByte & (1 << bitsLeft);
}

int main(int argc, char* argv[])
{
	mode = 0;

	printf("Carrier image: %s\n", argv[1]);
	FILE *hidden = fopen(argv[1], "rb");

	if (hidden == 0)
	{
		return -2;
	}

	fseek(hidden, 0L, SEEK_END);
	fileSize = ftell(hidden);
	fseek(hidden, 0L, SEEK_SET);

	stegFile = (char*)malloc(fileSize);
	fread(stegFile, fileSize, 1, hidden);

	image = imread(argv[2]);

	if (image.empty())
	{
		cout << "Couldn't load image" << endl;
		return -1;
	}

	struct node *root;
	struct node *currentNode;
	root = (struct node *) malloc(sizeof(struct node));
	root->value = fileSize;
	currentNode = root;

	namedWindow("Carrier", CV_WINDOW_NORMAL);
	//cvSetWindowProperty("Carrier", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
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

	if ((cloudLayer = (char**)malloc(image.cols*sizeof(char*))) == NULL)
	{
		cout << "Something went wrong";
		return -3;
	}

	for (int i = 0; i < image.cols; i++)
	{
		if ((cloudLayer[i] = (char*)malloc(image.rows)) == NULL)
		{
			cout << "Something went wrong";
			return -3;
		}

		for (int j = 0; j < image.rows; j++)
		{
			cloudLayer[i][j] = 0;
		}
	}

	original = imread(argv[2]);
	cvtColor(original, fullImageHSV, CV_BGR2HLS);

	while (1)
	{
		cout << "bitsLeft: " << bitsLeft << " ";
		cout << "Stegposition: " << stegPosition << " ";

		previousBitsLeft = bitsLeft;
		previousStegPosition = stegPosition;
		previousFullImageHSV = fullImageHSV.clone();

		for (int i = 0; i < image.cols; i++)
		{
			for (int j = 0; j < image.rows; j++)
			{
				cloudLayer[i][j] = 0;
			}
		}

		struct node *nextNode;
		nextNode = (struct node *) malloc(sizeof(struct node));
		nextNode->value = Pos1.x;
		nextNode->next = 0;
		currentNode->next = nextNode;
		currentNode = nextNode;

		nextNode = (struct node *) malloc(sizeof(struct node));
		nextNode->value = Pos1.y;
		nextNode->next = 0;
		currentNode->next = nextNode;
		currentNode = nextNode;

		nextNode = (struct node *) malloc(sizeof(struct node));
		nextNode->value = Pos2.x;
		nextNode->next = 0;
		currentNode->next = nextNode;
		currentNode = nextNode;

		nextNode = (struct node *) malloc(sizeof(struct node));
		nextNode->value = Pos2.y;
		nextNode->next = 0;
		currentNode->next = nextNode;
		currentNode = nextNode;

		int started = 0;
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

		//cvtColor(original, original, CV_BGR2HLS);

		int end = 0;
		int stop = 0;

		for (int x = Pos1.x; x < Pos2.x && !stop; x++)
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

			if (!started && x - Pos1.x > MINBORDER)
			{
				started = 1;
				int position = ((Pos2.y - Pos1.y) / 2) + Pos1.y;//(rand() % (Pos2.y - Pos1.y)) + Pos1.y;
				fullImageHSV.at<Vec3b>(Point(x, position)) = colour;
				//cloud[x][position] = 1;
				top = position;
				bottom = position;
			}

			else if (started)
			{
				if (top + 20 >= bottom && !end)
				{
					top--;
					bottom++;

					//cout << "Not wide enough!";
				}

				if (top - 5 < Pos1.y)
				{
					top = Pos1.y + 5;

					//cout << "Too close to top border!";
				}

				if (bottom + 5 > Pos2.y)
				{
					bottom = Pos2.y - 5;

					//cout << "Too close to bottom border";
				}

				if (Pos2.x - x < (bottom - top) * 2)
				{
					end = 1;
					//cout << "End started!";
				}

				if (end)
				{
					top++;
					bottom--;

					//cout << "Ending this";
				}

				if (bottom <= top)
				{
					stop = 1;
					//cout << "Ended!";
				}

				/*cout << "x: " << x << " top: " << top << " ";
				cout << "ColourTop: " << (int)colourTop[1] << " ";*/

				if (getBit())
				{
					top++;
					cout << "1";
				}

				else
				{
					top--;
					cout << "0";
				}

				if (getBit())
				{
					bottom++;
					cout << "1";
				}

				else
				{
					bottom--;
					cout << "0";
				}

				for (int y = top - 4; y < bottom + 4; y++)
				{
					colour[2] = 0;
					colour[1] = BASECOLOUR + (turbulence(x, y, 64)) / 6;
					fullImageHSV.at<Vec3b>(Point(x, y)) = colour;

					for (int vertical = x - MAXVERTICALCORRECTION; vertical < x + MAXVERTICALCORRECTION; vertical++)
					{
						colour[2] = 0;
						colour[1] = BASECOLOUR + (turbulence(vertical, y, 64)) / 6;
						fullImageHSV.at<Vec3b>(Point(vertical, y)) = colour;
					}

					if (y - top > 4 && bottom - y > 4)
					{
						cloudLayer[x][y] = 1;
					}
				}

				/*colour = fullImageHSV.at<Vec3b>(Point(x, top));
				int luminanceTop = colour[1];
				cout << endl << "LuminanceTop: " << luminanceTop << endl;
				colour = fullImageHSV.at<Vec3b>(Point(x, top - 10));
				int luminanceSky = colour[1];
				cout << endl << "LuminanceSky: " << luminanceSky << endl;

				for (int y = top - 10; y < top; y++)
				{
				colour = fullImageHSV.at<Vec3b>(Point(x, y));
				colour[2] = 0;
				colour[1] = (BASECOLOUR + (turbulence(x, y, 64)) / 6) + 1;
				fullImageHSV.at<Vec3b>(Point(x, y)) = colour;
				}*/

				//cout << "Top: " << top << " ";
			}

			if (x == Pos1.x + 70)
			{
				cvtColor(fullImageHSV, fullImageHSV, CV_HLS2BGR);
				imwrite("E:\\Pictures\\steg-crude-halfway.png", fullImageHSV);
				cvtColor(fullImageHSV, fullImageHSV, CV_BGR2HLS);
			}
		}

		Point topLeft, bottomRight;

		topLeft.x = Pos1.x;
		topLeft.y = Pos1.y;

		bottomRight.x = Pos2.x;
		bottomRight.y = Pos2.y;

		Rect region(topLeft, bottomRight);

		topLeft.x = Pos1.x - 20;
		topLeft.y = Pos1.y - 20;

		bottomRight.x = Pos2.x + 20;
		bottomRight.y = Pos2.y + 20;

		Rect secondRegion(topLeft, bottomRight);

		cvtColor(fullImageHSV, fullImageHSV, CV_HLS2BGR);
		imwrite("E:\\Pictures\\steg-crude.png", fullImageHSV);
		Mat subImage = fullImageHSV(region).clone();
		Mat blurredImage;

		//blur(subImage, blurredImage, Size(7, 7), Point(-1, -1));
		GaussianBlur(subImage, blurredImage, Size(11, 11), 0, 0);
		GaussianBlur(blurredImage, blurredImage, Size(11, 11), 0, 0);

		for (int x = Pos1.x; x < Pos2.x; x++)
		{
			for (int y = Pos1.y; y < Pos2.y; y++)
			{
				int cloudTop = 0;
				colour = fullImageHSV.at<Vec3b>(Point(x, y));

				for (int verticalTest = x - MAXBLURDISTANCE; verticalTest < x + MAXBLURDISTANCE; verticalTest++)
				{
					for (int test = y; test < y + MAXBLURDISTANCE; test++)
					{
						if (cloudLayer[verticalTest][test] == 1)
						{
							cloudTop = 1;
							break;
						}
					}
				}

				int cloudBottom = 0;

				for (int verticalTest = x - MAXBLURDISTANCE; verticalTest < x + MAXBLURDISTANCE; verticalTest++)
				{
					for (int test = y; test > y - MAXBLURDISTANCE; test--)
					{
						if (cloudLayer[verticalTest][test] == 1)
						{
							cloudBottom = 1;
							break;
						}
					}
				}

				int cloudLeft = 0;

				for (int test = x; test < x + MAXBLURDISTANCE; test++)
				{
					if (cloudLayer[test][y] == 1)
					{
						cloudLeft = 1;
						break;
					}
				}

				int cloudRight = 0;

				for (int test = x; test > x - MAXBLURDISTANCE; test--)
				{
					if (cloudLayer[test][y] == 1)
					{
						cloudRight = 1;
						break;
					}
				}

				if (cloudLayer[x][y] == 1)
				{
					//cout << "Using original";
					fullImageHSV.at<Vec3b>(Point(x, y)) = subImage.at<Vec3b>(Point(x - Pos1.x, y - Pos1.y));
				}

				else if (cloudTop || cloudBottom || cloudLeft || cloudRight)
				{
					//cout << "Using blurred version" << endl;
					fullImageHSV.at<Vec3b>(Point(x, y)) = blurredImage.at<Vec3b>(Point(x - Pos1.x, y - Pos1.y));
				}
			}
		}

		//GaussianBlur(fullImageHSV, fullImageHSV, Size(3, 3), 0, 0);

		/*Mat secondSubImage = fullImageHSV(secondRegion).clone();
		GaussianBlur(secondSubImage, blurredImage, Size(1, 1), 0, 0);

		for (int x = topLeft.x; x < bottomRight.x; x++)
		{
		for (int y = topLeft.y; y < bottomRight.y; y++)
		{
		if (cloudLayer[x][y] == 1)
		{
		//cout << "Using original";
		fullImageHSV.at<Vec3b>(Point(x, y)) = secondSubImage.at<Vec3b>(Point(x - topLeft.x, y - topLeft.y));
		}

		else
		{
		//cout << "Using blurred version" << endl;
		fullImageHSV.at<Vec3b>(Point(x, y)) = blurredImage.at<Vec3b>(Point(x - topLeft.x, y - topLeft.y));
		}
		}
		}
		*/
		//blur(subImage, blurredImage, Size(7, 7), Point(-1, -1));
		GaussianBlur(subImage, blurredImage, Size(11, 11), 0, 0);

		for (int x = Pos1.x; x < Pos2.x; x++)
		{
			for (int y = Pos1.y; y < Pos2.y; y++)
			{
				//fullImageHSV.at<Vec3b>(Point(x, y)) = blurredImage.at<Vec3b>(Point(x - Pos1.x, y - Pos1.y));
			}
		}
		//cvtColor(fullImageHSV, image, CV_HLS2BGR);
		/*L = 192 + Uint8(turbulence(x, y, 64)) / 3;
		color = HSLtoRGB(ColorHSL(169, 255, L));

		pset(x, y, color);
		}*/

		//namedWindow("subImage", 1);
		//namedWindow("info", 1);
		//imshow("subImage", blurredImage);
		//imshow("info", subImage);
		imwrite("E:\\Pictures\\steg-blur.png", fullImageHSV);
		cvtColor(fullImageHSV, fullImageHSV, CV_BGR2HLS);
		cout << "Hid " << stegPosition - 1 << " bytes so far." << endl;
		cout << "Key: " << fileSize;

		struct node *currentNode = root;

		do
		{
			currentNode = currentNode->next;
			cout << " " << currentNode->value;
		} while (currentNode->next);

		int previousTop = 0;
		int previousBottom = 0;

		//Correction

		temp = stegPositionBackup;
		stegPositionBackup = stegPosition;
		stegPosition = temp;

		temp = bitsLeftBackup;
		bitsLeftBackup = bitsLeft;
		bitsLeft = temp;
		reloadByte();

		cout << endl;
		cout << endl;
		cout << "bitsLeft: " << bitsLeft << " ";
		cout << "Stegposition: " << stegPosition << " ";

		//cout << "Pos1.x: " << Pos1.x << endl;
		//cout << "Pos2.x: " << Pos2.x << endl;

		started = 0;
		stop = 0;
		int top = 0;
		int bottom = 0;
		end = 0;

		for (int x = Pos1.x; x < Pos2.x && !stop; x++)
		{
			if (!started && x - Pos1.x > MINBORDER)
			{
				started = 1;
				int position = ((Pos2.y - Pos1.y) / 2) + Pos1.y;//(rand() % (Pos2.y - Pos1.y)) + Pos1.y;
				top = position;
				bottom = position;
				previousTop = top;
				previousBottom = bottom;

				previousColourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				previousColourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));
			}

			else if (started)
			{
				if (top + 20 >= bottom && !end)
				{
					top--;
					bottom++;

					//cout << "Not wide enough!";
				}

				if (top - 5 < Pos1.y)
				{
					top = Pos1.y + 5;

					//cout << "Too close to top border!";
				}

				if (bottom + 5 > Pos2.y)
				{
					bottom = Pos2.y - 5;

					//cout << "Too close to bottom border";
				}

				if (Pos2.x - x < (bottom - top) * 2)
				{
					end = 1;
					//cout << "End started!";
				}

				if (end)
				{
					top++;
					bottom--;

					//cout << "Ending this";
				}

				if (bottom <= top)
				{
					stop = 1;
					//cout << "Ended!";
				}

				colourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				//cout << "Top: " << top << " ";
				//cout << "x: " << x << " top: " << top << " ";
				//cout << "ColourTop: " << (int)colourTop[1] << " ";
				//cout << " Turbulence: " << ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6)) << " ";
				//cout << "ColourTop[1]: " << (int)colourTop[1] << " BASECOLOUR + (turbulence(x, top, 64)) / 6): " << (int)(BASECOLOUR + (turbulence(x, top, 64)) / 6) << " ";

				if (getBit())
				{
					colourTop[1] = ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6)) - 1;
					fullImageHSV.at<Vec3b>(Point(x, top)) = colourTop;
					top++;
					cout << "1";
				}

				else
				{
					//cout << " Turbulence: " << ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6)) << " ";
					colourTop[1] = ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6));
					fullImageHSV.at<Vec3b>(Point(x, top)) = colourTop;

					top--;
					cout << "0";
				}

				//cout << endl << "Bottom: " << bottom << endl;
				colourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));

				if (getBit())
				{
					colourBottom[1] = ((int)(BASECOLOUR + (turbulence(x, bottom, 64)) / 6));
					fullImageHSV.at<Vec3b>(Point(x, bottom)) = colourBottom;
					bottom++;
					cout << "1";
				}

				else
				{
					/*if (colourBottom[1] == ((int)(BASECOLOUR + (turbulence(x, bottom, 64)) / 6)))
					{
					colourBottom[1] = ((int)(BASECOLOUR + (turbulence(x, bottom, 64)) / 6)) - 1;
					fullImageHSV.at<Vec3b>(Point(x, bottom)) = colourBottom;
					}
					*/

					colourBottom[1] = ((int)(BASECOLOUR + (turbulence(x, bottom, 64)) / 6)) - 1;
					fullImageHSV.at<Vec3b>(Point(x, bottom)) = colourBottom;

					bottom--;
					cout << "0";
				}

				previousColourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				previousColourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));
			}
		}

		//Decoding

		cout << endl;
		cout << endl;
		cout << "bitsLeft: " << bitsLeft << " ";
		cout << "Stegposition: " << stegPosition << " ";

		started = 0;
		stop = 0;
		previousTop = 0;
		previousBottom = 0;
		top = 0;
		bottom = 0;
		end = 0;

		for (int x = Pos1.x; x < Pos2.x && !stop; x++)
		{
			if (!started && x - Pos1.x > MINBORDER)
			{
				started = 1;
				int position = ((Pos2.y - Pos1.y) / 2) + Pos1.y;//(rand() % (Pos2.y - Pos1.y)) + Pos1.y;
				top = position;
				bottom = position;
				previousTop = top;
				previousBottom = bottom;

				previousColourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				previousColourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));
			}

			else if (started)
			{
				if (top + 20 >= bottom && !end)
				{
					top--;
					bottom++;

					//cout << "Not wide enough!";
				}

				if (top - 5 < Pos1.y)
				{
					top = Pos1.y + 5;

					//cout << "Too close to top border!";
				}

				if (bottom + 5 > Pos2.y)
				{
					bottom = Pos2.y - 5;

					//cout << "Too close to bottom border";
				}

				if (Pos2.x - x < (bottom - top) * 2)
				{
					end = 1;
					//cout << "End started!";
				}

				if (end)
				{
					top++;
					bottom--;

					//cout << "Ending this";
				}

				if (bottom <= top)
				{
					stop = 1;
					//cout << "Ended!";
				}

				colourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				//cout << "x: " << x << " top: " << top << " ";
				//cout << "ColourTop: " << (int)colourTop[1] << " ";
				//cout << " Turbulence: " << ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6)) << " ";

				if (((int)(colourTop[1])) == ((int)(BASECOLOUR + (turbulence(x, top, 64)) / 6)))
				{
					top--;
					cout << "0";
				}

				else
				{
					top++;
					cout << "1";
				}

				//cout << endl << "Bottom: " << bottom << endl;
				colourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));

				if (((int)(colourBottom[1])) == ((int)(BASECOLOUR + (turbulence(x, bottom, 64)) / 6)))
				{
					bottom++;
					cout << "1";
				}

				else
				{
					bottom--;
					cout << "0";
				}

				previousColourTop = fullImageHSV.at<Vec3b>(Point(x, top));
				previousColourBottom = fullImageHSV.at<Vec3b>(Point(x, bottom));
			}
		}

		if (bitsLeft <= 5)
		{
			bitsLeft += 2;
			bitsLeftBackup = bitsLeft;
		}

		else
		{
			stegPosition--;
			stegPositionBackup = stegPosition;

			bitsLeft = -5 + bitsLeft;
			bitsLeftBackup = bitsLeft;
			reloadByte();
		}

		cvtColor(fullImageHSV, fullImageHSV, CV_HLS2BGR);

		cout << "Bits left: " << bitsLeft;
		imwrite("E:\\Pictures\\steg.png", fullImageHSV);
		imshow("Carrier", fullImageHSV);
		waitKey(0);
		cvtColor(fullImageHSV, fullImageHSV, CV_BGR2HLS);
	}

	return 0;
}
