//
// Created by Aneta on 08/13/2019.
//


/*

#include "FacemarkDetector.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/face.hpp>

#include <string>
#include <stdexcept>
#include "time_helper.h"
#include "common_utils.h"
#include <opencv2\imgcodecs.hpp>


FacemarkDetector::FacemarkDetector(cv::String faceModelPath, double scaleFactor, int minNeighbors, cv::String landmarkModelPath)
{
	mScaleFactor = scaleFactor;
	mMinNeighbors = minNeighbors;
	if (not mFaceCascadeClf.load(faceModelPath))
        Log_e("HACK", std::string("Face cascade model not loaded"));
		//throw std::invalid_argument("Cannot load cascade classifier from file: " + pFaceModelPath);

	mFacemark = cv::face::FacemarkLBF::create(); // Options: FacemarkLBF, FacemarkKazemi. The same face models, different landmark models!
	mFacemark->loadModel(landmarkModelPath);
}

void FacemarkDetector::prepareImage(const cv::Mat& src, cv::Mat& dst)
{
	if (src.channels() > 1)
		cvtColor(src, dst, cv::COLOR_BGR2GRAY); // The cascade classifier works best on grayscale images
	else
		dst = src.clone();

	cv::equalizeHist(dst, dst); // Histogram equalization generally aids in face detection
}

bool FacemarkDetector::detectFacemarks(const cv::Mat& image, std::pair<cv::Rect, std::vector<cv::Point2i>>& result)
{
	std::vector<cv::Rect> faces;
	// Run the cascade classifier
	mFaceCascadeClf.detectMultiScale(image, faces, mScaleFactor, mMinNeighbors, cv::CASCADE_SCALE_IMAGE + cv::CASCADE_FIND_BIGGEST_OBJECT /* hint the classifier to only look for one face */ /*);

	std::vector<std::vector<cv::Point2f>> landmarks;
	if (faces.size() != 0)
	{
		//cv::rectangle(pImage, face[0], cv::Scalar(255, 0, 0), 2); // Draw rectangle. We assume a single face so we look at the first only

		if (mFacemark->fit(image, faces, landmarks))
		{
			//cv::face::drawFacemarks(pImage, landmarks[0], cv::Scalar(0, 255, 0)); // Draw the detected landmarks
			std::vector<cv::Point2i> landmarks2i(landmarks[0].begin(), landmarks[0].end()); // convert vector<Point2f> to vector<Point2i>
			result = std::pair<cv::Rect, std::vector<cv::Point2i>>(faces[0], landmarks2i);
		}
		else
			return false;
	}
	else
		return false;

	return true;
}
*/