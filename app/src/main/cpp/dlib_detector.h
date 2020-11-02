//
// Created by Aneta on 08/19/2019.
//



#ifndef STYLECAM_DLIBDETECTOR_H
#define STYLECAM_DLIBDETECTOR_H

#include <opencv2/core.hpp>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>


class DlibDetector
{
public:
	DlibDetector(std::string landmarkModelPath);
	//~DlibDetector();

	bool detectFacemarks(const cv::Mat& image, std::pair<cv::Rect, std::vector<cv::Point2i>>& result);

private:
	dlib::frontal_face_detector mFaceDetector;
	dlib::shape_predictor mLandmarkPredictor;
};


#endif //STYLECAM_DLIBDETECTOR_H
