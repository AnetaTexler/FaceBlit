//
// Created by Aneta on 08/13/2019.
//

/*
#ifndef STYLECAM_FACEMARKDETECTOR_H
#define STYLECAM_FACEMARKDETECTOR_H

#include <opencv2/core.hpp>
#include <opencv2/face.hpp>

class FacemarkDetector
{
	public:
		FacemarkDetector(cv::String faceModelPath, double scaleFactor, int minNeighbors, cv::String landmarkModelPath);
		//~FacemarkDetector();

		void prepareImage(const cv::Mat& src, cv::Mat& dst);
		bool detectFacemarks(const cv::Mat& image, std::pair<cv::Rect, std::vector<cv::Point2i>>& result);

	private:
		cv::CascadeClassifier mFaceCascadeClf;
		double mScaleFactor; // In each step of the image pyramid, the image is downscaled by this factor; a small factor (closer to 1.0) will cause longer runtime, but more accurate results. 
		int mMinNeighbors; // Lower threshold for a number of neighbors (multiple positive face classifications in close proximity). Face bound will be returned if it has at least "minNeighbors" neighboring positive face classifications. A lower number (an integer, close to 1) will return more detections, but will also introduce false positives.

		cv::Ptr<cv::face::Facemark> mFacemark;
};


#endif //STYLECAM_FACEMARKDETECTOR_H
*/