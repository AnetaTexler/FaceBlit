//
// Created by Aneta on 08/19/2019.
//



#include "DlibDetector.h"

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
#include <iostream>


DlibDetector::DlibDetector(std::string pLandmarkModelPath)
{
	mFaceDetector = dlib::get_frontal_face_detector(); // Face detector delivers bounding boxes for each face in an image.
	dlib::deserialize(pLandmarkModelPath) >> mLandmarkPredictor; // Load the model for prediction of face landmark positions given an image and face bounding box.
}

bool DlibDetector::detectFacemarks(const cv::Mat& pImage, std::pair<cv::Rect, std::vector<cv::Point2i>>& pResult)
{
	dlib::cv_image<dlib::bgr_pixel> dlibImg(pImage); // Change to dlib's image format. No memory is copied.
	//dlib::pyramid_up(pImage); // Make the image larger so we can detect small faces.
	
	std::vector<dlib::rectangle> faces = mFaceDetector(dlibImg); // Face detector gives a list of bounding boxes around all the faces in the image.
	std::vector<cv::Point2i> landmarks;

	if (faces.size() != 0)
	{
		dlib::full_object_detection shape = mLandmarkPredictor(dlibImg, faces[0]);
		
		if (shape.num_parts() != 0)
		{
			//dlib::image_window win;
			//win.clear_overlay();
			//win.set_image(dlibImg);
			//win.add_overlay(dlib::render_face_detections(shape));
			
			for (int i = 0; i < shape.num_parts(); i++)
				landmarks.push_back(cv::Point2i(shape.part(i).x(), shape.part(i).y()));
			
			pResult = std::pair<cv::Rect, std::vector<cv::Point2i>>(cv::Rect(faces[0].bottom(), faces[0].left(), faces[0].width(), faces[0].height()), landmarks);
		}
		else
			return false;
	}
	else
		return false;

	return true;
}
