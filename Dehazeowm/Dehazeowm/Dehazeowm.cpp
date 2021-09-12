#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include "math.h"
#include "time.h"

using namespace std;
using namespace cv;
clock_t start, finish;

Mat srcimg, dst1;
Mat dcimg(566,850,CV_8UC1);//750,1000 
float kernRatio = 0.01;//自适应核比例
//Mat dst(srcimg.size(), srcimg.type());
Mat dst = Mat::zeros(dcimg.rows,dcimg.cols, CV_8UC3);
//max value of SRC-image
int getMax(Mat src) {
	int row = src.rows;
	int col = src.cols;
	int temp = 0;
	for (int i = 0; i < row; i++){
		for (int j = 0; j < col; j++) {
			temp = max((int)(src.at<uchar>(i, j)), temp);
		}
		if (temp==255)
		{
			return temp;
		}
	}
	return temp;
}
//图像拉伸
//Mat grayStretch(Mat src, double lowcut, double highcut) {
//	const int bins = 256;
//	int hist_size = bins;
//	float range[] = { 0,255 };
//	const float *ranges[] = { range };
//	MatND desHist;
//	int channels = 0;
//	//计算直方图
//	calcHist(&src, 1,&channels, Mat(), desHist, 1, &hist_size, ranges, true, false);
//}


int main()
{
	srcimg = imread("F:/dehaze/Dehazeown/Hazecity.jpg");

	if (!srcimg.data) {
		printf("could not load image...");
		return -1;
	}
	start = clock();
	char Src_Image[] = "Source Image";
	char Dst_Image[] = "Dehaze Image";
	namedWindow(Src_Image, CV_WINDOW_AUTOSIZE);
	imshow(Src_Image, srcimg);

	//GetBlackchannel image
	Mat dcimg(srcimg.rows, srcimg.cols, CV_8UC1);
	Mat dcmaximg(srcimg.rows, srcimg.cols, CV_8UC1);
	uchar cminval = 255;
	uchar cmaxval = 0;
	double duration;
	//uchar r, g, b, temp1, temp2;
	
	for (int row = 0; row < srcimg.rows; row++) {
		/*uchar cminval = 255;
		uchar cmaxval = 0;*/
		for (int col = 0; col <srcimg.cols; col++) {
			//uchar cminval = 255;
			//uchar cmaxval = 0;
			for (int c = 0; c < srcimg.channels(); c++)
			{	
			/*	cminval = min(cminval, srcimg.at<Vec3b>(row, col)[c]);
				cmaxval = max(cmaxval, srcimg.at<Vec3b>(row, col)[c]);*/
			if (cminval> srcimg.at<Vec3b>(row, col)[c] && srcimg.at<Vec3b>(row, col)[c]>10)
					cminval = srcimg.at<Vec3b>(row, col)[c];
				else cmaxval = srcimg.at<Vec3b>(row, col)[c];
			 }
			dcimg.at<uchar>(row, col) = cminval;
			dcmaximg.at<uchar>(row, col) = cmaxval;
		}
	}

	imshow("Darkchannel image", dcimg);

	//指针方法Mat

	//Mean Filter
	Mat MFdcimg(srcimg.rows, srcimg.cols, CV_8UC1);
	//Mat MF1dcimg(srcimg.rows, srcimg.cols, CV_8UC1);
	double eps;
	/*求取自适应核大小*/
	int ksize = max(3, max((int)(srcimg.cols*kernRatio), (int)(srcimg.rows*kernRatio))); 
	//boxFilter(dcimg, MFdcimg, CV_8UC1, Size(20, 20),Point(-1,-1));
	//medianBlur(dcimg, MFdcimg, 3);
	//blur(dcimg, MFdcimg, Size(ksize, ksize), Point(-1, -1));
	GaussianBlur(dcimg, MFdcimg, Size(5, 5),0,0);//采用5*5的高斯滤波核
	
	//GaussianBlur(dcimg, MF1dcimg, Size(3, 3), 0, 0);
	imshow("MeanFdcimg", MFdcimg);
	//imshow("Darkchannel of srcimage", dcimg);

	//4.Get Mean val of MFimage求均值
	int tempmv = 0;double mean_val_dc = 0;
	for (int i = 0; i < srcimg.rows; i++)
	{
		for (size_t j = 0; j < srcimg.cols; j++)
		{
			tempmv += MFdcimg.at<uchar>(i, j);
		}
	}
	mean_val_dc = tempmv / (srcimg.rows*srcimg.cols ); //
	mean_val_dc = mean_val_dc / 255;
	printf("The meandc img val is %f ", mean_val_dc);
	/*指针方式*/

	//uchar* temp = NULL;
	//for (int i = 0; i < srcimg.rows; i++){
	//	for (int j = 0; j < srcimg.cols; j++)
	//	{
	//		*temp += (int)srcimg.ptr<uchar>(i)[j];
	//	}
	//}
	//mean_val_dc = ((int)temp) / (srcimg.rows*srcimg.cols);
	//printf("The meandc img test val is %f ", mean_val_dc);

	
	//5.环境光L(x),全局大气光A:L(X)=A(1-exp(-rd(x)))
	eps =1.3*  mean_val_dc;//可调变量
	double delta = min(0.9, eps);
	
	Mat L = Mat::zeros(srcimg.rows, srcimg.cols, CV_8UC1);
	for (int i = 0; i < srcimg.rows; i++)
	{
		for (int j = 0; j < srcimg.cols; j++) {
			L.at<uchar>(i, j) = min((int)(delta*MFdcimg.at<uchar>(i, j)), (int)(srcimg.at<uchar>(i, j)));
		}
	}
	imshow("The envirment light ", L);
	//求解去雾图像
	double A;
	int temp = 0;
	//int val = 0;
	A = 0.5*(getMax(dcmaximg) + getMax(MFdcimg));
	for (int i = 0; i < srcimg.rows; i++)
	{
		for (int j = 0; j < srcimg.cols; j++) {
			int temp = L.at<uchar>(i, j);
			for (int k = 0; k < srcimg.channels(); k++)
			{
				int val = (int)(A*(srcimg.at<Vec3b>(i, j)[k] - temp)) / (A - temp);
				if (val > 255) { val = 255; }
				if (val < 0) { val = 0; }
				dst.at<Vec3b>(i, j)[k] = val;
			}
		}
	}
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf("%f seconds\n", duration);
	namedWindow(Dst_Image, CV_WINDOW_AUTOSIZE);
	imshow(Dst_Image, dst);
	imwrite("F:/dehaze/Dehazeown/DeHazecity+Gaustest.jpg", dst);
	waitKey(0);
    return 0;
}


