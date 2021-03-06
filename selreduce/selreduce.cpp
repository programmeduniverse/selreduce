// selreduce.cpp 
// Selective Gaussian Noise Reduction Using OpenCV
// John R.
// for Dr. Muhammad Abid - Computer Vision, CAP4410.01I&T
// 5/1/2018

#include "stdafx.h"

using namespace cv;
using namespace std;

// Defines the structure in which arguments are passed to the program.
struct ARGS
{
	string in;
	string out;
	int radius;
	int thresh;
	float scale;
	bool verbose;
};

// Lists usage instructions. Appears when the program is run with improper args.
void usage()
{
	cout << "Usage: selreduce <image file> [-o file] [-r #] [-t 0-100] [-s #f] [-v]\n\
selreduce is a tool implementing OpenCV to selectively blur images in low-contrast regions for noise reduction.\n\
<image file> = the filename of the image to effect.\n\
[-o] = the filename of the file to output. Defaults to out_<image file>.ext\n\
[-r] = the radius of the blur to apply. Default value is 5, must be odd\n\
[-t] = the threshold of value difference between pixels required to perform blur in that region. Default value is 50, range 0-100\n\
[-s] = factor to scale the image by. Default is 1 (no scaling applied)\n\
[-v] = enables verbose console output" << endl;
	return;
}

// Handles the population of an ARGS struct from argc/argv passed from main call
ARGS processArgs(int argc, char* argv[])
{
	ARGS theArgs;
	theArgs.in = "";
	theArgs.radius = 5;
	theArgs.thresh = 50;
	theArgs.scale = 1.0f;
	theArgs.verbose = false;

	int currentArg = 1;

	if (argc <= 1)
	{
		usage();
		exit(-1);
	}

	try
	{
		theArgs.in = argv[currentArg];
		currentArg++;
		theArgs.out = "out_" + theArgs.in;

		while (currentArg < argc)
		{
			if ((string)argv[currentArg] == "-o")
			{
				theArgs.out = argv[currentArg + 1];
				currentArg += 2;
			}
			else if ((string)argv[currentArg] == "-r")
			{
				if (stoi(argv[currentArg + 1]) % 2 == 1) theArgs.radius = stoi(argv[currentArg + 1]);
				else throw Exception();
				currentArg += 2;
			}
			else if ((string)argv[currentArg] == "-t")
			{
				if (stoi(argv[currentArg + 1]) >= 0 && stoi(argv[currentArg + 1]) <= 200) theArgs.thresh = stoi(argv[currentArg + 1]);// +155;
				else throw Exception();
				currentArg += 2;
			}
			else if ((string)argv[currentArg] == "-s")
			{
				if (stof(argv[currentArg + 1]) > 0) theArgs.scale = stof(argv[currentArg + 1]);
				else throw Exception();
				currentArg += 2;
			}
			else if ((string)argv[currentArg] == "-v")
			{
				theArgs.verbose = true;
				currentArg += 1;
			}
			else throw Exception();
		}
	}
	catch (...)
	{
		usage();
		exit(-1);
	}



	return theArgs;
}

// Entry point
int main(int argc, char* argv[])
{
	// populate easily-accessed ARGS struct using above method
	ARGS args = processArgs(argc, argv);

	// If verbose logging enabled, show all inputs tab-spaced in console window
	// (anywhere you see "if (args.verbose)", it is simply to log what is happening to the console
	if (args.verbose)
	{
		cout << "Verbose logging enabled.\n\
Image in:\t" + args.in + "\n\
Image out:\t" + args.out + "\n\
Blur radius:\t" + to_string(args.radius) + "\n\
Delta thres.:\t" + to_string(args.thresh) + "\n\
Scale factor:\t" + to_string(args.scale) << endl;
	}

	// read file at specified location and populate an OpenCV Mat object
	if (args.verbose) cout << "Loading image...";
	Mat imageIn = imread(args.in, 1);

	// Did the Mat come back as NULL due to an error from imread()? They probably typed it wrong
	if (imageIn.data == NULL)
	{
		cout << "Image " + args.in + " could not be loaded. Please ensure you typed the name correctly!" << endl;
		return -1;	// error code exit
	}
	if (args.verbose) cout << "done." << endl;

	// Resize the image to the desired factor (this has no effect if scale = 1.0)
	if (args.verbose) cout << "Scaling image...";
	resize(imageIn, imageIn, Size(imageIn.cols * args.scale, imageIn.rows * args.scale), (0.0), (0.0), INTER_LINEAR);
	if (args.verbose) cout << "done." << endl;

	// Clone the input image into a container that will become our mask
	Mat mask = imageIn.clone();

	uchar avgHere, avgLocal;

	// Generate a black-and-white mask based on the difference between surrounding pixels.
	// For each pixel in the original image, check the averaged brightness value of all the valid neighboring pixels
	// if the difference between any of the neighbors and the center pixel exceeds the specified threshold...
	// set the value of the mask at those coordinates to white. Otherwise, leave it black.
	if (args.verbose) cout << "Generating delta mask...";
	for (int i = 0; i < imageIn.rows; i++)
	{
		for (int j = 0; j < imageIn.cols; j++)
		{
			avgHere = (imageIn.at<Vec3b>(Point(j, i)).val[0]
				+ imageIn.at<Vec3b>(Point(j, i)).val[1]
				+ imageIn.at<Vec3b>(Point(j, i)).val[2])
				/ 3;
			mask.at<Vec3b>(Point(j, i)) = Vec3b(0, 0, 0);

			for (int localRows = -1; localRows <= 1; localRows++)
			{
				if (i == 0 && localRows == -1)
					localRows++;
				

				for (int localCols = -1; localCols <= 1; localCols++)
				{
					
					if (j == 0 && localCols == -1)
						localCols++;

					avgLocal = (imageIn.at<Vec3b>(Point(j + localCols, i + localRows)).val[0]
						+ imageIn.at<Vec3b>(Point(j + localCols, i + localRows)).val[1]
						+ imageIn.at<Vec3b>(Point(j + localCols, i + localRows)).val[2])
						/ 3;
					if (abs(avgHere - avgLocal) > args.thresh)
						mask.at<Vec3b>(Point(j, i)) = Vec3b(255, 255, 255);

					
					if (j >= imageIn.cols - 1 && localCols == 0)
						localCols += 1;
				}

				if (i >= imageIn.rows - 1 && localRows == 0)
					localRows += 1;
			}
		}
	}
	if (args.verbose) cout << "done." << endl;

	// Blur the mask to the desired radius
	if (args.verbose) cout << "Blurring...";
	cv::GaussianBlur(mask, mask, Size(args.radius, args.radius), 0);

	Mat blurLayer = imageIn.clone();
	Mat imageOut = imageIn.clone();

	// Blur a copy of the original image by the desired radius
	cv::GaussianBlur(imageIn, blurLayer, Size(args.radius, args.radius), 0);
	if (args.verbose) cout << "done." << endl;

	Vec3b baseValue, blurValue, newValue;
	float maskStrength;
	float difference, change;

	// Invert the mask to endure that it has the correct effect later
	cv::absdiff(mask, 255, mask);

	// For every pixel in the original image...
	// Find the RGB value of that coordinate in both the base image and the blur layer
	// Then find the strength of the mask at that point.
	// Calculate the difference between the base and blur layers, and multiply that difference by the mask strength.
	// The new value of that color channel of that pixel is equal to the base value plus this difference.
	if (args.verbose) cout << "Applying masked blur to image...";
	for (int i = 0; i < imageIn.rows; i++)
	{
		for (int j = 0; j < imageIn.cols; j++)
		{
			baseValue = imageIn.at<Vec3b>(Point(j, i));
			blurValue = blurLayer.at<Vec3b>(Point(j, i));
			maskStrength = mask.at<Vec3b>(Point(j, i)).val[0] / 255.0f;

			for (int i = 0; i < 3; i++)
			{
				difference = blurValue.val[i] - baseValue.val[i];
				change = difference * maskStrength;
				newValue.val[i] = (float)baseValue.val[i] + (difference * maskStrength);
			}

			imageOut.at<Vec3b>(Point(j, i)) = newValue;
		}
	}
	if (args.verbose) cout << "done." << endl;

	// Write out the final product to a desired filename.
	if (args.verbose) cout << "Writing output to " + args.out + "...";
	imwrite(args.out, imageOut);
	if (args.verbose) cout << "done." << endl;

    return 0;	// successful exit!
}