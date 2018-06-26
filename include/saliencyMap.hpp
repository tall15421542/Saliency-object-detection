#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unordered_map>
#include<iostream>
#include <boost/algorithm/string.hpp>

#ifndef _SALIENCY_H_
#define _SALIENCY_H_

#define BLACK 0
#define WHITE 255

#define NUM_BITS 4
#define BIN_COUNT 12

#define DELTA_S 0.4
#define Tb 70
#define COVER_PERCENTAGE 0.9
#define SMOOTH_RATIO 0.1
#define S_RATIO 0.5

using namespace std;

struct point{
	int row;
	int col;
};

struct float_point{
    double row;
    double col;
};

struct rgb{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int frequency;
};

struct lab_dis{
    int colorInt;
    double distance;
};

struct region{
    int value;
	int area;
    double saliency;
	struct point* pointArray;
    struct float_point centroid;
    unordered_map<int, int> histogram;
};

/* quantizing the image to numBits */
cv::Mat quantizeImage(const cv::Mat& inImage);

/* implement function which can output region, including centroid, histogram, area, point array of all region */
struct region* initializeRegion(char*** lab, int** segmentation, int rowSize, int colSize, int* regionCount);

/* free all region */
void freeRegionArray(struct region* regionArray, int count);

/* implementing calculating saliency value for each region */
void calculateSaliency(struct region* regionArray, int regionNum, char*** lab, const int rowSize, const int colSize);

/* color smooth*/
void colorSmooth(struct region* regionArray, int regionNum, char*** lab, int rowSize, int colSize);

/* output saliency Map, now doesn't implement trimap initialization */
void initializeTrimap(struct region* regionArray, int regionCount, int** trimap, int rowSize, int colSize);


#endif
