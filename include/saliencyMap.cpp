#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unordered_map>
#include<iostream>
#include <boost/algorithm/string.hpp>
#include <opencv2/opencv.hpp>
#include <limits.h>
#include "saliencyMap.hpp"
using namespace std;

const int COLOR_NUM = (1 << NUM_BITS);

double** regionLookupTable;
int labToIntLookupTable[100][255][255] = {{{}}};
double *oldSaliency;
char int2labLookupTable[6620000][3];

/* this part is common used  function */
/**************************************/
/* encode rgb to int */
int rgbToInt(int r, int g, int b){
    int ret = r;
    ret <<= 8;
    ret += g;
    ret <<= 8;
    ret += b;
    return ret;
}

/* decode int to rgb */
void intToRGB(int hashNum, int rgb[3]){
   	rgb[2] = hashNum & 0xFF;
	hashNum >>= 8;
	rgb[1] = hashNum & 0xFF;
	hashNum >>= 8;
	rgb[0] = hashNum;
}

/* decode string to lab */
void intToLAB(int hashNum, char lab[3]){
   	lab[2] = hashNum & 0xFF;
	hashNum >>= 8;
	lab[1] = hashNum & 0xFF;
	hashNum >>= 8;
	lab[0] = hashNum;
}

/* encode lab to string */
int labToInt(char lab[3]){
    if(labToIntLookupTable[lab[0]][lab[1]+128][lab[2]+128])
        return labToIntLookupTable[lab[0]][lab[1]+128][lab[2]+128];
    int ret = lab[0];
	ret <<= 8;
	ret += lab[1];
	ret <<= 8;
	ret += lab[2];
    labToIntLookupTable[lab[0]][lab[1]+128][lab[2]+128] = ret;
	return ret;
}

/* output grey level image */
void fileGreyImage(unsigned char** imageData, int rowSize, int colSize, const char* fileName){
    FILE *fp = fopen(fileName, "wb");
    assert(fp != NULL);
    for(int row = 0 ; row < rowSize ; row++){
        fwrite(imageData[row], sizeof(unsigned char), colSize, fp);
    }
    fclose(fp);
}

void printSaliencyMap(struct region* regionArray, int regionNum, int rowSize, int colSize, const char* fileName){
    unsigned char** saliencyMap = new unsigned char*[rowSize];
    for(int row = 0 ; row < rowSize ; row++){
        saliencyMap[row] = new unsigned char[colSize];
    }
    for(int region = 0 ; region < regionNum ; region++){
        for(int index = 0 ; index < regionArray[region].area; index++){
            int row = regionArray[region].pointArray[index].row;
            int col = regionArray[region].pointArray[index].col;
            saliencyMap[row][col] = regionArray[region].saliency;
        }
    }
    fileGreyImage(saliencyMap, rowSize, colSize, fileName);
}
/* this part is for quantizing the image to numBits */
/***************************************************/

/* update histogram of region */
void updateRGB_map(unordered_map<int, int> &histogram, int r, int g, int b){
    int hashInt = rgbToInt(r, g, b);
    unordered_map<int, int>::iterator it;
    /* if color exist */
    if((it = histogram.find(hashInt)) != histogram.end()){
        it->second ++;
    }
    /* if color appear in the first time */
    else{
        histogram.insert(make_pair(hashInt, 1));
    }
}

int compare(const void* a, const void* b){
    struct rgb* rgb_a = (struct rgb*)a;
    struct rgb* rgb_b = (struct rgb*)b;
    return -(rgb_a->frequency - rgb_b->frequency);
}

void initializeRGBArray(unordered_map<int, int> &histogram, struct rgb* rgbArray){
    int index = 0;
    unordered_map<int, int>::iterator it = histogram.begin();
    /* calculte frequency for each color */
    while(it != histogram.end()){
        int rgb[3];
        intToRGB(it->first, rgb);
        rgbArray[index].r = rgb[0];
        rgbArray[index].g = rgb[1];
        rgbArray[index].b = rgb[2];
        rgbArray[index].frequency = it->second;
        it++;
        index++;
    }

    /* sort by frequency, from high to low */
    qsort(rgbArray, histogram.size(), sizeof(struct rgb), compare);
}

/* reduce color */
int reduceColor(struct rgb* rgb, int rowSize, int colSize){

    /* make sure left color cover COVER_PERCENTAGE of whole image */
    int cover = rowSize * colSize * COVER_PERCENTAGE;
    int count = 0, index = 0;
    while(count < cover){
        count += rgb[index].frequency;
        index++;
    }
    return index;
}

/* calculate rgb distance */
double rgbDistance(struct rgb a, struct rgb b){
    return (a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b);
}

/* map old color to new color */
void makeColorToColorDict(struct rgb* rgbArray, int boundIndex, int colorNum, unordered_map<int, int> &colorToColor){
    /* original color remain the same */
    for(int index = 0 ; index < boundIndex ; index++){
        int colorInt = rgbToInt(rgbArray[index].r, rgbArray[index].g, rgbArray[index].b);
        colorToColor.insert(make_pair(colorInt, colorInt));
    }

    /* old color find closest to fill */
    for(int index = boundIndex; index < colorNum ; index++){
        double minDistance = DBL_MAX;
        int minIndex = 0;
        for(int mapIndex = 0 ; mapIndex < boundIndex ; mapIndex++){
            double distance = rgbDistance(rgbArray[index], rgbArray[mapIndex]);
            if(distance < minDistance){
                minIndex = mapIndex;
                minDistance = distance;
            }
        }
        int outColorInt = rgbToInt(rgbArray[index].r, rgbArray[index].g, rgbArray[index].b);
        int minColorInt = rgbToInt(rgbArray[minIndex].r, rgbArray[minIndex].g, rgbArray[minIndex].b);
        colorToColor.insert(make_pair(outColorInt, minColorInt));
    }
}

void matchColor(int inputRGB[3], int outputRGB[3],unordered_map<int, int> &colorToColor){
    int inputInt = rgbToInt(inputRGB[0], inputRGB[1], inputRGB[2]);
    unordered_map<int, int>::iterator it = colorToColor.find(inputInt);
    if(it == colorToColor.end())
        printf("not found\n");
    int outputInt = it->second;
    intToRGB(outputInt, outputRGB);
}

void reduceImage(cv::Mat& image, unordered_map<int, int> &colorToColor){
    for(int row = 0 ; row < image.rows; row++){
        for(int col = 0 ; col < image.cols; col++){
            int inputRGB[3];
            int outputRGB[3];
            cv::Vec3b valVec = image.at<cv::Vec3b>(row, col);
            inputRGB[0] = valVec[0];
            inputRGB[1] = valVec[1];
            inputRGB[2] = valVec[2];

            /* outputRGB contains new rgb corresponds to old rgb */
            matchColor(inputRGB, outputRGB, colorToColor);
            valVec[0] = (unsigned char)outputRGB[0];
            valVec[1] = (unsigned char)outputRGB[1];
            valVec[2] = (unsigned char)outputRGB[2];
            image.at<cv::Vec3b>(row, col) = valVec;
        }
    }
}

cv::Mat quantizeImage(const cv::Mat& inImage)
{
    int numBits = NUM_BITS;
    unordered_map<int, int> rgbHistogram;
    int colorCount = 0;
    cv::Mat retImage = inImage.clone();

    uchar maskBit = 0xFF;
    // keep numBits as 1 and (8 - numBits) would be all 0 towards the right
    maskBit = maskBit << (8 - numBits);

    for(int j = 0; j < retImage.rows; j++){
        for(int i = 0; i < retImage.cols; i++)
        {
            cv::Vec3b valVec = retImage.at<cv::Vec3b>(j, i);
            valVec[0] = valVec[0] & maskBit;
            valVec[1] = valVec[1] & maskBit;
            valVec[2] = valVec[2] & maskBit;
            retImage.at<cv::Vec3b>(j, i) = valVec;
            updateRGB_map(rgbHistogram, valVec[0], valVec[1], valVec[2]);
        }
    }
    /* find frequency for each color */
    struct rgb* rgbArray = new struct rgb[rgbHistogram.size()];
    initializeRGBArray(rgbHistogram, rgbArray);

    /* reduce color number, and make sure it covers COVER_PERCENTAGES of whole image */
    int boundIndex = reduceColor(rgbArray, retImage.rows, retImage.cols);

    /* map old color to new colow */
    unordered_map<int, int> colorToColor;
    makeColorToColorDict(rgbArray, boundIndex, rgbHistogram.size(), colorToColor);

    /* replace old by new in the image */
    reduceImage(retImage, colorToColor);
    delete[](rgbArray);
#ifdef PRINT_QUANTIZATION
    imshow("quantizstion", retImage);
#endif
    return retImage;
}

/* this part is for implementaing function initializeingRegion */
/***************************************************************/

/* calulate centroid for each region */
void initializeCentroid(struct region* A, int regionCount){
    for(int no = 0 ; no < regionCount ; no++){
        double row = 0;
        double col = 0;
        for(int index = 0 ; index < A[no].area; index++){
            row += A[no].pointArray[index].row;
            col += A[no].pointArray[index].col;
        }
        A[no].centroid.row = row / double(A[no].area);
        A[no].centroid.col = col / double(A[no].area);
    }
}


/* update histogram of region */
void updateLAB_map(unordered_map<int, int> &histogram, char lab[3]){
    int hashInt = labToInt(lab);
    unordered_map<int, int>::iterator it;
    /* if color exist */
    if((it = histogram.find(hashInt)) != histogram.end()){
        it->second ++;
    }
    /* if color appear in the first time */
    else{
        histogram.insert(make_pair(hashInt, 1));
    }
}

/* initialize histogram */
/* this part can perform parell */
void initializeHistogram(struct region* regionArray, int regionCount, char** lab[3], int rowSize, int colSize){
    for(int region = 0; region < regionCount ; region++){
        unordered_map<int, int> working_map;
        for(int pointIndex = 0 ; pointIndex < regionArray[region].area; pointIndex++){
            struct point point = regionArray[region].pointArray[pointIndex];
            char newLAB[3] = {};
            for(int dim = 0 ; dim < 3 ; dim++){
                char bin = lab[point.row][point.col][dim];
                newLAB[dim] = bin;
            }
            updateLAB_map(working_map, newLAB);
        }
        /* assign completed histogram to region */
        regionArray[region].histogram = working_map;
    }
}

/* allocate meomery for single region */
void allocateRegion(struct region* region, int value, int area){
    region->value = value;
	region->area = 0;
    region->saliency = 0;
	region->pointArray = (struct point*)calloc(area, sizeof(struct point));
}

/* allocate meomery for all region */
struct region* allocateRegionArray(unordered_map<int, int> &countMap, unordered_map<int, int> &indexMap){
    struct region* regionArray = (struct region*)calloc(countMap.size(), sizeof(struct region));
    int index = 0;
    /* use unordered_map to query the area of region correspoinding to segmentation value */
    for( unordered_map<int, int>::iterator it = countMap.begin(); it != countMap.end(); it++){
        allocateRegion(&regionArray[index], it->first, it->second);
        /* usr unordered map to map value and index */
        indexMap.insert(make_pair(it->first, index));
        index++;
    }
    return regionArray;
}

void insertPointToRegion(struct region* region, int row, int col){
    region->pointArray[region->area].row = row;
    region->pointArray[region->area].col = col;
    region->area ++;
}

/* map segmentation value to the area of region */
void makeValueToCountDict(int** segmentation, int rowSize, int colSize, unordered_map<int, int> &countMap){
    for(int row = 0 ; row < rowSize; row++){
		for(int col = 0 ; col < colSize; col++){
			int value = segmentation[row][col];
			/* confirm it appears in the first time */
			unordered_map<int, int>::iterator it;
			if((it = countMap.find(value)) != countMap.end())
				it->second ++;
			else{
				countMap.insert(make_pair(value, 1));
            }
		}
	}
}

/* implement function which can output region, including centroid, histogram, area, point array of all region */
struct region* initializeRegion(char*** lab_image, int** segmentation, int rowSize, int colSize, int* regionCount){
    struct region* regionArray;
	unordered_map<int, int> countMap;
    unordered_map<int, int> indexMap;

    makeValueToCountDict(segmentation, rowSize, colSize, countMap);
    /* allocate meomery for region array
     * and make a dic value -> index in regionArray */
    regionArray = allocateRegionArray(countMap, indexMap);

    /* complete the region Array */
    for(int row = 0 ; row < rowSize; row++){
        for(int col = 0 ; col < colSize; col++){
            int value = segmentation[row][col];
            unordered_map<int, int>::iterator it = indexMap.find(value);
            int index = it->second;
            insertPointToRegion(&regionArray[index], row, col);
        }
    }
    /* return how many region there exist */
    *regionCount = countMap.size();

    /* initialize centroid */
    initializeCentroid(regionArray, *regionCount);

    /* initialize histogram */
    initializeHistogram(regionArray, *regionCount, lab_image, rowSize, colSize);
    return regionArray;
}

/* free all region */
void freeRegionArray(struct region* regionArray, int count){
    for(int index = 0 ; index < count; index++){
        free(regionArray[index].pointArray);
    }
    free(regionArray);
}

/* this part is for implementing the function calculate saliency value */
/***********************************************************************/

double pointLabDistance(char a[3], char b[3]){
	double dis1 = a[0] - b[0];
	double dis2 = a[1] - b[1];
	double dis3 = a[2] - b[2];
	return sqrt(dis1 * dis1 + dis2 * dis2 + dis3 * dis3);
}

double** allocateDoubleMatrix(int row, int col){
    double** matrix = new double*[row];
    for(int index = 0 ; index < row ; index++)
        matrix[index] = new double[col];
    for(int i = 0 ; i < row ; i++){
        for(int j = 0 ; j < col; j++)
            matrix[i][j] = 0;
    }
    return matrix;
}

void freeDoubleMatrix(double** matrix, int row){
    for(int index = 0 ; index < row ; index++)
        delete(matrix[index]);
    delete(matrix);
}

/* calculate d_r in formula (7) of paper */
double regionLabDistance(struct region* regionArray, int indexA, int indexB, char*** lab){
    if(regionLookupTable[indexA][indexB])
        return regionLookupTable[indexA][indexB];

    double distance = 0;
	unordered_map<int, int>::iterator itENDA = regionArray[indexA].histogram.end();
	unordered_map<int, int>::iterator itENDB = regionArray[indexB].histogram.end();

	unordered_map<int, int>::iterator itA = regionArray[indexA].histogram.begin();
    while(itA != itENDA){
		char *A_LAB = int2labLookupTable[itA->first];
		double distance2 = 0;

		unordered_map<int, int>::iterator itB = regionArray[indexB].histogram.begin();
         while(itB != itENDB){
			distance2 += itB->second * pointLabDistance(A_LAB, int2labLookupTable[itB->first]);
			itB++;
		}
		distance += distance2 * itA->second;
		itA++;
    }
	distance /= (regionArray[indexA].area * regionArray[indexB].area);
    regionLookupTable[indexA][indexB] = distance;
    regionLookupTable[indexB][indexA] = distance;
    return distance;
}

/* calculate w_s in formula (7) in the paper */
double priorWeight(struct region A, int centerRow, int centerCol){
    double distance = 0;
    for(int index = 0 ; index < A.area; index++){
        distance += pow(( (A.pointArray[index].row - centerRow) * (A.pointArray[index].row-centerRow) / (double)(2*centerRow) / (double)(2*centerRow)
                        + (A.pointArray[index].col - centerCol) * (A.pointArray[index].col-centerCol) / (double)(2*centerCol) / (double)(2*centerCol))
                        , 0.5);
    }
    double average = distance / A.area;
    return exp(-9 * pow(average, 2));
}

/* calculate d_s in formula (7) in the paper */
double centroidDistance(struct float_point A, struct float_point B, int rowSize, int colSize){
    return pow((A.row - B.row)/(double)(rowSize) * (A.row - B.row)/(double)(rowSize)
             + (A.col - B.col)/(double)(colSize) * (A.col - B.col)/(double)(colSize), 0.5);
}

/* normalize saliency value to 0-255 */
void normalizeSaliency(struct region* regionArray, int regionNum){
    double max = 0;
    for(int index = 0 ; index < regionNum ; index++){
        max = max > regionArray[index].saliency ? max : regionArray[index].saliency;
    }
    for(int index = 0 ; index < regionNum ; index++){
        regionArray[index].saliency = regionArray[index].saliency / max * 255;
    }
}

/* initialize array "int2labLookupTable" */
void initialize_int2labLookupTable()
{
	for(int i = 0; i < 6620000; i++)
		intToLAB(i, int2labLookupTable[i]);
}

/* implementing calculating saliency value for each region */
void calculateSaliency(struct region* regionArray, int regionNum, char*** lab,
                         const int rowSize, const int colSize){
    initialize_int2labLookupTable();
    regionLookupTable = allocateDoubleMatrix(regionNum, regionNum);
    for(int index = 0 ; index < regionNum; index++){
        double saliency = 0;
        double w_s = priorWeight(regionArray[index], rowSize/2, colSize/2);

        for(int region = 0 ; region < regionNum ; region++){
            if(region == index)
                continue;
            else{
                double d_s = centroidDistance(regionArray[index].centroid, regionArray[region].centroid, rowSize, colSize);
                double d_r = regionLabDistance(regionArray, index, region, lab);
                double saliencyValue = w_s * exp(d_s/-DELTA_S) * regionArray[region].area * d_r;
                saliency += saliencyValue;
            }
        }
        regionArray[index].saliency = saliency;
    }
    normalizeSaliency(regionArray, regionNum);
    freeDoubleMatrix(regionLookupTable, regionNum);
#ifdef PRINT_REGION_SALIENCY
    printSaliencyMap(regionArray, regionNum, rowSize, colSize, "../img/region_saliency.raw");
#endif
}

/* this part is for color smoothing */
/******************************************/

void updateColorCount(unordered_map<int,int> &colorCount, char* lab){
    int colorInt = labToInt(lab);
    unordered_map<int, int>::iterator it = colorCount.find(colorInt);
    if(it != colorCount.end())
        it->second ++;
    else
        colorCount.insert(make_pair(colorInt, 1));
}

void updateSaliencySum(unordered_map<int, double> &saliencySum, char* lab, double saliency){
    int colorInt = labToInt(lab);
    unordered_map<int, double>::iterator it = saliencySum.find(colorInt);
    if(it !=saliencySum.end())
        it->second += saliency;
    else
        saliencySum.insert(make_pair(colorInt, saliency));
}

void calColorAverage(unordered_map<int ,double> &saliencySum, unordered_map<int, int> &colorCount){
    unordered_map<int, double>::iterator sal_it;
    unordered_map<int, int>::iterator cnt_it;
    int cnt = 0;
    for(sal_it = saliencySum.begin(); sal_it != saliencySum.end(); sal_it++){
        int colorInt = sal_it->first;
        cnt_it = colorCount.find(colorInt);
        sal_it->second = sal_it->second / (double)cnt_it->second;
        cnt++;
    }
}

int labCompare(const void* a, const void* b){
    struct lab_dis* lab_a = (struct lab_dis*)a;
    struct lab_dis* lab_b = (struct lab_dis*)b;
    return lab_a->distance - lab_b->distance;
}

/* sort closest colors in lab space, put them in lab_dis */
void initializeLabDis(struct lab_dis* lab_dis, int colorInt, int colorNum, unordered_map<int, double> &saliencySum){
        unordered_map<int, double>::iterator it = saliencySum.begin();
        int index = 0;
        char lab[3];
        intToLAB(colorInt, lab);
        while(it != saliencySum.end()){
            if(it->first == colorInt){
                it++;
                continue;
            }
            char pickLab[3];
            intToLAB(it->first, pickLab);
            double distance = pointLabDistance(lab, pickLab);
            lab_dis[index].colorInt = it->first;
            lab_dis[index].distance = distance;
            index++;
            it++;
        }
        qsort(lab_dis, colorNum - 1, sizeof(struct lab_dis), labCompare);
}

int max(int a, int b){
    return a > b ? a : b;
}

double calculateT(struct lab_dis* lab_dis, int m){
    double T = 0;
    for(int index = 0 ; index < m ; index++){
        T += lab_dis[index].distance;
    }
    return T;
}

double calculateNewSaliency(struct lab_dis* lab_dis, int m, double T, unordered_map<int, double> &saliencySum){
    double newSaliencyValue = 0;
    for(int index = 0 ; index < m ; index++){
       int colorInt = lab_dis[index].colorInt;
       unordered_map<int, double>::iterator it = saliencySum.find(colorInt);
       newSaliencyValue += (T-lab_dis[index].distance)*(it->second);
    }
    newSaliencyValue *= 1 / ((double)(m-1) * T);
    return newSaliencyValue;
}

/* map color to new saliency */
void makeColorToSaliencyDict(unordered_map<int, double> &saliencySum, unordered_map<int ,double> &colorToSaliency)
{
    unordered_map<int ,double>::iterator colorIndex = saliencySum.begin();
    for(colorIndex = saliencySum.begin(); colorIndex != saliencySum.end(); colorIndex++){
        int colorInt = colorIndex->first, colorNum = saliencySum.size();
        struct lab_dis* lab_dis = new struct lab_dis[colorNum - 1];

        /* find m closest color */
        initializeLabDis(lab_dis, colorInt, colorNum, saliencySum);
        int m = max(colorNum * SMOOTH_RATIO, 2);

        /* calculate new saliency */
        double T = calculateT(lab_dis, m);
        double newSaliencyValue = calculateNewSaliency(lab_dis, m, T, saliencySum);

        /* update color to saliency dict */
        colorToSaliency.insert(make_pair(colorInt, newSaliencyValue));
        delete[](lab_dis);
    }
}

void refineSaliencyValue(struct region* regionArray, int regionNum, unordered_map<int, double> colorToSaliency,
                         char*** lab){
    for(int region = 0 ; region < regionNum ; region++){
        double newValue = 0;
        for(int index = 0 ; index < regionArray[region].area; index++){
            int row = regionArray[region].pointArray[index].row;
            int col = regionArray[region].pointArray[index].col;
            char* labValue = (char*)lab[row][col];
            int colorInt = labToInt(labValue);
            unordered_map<int, double>::iterator it = colorToSaliency.find(colorInt);
            newValue += it->second;
        }
        newValue /= (double)regionArray[region].area;
        /* update saliency and store old saliency in oldSaliency */
        oldSaliency[region] = regionArray[region].saliency;
        regionArray[region].saliency = newValue;
    }
}

/* weighted new and old saliency value */
void balanceSaliency(struct region* regionArray, int regionNum){
    for(int region = 0 ; region < regionNum ; region++){
        regionArray[region].saliency = S_RATIO * regionArray[region].saliency + (1 - S_RATIO) * oldSaliency[region];
    }
}

/* implementint color smooth */
void colorSmooth(struct region* regionArray, int regionNum, char*** lab, int rowSize, int colSize){
    unordered_map<int, int> colorCount;
    unordered_map<int, double> saliencySum;
    for(int region = 0 ; region < regionNum ; region++){
        for(int index = 0 ; index < regionArray[region].area; index++){
            int row = regionArray[region].pointArray[index].row;
            int col = regionArray[region].pointArray[index].col;
            char* labValue = (char*)lab[row][col];
            updateColorCount(colorCount, labValue);
            updateSaliencySum(saliencySum, labValue, regionArray[region].saliency);
        }
    }
    /* calculate old color saliency */
    calColorAverage(saliencySum, colorCount);

    /* map color to new saliency */
    unordered_map<int, double> colorToSaliency;
    makeColorToSaliencyDict(saliencySum, colorToSaliency);

    /* refine saliency value */
    oldSaliency = new double[regionNum];
    refineSaliencyValue(regionArray, regionNum, colorToSaliency, lab);

    /* normalize saliency from 0 to 255 */
    normalizeSaliency(regionArray, regionNum);

#ifdef PRINT_COLOR_SMOOTH
    printSaliencyMap(regionArray, regionNum, rowSize, colSize, "../img/color_smooth.raw");
#endif

    /* balance afrer normalize */
    balanceSaliency(regionArray, regionNum);

#ifdef PRINT_BALANCE
    printSaliencyMap(regionArray, regionNum, rowSize, colSize, "../img/balance.raw");
#endif

    /* print saliency map */
    delete[](oldSaliency);
}

/* this part is for generating trimap */
/***************************************************************/

/* output saliency Map, now doesn't implement trimap initialization */
void initializeTrimap(struct region* regionArray, int regionNum, int** trimap, int rowSize, int colSize){
    for(int region = 0 ; region < regionNum ; region++){
        unsigned char fill = regionArray[region].saliency > Tb ? 1 : 0 ;
        for(int index = 0 ; index < regionArray[region].area ; index++){
            int row = regionArray[region].pointArray[index].row;
            int col = regionArray[region].pointArray[index].col;
            trimap[row][col] = fill;
        }
    }
}
