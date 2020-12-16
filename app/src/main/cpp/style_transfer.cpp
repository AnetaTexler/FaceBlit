//
// Created by Aneta on 3/3/2019.
//

#include "style_transfer.h"
#include "imgwarp_mls_similarity.h"
#include "imgwarp_mls_rigid.h"
#include "imgwarp_piecewiseaffine.h"
#include <opencv2/core.hpp>
#include "time_helper.h"
#include "window_helper.h"
#include "cartesian_coordinate_system_helper.h"
#include "common_utils.h"
#include "style_cache.h"
//#include <opencv2/face.hpp>
//#include "facemark_detector.h"
#include "dlib_detector.h"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>
#include <queue>
#include <opencv2/imgproc/imgproc.hpp>



int GRID_SIZE = 40;
int MEAN = 128;


// Function called only from JNI
unsigned char* stylize(const char* modelPath, const char* styleLandmarkStr, unsigned char* lookupCubeData, unsigned char* styleData, unsigned char* targetData, int width, int height, bool stylizeFaceOnly)
{
    cv::Mat targetImg = cv::Mat(height, width, CV_8UC4);
    targetImg.data = targetData;
    cvtColor(targetImg, targetImg, cv::COLOR_RGBA2BGR);

    if (StyleCache::getInstance().dlibDetector == nullptr)
    {
        StyleCache::getInstance().dlibDetector = new DlibDetector(std::string(modelPath) + "/shape_predictor_68_face_landmarks.dat"); 
    }

    //facemarkDetector.prepareImage(targetImg, targetImg);
    //cv::resize(targetImg, targetImg, cv::Size(width / 2, height / 2));

    std::pair<cv::Rect, std::vector<cv::Point2i>> detectionResult; // face and its landmarks
	std::vector<cv::Point2i> targetLandmarks;

    TimeMeasure t_landmarks;
    bool success = StyleCache::getInstance().dlibDetector->detectFacemarks(targetImg, detectionResult); // Detect landmarks in target image
    Log_i("FACEBLIT", "Landmarks time: " + std::to_string(t_landmarks.elapsed_milliseconds()) + " ms");

	if (success)
		targetLandmarks = detectionResult.second; 
	else
		return NULL;


    if (styleLandmarkStr != NULL)
	{ 
	    StyleCache::getInstance().styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr);
	}

	if (styleData != NULL)
	{
	    cv::Mat styleImg = cv::Mat(height, width, CV_8UC4);
    	styleImg.data = styleData;
        cvtColor(styleImg, styleImg, cv::COLOR_RGBA2BGR);
        styleImg.copyTo(StyleCache::getInstance().styleImg);
        cv::imwrite("/storage/emulated/0/Download/models/styleImg.png", styleImg);
	}

	TimeMeasure t_stylePosGuide;
	if (StyleCache::getInstance().stylePosGuide.empty())
	{
		StyleCache::getInstance().stylePosGuide = getGradient(width, height, false); // style positional guide - G_pos
	}
	Log_i("FACEBLIT", std::string() + "StylePosGuide time: " + std::to_string(t_stylePosGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_targetPosGuide;
	cv::Mat targetPosGuide = MLSDeformation(StyleCache::getInstance().stylePosGuide, StyleCache::getInstance().styleLandmarks, targetLandmarks); // target positional guide - G_pos
	Log_i("FACEBLIT", std::string() + "TargetPosGuide time: " + std::to_string(t_targetPosGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_styleAppGuide;
	if (styleData != NULL)
	{
		StyleCache::getInstance().styleAppGuide = getAppGuide(StyleCache::getInstance().styleImg, true); // style appearance guide - G_app
	}
	Log_i("FACEBLIT", std::string() + "StyleAppGuide time: " + std::to_string(t_styleAppGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_targetAppGuide;
	cv::Mat targetAppGuide = getAppGuide(targetImg, false); // target appearance guide - G_app
	Log_i("FACEBLIT", std::string() + "TargetAppGuide time: " + std::to_string(t_targetAppGuide.elapsed_milliseconds()) + " ms");
	TimeMeasure t_targetAppGuide_hist;
	targetAppGuide = grayHistMatching(targetAppGuide, StyleCache::getInstance().styleAppGuide);
	Log_i("FACEBLIT", std::string() + "TargetAppGuide_hist time: " + std::to_string(t_targetAppGuide_hist.elapsed_milliseconds()) + " ms");


    if (lookupCubeData != NULL)
    {
        int sizes[] = { 256, 256, 256 };
        cv::Mat lookUpCube = cv::Mat(3, sizes, CV_16UC2);
        lookUpCube.data = lookupCubeData;
        lookUpCube.copyTo(StyleCache::getInstance().lookUpCube);
    }

	cv::Rect2i stylizationRangeRect;
	if (stylizeFaceOnly)
	{
		int width = targetLandmarks[16].x - targetLandmarks[0].x;
		int higherY = targetLandmarks[0].y < targetLandmarks[16].y ? targetLandmarks[0].y : targetLandmarks[16].y;
		int height = targetLandmarks[8].y - (higherY - width / 2);
		stylizationRangeRect.x = std::max(targetLandmarks[0].x - (width * 0.25), 0.0);
		stylizationRangeRect.y = std::max((higherY - width / 2) - (height * 0.25), 0.0);
		stylizationRangeRect.width = std::min(width * 1.5, (double)targetImg.cols);
		stylizationRangeRect.height = std::min(height * 1.5, (double)targetImg.rows);
	}
	else
	{
		stylizationRangeRect.x = 0;
		stylizationRangeRect.y = 0;
		stylizationRangeRect.width = targetImg.cols;
		stylizationRangeRect.height = targetImg.rows;
	}

	TimeMeasure t_styleBlit;
	cv::Mat stylizedImg = styleBlit(StyleCache::getInstance().stylePosGuide, targetPosGuide, StyleCache::getInstance().styleAppGuide, targetAppGuide, StyleCache::getInstance().lookUpCube, StyleCache::getInstance().styleImg, stylizationRangeRect);
	Log_i("FACEBLIT", std::string() + "StyleBlit time: " + std::to_string(t_styleBlit.elapsed_milliseconds()) + " ms");


	// SHOW ONLY STYLIZED FACE BLENDED TO TARGET
    if (stylizeFaceOnly)
	{
		TimeMeasure t_getFaceMask;
        cv::Mat faceMask = getSkinMask(targetImg, targetLandmarks);
		Log_i("FACEBLIT", std::string() + "GetTargetFaceMask time: " + std::to_string(t_getFaceMask.elapsed_milliseconds()) + " ms");
		TimeMeasure t_alphaMaskBlend;
		cv::Mat alphaBlendResult = alphaBlendFG_BG(stylizedImg, targetImg, faceMask, 25.0f);
		Log_i("FACEBLIT", std::string() + "AlphaBlend time: " + std::to_string(t_alphaMaskBlend.elapsed_milliseconds()) + " ms");

		cvtColor(alphaBlendResult, alphaBlendResult, cv::COLOR_BGR2RGBA);
        return alphaBlendResult.data;
    }

    // Convert from BGR (OpenCV default) to RGBA (Java Bitmap default)
	cvtColor(stylizedImg, stylizedImg, cv::COLOR_BGR2RGBA);
	return stylizedImg.data;
}


std::vector<cv::Point2i> getLandmarkPointsFromString(const char* landmarks)
{
	float x, y;
	std::vector<cv::Point2i> points;
	std::istringstream iss(landmarks);

	std::string firstLine;
	std::getline(iss, firstLine);
	if (firstLine != "68")
	{
		std::istringstream iss2(firstLine);
		iss2 >> x >> y;
		points.push_back(cv::Point2i(x, y));
	}

	while (iss >> x >> y)
		points.push_back(cv::Point2i(x, y));
	
	return points;
}


// scale and translate a target to be aligned with a style (targetImg and targetLandmarks are modified)
void alignTargetToStyle(cv::Mat& targetImg, std::vector<cv::Point2i>& targetLandmarks, const std::vector<cv::Point2i>& styleLandmarks)
{
	cv::Point2i targetLeftEye = CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 36, targetLandmarks.begin() + 41)); // left eye center
	cv::Point2i targetRightEye = CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 42, targetLandmarks.begin() + 47)); // right eye center
	float targetEyesDist = CartesianCoordinateSystem::euclideanDistance(targetLeftEye, targetRightEye);
	
	cv::Point2i styleLeftEye = CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 36, styleLandmarks.begin() + 41));
	cv::Point2i styleRightEye = CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 42, styleLandmarks.begin() + 47));
	float styleEyesDist = CartesianCoordinateSystem::euclideanDistance(styleLeftEye, styleRightEye);
	
	// Estimate the face region using landmarks
	int rx = std::max(0.0f, (float)targetLandmarks[0].x - (targetEyesDist / 2));
	int ry = std::max(0.0f, ((targetLandmarks[24].y + targetLandmarks[19].y) / 2) - targetEyesDist);
	int rw = std::min((float)targetImg.cols, targetLandmarks[16].x - targetLandmarks[0].x + targetEyesDist);
	int rh = std::min((float)targetImg.rows, targetLandmarks[8].y - ((targetLandmarks[24].y + targetLandmarks[19].y) / 2) + (targetEyesDist * 1.5f));
	//cv::rectangle(targetImg, cv::Rect(rx, ry, rw, rh), cv::Scalar(0, 0, 255));
	cv::Mat targetFace = cv::Mat(targetImg, cv::Rect(rx, ry, rw, rh));

	// SCALE
	float scaleRatio = styleEyesDist / targetEyesDist;
	float targetFaceRatio = (float)targetFace.rows / targetFace.cols;

	int newWidth = round(targetFace.cols * scaleRatio);
	int newHeight = round(newWidth * targetFaceRatio);
	cv::resize(targetFace, targetFace, cv::Size(newWidth, newHeight));

	targetLandmarks = CartesianCoordinateSystem::recomputePointsDueToScale(targetLandmarks, scaleRatio, cv::Point2i(rx, ry), cv::Size(targetImg.cols, targetImg.rows));

	// TRANSLATION
	cv::Point2i translationShift = styleLandmarks[30] - targetLandmarks[30]; // x and y shift of the target's and style's nose tip
	targetLandmarks = CartesianCoordinateSystem::recomputePointsDueToTranslation(targetLandmarks, translationShift, cv::Size(targetImg.cols, targetImg.rows));

	//cv::rectangle(targetImg, cv::Rect(rx + translationShift.x, ry + translationShift.y, newWidth, newHeight), cv::Scalar(0, 255, 0));
	//cv::Mat blackImg = cv::Mat::zeros(targetImg.rows, targetImg.cols, targetImg.type());
	//CartesianCoordinateSystem::drawLandmarks(blackImg, styleLandmarks, cv::Scalar(0,0,255));
	//CartesianCoordinateSystem::drawLandmarks(blackImg, targetLandmarks, cv::Scalar(0, 255, 0), "circles", "TESTS\\frames\\" + std::to_string(i + 1000) + ".png");
	//Window::imgShow("res", blackImg);

	// INSET the face region (scaled and translated) into the target (black) image
	targetImg = cv::Mat::zeros(targetImg.rows, targetImg.cols, targetImg.type());
	cv::Mat destROI(targetImg, cv::Rect(rx + translationShift.x, ry + translationShift.y, newWidth, newHeight));
	targetFace.copyTo(destROI);
	
	//CartesianCoordinateSystem::drawLandmarks(targetImg, targetLandmarks);
	//Window::imgShow("res", targetImg);
}


cv::Mat getGradient(int width, int height, bool drawGrid)
{
	cv::Mat gradientImg(height, width, CV_8UC3);
	float widthNorm = 256.0 / width;
	float heightNorm = 256.0 / height;

	for (int row = 0; row < gradientImg.rows; row++)
	{
		for (int col = 0; col < gradientImg.cols; col++)
		{
			if (drawGrid && ((row > 1 && row % GRID_SIZE == 0) || (col > 1 && col % GRID_SIZE == 0)))
				gradientImg.at<cv::Vec3b>(row, col) = cv::Vec3b(255, 255, 255);
			else
				gradientImg.at<cv::Vec3b>(row, col) = cv::Vec3b(0, row * heightNorm, col * widthNorm); // BGR - increasing red right and green down, no blue
		}
	}

	return gradientImg;
}


cv::Mat MLSDeformation(const cv::Mat& gradientImg, const std::vector<cv::Point2i> styleLandmarks, const std::vector<cv::Point2i> targetLandmarks)
{
	ImgWarp_MLS_Similarity imgTrans = ImgWarp_MLS_Similarity();
	//ImgWarp_MLS *imgTrans = new ImgWarp_MLS_Rigid();
	//ImgWarp_MLS *imgTrans = new ImgWarp_PieceWiseAffine(); ((ImgWarp_PieceWiseAffine *)imgTrans)->backGroundFillAlg = ImgWarp_PieceWiseAffine::BGMLS;

	imgTrans.alpha = 1.0f; // 0.1 - 3.0, default 1.0, step 0.1
	imgTrans.gridSize = GRID_SIZE; // 1 - 20, default 5

	return imgTrans.setAllAndGenerate(gradientImg, styleLandmarks, targetLandmarks, gradientImg.cols, gradientImg.rows, 1);
}


cv::Mat getAppGuide(const cv::Mat& image, bool stretchHist)
{
	// orig_img = blur_orig_img + G_app ==> G_app = orig_img - blur_orig_img
	cv::Mat grayImg, resultImg, blurImg;
	cv::cvtColor(image, grayImg, cv::COLOR_BGR2GRAY);
	cv::pyrDown(grayImg, blurImg);
	cv::pyrDown(blurImg, blurImg);
	cv::pyrDown(blurImg, blurImg);
	
	cv::resize(blurImg, resultImg, cv::Size(grayImg.cols, grayImg.rows), 0, 0, cv::INTER_LINEAR);
	//imwrite("C:\\Users\\Aneta\\Desktop\\intermed_results\\appGuideBlur" + std::to_string(stretchHist) + ".png", resultImg);
	//cv::GaussianBlur(grayImg, resultImg, cv::Size(5, 5), 0, 0, 4);
	int min = 255;
	int max = 0;

	for (int row = 0; row < grayImg.rows; row++)
	{
		for (int col = 0; col < grayImg.cols; col++)
		{
			int diff = grayImg.at<uchar>(row, col) - resultImg.at<uchar>(row, col);
			resultImg.at<uchar>(row, col) = (diff / 2) + MEAN;
			
			if ((int)resultImg.at<uchar>(row, col) < min) 
				min = resultImg.at<uchar>(row, col);
			if ((int)resultImg.at<uchar>(row, col) > max) 
				max = resultImg.at<uchar>(row, col);
		}
	}

	if (!stretchHist) return resultImg;

	// Prevent mean shift
	int x = MIN(min, (255 - max));
	min = x;
	max = 255 - x;

	// Histogram stretching - contrast enhancement
	for (int row = 0; row < grayImg.rows; row++)
	{
		for (int col = 0; col < grayImg.cols; col++)
		{
			resultImg.at<uchar>(row, col) = (((double)resultImg.at<uchar>(row, col) - min) / (max - min)) * 255;
		}
	}

	return resultImg;
}


cv::Mat getSkinMask(const cv::Mat& image, const std::vector<cv::Point2i>& landmarks)
{
	///*
	cv::Mat resultImg = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);

	cv::Point2i faceContourPoints[17];
	std::copy(landmarks.begin(), landmarks.begin() + 17, faceContourPoints);

	// ellipse (circle) - face
	cv::fillConvexPoly(resultImg, faceContourPoints, 17, cv::Scalar(1.0), cv::LINE_AA);
	cv::Point2i center(faceContourPoints[0] + (faceContourPoints[16] - faceContourPoints[0]) / 2);
	cv::Size axes((faceContourPoints[16].x - faceContourPoints[0].x) / 2, (faceContourPoints[16].x - faceContourPoints[0].x) / 2);
	cv::ellipse(resultImg, center, axes, 0.0, 0.0, 360.0, cv::Scalar(1.0), cv::FILLED, cv::LINE_AA); // TODO: SOLVE ANGLE!!


	return resultImg;
	//*/
	
	/*
	cv::Mat HSV_mask, YCrCb_mask, global_mask;

	cv::cvtColor(image, HSV_mask, cv::COLOR_BGR2HSV); // Converting from RGB to HSV color space
	cv::inRange(HSV_mask, cv::Scalar(0, 15, 0), cv::Scalar(17,170,255), HSV_mask); // Skin color range for HSV color space
	cv::morphologyEx(HSV_mask, HSV_mask, cv::MORPH_OPEN, cv::Mat::ones(cv::Size(3,3), CV_8U));

	cv::cvtColor(image, YCrCb_mask, cv::COLOR_BGR2YCrCb); // Converting from RGB to YCbCr color space
	cv::inRange(YCrCb_mask, cv::Scalar(0, 135, 85), cv::Scalar(255,180,135), YCrCb_mask); // Skin color range for YCbCr color space
	cv::morphologyEx(YCrCb_mask, YCrCb_mask, cv::MORPH_OPEN, cv::Mat::ones(cv::Size(3, 3), CV_8U));

	// Merge skin detection (YCbCr and HSV)
	cv::bitwise_and(YCrCb_mask, HSV_mask, global_mask);
	cv::medianBlur(global_mask, global_mask, 3);
	cv::morphologyEx(global_mask, global_mask, cv::MORPH_OPEN, cv::Mat::ones(cv::Size(4, 4), CV_8U));

	// Cover open mouth, nostrils 
	cv::Point2i faceContourPoints[17];
	std::copy(landmarks.begin(), landmarks.begin() + 17, faceContourPoints);
	// reduce area
	for (int i = 0; i < 7; i++)
		faceContourPoints[i] += cv::Point2i(10, 0);
	for (int i = 7; i < 10; i++)
		faceContourPoints[i] += cv::Point2i(0, -10);
	for (int i = 10; i < 17; i++)
		faceContourPoints[i] += cv::Point2i(-10, 0);
	cv::fillConvexPoly(global_mask, faceContourPoints, 17, cv::Scalar(255, 255, 255), cv::LINE_AA);

	// Cover eyes 
	cv::Point2i leftEyePoints[5];
	leftEyePoints[0] = landmarks[17];
	leftEyePoints[1] = landmarks[19];
	leftEyePoints[2] = landmarks[21];
	leftEyePoints[3] = landmarks[27];
	leftEyePoints[4] = landmarks[48];
	cv::fillConvexPoly(global_mask, leftEyePoints, 5, cv::Scalar(255, 255, 255), cv::LINE_AA);

	cv::Point2i rightEyePoints[5];
	rightEyePoints[0] = landmarks[22];
	rightEyePoints[1] = landmarks[24];
	rightEyePoints[2] = landmarks[26];
	rightEyePoints[3] = landmarks[54];
	rightEyePoints[4] = landmarks[27];
	cv::fillConvexPoly(global_mask, rightEyePoints, 5, cv::Scalar(255, 255, 255), cv::LINE_AA);

	// Cover eyebrows
	for (int i = 18; i < 25; i++)
		cv::line(global_mask, landmarks[i], landmarks[i+1], cv::Scalar(255, 255, 255), 30, cv::LINE_AA);
	
	// Another details
	cv::Point2i triPoints[3];
	triPoints[0] = landmarks[20];
	triPoints[1] = landmarks[23];
	triPoints[2] = landmarks[30];
	cv::fillConvexPoly(global_mask, triPoints, 3, cv::Scalar(255, 255, 255), cv::LINE_AA);
	triPoints[0] = landmarks[0] + cv::Point2i(10, 0);
	triPoints[1] = landmarks[31];
	triPoints[2] = landmarks[17];
	cv::fillConvexPoly(global_mask, triPoints, 3, cv::Scalar(255, 255, 255), cv::LINE_AA);
	triPoints[0] = landmarks[16] + cv::Point2i(-10, 0);
	triPoints[1] = landmarks[35];
	triPoints[2] = landmarks[26];
	cv::fillConvexPoly(global_mask, triPoints, 3, cv::Scalar(255, 255, 255), cv::LINE_AA);
	triPoints[0] = landmarks[17];
	triPoints[1] = landmarks[18];
	triPoints[2] = landmarks[36];
	cv::fillConvexPoly(global_mask, triPoints, 3, cv::Scalar(255, 255, 255), cv::LINE_AA);
	triPoints[0] = landmarks[25];
	triPoints[1] = landmarks[26];
	triPoints[2] = landmarks[45];
	cv::fillConvexPoly(global_mask, triPoints, 3, cv::Scalar(255, 255, 255), cv::LINE_AA);

	//Window::imgShow("jhgth", global_mask);
	//cv::waitKey(0);

	return global_mask;
	*/
}


cv::Mat alphaBlendFG_BG(cv::Mat foreground, cv::Mat background, cv::Mat alpha, float sigma)
{
	// Convert Mat to float data type
	foreground.convertTo(foreground, CV_32FC3, 1.0 / 255.0);
	background.convertTo(background, CV_32FC3, 1.0 / 255.0);
	if (alpha.type() != CV_32FC3 && alpha.type() != CV_32FC1)							
		alpha.convertTo(alpha, CV_32FC3, 1.0 / 255.0);

	// Set the kernel size specifically. If it is cv::Size(0, 0), it is computed from sigma, so it works, but it is super slow, because the kernel is probably unnecessarily large  
	// GaussianBlur(alpha, alpha, cv::Size(51, 51), sigma);
	// BoxBlur is faster
	blur(alpha, alpha, cv::Size(20, 20)); //Box blur

	if (alpha.channels() != 3)
		cv::cvtColor(alpha, alpha, cv::COLOR_GRAY2BGR);

	multiply(alpha, foreground, foreground); // Multiply the foreground with the alpha matte
	multiply(cv::Scalar::all(1.0) - alpha, background, background); // Multiply the background with ( 1 - alpha )

	// Add the masked foreground and background.
	Mat result = Mat::zeros(foreground.size(), foreground.type());
	add(foreground, background, result);

	result.convertTo(result, CV_8UC3, 255.0);

	return result;
}


cv::Mat grayHistMatching(cv::Mat I, cv::Mat R)
{
	/* Histogram Matching of a gray image with a reference*/
	// accept two images I (input image)  and R (reference image)

	cv::Mat result;     // The Result image

	int L = 256;    // Establish the number of bins
	if (I.channels() != 1)
	{
		//cout << "Please use Gray image" << endl;
		return cv::Mat::zeros(I.size(), CV_8UC1);
	}
	cv::Mat G, S, F; // G is the reference CDF (cumulative distribution function), S the CDF of the equlized given image, F is the map from S->G
	if (R.cols > 1)
	{
		if (R.channels() != 1)
		{
			//cout << "Please use Gray reference" << endl;
			return cv::Mat::zeros(I.size(), CV_8UC1);
		}

		cv::Mat R_hist, Rp_hist; // R_hist - the counts of pixels for each level, Rp_hist is the PDF (probability density function) of each level
		// Set the ranges ( for B,G,R) )
		float range[] = { 0, 256 };
		const float* histRange = { range };

		// calcHist(const Mat* images, int nimages, const int* channels, InputArray mask, OutputArray hist, int dims, const int* histSize, const float** ranges, bool uniform=true, bool accumulate=false )
		cv::calcHist(&R, 1, 0, cv::Mat(), R_hist, 1, &L, &histRange, true, false);
		//Calc PDF of the image
		Rp_hist = R_hist / (I.rows * I.cols);
		//calc G=(L-1) * CDF(p)
		cv::Mat CDF = cumSum(Rp_hist);
		G = (L - 1) * CDF;
		for (int i = 0; i < G.rows; i++)
			G.at<cv::Point2f>(i, 0).x = (float)cvRound(G.at<cv::Point2f>(i, 0).x); // round G
	}
	else
	{
		//else, the given R is the reference PDF
		cv::Mat CDF = cumSum(R);
		G = (L - 1) * CDF;
		for (int i = 0; i < G.rows; i++)
			G.at<cv::Point2f>(i, 0).x = (float)cvRound(G.at<cv::Point2f>(i, 0).x); // round G
	}
	
	cv::Mat S_hist, Sp_hist; // S_hist the counts of pixels for each level, Sp_hist is the PDF of each level
	// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 };
	const float* histRange = { range };

	//calcHist(const Mat* images, int nimages, const int* channels, InputArray mask, OutputArray hist, int dims, const int* histSize, const float** ranges, bool uniform=true, bool accumulate=false )
	cv::calcHist(&I, 1, 0, cv::Mat(), S_hist, 1, &L, &histRange, true, false);

	// Calc PDF of the image
	Sp_hist = S_hist / (I.rows * I.cols);
	//calc s=(L-1) * CDF(p)
	cv::Mat CDF = cumSum(Sp_hist);
	S = (L - 1) * CDF;
	for (int i = 0; i < S.rows; i++)
		S.at<cv::Point2f>(i, 0).x = (float)cvRound(S.at<cv::Point2f>(i, 0).x); // round S

	F = cv::Mat::zeros(S.size(), CV_32F);
	int minIndex = -1;
	double T, min = 100000;
	for (int i = 0; i < S.rows; i++)
	{
		for (int j = 0; j < G.rows; j++)
		{
			T = abs(double(S.at<cv::Point2f>(i, 0).x) - double(G.at<cv::Point2f>(j, 0).x));
			if (T == 0)
			{
				minIndex = j;
				break;
			}
			else
			{
				if (T < min)
				{
					minIndex = j;
					min = T;
				}
			}
		}

		F.at<cv::Point2f>(i, 0).x = (float)minIndex;
		minIndex = -1;
		min = 1000000;
	}

	uchar table[256];
	for (int i = 0; i < 256; i++)
		table[i] = (int)F.at<cv::Point2f>(i, 0).x;

	result = scanImageAndReduceC(I, table);

	return result;
}


cv::Mat cumSum(cv::Mat & src)
{
	cv::Mat result = cv::Mat::zeros(cv::Size(src.cols, src.rows), CV_32FC1);
	for (int i = 0; i < src.rows; ++i)
	{
		for (int j = 0; j < src.cols; ++j)
		{
			if (i == 0)
				result.at<float>(i, j) = src.at<float>(i, j);
			else
				result.at<float>(i, j) = src.at<float>(i, j) + result.at<float>(i - 1, j);
		}
	}

	return result;
}


cv::Mat& scanImageAndReduceC(cv::Mat& image, const unsigned char* const table)
{
	// accept only char type matrices
	CV_Assert(image.depth() != sizeof(uchar));

	int nRows = image.rows;
	int nCols = image.cols * image.channels();

	if (image.isContinuous())
	{
		nCols *= nRows;
		nRows = 1;
	}

	uchar* p;
	for (int i = 0; i < nRows; i++)
	{
		p = image.ptr<uchar>(i);
		for (int j = 0; j < nCols; j++)
		{
			p[j] = table[p[j]];
		}
	}
	return image;
}


cv::Mat getLookUpCube(const cv::Mat& stylePosGuide, const cv::Mat& styleAppGuide, const int lambdaPos, const int lambdaApp)
{
	int sizes[] = { 256, 256, 256 };
	cv::Mat lookUpCube(3, sizes, CV_16UC2); // 16bit unsigned short 0 - 65535 (2 channels - u, v coordinates in style)
	float xNorm = stylePosGuide.cols / 256.0;
	float yNorm = stylePosGuide.rows / 256.0;

	#pragma omp parallel for
	for (int x = 0; x < 256; x++) // pos red
	{
		//std::cout << x << std::endl;
		for (int y = 0; y < 256; y++) // pos green
		{
			const int seedRow = y * yNorm;
			const int seedCol = x * xNorm;
			
			for (int z = 0; z < 256; z++) // app gray
			{
				//if (styleAppGuide.empty()) // ignore app guide
				//{
				//	lookUpCube.at<cv::Vec2w>(x, y, z) = cv::Vec2w(seedCol, seedRow); 
				//	continue;
				//}

				int minError = INT_MAX;
				const int radius = 100; // radius of searching
				for (int row = MAX(0, seedRow - radius); row < MIN(stylePosGuide.rows, seedRow + radius); row++)
				{
					for (int col = MAX(0, seedCol - radius); col < MIN(stylePosGuide.cols, seedCol + radius); col++)
					{
						int error = getError(stylePosGuide.at<cv::Vec3b>(row, col).val[1], stylePosGuide.at<cv::Vec3b>(row, col).val[2], y, x, styleAppGuide.at<uchar>(row, col), z, lambdaPos, lambdaApp);
						if (error < minError)
						{
							minError = error;
							lookUpCube.at<cv::Vec2w>(x, y, z) = cv::Vec2w(col, row);
						}

					}
				}
			}
			/*
			//////// VERSION WITHOUT APPEARANCE ///////////////////////////
			int minError = INT_MAX;
			cv::Vec2w minCoord;
			const int radius = 1280; // radius of searching
			for (int row = MAX(0, seedRow - radius); row < MIN(stylePosGuide.rows, seedRow + radius); row++)
			{
				for (int col = MAX(0, seedCol - radius); col < MIN(stylePosGuide.cols, seedCol + radius); col++)
				{
					int error = getError(stylePosGuide.at<cv::Vec3b>(row, col).val[1], stylePosGuide.at<cv::Vec3b>(row, col).val[2], y, x, 0, 0);
					if (error < minError)
					{
						minError = error;
						minCoord = cv::Vec2w(col, row);
					}

				}
			}

			for (int z = 0; z < 256; z++) // app gray
			{
				lookUpCube.at<cv::Vec2w>(x, y, z) = minCoord;
			}
			///////////////////////////////////////////////////
			*/
		}
	}

	return lookUpCube;
}


void saveLookUpCube(const cv::Mat& lookUpCube, const std::string& path)
{
	std::ofstream ofs(path, std::ios::binary);
	ofs.write((const char*)lookUpCube.data, 256 * 256 * 256 * 2 * (sizeof(ushort) / sizeof(uchar)));

	ofs.close();
}


cv::Mat loadLookUpCube(const std::string& path)
{
	int sizes[] = { 256, 256, 256 };
	cv::Mat lookUpCube(3, sizes, CV_16UC2); // 16bit unsigned short 0 - 65535 (2 channels - u, v coordinates in style)
	
	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail())
	{
		Log_e("FACEBLIT", "Fail to read lookup cube!");
		throw "Fail to read lookup cube!";
	}

	ifs.read((char*)lookUpCube.data, 256 * 256 * 256 * 2 * (sizeof(ushort) / sizeof(uchar)));
	ifs.close();

	return lookUpCube;
}


cv::Mat styleBlit(const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, const cv::Mat& lookUpCube, const cv::Mat& styleImg, 
				  const cv::Rect2i stylizationRangeRect, const int threshold, const int lambdaPos, const int lambdaApp)
{
	int boxSize = 25;
	int width = styleImg.cols;
	int height = styleImg.rows;
	cv::Mat resultImg(height, width, CV_8UC3);
	cv::Mat1i coveredPixels = cv::Mat1i::zeros(height, width); // helper for recording already stylized pixels
	float xNorm = width / 256.0;
	float yNorm = height / 256.0;
	int chunkNumber = 1; // for visualization of chunks
	
	for (int rowT = stylizationRangeRect.y; rowT < stylizationRangeRect.y + stylizationRangeRect.height; rowT++) // RESULT TARGET IMAGE
	{
		for (int colT = stylizationRangeRect.x; colT < stylizationRangeRect.x + stylizationRangeRect.width; colT++)
		{
			if (coveredPixels.at<int>(rowT, colT) == 0) // not stylized pixel yet
			{
				// normalized values of G (rows) and R (cols) channels are coordinates of a corresponding pixel in the original gradient:
				//const cv::Point2i styleSeedPoint(targetPosGuide.at<cv::Vec3b>(rowT, colT).val[2] * xNorm, /*R*/ targetPosGuide.at<cv::Vec3b>(rowT, colT).val[1] * yNorm); /*G*/

				const cv::Point2i styleSeedPoint(lookUpCube.at<cv::Vec2w>(targetPosGuide.at<cv::Vec3b>(rowT, colT).val[2],
																		  targetPosGuide.at<cv::Vec3b>(rowT, colT).val[1],
																		  targetAppGuide.empty() ? 0 : targetAppGuide.at<uchar>(rowT, colT)));

				//BoxSeedGrow(rowT, colT, styleSeedPoint, boxSize, stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, resultImg, styleImg, coveredPixels, chunkNumber, threshold);
				DFSSeedGrow(cv::Point2i(colT, rowT), styleSeedPoint, stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, resultImg, styleImg, coveredPixels, chunkNumber, threshold, lambdaPos, lambdaApp);
				chunkNumber++;
			}
		}
	}
	//visualizeChunks(coveredPixels);
	//chunkStatistics(coveredPixels);

	return resultImg;
}


cv::Mat styleBlit_voting(const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, const cv::Mat& lookUpCube, const cv::Mat& styleImg, 
						 const cv::Rect2i stylizationRangeRect, const int patchsizeNNF, const int threshold, const int lambdaPos, const int lambdaApp)
{
	int boxSize = 25;
	int width = styleImg.cols;
	int height = styleImg.rows;
	//cv::Mat resultImg(height, width, CV_8UC3);
	cv::Mat2i NNF(height, width); // Nearest Neighbour Field
	cv::Mat1i coveredPixels = cv::Mat1i::zeros(height, width); // helper for recording already stylized pixels
	float xNorm = width / 256.0;
	float yNorm = height / 256.0;
	int chunkNumber = 1; // for visualization of chunks

	for (int rowT = stylizationRangeRect.y; rowT < stylizationRangeRect.y + stylizationRangeRect.height; rowT++) // RESULT TARGET IMAGE
	{
		for (int colT = stylizationRangeRect.x; colT < stylizationRangeRect.x + stylizationRangeRect.width; colT++)
		{
			if (coveredPixels.at<int>(rowT, colT) == 0) // not stylized pixel yet
			{
				cv::Point2i styleSeedPoint;
				if (lambdaApp == 0) // Without appearance
				{
					// normalized values of G (rows) and R (cols) channels are coordinates of a corresponding pixel in the original gradient:
					styleSeedPoint = cv::Point2i(targetPosGuide.at<cv::Vec3b>(rowT, colT).val[2] * xNorm, /*R*/ targetPosGuide.at<cv::Vec3b>(rowT, colT).val[1] * yNorm); /*G*/
				}
				else
				{
					styleSeedPoint = cv::Point2i(lookUpCube.at<cv::Vec2w>(targetPosGuide.at<cv::Vec3b>(rowT, colT).val[2],
																		  targetPosGuide.at<cv::Vec3b>(rowT, colT).val[1],
																		  targetAppGuide.at<uchar>(rowT, colT)));
				}

				//boxSeedGrow_voting(rowT, colT, styleSeedPoint, boxSize, stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, resultImg, styleImg, coveredPixels, chunkNumber, threshold);
				DFSSeedGrow_voting(cv::Point2i(colT, rowT), styleSeedPoint, stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, NNF, /*styleImg,*/ coveredPixels, chunkNumber, threshold, lambdaPos, lambdaApp);
				chunkNumber++;
			}
		}
	}

	NNF = denoiseNNF(NNF, patchsizeNNF);

	cv::Mat resultImg = votingOnRGB(styleImg, NNF, patchsizeNNF);
	//cv::Mat resultImg = votingOnNNF(styleImg, NNF, patchsizeNNF);
	
	//visualizeChunks(coveredPixels);
	//chunkStatistics(coveredPixels);

	return resultImg;
}


int getError(int stylePosPixelG, int stylePosPixelR, int targetPosPixelG, int targetPosPixelR, uchar styleAppPixel, uchar targetAppPixel, const int LAMBDA_POS, const int LAMBDA_APP)
{
	//int LAMBDA_POS = 10, LAMBDA_APP = 2; // weights

	// Positional guide
	int posErr = abs(stylePosPixelG - targetPosPixelG); // green
	posErr += abs(stylePosPixelR - targetPosPixelR); // red
	// Appearance guide
	int appErr = abs(styleAppPixel - targetAppPixel);

	int totalError = posErr * LAMBDA_POS + appErr * LAMBDA_APP;

	return totalError;
}

/*
// Grow the seed pixel while the chunk is smaller than given box size OR error < threshold
void boxSeedGrow(int rowT, int colT, cv::Point2i seedPoint, int boxSize, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide,
				 cv::Mat& resultImg, const cv::Mat& styleImg, cv::Mat1i& coveredPixels, int chunkNumber, const int threshold)
{
	//int threshold = 50;

	for (int rowOffset = 0; rowOffset <= boxSize; rowOffset++)
	{
		if (rowT + rowOffset < 0 || seedPoint.y + rowOffset < 0 || rowT + rowOffset > resultImg.rows - 1 || seedPoint.y + rowOffset > resultImg.rows - 1)
			continue;
		for (int colOffset = 0; colOffset <= boxSize; colOffset++)
		{
			if (colT + colOffset < 0 || seedPoint.x + colOffset < 0 || colT + colOffset > resultImg.cols - 1 || seedPoint.x + colOffset > resultImg.cols - 1)
				continue;
			if (coveredPixels.at<int>(rowT + rowOffset, colT + colOffset) == 0) // not stylized pixel yet
			{
				int error = getError(stylePosGuide.at<cv::Vec3b>(seedPoint.y + rowOffset, seedPoint.x + colOffset).val[1],
									 stylePosGuide.at<cv::Vec3b>(seedPoint.y + rowOffset, seedPoint.x + colOffset).val[2],
									 targetPosGuide.at<cv::Vec3b>(rowT + rowOffset, colT + colOffset).val[1],
									 targetPosGuide.at<cv::Vec3b>(rowT + rowOffset, colT + colOffset).val[2],
									 styleAppGuide.empty() ? 0 : styleAppGuide.at<uchar>(seedPoint.y + rowOffset, seedPoint.x + colOffset), 
									 targetAppGuide.empty() ? 0 : targetAppGuide.at<uchar>(rowT + rowOffset, colT + colOffset));
				if (error < threshold || cv::Point2i(rowOffset, colOffset) == cv::Point2i(0, 0))
				{
					resultImg.at<cv::Vec3b>(rowT + rowOffset, colT + colOffset) = styleImg.at<cv::Vec3b>(seedPoint.y + rowOffset, seedPoint.x + colOffset);
					coveredPixels.at<int>(rowT + rowOffset, colT + colOffset) = chunkNumber; // pixel is stylized
				}
			}
		}
	}
}*/


// Grow the seed pixel according to the DFS while error < threshold
void DFSSeedGrow(cv::Point2i targetSeedPoint, cv::Point2i styleSeedPoint, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide,
				 cv::Mat& resultImg, const cv::Mat& styleImg, cv::Mat1i& coveredPixels, int chunkNumber, const int threshold, const int lambdaPos, const int lambdaApp)
{
	//int threshold = 50;

	std::queue<cv::Point2i> offsetQueue;
	offsetQueue.push(cv::Point2i(0, 0));

	while (!offsetQueue.empty())
	{
		cv::Point2i offset = offsetQueue.front();
		offsetQueue.pop();

		//if (PixelOutOfImageRange(targetSeedPoint + offset, targetPosGuide) || PixelOutOfImageRange(styleSeedPoint + offset, stylePosGuide))
		//	continue;

		if (pixelIsNotCovered(targetSeedPoint + offset, coveredPixels))
		{
			int error = getError(stylePosGuide.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x).val[1], 
								 stylePosGuide.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x).val[2],
								 targetPosGuide.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x).val[1],
								 targetPosGuide.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x).val[2],
								 styleAppGuide.empty() ? 0 : styleAppGuide.at<uchar>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x),
								 targetAppGuide.empty() ? 0 : targetAppGuide.at<uchar>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x),
								 lambdaPos, lambdaApp);
			if (error < threshold || offset == cv::Point2i(0, 0)) // always stylize seed point (cannot be found better correspondence) OR error < threshold
			{
				resultImg.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x) = styleImg.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x);
				coveredPixels.at<int>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x) = chunkNumber; // pixel is covered
				
				// add neighbours if they are not out of image range and not stylized yet
				cv::Point2i left(offset.x - 1, offset.y);
				cv::Point2i right(offset.x + 1, offset.y);
				cv::Point2i up(offset.x, offset.y - 1);
				cv::Point2i down(offset.x, offset.y + 1);

				if (!pixelOutOfImageRange(targetSeedPoint + left, styleSeedPoint + left, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + left, coveredPixels))
					offsetQueue.push(left);
				if (!pixelOutOfImageRange(targetSeedPoint + right, styleSeedPoint + right, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + right, coveredPixels))
					offsetQueue.push(right);
				if (!pixelOutOfImageRange(targetSeedPoint + up, styleSeedPoint + up, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + up, coveredPixels))
					offsetQueue.push(up);
				if (!pixelOutOfImageRange(targetSeedPoint + down, styleSeedPoint + down, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + down, coveredPixels))
					offsetQueue.push(down);
				// Possible optimization:
				//if (targetSeedPoint.x + offset.x - 1 >= 0 && coveredPixels.at<int>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x - 1) == 0)
				//	offsetQueue.push(cv::Point2i(offset.x - 1, offset.y)); // left
			}
		}
	}
}


class cv_vec2i_hash {
public:
	int operator()(const cv::Vec2i& p) const
	{
		return p[0] * p[1] + p[0] + p[1];
	}
};

cv::Mat2i denoiseNNF(const cv::Mat2i& noisyNNF, const int patchsizeNNF)
{
	cv::Mat2i denoisedNNF = noisyNNF.clone();
	const int halfPatchsize = patchsizeNNF / 2;

	//Iterate throughout the noisyNNF/denoisedNNF
#pragma omp parallel for
	for (int row = 0; row < noisyNNF.rows; row++)
	{
		for (int col = 0; col < noisyNNF.cols; col++)
		{
			std::unordered_map<cv::Vec2i, size_t, cv_vec2i_hash> relativeOffsetCoordsMap;

			//Iterate throughout the patch
			for (int rowOffsetInPatch = -1 * halfPatchsize; rowOffsetInPatch <= halfPatchsize; rowOffsetInPatch++)
			{
				for (int colOffsetInPatch = -1 * halfPatchsize; colOffsetInPatch <= halfPatchsize; colOffsetInPatch++)
				{
					const int rowInNNF = row + rowOffsetInPatch;
					const int colInNNF = col + colOffsetInPatch;

					// Check if it is safe to access NNF at this position
					if (rowInNNF < 0 || rowInNNF >= noisyNNF.rows || colInNNF < 0 || colInNNF >= noisyNNF.cols)
					{
						continue;
					}

					const cv::Vec2i nearest = noisyNNF.at<cv::Vec2i>(rowInNNF, colInNNF);
					relativeOffsetCoordsMap[nearest - cv::Vec2i(rowInNNF, colInNNF)]++;
				}
			}

			// Select most occurent from relativeOffsetCoordsMap
			cv::Vec2i mostOccurentOffsetValue;
			size_t mostOccurentOffsetCount = 0;
			for (auto& e : relativeOffsetCoordsMap)
			{
				if (mostOccurentOffsetCount < e.second)
				{
					mostOccurentOffsetCount = e.second;
					mostOccurentOffsetValue = e.first;
				}
			}

			denoisedNNF.at<cv::Vec2i>(row, col) = mostOccurentOffsetValue + cv::Vec2i(row, col);
		}
	}

	return denoisedNNF;
}


cv::Mat votingOnRGB(const cv::Mat& style, const cv::Mat2i& NNF, const int patchsizeNNF)
{
	cv::Mat output = cv::Mat::zeros(NNF.rows, NNF.cols, CV_8UC3);
	const int halfPatchsize = patchsizeNNF / 2;

	//Iterate throughout the NNF/output image
#pragma omp parallel for
	for (int row = 0; row < output.rows; row++)
	{
		for (int col = 0; col < output.cols; col++)
		{
			short accumulatedB = 0;
			short accumulatedG = 0;
			short accumulatedR = 0;
			short acumulatedPixelCount = 0;

			//Iterate throughout the patch
			for (int rowOffsetInPatch = -1 * halfPatchsize; rowOffsetInPatch <= halfPatchsize; rowOffsetInPatch++)
			{
				for (int colOffsetInPatch = -1 * halfPatchsize; colOffsetInPatch <= halfPatchsize; colOffsetInPatch++)
				{
					const int rowInNNF = row + rowOffsetInPatch;
					const int colInNNF = col + colOffsetInPatch;

					if (pixelOutOfImageRange(cv::Point2i(colInNNF, rowInNNF), cv::Point2i(colInNNF, rowInNNF), output))
					{
						continue;
					}

					const cv::Vec2i nearest = NNF.at<cv::Vec2i>(rowInNNF, colInNNF);
					const int rowInStyle = nearest[0] - rowOffsetInPatch;
					const int colInStyle = nearest[1] - colOffsetInPatch;

					if (pixelOutOfImageRange(cv::Point2i(colInStyle, rowInStyle), cv::Point2i(colInStyle, rowInStyle), style))
					{
						continue;
					}

					const cv::Vec3i stylePixel = style.at<cv::Vec3b>(rowInStyle, colInStyle);
					accumulatedB += stylePixel[0];
					accumulatedG += stylePixel[1];
					accumulatedR += stylePixel[2];
					acumulatedPixelCount++;
				}
			}

			output.at<cv::Vec3b>(row, col) = cv::Vec3b((float)accumulatedB / (float)acumulatedPixelCount, (float)accumulatedG / (float)acumulatedPixelCount, (float)accumulatedR / (float)acumulatedPixelCount);
		}
	}

	return output;
}


cv::Mat votingOnNNF(const cv::Mat& style, const cv::Mat2i& NNF, const int patchsizeNNF)
{
	cv::Mat output = cv::Mat::zeros(NNF.rows, NNF.cols, CV_8UC3);
	const int halfPatchsize = patchsizeNNF / 2;

	//Iterate throughout the NNF/output image
#pragma omp parallel for
	for (int row = 0; row < output.rows; row++)
	{
		for (int col = 0; col < output.cols; col++)
		{
			int accumulatedRow = 0;
			int accumulatedCol = 0;
			short acumulatedPixelCount = 0;

			//Iterate throughout the patch
			for (int rowOffsetInPatch = -1 * halfPatchsize; rowOffsetInPatch <= halfPatchsize; rowOffsetInPatch++)
			{
				for (int colOffsetInPatch = -1 * halfPatchsize; colOffsetInPatch <= halfPatchsize; colOffsetInPatch++)
				{
					const int rowInNNF = row + rowOffsetInPatch;
					const int colInNNF = col + colOffsetInPatch;

					// Check if it is safe to access NNF at this position
					if (pixelOutOfImageRange(cv::Point2i(colInNNF, rowInNNF), cv::Point2i(colInNNF, rowInNNF), output))
					{
						continue;
					}

					const cv::Vec2i nearest = NNF.at<cv::Vec2i>(rowInNNF, colInNNF);

					const int rowInStyle = nearest[0] - rowOffsetInPatch;
					const int colInStyle = nearest[1] - colOffsetInPatch;

					accumulatedRow += rowInStyle;
					accumulatedCol += colInStyle;

					acumulatedPixelCount++;
				}
			}

			const int averageRowInStyle = accumulatedRow / acumulatedPixelCount;
			const int averageColInStyle = accumulatedCol / acumulatedPixelCount;


			// Check if it is safe to access style at this position
			if (pixelOutOfImageRange(cv::Point2i(averageColInStyle, averageRowInStyle), cv::Point2i(averageColInStyle, averageRowInStyle), style))
			{
				// Might cause holes in the output, better clamp it to style dimensions. Actually, does this ever happen ???
				continue;
			}
			const cv::Vec3i stylePixel = style.at<cv::Vec3b>(averageRowInStyle, averageColInStyle);

			output.at<cv::Vec3b>(row, col) = stylePixel; 
		}
	}

	return output;
}


// Grow the seed pixel according to the DFS while error < threshold
void DFSSeedGrow_voting(cv::Point2i targetSeedPoint, cv::Point2i styleSeedPoint, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide,
	cv::Mat2i& NNF,/* const cv::Mat& styleImg,*/ cv::Mat1i& coveredPixels, int chunkNumber, const int threshold, const int lambdaPos, const int lambdaApp)
{
	//int threshold = 50;

	std::queue<cv::Point2i> offsetQueue;
	offsetQueue.push(cv::Point2i(0, 0));

	while (!offsetQueue.empty())
	{
		cv::Point2i offset = offsetQueue.front();
		offsetQueue.pop();

		//if (PixelOutOfImageRange(targetSeedPoint + offset, targetPosGuide) || PixelOutOfImageRange(styleSeedPoint + offset, stylePosGuide))
		//	continue;

		if (pixelIsNotCovered(targetSeedPoint + offset, coveredPixels))
		{
			int error = getError(stylePosGuide.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x).val[1],
								 stylePosGuide.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x).val[2],
								 targetPosGuide.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x).val[1],
								 targetPosGuide.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x).val[2],
								 styleAppGuide.empty() ? 0 : styleAppGuide.at<uchar>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x),
								 targetAppGuide.empty() ? 0 : targetAppGuide.at<uchar>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x),
								 lambdaPos, lambdaApp);
			
			if (error < threshold || offset == cv::Point2i(0, 0)) // always stylize seed point (cannot be found better correspondence) OR error < threshold
			{
				//resultImg.at<cv::Vec3b>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x) = styleImg.at<cv::Vec3b>(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x);
				NNF.at<cv::Vec2i>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x) = cv::Vec2i(styleSeedPoint.y + offset.y, styleSeedPoint.x + offset.x);
				coveredPixels.at<int>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x) = chunkNumber; // pixel is covered

				// add neighbours if they are not out of image range and not stylized yet
				cv::Point2i left(offset.x - 1, offset.y);
				cv::Point2i right(offset.x + 1, offset.y);
				cv::Point2i up(offset.x, offset.y - 1);
				cv::Point2i down(offset.x, offset.y + 1);

				if (!pixelOutOfImageRange(targetSeedPoint + left, styleSeedPoint + left, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + left, coveredPixels))
					offsetQueue.push(left);
				if (!pixelOutOfImageRange(targetSeedPoint + right, styleSeedPoint + right, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + right, coveredPixels))
					offsetQueue.push(right);
				if (!pixelOutOfImageRange(targetSeedPoint + up, styleSeedPoint + up, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + up, coveredPixels))
					offsetQueue.push(up);
				if (!pixelOutOfImageRange(targetSeedPoint + down, styleSeedPoint + down, targetPosGuide) && pixelIsNotCovered(targetSeedPoint + down, coveredPixels))
					offsetQueue.push(down);
				// Possible optimization:
				//if (targetSeedPoint.x + offset.x - 1 >= 0 && coveredPixels.at<int>(targetSeedPoint.y + offset.y, targetSeedPoint.x + offset.x - 1) == 0)
				//	offsetQueue.push(cv::Point2i(offset.x - 1, offset.y)); // left
			}
		}
	}
}


bool pixelOutOfImageRange(cv::Point2i stylePixel, cv::Point2i targetPixel, const cv::Mat& image)
{
	return (stylePixel.x < 0 || targetPixel.x < 0 || stylePixel.y < 0 || targetPixel.y < 0 ||
			stylePixel.x > image.cols - 1 || targetPixel.x > image.cols - 1 || stylePixel.y > image.rows - 1 || targetPixel.y > image.rows - 1);
}


bool pixelIsNotCovered(cv::Point2i pixel, const cv::Mat1i& coveredPixels)
{
	return (coveredPixels.at<int>(pixel.y, pixel.x) == 0);
}


void visualizeChunks(const cv::Mat1i& coveredPixels) // Random color for each chunk
{
#ifdef WINDOWS
	cv::Mat result(coveredPixels.rows, coveredPixels.cols, CV_8UC3);
	for (int row = 0; row < coveredPixels.rows; row++)
	{
		for (int col = 0; col < coveredPixels.cols; col++)
		{
			uchar B = (coveredPixels.at<int>(row, col) * 31725) % 256;
			uchar G = (coveredPixels.at<int>(row, col) * 24938) % 256;
			uchar R = (coveredPixels.at<int>(row, col) * 68239) % 256;

			result.at<cv::Vec3b>(row, col) = cv::Vec3b(B, G, R);
		}
	}
	Window::imgShow("Chunks", result);
	//cv::imwrite("C:\\Users\\Aneta\\Desktop\\FFHQ_sub\\chunks.png", result);

	cv::waitKey(1);
#endif
}


void chunkStatistics(const cv::Mat1i& coveredPixels)
{

	std::set<int> acc;
	for (int row = 0; row < coveredPixels.rows; row++)
	{
		for (int col = 0; col < coveredPixels.cols; col++)
		{
			if (row > 230 && col > 130 && row < 850 && col < 550)
				acc.insert(coveredPixels.at<int>(row, col));
		}
	}

	std::cout << "Avg number of pixels per chunk: " << (620 * 420) / (float)acc.size() << std::endl;
}


void visualizeLookUpCube(const cv::Mat& lookUpCube, const std::string dirPath)
{
	cv::Mat cubeLayer(256, 256, CV_8UC3); 

	for (int z = 0; z < 256; z++) // app
	{
		for (int x = 0; x < 256; x++) // pos red
		{
			for (int y = 0; y < 256; y++) // pos green
			{
				cubeLayer.at<cv::Vec3b>(x, y) = cv::Vec3b(0, lookUpCube.at<cv::Vec2w>(x, y, z).val[0] / 3, lookUpCube.at<cv::Vec2w>(x, y, z).val[1] / 4);
			}
		}
		cv::imwrite(dirPath + "\\app" + std::string(3 - std::to_string(z).length(), '0') + std::to_string(z) + ".png", cubeLayer);
	}
}
