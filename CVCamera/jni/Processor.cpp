/*
 * Processor.cpp
 *
 *  Created on: Jun 13, 2010
 *      Author: ethan
 */

#include "Processor.h"


#include <sys/stat.h>
#include <android/log.h>
#include <sstream>

using namespace cv;

Processor::Processor() :
			stard(20/*max_size*/, 8/*response_threshold*/,
					15/*line_threshold_projected*/,
					8/*line_threshold_binarized*/, 5/*suppress_nonmax_size*/),
			fastd(20/*threshold*/, true/*nonmax_suppression*/),
			surfd(100./*hessian_threshold*/, 1/*octaves*/, 2/*octave_layers*/)

{

}

Processor::~Processor() {
	// TODO Auto-generated destructor stub
}

void Processor::detectAndDrawFeatures(int input_idx, image_pool* pool,int feature_type) {
	FeatureDetector* fd = 0;

	switch (feature_type) {
	case DETECT_SURF:
		fd = &surfd;
		break;
	case DETECT_FAST:
		detectAndDrawCircles( input_idx, pool );
		return;
		//fd = &fastd;
		//break;
	case DETECT_STAR:
		fd = &stard;
		break;
	}

	Mat greyimage;
	pool->getGrey(input_idx, greyimage);
	//Mat* grayimage = pool->getYUV(input_idx);

	Mat* img = pool->getImage(input_idx);

	if (!img || greyimage.empty() || fd == 0)
		return; //no image at input_idx!


	keypoints.clear();

	//if(grayimage->step1() > sizeof(uchar)) return;
	//cvtColor(*img,*grayimage,CV_RGB2GRAY);


	fd->detect(greyimage, keypoints);

	for (vector<KeyPoint>::const_iterator it = keypoints.begin(); it
			!= keypoints.end(); ++it) {
		circle(*img, it->pt, 3, cvScalar(255, 0, 255, 0));
	}

	//pool->addImage(output_idx,outimage);

}

void Processor::detectAndDrawCircles(int input_idx, image_pool* pool) {
	//input_idx is input index
	Mat hsvImg;
	Mat mask;
	Mat* img = pool->getImage(input_idx);

	cvtColor(*img, hsvImg, CV_RGB2HLS);

/*	inRange(*img, Scalar(0, 0*255, 0*255, 0),
	            Scalar(0*255, 1*255, 0*255, 0),mask);*/
	//bitwise_and(*img, Scalar(0,0,255), *img);

	//Saturation seems to do a good job of removing floor clutter 48-122?
	//Value does both floor and other object clutter removal 190-245?

	//Good H range, stage 1: .14-.26
	//V range (unc) .6-.95

	//s far lies somewhere around .1-.2, blankets and wall are .0-.1
	//v lower bound is .67n-.92
	//float h[2] = {0.1,0.3};//{0.134,0.26};//{0.2,0.33};//{0,1};
	//float s[2] = {0.13,0.7};//{0.06,0.455};//{0.04,0.277};
	//float v[2] = {0.73,1};//{0.61,0.94};
	float h[2] = {0.1425,0.31};//{0.134,0.26};//{0.2,0.33};//{0,1}; 37-79?
	float s[2] = {0.12,1};//{0.06,0.455};//{0.04,0.277}; 30-255
	float l[2] = {0.48,1};//{0.61,0.94}; //.48 for S is WAY too high for bright pics, maybe meant L
	inRange(hsvImg, Scalar(h[0]*180, l[0]*256, s[0]*256, 0),
            Scalar(h[1]*180, l[1]*256, s[1]*256, 0), mask);
	//adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 0);
	//Mat edges;
	//Canny(mask, edges, 300, 240);
	//std::stringstream output;
	//output << "mask.type(): " << mask.type();
	//__android_log_print(ANDROID_LOG_INFO, "processor", output.str().c_str());


	//mask.copyTo(*img);
	hsvImg.release();
	//Mat gray;
//	cvtColor(mask, mask, CV_HSV2BGR);
	//cvtColor(mask, hsvImg, CV_BGR2GRAY);
//	GaussianBlur(mask, mask, Size(3, 3), 2, 2 );
	morphologyEx(mask, mask, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(11,11)));
	morphologyEx(mask, mask, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(11,11)));
	GaussianBlur(mask, mask, Size(9, 9), 2, 2 );
	Mat tmp;
	cvtColor(mask, tmp, CV_GRAY2BGR);

	bitwise_or(*img, tmp, *img);

	vector<Vec3f> circles;
	HoughCircles(	mask,
					circles,
					CV_HOUGH_GRADIENT, 	//method
					2, 					//precision
					mask.rows/2,		//minimum distance
					150,
					75 );
/*	std::stringstream output;
	output << "circles.size(): " << circles.size();
	__android_log_print(ANDROID_LOG_INFO, "processor", output.str().c_str());*/

	//cvtColor(hsvImg, *img, CV_GRAY2RGB);

   for( size_t i = 0; i < circles.size(); i++ )
	{
		 Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
		 int radius = cvRound(circles[i][2]);
		 // draw the circle center
		 circle( *img, center, 3, Scalar(0,255,0), -1, 8, 0 );
		 // draw the circle outline
		 circle( *img, center, radius, Scalar(0,0,255), 3, 8, 0 );
	}

	//keypoints.clear();
/*
	//if(grayimage->step1() > sizeof(uchar)) return;
	//cvtColor(*img,*grayimage,CV_RGB2GRAY);


	fd->detect(greyimage, keypoints);

	for (vector<KeyPoint>::const_iterator it = keypoints.begin(); it
			!= keypoints.end(); ++it) {
		circle(*img, it->pt, 3, cvScalar(255, 0, 255, 0));
	}*/

	//pool->addImage(output_idx,outimage);

}

static double computeReprojectionErrors(
		const vector<vector<Point3f> >& objectPoints, const vector<vector<
				Point2f> >& imagePoints, const vector<Mat>& rvecs,
		const vector<Mat>& tvecs, const Mat& cameraMatrix,
		const Mat& distCoeffs, vector<float>& perViewErrors) {
	vector<Point2f> imagePoints2;
	int i, totalPoints = 0;
	double totalErr = 0, err;
	perViewErrors.resize(objectPoints.size());

	for (i = 0; i < (int) objectPoints.size(); i++) {
		projectPoints(Mat(objectPoints[i]), rvecs[i], tvecs[i], cameraMatrix,
				distCoeffs, imagePoints2);
		err = norm(Mat(imagePoints[i]), Mat(imagePoints2), CV_L1 );
		int n = (int) objectPoints[i].size();
		perViewErrors[i] = err / n;
		totalErr += err;
		totalPoints += n;
	}

	return totalErr / totalPoints;
}

static void calcChessboardCorners(Size boardSize, float squareSize, vector<
		Point3f>& corners) {
	corners.resize(0);

	for (int i = 0; i < boardSize.height; i++)
		for (int j = 0; j < boardSize.width; j++)
			corners.push_back(Point3f(float(j * squareSize), float(i
					* squareSize), 0));
}

/**from opencv/samples/cpp/calibration.cpp
 *
 */
static bool runCalibration(vector<vector<Point2f> > imagePoints,
		Size imageSize, Size boardSize, float squareSize, float aspectRatio,
		int flags, Mat& cameraMatrix, Mat& distCoeffs, vector<Mat>& rvecs,
		vector<Mat>& tvecs, vector<float>& reprojErrs, double& totalAvgErr) {
	cameraMatrix = Mat::eye(3, 3, CV_64F);
	if (flags & CV_CALIB_FIX_ASPECT_RATIO)
		cameraMatrix.at<double> (0, 0) = aspectRatio;

	distCoeffs = Mat::zeros(5, 1, CV_64F);

	vector<vector<Point3f> > objectPoints(1);
	calcChessboardCorners(boardSize, squareSize, objectPoints[0]);
	for (size_t i = 1; i < imagePoints.size(); i++)
		objectPoints.push_back(objectPoints[0]);

	calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
			distCoeffs, rvecs, tvecs, flags);

	bool ok = checkRange(cameraMatrix, CV_CHECK_QUIET ) && checkRange(
			distCoeffs, CV_CHECK_QUIET );

	totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs,
			tvecs, cameraMatrix, distCoeffs, reprojErrs);

	return ok;
}

bool Processor::detectAndDrawChessboard(int idx,image_pool* pool) {

	Mat grey;
	pool->getGrey(idx, grey);
	if (grey.empty())
		return false;
	vector<Point2f> corners;

	IplImage iplgrey = grey;
	if (!cvCheckChessboard(&iplgrey, Size(6, 8)))
		return false;
	bool patternfound = findChessboardCorners(grey, Size(6, 8), corners);

	Mat * img = pool->getImage(idx);

	if (corners.size() < 1)
		return false;

	cornerSubPix(grey, corners, Size(11, 11), Size(-1, -1), TermCriteria(
			CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

	if(patternfound)
		imagepoints.push_back(corners);

	drawChessboardCorners(*img, Size(6, 8), Mat(corners), patternfound);

	imgsize = grey.size();

	return patternfound;

}

void Processor::drawText(int i, image_pool* pool, const char* ctext){
	// Use "y" to show that the baseLine is about
	string text = ctext;
	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	double fontScale = .8;
	int thickness = .5;

	Mat img = *pool->getImage(i);

	int baseline=0;
	Size textSize = getTextSize(text, fontFace,
	                            fontScale, thickness, &baseline);
	baseline += thickness;

	// center the text
	Point textOrg((img.cols - textSize.width)/2,
	              (img.rows - textSize.height *2));

	// draw the box
	rectangle(img, textOrg + Point(0, baseline),
	          textOrg + Point(textSize.width, -textSize.height),
	          Scalar(0,0,255),CV_FILLED);
	// ... and the baseline first
	line(img, textOrg + Point(0, thickness),
	     textOrg + Point(textSize.width, thickness),
	     Scalar(0, 0, 255));

	// then put the text itself
	putText(img, text, textOrg, fontFace, fontScale,
	        Scalar::all(255), thickness, 8);
}
void saveCameraParams(const string& filename, Size imageSize, Size boardSize,
		float squareSize, float aspectRatio, int flags,
		const Mat& cameraMatrix, const Mat& distCoeffs,
		const vector<Mat>& rvecs, const vector<Mat>& tvecs,
		const vector<float>& reprojErrs,
		const vector<vector<Point2f> >& imagePoints, double totalAvgErr) {
	FileStorage fs(filename, FileStorage::WRITE);

	time_t t;
	time(&t);
	struct tm *t2 = localtime(&t);
	char buf[1024];
	strftime(buf, sizeof(buf) - 1, "%c", t2);

	fs << "calibration_time" << buf;

	if (!rvecs.empty() || !reprojErrs.empty())
		fs << "nframes" << (int) std::max(rvecs.size(), reprojErrs.size());
	fs << "image_width" << imageSize.width;
	fs << "image_height" << imageSize.height;
	fs << "board_width" << boardSize.width;
	fs << "board_height" << boardSize.height;
	fs << "squareSize" << squareSize;

	if (flags & CV_CALIB_FIX_ASPECT_RATIO)
		fs << "aspectRatio" << aspectRatio;

	if (flags != 0) {
		sprintf(buf, "flags: %s%s%s%s",
				flags & CV_CALIB_USE_INTRINSIC_GUESS ? "+use_intrinsic_guess"
						: "",
				flags & CV_CALIB_FIX_ASPECT_RATIO ? "+fix_aspectRatio" : "",
				flags & CV_CALIB_FIX_PRINCIPAL_POINT ? "+fix_principal_point"
						: "",
				flags & CV_CALIB_ZERO_TANGENT_DIST ? "+zero_tangent_dist" : "");
		cvWriteComment(*fs, buf, 0);
	}

	fs << "flags" << flags;

	fs << "camera_matrix" << cameraMatrix;
	fs << "distortion_coefficients" << distCoeffs;

	fs << "avg_reprojection_error" << totalAvgErr;
	if (!reprojErrs.empty())
		fs << "per_view_reprojection_errors" << Mat(reprojErrs);

	if (!rvecs.empty() && !tvecs.empty()) {
		Mat bigmat(rvecs.size(), 6, CV_32F);
		for (size_t i = 0; i < rvecs.size(); i++) {
			Mat r = bigmat(Range(i, i + 1), Range(0, 3));
			Mat t = bigmat(Range(i, i + 1), Range(3, 6));
			rvecs[i].copyTo(r);
			tvecs[i].copyTo(t);
		}
		cvWriteComment(
				*fs,
				"a set of 6-tuples (rotation vector + translation vector) for each view",
				0);
		fs << "extrinsic_parameters" << bigmat;
	}

	if (!imagePoints.empty()) {
		Mat imagePtMat(imagePoints.size(), imagePoints[0].size(), CV_32FC2);
		for (size_t i = 0; i < imagePoints.size(); i++) {
			Mat r = imagePtMat.row(i).reshape(2, imagePtMat.cols);
			Mat(imagePoints[i]).copyTo(r);
		}
		fs << "image_points" << imagePtMat;
	}
}
void Processor::resetChess() {

	imagepoints.clear();
}

void Processor::calibrate(const char* filename) {

	vector<Mat> rvecs, tvecs;
	vector<float> reprojErrs;
	double totalAvgErr = 0;
	int flags = 0;
	bool writeExtrinsics = true;
	bool writePoints = true;

	bool ok = runCalibration(imagepoints, imgsize, Size(6, 8), 1.f, 1.f,
			flags, K, distortion, rvecs, tvecs, reprojErrs, totalAvgErr);


	if (ok){

		saveCameraParams(filename, imgsize, Size(6, 8), 1.f,
				1.f, flags, K, distortion, writeExtrinsics ? rvecs
						: vector<Mat> (), writeExtrinsics ? tvecs
						: vector<Mat> (), writeExtrinsics ? reprojErrs
						: vector<float> (), writePoints ? imagepoints : vector<
						vector<Point2f> > (), totalAvgErr);
	}

}

int Processor::getNumberDetectedChessboards() {
	return imagepoints.size();
}
