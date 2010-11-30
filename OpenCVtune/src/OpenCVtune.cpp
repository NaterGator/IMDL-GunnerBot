//============================================================================
// Name        : OpenCVtune.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <sstream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "yuv2rgb.h"

using namespace std;
using namespace cv;

extern "C" {
	#include <jpeglib.h>
	#include <string.h>

}

struct hsvLimits {
	    	int *hmin;
	    	int *hmax;
	    	int *lmin;
	    	int *lmax;
	    	int *smin;
	    	int *smax;
	    	int *morpho;
	    	int *morphc;
	    	int *canny;
	    	int *accum;
	    	int *gauss;
	    	int *sigma;
	    	Mat *temp;
};

void updateImages( int i, void *plimits )
{
	struct hsvLimits *limits = (struct hsvLimits *) plimits;
	Mat *temp = limits->temp;
	Mat hsvImg;
	Mat mask;
	Mat wind = temp->clone();
	if(*(limits->canny) < 8 ) *(limits->canny) = 8;
	if(*(limits->accum) < 8 ) *(limits->accum) = 8;
		cvtColor(*temp, hsvImg, CV_RGB2HLS);

		inRange(hsvImg, Scalar(*(limits->hmin), *(limits->lmin), *(limits->smin), 0),
				Scalar(*(limits->hmax), *(limits->lmax), *(limits->smax), 0), mask);
		imshow("hough", mask);
		hsvImg.release();
		morphologyEx(mask, mask, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(*(limits->morphc),*(limits->morphc))));
		imshow("hough", mask);
		morphologyEx(mask, mask, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(*(limits->morpho),*(limits->morpho))));
		imshow("hough", mask);

		GaussianBlur(mask, mask, Size(*(limits->gauss)*2+1, *(limits->gauss)*2+1), *(limits->sigma), *(limits->sigma) );
		imshow("hough", mask);

		vector<Vec3f> circles;
		HoughCircles(	mask,
						circles,
						CV_HOUGH_GRADIENT, 	//method
						2, 					//precision
						mask.rows/2,		//minimum distance
						*(limits->canny),
						*(limits->accum) );


	   for( size_t i = 0; i < circles.size(); i++ )
		{
			 Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			 int radius = cvRound(circles[i][2]);
			 // draw the circle center
			 circle( wind, center, 3, Scalar(0,255,0), -1, 8, 0 );
			 // draw the circle outline
			 circle( wind, center, radius, Scalar(0,0,255), 3, 8, 0 );
		}

		imshow("fullimg", wind);
		wind.release();
}



int main() {
	stringstream imgData (stringstream::in | stringstream::out | stringstream::binary);

		struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
		uint16_t w = 848;
		uint16_t h = 480;


		Mat *temp = new Mat(h, w, CV_8UC3);

	    int s, client, bytes_read;
	    socklen_t opt = sizeof(rem_addr);
	    char buf[1024*16] = { 0 };
	    // allocate socket
	    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	    // bind socket to port 1 of the first available
	    // local bluetooth adapter
	    loc_addr.rc_family = AF_BLUETOOTH;
	    loc_addr.rc_bdaddr = *BDADDR_ANY;
	    loc_addr.rc_channel = (uint8_t) 1;
	    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	    // put socket into listening mode
	    listen(s, 1);

	    // accept one connection
	    client = accept(s, (struct sockaddr *)&rem_addr, &opt);

	    ba2str( &rem_addr.rc_bdaddr, buf );
	    fprintf(stderr, "accepted connection from %s\n", buf);
	    memset(buf, 0, sizeof(buf));
	    char state = 1;

	    int *hmin = new int;
	    int *hmax = new int;
	    int *lmin = new int;
	    int *lmax = new int;
	    int *smin = new int;
	    int *smax = new int;
	    int *morpho = new int;
	    int *morphc = new int;
	    int *canny = new int;
	    int *accum = new int;
	    int *gauss = new int;
	    int *sigma = new int;
	    *hmin=48;//(int) (0.1425*180);
	    *hmax=93;//(int) (0.31*180);
	    *lmin=95;//(int) (0.12*256);
	    *lmax=250;
	    *smin=57;//87;//(int) (0.48*256);
	    *smax=256;
	    *morpho = 10;
	    *morphc = 3;
	    *canny = 225;
	    *accum = 50;
	    *gauss = 4;
	    *sigma = 5;

	    struct hsvLimits limits = { hmin, hmax, lmin, lmax, smin, smax, morpho, morphc, canny, accum, gauss, sigma, temp };

		namedWindow("controls");
		namedWindow("fullimg");
		namedWindow("hough");
		createTrackbar("hmin", "controls", hmin, 181, updateImages, (void *) &limits);
		createTrackbar("hmax", "controls", hmax, 181, updateImages, (void *) &limits);
		createTrackbar("lmin", "controls", lmin, 256, updateImages, (void *) &limits);
		createTrackbar("lmax", "controls", lmax, 256, updateImages, (void *) &limits);
		createTrackbar("smin", "controls", smin, 256, updateImages, (void *) &limits);
		createTrackbar("smax", "controls", smax, 256, updateImages, (void *) &limits);
		createTrackbar("morphopen", "controls", morpho, 20, updateImages, (void *) &limits);
		createTrackbar("morphclose", "controls", morphc, 20, updateImages, (void *) &limits);
		createTrackbar("canny", "controls", canny, 300, updateImages, (void *) &limits);
		createTrackbar("accum", "controls", accum, 300, updateImages, (void *) &limits);
		createTrackbar("gaussian", "controls", gauss, 30, updateImages, (void *) &limits);
		createTrackbar("sigma", "controls", sigma, 30, updateImages, (void *) &limits);

	    while(true) {
	    	switch(state) {
				case 0:
					bytes_read = read(client, buf, sizeof(buf));
					if( bytes_read > 0) {
						fprintf(stderr, "Got bytes: [%s]\n", buf);
						if(strcmp(buf, "hello world") == 0) {
							state = 1;
						}
					}
					break;
				case 1:

					fprintf(stderr, "Asking for a new image.\n");


					fprintf(stderr, "Write = %d", write(client, " $FSTART$getImg$FEND$ ", strlen(" $FSTART$getImg$FEND$ ")));

					state = 2;
					break;
				case 2:
					//should be getting image data
					unsigned int len = 0;
					char *bytes = new char[20];
					read(client, bytes, 19);

					if(strstr(bytes, "Length: ") == NULL) {
						fprintf(stderr, "error on %s", bytes);
						return 2;
					}
					len = atoi(bytes+7);
					fprintf(stderr, "Len = %d\n", len);

					char *pszJpeg = new char[len];

					unsigned int offset = 0;
					while(offset < len) {

						offset += read(client, (pszJpeg+offset), len-offset);
						fprintf(stderr, "  Offset: %d\n", offset);
					}
					fprintf(stderr, "bytes_read = %d\n", offset);




					unsigned char *y;
					unsigned char *u, *v;

					y = (unsigned char *) pszJpeg;
					offset = w*h>>2;
					v = y + w*h;
					u = y + w*h+1;

					yuv420_2_rgb888(temp->ptr(0),
									y,
									u,
									v,
									w,
									h,
									w,
									w>>1,
									w*3,
									yuv2rgb565_table,
									0);



/*
					ofstream outfile ("/home/nate/camjpeg.jpg", ofstream::binary | ofstream::out);
					outfile.write(pszJpeg, len);
					outfile.close();
					delete[] pszJpeg;
					*temp=imread("/home/nate/camjpeg.jpg", 1);*/


					imshow("fullimg", *temp);


					Mat hsvImg, mask, wind;

						wind = temp->clone();
						cvtColor(*temp, hsvImg, CV_RGB2HLS);
						//s far lies somewhere around .1-.2, blankets and wall are .0-.1
						//v lower bound is .67n-.92
						//float h[2] = {0.1,0.3};//{0.134,0.26};//{0.2,0.33};//{0,1};
						//float s[2] = {0.13,0.7};//{0.06,0.455};//{0.04,0.277};
						//float v[2] = {0.73,1};//{0.61,0.94};
					/*	float h[2] = {0.1425,0.31};//{0.134,0.26};//{0.2,0.33};//{0,1}; 37-79?
						float s[2] = {0.12,1};//{0.06,0.455};//{0.04,0.277}; 30-255
						float l[2] = {0.48,1};//{0.61,0.94}; //.48 for S is WAY too high for bright pics, maybe meant L*/
						inRange(hsvImg, Scalar(*hmin, *lmin, *smin, 0),
								Scalar(*hmax, *lmax, *smax, 0), mask);
						imshow("hough", mask);
						hsvImg.release();
						waitKey();
						fprintf(stderr, " Applying morph_close.\n");
						morphologyEx(mask, mask, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(*morphc,*morphc)));
						imshow("hough", mask);
						waitKey();
						fprintf(stderr, " Applying morph_open.\n");
						morphologyEx(mask, mask, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(*morpho,*morpho)));
						imshow("hough", mask);
						waitKey();
						fprintf(stderr, " Applying gauss blur.\n");
						GaussianBlur(mask, mask, Size((*gauss)*2+1, (*gauss)*2+1), *sigma, *sigma );
						imshow("hough", mask);
						waitKey();
						vector<Vec3f> circles;
						HoughCircles(	mask,
										circles,
										CV_HOUGH_GRADIENT, 	//method
										2, 					//precision
										mask.rows/2,		//minimum distance
										*canny,
										*accum );


					   for( size_t i = 0; i < circles.size(); i++ )
						{
							 Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
							 int radius = cvRound(circles[i][2]);
							 // draw the circle center
							 circle( wind, center, 3, Scalar(0,255,0), -1, 8, 0 );
							 // draw the circle outline
							 circle( wind, center, radius, Scalar(0,0,255), 3, 8, 0 );
						}

					   mask.release();
					   imshow("fullimg", wind);
					   wind.release();






					fprintf(stderr, "Got a full image.\n");
					waitKey();
					state = 1;

					imgData.str("");
	    	}

	    }


	    // read data from the client

	    // close connection
	    close(client);
	    close(s);
	    return 0;

}
