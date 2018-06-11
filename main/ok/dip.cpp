#include <cstdio>
#include <cstring>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <assert.h>
#include "saliencyMap.hpp"
#define GRABCUT_TIMES 4	//grabcut N times
#define GRABCUT_ITER 4	//the parameter ITER of grabCut

//#define debug 

using namespace cv;
using namespace std;

char tttbuf[1000000];	//used in function : fill_segmentation_matrix
int row, column;
unsigned char ***image;	//RGB
char ***image_lab;	//LAB
int **segmentation;
int ** init_trimap;	/*<--------------------------------------------------------------------need to be revised */

/* part of run segmentation and construct arrays */
void fail(string msg);	/* print fail message */
void run_segmentation(Mat image, char *imagename);	/* run segmentation and generate .csv file */
void mk_csv_filename(char *filename);	/* get the .csv file name to open it */
void build_image_matrix();	/* build 3d matrix for image */
void build_lab_matrix();	/* build 3d matrix for image_lab*/
void build_segmentation_matrix();	/* build 2d matrix for segmentation */
void fill_image_matrix(Mat myimg);	/* write data into matrix "image" */
void fill_image_lab_matrix(Mat lab_img);	/* write data into matrix "image_lab" */
void fill_segmentation_matrix(FILE *fp);	/* write data into matrix "segmentation" */


/* part of grabcut*/
void init_trimap_mask(Mat mask);	//construct initial mask of grabCut
void dilation_and_erosion(Mat mask);	//apply dilation and erosion on mask
void binarize_mask(Mat mask);	//convert pro_background & pro_foreground --> background & foreground
void output_result(Mat mask, Mat img, int times);	//print img when grabcut iteration


/* this function is applied for testing this program, it will be replace by your code */
void initail_for_program_test(Mat a)	/*<-----------------------------------------------------------------need to be deleted */
{
	row = a.rows;
	column = a.cols;

	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			if(i > 1 && j > 1)	init_trimap[i][j] = 1;
			else	init_trimap[i][j] = 0;
		}
	}
}

/* file image */

void fileImage(unsigned char*** image, int rowSize, int colSize, const char* fileName){
    FILE* fp = fopen(fileName, "wb");
    assert(fp!=NULL);
    for(int row = 0 ; row < rowSize ; row++){
        for(int col = 0 ; col < colSize ; col++){
            fwrite(image[row][col], sizeof(unsigned char), 3, fp);
        }
    }
    fclose(fp);
}



unsigned char** buildUcharMatrix(int rowSize, int colSize){
    unsigned char** matrix = new unsigned char*[rowSize];
    for(int row = 0 ; row < rowSize ; row++)
        matrix[row] = new unsigned char[colSize];
    return matrix;
}

void freeUcharMatrix(unsigned char** matrix, int rowSize){
    for(int row = 0 ; row < rowSize ; row++){
        delete(matrix[row]);
    }
    delete(matrix);
}

int** buildIntMatrix(int rowSize, int colSize){
    int** matrix = new int*[rowSize];
    for(int row = 0 ; row < rowSize ; row++)
        matrix[row] = new int[colSize];
    return matrix;
}

void freeIntMatrix(int** matrix, int rowSize){
    for(int row = 0 ; row < rowSize ; row++)
        delete(matrix[row]);
    delete(matrix);
}

int main(int argc, char *argv[])
{
	
    FILE *fp;
	Mat myimg, lab_img; 
    Mat quan_rgb_img;
	Mat bgModel, fgModel;	// for grabCut, though it is useless
	Rect rect;	//for grabCut too
	int i;

	if(argc != 2)	fail("usage : $program image_name\n");

	/* read image */
	myimg = imread(argv[1], CV_LOAD_IMAGE_COLOR);
	if(!myimg.data)	fail("no such image exist\n");

    
	row = myimg.rows;
	column = myimg.cols;
	
    /* build matrix */
	build_image_matrix();
	build_lab_matrix();
	build_segmentation_matrix();

    
	
    /* quantize image */
    quan_rgb_img = quantizeImage(myimg);
	/* fill data in matrix image */
	fill_image_matrix(quan_rgb_img);
	
    /* fill data in matrix iamge_lab */
	cvtColor(quan_rgb_img, lab_img, CV_BGR2Lab);
	fill_image_lab_matrix(lab_img);

	/* fill data in matrix segmentation */
    /* segmentation */
	run_segmentation(myimg, argv[1]);
    sleep(5);
	fp = fopen("./segmentation/output/target.csv", "r");
    assert(fp != NULL);
	fill_segmentation_matrix(fp);
	fclose(fp);

	/* initializeRegion */
    int regionCount = 0;
    int indexCount = 0;

    /* centroid and histogram is calculated in function initializeRegion(...) */
    struct region* regionArray = initializeRegion(image_lab, segmentation, row, column, &regionCount); /* remember to free */
   
    /* calculate saliency value */
    calculateSaliency(regionArray, regionCount, image_lab, row, column);

    colorSmooth(regionArray, regionCount, image_lab, row, column);
//#ifdef GRAB_CUT
    /* this is part of grabcut */

	/* for test this program, it will be replaced by your code*/
	init_trimap = buildIntMatrix(row, column); /* remember to free */
    initializeTrimap(regionArray, regionCount, init_trimap, row, column);

	/* initial mask for first time grabCut */
	Mat grabcut_mask(row, column, CV_8U);
	init_trimap_mask(grabcut_mask);

	/* print origin img for comparing */
//	imshow("origin", myimg);

	for(i = 0; i < GRABCUT_TIMES; i++){
		/* apply dilation and erosion on mask after first grabCut */
		if(i)	dilation_and_erosion(grabcut_mask);
/* print the information of mask */
#ifdef debug
		int j, k;
		int numss[5] = {0, 0, 0, 0, 0};
		for(k = 0; k < myimg.rows; k++){
			for(j = 0; j < myimg.cols; j++){
				numss[(int)grabcut_mask.at<uchar>(k, j)]++;
			}
		}
		printf("after d&e :\n\nback=%d\nfore=%d\npro_back=%d\npro_fore=%d\n\n", numss[0], numss[1], numss[2], numss[3]);
#endif
		/* grabCut, output will be saved in grabcut_mask */
		grabCut(myimg, grabcut_mask, rect, bgModel, fgModel, GRABCUT_ITER, GC_INIT_WITH_MASK);
/* print the information of mask */
#ifdef debug
		int nums[5] = {0, 0, 0, 0, 0};
		for(k = 0; k < myimg.rows; k++){
			for(j = 0; j < myimg.cols; j++){
				nums[(int)grabcut_mask.at<uchar>(k, j)]++;
			}
		}
		printf("after grabcut :\n\nback=%d\nfore=%d\npro_back=%d\npro_fore=%d\n\n", nums[0], nums[1], nums[2], nums[3]);
#endif

		/* let pro_background -> background, pro_foreground -> foreground */
		binarize_mask(grabcut_mask);
	
/* print the information of mask */
#ifdef debug
		int numsss[5] = {0, 0, 0, 0, 0};
		for(k = 0; k < myimg.rows; k++){
			for(j = 0; j < myimg.cols; j++){
				numsss[(int)grabcut_mask.at<uchar>(k, j)]++;
			}
		}
		printf("after binarize :\n\nback=%d\nfore=%d\npro_back=%d\npro_fore=%d\n\n", numsss[0], numsss[1], numsss[2], numsss[3]);
#endif

		/* print current img */
		output_result(grabcut_mask, myimg, i);
	}

	/* pause the program for user viewing imgs */
	waitKey(0);
//#endif
	return 0;
}

/* print fail message */

void fail(string msg)
{
	printf("%s", msg.c_str());
	exit(1);
}

/* run segmentation and generate .csv file */
void run_segmentation(Mat image, char *imagename)
{
	char path[100] = "./segmentation/data/target.jpg";
	vector<int> quality;
	quality.push_back(CV_IMWRITE_JPEG_QUALITY);
	quality.push_back(100);
	imwrite(path, image, quality);
	pid_t pid;
	if((pid = fork()) < 0)	fail("fail to fork");
	else if(pid == 0){
		if(execl("./segmentation/bin/refh_cli", "./segmentation/bin/refh_cli", "./segmentation/data/", "./segmentation/output/", "--threshold", "255", (char *)NULL) < 0)	fail("fail to execv segmentation");
	}

}

/* get the .csv file name to open it */

void mk_csv_filename(char *filename)
{
	int i = -1;
	while(filename[++i] != '\0');
	while(filename[--i] != '.');
	filename[i + 1] = 'c';
	filename[i + 2] = 's';
	filename[i + 3] = 'v';
	filename[i + 4] = '\0';
}

/* build 3d matrix for image */

void build_image_matrix()
{
	int i, j;
	image = new unsigned char **[row];
	for(i = 0; i < row; i++){
		image[i] = new unsigned char *[column];
		for(j = 0; j < column; j++){
			image[i][j] = new unsigned char [3];
		}
	}
}

/* build 3d matrix for image_lab */
void build_lab_matrix()
{
	int i, j;
	image_lab = new char **[row];
	for(i = 0; i < row; i++){
		*(image_lab + i) = new char *[column];
		for(j = 0; j < column; j++){
			*(*(image_lab + i) + j) = new char [3];
		}
	}
}

/* build 2d matrix for segmentation */

void build_segmentation_matrix()
{
	int i;
	segmentation = new int *[row];
	for(i = 0; i < row; i++){
		*(segmentation + i) = new int [column];
	}
}

/* write data into matrix "image" */

void fill_image_matrix(Mat myimg)
{
	int i, j, k;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			for(k = 0; k < 3; k++){
				image[i][j][2 - k] = myimg.at<Vec3b>(i, j)[k];	//RGB vs BGR
			}
		}
	}
}

/* write data into matrix "image_lab" */
void fill_image_lab_matrix(Mat lab_img)
{
	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
				image_lab[i][j][0] = (char)((int)lab_img.at<Vec3b>(i, j)[0] * 100 / 255);
				image_lab[i][j][1] = (char)((int)lab_img.at<Vec3b>(i, j)[1] - 128);
				image_lab[i][j][2] = (char)((int)lab_img.at<Vec3b>(i, j)[2] - 128);
		}
	}
}

/* write data into matrix "segmentation" */

void fill_segmentation_matrix(FILE *fp)
{
	int i, j;
	for(i = 0; i < row; i++){
		fgets(tttbuf, 1000000, fp);
		int cur = 0;
		for(j = 0; j < column; j++){
			int num = 0;
			while(1){
				if(tttbuf[cur] != ',' && tttbuf[cur] != '\n' && tttbuf[cur] != '\0'){
					num *= 10;
					num += tttbuf[cur] - '0';
					cur++;
				}
				else{
					cur++;
					segmentation[i][j] = num;
					break;
				}
			}
		}
	}
}

/* construct initial mask of grabCut */
void init_trimap_mask(Mat mask)
{
	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			if(init_trimap[i][j] == 0)	mask.at<uchar>(i, j) = GC_BGD;	// set to background
			else if(init_trimap[i][j] == 1)	mask.at<uchar>(i, j) = GC_PR_FGD;	//set to pro_foreground
			else	fail("wrong init_trimap");
		}
	}
}

/* apply dilation and erosion on mask */
void dilation_and_erosion(Mat mask)
{
	int DIL_LEN, ERO_LEN;
	int num = 0;
	for(int i = 0; i < row; i++)
		for(int j = 0; j < column; j++)
			if(mask.at<uchar>(i, j) == 1)	num++;
	
	DIL_LEN = 30;
	ERO_LEN = 30; 

	Mat AfterDilate, AfterErode;
	Mat DilateStruct, ErodeStruct;

	DilateStruct = getStructuringElement(MORPH_ELLIPSE, Size(DIL_LEN, DIL_LEN));	//MORPH_ELLIPSE -> circle,  MORPH_RECT -> rectangle
	ErodeStruct = getStructuringElement(MORPH_ELLIPSE, Size(ERO_LEN, ERO_LEN));

	dilate(mask, AfterDilate, DilateStruct);
	erode(mask, AfterErode, ErodeStruct);

	int first = 1;
	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			if(!mask.at<uchar>(i, j) && AfterDilate.at<uchar>(i, j))	// mask=0, dilate=1 -> pro_foreground
				if(first){
					mask.at<uchar>(i, j) = 2;
					first = 0;
				}
				else
					mask.at<uchar>(i, j) = 3;
			else if(mask.at<uchar>(i, j) && !AfterErode.at<uchar>(i, j))	// mask=1, erode=0 -> pro_foreground
				mask.at<uchar>(i, j) = 3;
		}
	}
}	

/* conver pro_background -> background, pro_foreground -> foreground */
void binarize_mask(Mat mask)
{
	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			unsigned char num = mask.at<uchar>(i, j);
			if(num == 0 || num == 2)	mask.at<uchar>(i, j) = 0;
			else	mask.at<uchar>(i, j) = 1;
		}
	}
}

/* print img when grabCut iteration */
void output_result(Mat mask, Mat img, int times)
{
	Mat result_img = img.clone();
	int i, j;
	for(i = 0; i < row; i++){
		for(j = 0; j < column; j++){
			if(mask.at<uchar>(i, j) == 0){
				result_img.at<Vec3b>(i, j)[0] = 0;
				result_img.at<Vec3b>(i, j)[1] = 0;
				result_img.at<Vec3b>(i, j)[2] = 0;
			}
		}
	}
	char msg[10] = " ";
	msg[0] = times + '0';

	imshow(msg, result_img);
	return ;
}

