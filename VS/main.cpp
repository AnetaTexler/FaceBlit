#include "time_helper.h"
#include "window_helper.h"
#include "cartesian_coordinate_system_helper.h"
#include "common_utils.h"
//#include "facemark_detector.h"
#include "dlib_detector.h"
#include "style_transfer.h"
#include "imgwarp_mls_similarity.h"
#include "imgwarp_mls_rigid.h"
#include "imgwarp_piecewiseaffine.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <opencv2/core.hpp>

#include <opengl_main.hpp>


cv::Mat alphaBlendTransparentBG(cv::Mat foreground, cv::Mat alphaMask)
{
	cv::Mat background(foreground.rows, foreground.cols, CV_32FC3, cv::Scalar(0, 0, 0));

	// Convert Mats to float data type
	foreground.convertTo(foreground, CV_32FC3, 1.0 / 255.0);
	alphaMask.convertTo(alphaMask, CV_32FC3, 1.0 / 255.0);

	blur(alphaMask, alphaMask, cv::Size(20, 20)); // Box blur

	multiply(alphaMask, foreground, foreground); // Multiply the foreground with the alpha matte
	multiply(cv::Scalar::all(1.0) - alphaMask, background, background); // Multiply the background with ( 1 - alpha )

	add(foreground, background, background);

	std::vector<cv::Mat> bgChannels;
	cv::split(background, bgChannels);

	std::vector<cv::Mat> alpChannels;
	cv::split(alphaMask, alpChannels);

	bgChannels.push_back(alpChannels.at(0)); // create alpha channel

	Mat result;
	cv::merge(bgChannels, result);

	result.convertTo(result, CV_8UC4, 255.0);

	return result;
}

std::string padLeft(std::string target, int totalWidth, char paddingChar)
{
	std::ostringstream ss;
	ss << std::setw(totalWidth) << std::setfill(paddingChar) << target;
	std::string result = ss.str();
	if (result.length() > totalWidth)
		result.erase(0, result.length() - totalWidth);

	return result;
}

bool makeDir(const std::string& path)
{
	if (!std::filesystem::exists(path)) {
		if (std::filesystem::create_directories(path) == false) {
			std::cout << "Creating an output directory \"" << path << "\" failed." << std::endl;
			system("pause");
			return false;
		}
	}
	return true;
}

// Creates diff image of Laplacian pyramid
cv::Mat createDiff(const cv::Mat& original, const cv::Mat& filtered)
{
	// Must have the same rows:cols ratio
	if ((float)original.rows / (float)filtered.rows != (float)original.cols / (float)filtered.cols)
	{
		std::cout << original.size << ", " << filtered.size << std::endl;
		throw std::runtime_error("@original and @filtered must have the same rows:cols ratio");
	}

	cv::Mat filtered_deep_copy;
	cv::resize(filtered, filtered_deep_copy, cv::Size(original.cols, original.rows), 0, 0, cv::INTER_LINEAR);

	cv::Mat diff(original.rows, original.cols, CV_8UC3);
	for (int row = 0; row < original.rows; row++)
	{
		for (int col = 0; col < original.cols; col++)
		{
			int B = original.at<cv::Vec3b>(row, col)[0] - filtered_deep_copy.at<cv::Vec3b>(row, col)[0];
			int G = original.at<cv::Vec3b>(row, col)[1] - filtered_deep_copy.at<cv::Vec3b>(row, col)[1];
			int R = original.at<cv::Vec3b>(row, col)[2] - filtered_deep_copy.at<cv::Vec3b>(row, col)[2];

			diff.at<cv::Vec3b>(row, col) = cv::Vec3b((B / 2) + 128, (G / 2) + 128, (R / 2) + 128);
		}
	}
	return diff;
}

void createInputImages_LVLS2(const cv::Mat& style, cv::Mat& style_base_only, cv::Mat& style_diff_only)
{
	cv::pyrDown(style, style_base_only, cv::Size(style.cols / 2, style.rows / 2));
	cv::pyrDown(style_base_only, style_base_only, cv::Size(style_base_only.cols / 2, style_base_only.rows / 2));
	cv::pyrDown(style_base_only, style_base_only, cv::Size(style_base_only.cols / 2, style_base_only.rows / 2));

	/*cv::bilateralFilter(style, style_base_only, 9, 31, 31);
	for (int i = 0; i < 5; i++)
	{
		cv::Mat tmp(style_base_only.rows, style_base_only.cols, CV_8UC3);
		cv::bilateralFilter(style_base_only, tmp, 9, 31, 31);
		tmp.copyTo(style_base_only);
	}*/

	style_diff_only = createDiff(style, style_base_only);

	if (style.size != style_base_only.size)
	{
		cv::resize(style_base_only, style_base_only, cv::Size(style.cols, style.rows), 0, 0, cv::INTER_LINEAR);
	}

	/*imshowInWindow("style", style);
	imshowInWindow("style_base_only", style_base_only);
	imshowInWindow("style_diff_only", style_diff_only);

	cv::Mat reconstructed_only = reconstructDiff(style_base_only, style_diff_only);

	imshowInWindow("reconstructed_only", reconstructed_only);

	cv::waitKey(0);*/
}




// ############################################################################
// #							VIDEO STYLIZATION							  #
// ############################################################################
int xmain()
{
	// *** PARAMETERS TO SET ***********************************************************************************************************
	const std::string root = "C:\\Users\\Aneta\\Desktop\\videos";
	//const std::string root = std::filesystem::current_path().string();

	std::string styleName = "watercolorgirl.png";			// name of a style image from dir root/styles with extension
	std::string videoName = "target6.mp4";					// name of a target video with extension
	const int NNF_patchsize = 3;							// voting patch size (0 for no voting)
	const bool transparentBG = false;						// choice of background (true = transparent bg, false = target image bg)
	//const bool frontCamera = true;							// camera facing
	// *********************************************************************************************************************************
	
	std::string styleNameNoExtension = styleName.substr(0, styleName.find_last_of("."));
	
	// --- READ STYLE FILES --------------------------------------------------
	cv::Mat styleImg = cv::imread(root + "\\styles\\" + styleName);
	std::ifstream styleLandmarkFile(root + "\\styles\\" + styleNameNoExtension + "_lm.txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	cv::Mat lookUpCube = loadLookUpCube(root + "\\styles\\" + styleNameNoExtension + "_lut.bytes");
	// -----------------------------------------------------------------------
	cv::Mat stylePosGuide = getGradient(styleImg.cols, styleImg.rows, false); // G_pos
	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());

	// Add 3 landmarks on the bottom of the notional circle
	//int radius = (styleLandmarks[45].x - styleLandmarks[36].x) * 2; // distance of outer eye corners + 80%
	//styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 45.0));
	//styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 90.0));
	//styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 135.0));
	// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
	styleLandmarks.push_back(cv::Point2i(0, styleImg.rows)); // left bottom corner
	styleLandmarks.push_back(cv::Point2i(styleImg.cols, styleImg.rows)); // right bottom corner
	
	// Only for generating inputs for NN
	//cv::imwrite(root + "\\styles\\" + styleNameNoExtension + "_gpos.png", stylePosGuide);
	//cv::imwrite(root + "\\styles\\" + styleNameNoExtension + "_gapp.png", styleAppGuide);
	//cv::Mat styleFaceMask = getSkinMask(styleImg, styleLandmarks);
	//styleFaceMask.convertTo(styleFaceMask, CV_8UC3, 255.0);
	//cv::imwrite(root + "\\styles\\" + styleNameNoExtension + "_facemask.png", styleFaceMask);
	//cv::Mat styleBase, styleDetail;
	//createInputImages_LVLS2(styleImg, styleBase, styleDetail);
	//cv::imwrite(root + "\\styles\\" + styleNameNoExtension + "_base.png", styleBase);
	//cv::imwrite(root + "\\styles\\" + styleNameNoExtension + "_detail.png", styleDetail);

	// Open a video file
	cv::VideoCapture cap(root + "\\" + videoName);
	if (!cap.isOpened()) {
		std::cout << "Unable to open the video." << std::endl;
		system("pause");
		return 1;
	}
	
	// Get the width/height and the FPS of the video
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	double FPS = cap.get(cv::CAP_PROP_FPS) / 2;
	
	// Create output directory
	std::string outputDirPath = root + "\\out_" + videoName.substr(0, videoName.find_last_of(".")) + "\\" + styleNameNoExtension + "_" + std::to_string(NNF_patchsize) + "nnf_" + std::to_string((int)std::ceil(FPS)) + "fps";
	makeDir(outputDirPath);

	// Open a video file for writing (the MP4V codec works on OS X and Windows)
	cv::VideoWriter out(outputDirPath + "\\result.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), FPS, cv::Size(styleImg.cols, styleImg.rows));
	if (!out.isOpened()) {
		std::cout << "Error! Unable to open video file for output." << std::endl;
		system("pause");
		return 1;
	}
	
	// Load facemark models
	//FacemarkDetector faceDetector("facemark_models\\lbpcascade_frontalface.xml", 1.1, 3, "facemark_models\\lbf_landmarks.yaml");
	DlibDetector faceDetector("facemark_models\\shape_predictor_68_face_landmarks.dat");
	std::pair<cv::Rect, std::vector<cv::Point2i>> faceDetResult; // face and its landmarks
	std::vector<cv::Point2i> targetLandmarks;

	cv::Mat frame;
	int i = 0;
	///*
	// --- CLASSIC VIDEO STYLIZATION --------------------------------------------------
	while (true) {
		cap >> frame;
		if (frame.empty()) {
			std::cout << "No more frames." << std::endl;
			break;
		}

		if (i % 2 == 1) // skip odd frames when we want the half FPS (not in case of target12!!!)
		{
			i++; 
			continue;
		}  
		/*
		frame = frame(cv::Rect(240, 0, width - 480, height)); // crop to fit 4:3 ratio
		cv::resize(frame, frame, cv::Size(styleImg.rows, styleImg.cols)); // resize to fit style size
		if (frontCamera)
			cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); // from landscape to portrait - COUNTERCLOCKWISE front camera, CLOCKWISE back camera
		else
			cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
		*/
		//Window::imgShow("lm", frame);
		//frame = cv::imread(outputDirPath + "\\frames\\0.png");

		//faceDetector.prepareImage(frame, frame);
		/*TimeMeasure tm;
		bool success = faceDetector.detectFacemarks(frame, faceDetResult);
		std::cout << "Landmarks: " << tm.elapsed_milliseconds() << " ms" << std::endl;
		if (!success) {
			std::cout << "Face detection failed." << std::endl;
			system("pause");
			return 1;
		}

		targetLandmarks = faceDetResult.second;

		if (targetLandmarks[1].y - targetLandmarks[0].y < 30) continue;	
		*/

		
		/*
		std::string lmPath = root + "\\landmarks\\" + videoName.substr(0, videoName.find_last_of(".")) + "\\" + padLeft(std::to_string(i), 4, '0') + ".txt";
		std::ifstream targetLandmarkFile(lmPath);
		if (targetLandmarkFile.fail()) {
			std::cout << "No landmarks for this video are available." << std::endl;
			system("pause");
			return 1;
		}
		std::string targetLandmarkStr((std::istreambuf_iterator<char>(targetLandmarkFile)), std::istreambuf_iterator<char>());
		targetLandmarkFile.close();
		targetLandmarks = getLandmarkPointsFromString(targetLandmarkStr.c_str());
		*/
		faceDetector.detectFacemarks(frame, faceDetResult);
		targetLandmarks = faceDetResult.second;

		alignTargetToStyle(frame, targetLandmarks, styleLandmarks);
		//Window::imgShow("frame", frame);
		
		// Add 3 landmarks on the bottom of the notional circle
		//radius = (targetLandmarks[45].x - targetLandmarks[36].x) * 2; // distance of outer eye corners + 80%
		//targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 45.0));
		//targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 90.0));
		//targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 135.0));
		// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
		targetLandmarks.push_back(cv::Point2i(0, frame.rows)); // left bottom corner
		targetLandmarks.push_back(cv::Point2i(frame.cols, frame.rows)); // right bottom corner

		cv::Mat faceMask = getSkinMask(frame, targetLandmarks);
		
		// Generate target guidance channels
		cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
		cv::Mat targetAppGuide = getAppGuide(frame, false); // G_app
		targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

		// StyleBlit
		cv::Mat stylizedImg, stylizedImgNoApp;
		if (NNF_patchsize > 0)
		{
			stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize);
			stylizedImgNoApp = styleBlit_voting(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize, 20, 10, 0);
		}
		else
			stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows));

		// Alpha blending
		cv::Mat alphaBlendResult;
		if (transparentBG)
			alphaBlendResult = alphaBlendTransparentBG(stylizedImg, faceMask);
		else
			//alphaBlendResult = alphaBlendFG_BG(stylizedImg, frame, faceMask, 25.0);
			alphaBlendResult = alphaBlendFG_BG(stylizedImg, stylizedImgNoApp, faceMask, 25.0);

		//cv::circle(alphaBlendResult, targetLandmarks[30], radius, cv::Scalar(0, 255, 0), 3);
		//CartesianCoordinateSystem::drawLandmarks(alphaBlendResult, targetLandmarks);
		//cv::circle(alphaBlendResult, getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), 5, cv::Scalar(0, 0, 255), 3); // circle center
		// Save frame to video
		out << alphaBlendResult;

		// Write frames (stylized, Gpos, Gapp)
		makeDir(outputDirPath + "\\stylized_frames");
		makeDir(outputDirPath + "\\gpos_frames");
		makeDir(outputDirPath + "\\gapp_frames");
		makeDir(outputDirPath + "\\facemask_frames");
		makeDir(outputDirPath + "\\base_frames");
		makeDir(outputDirPath + "\\detail_frames"); 
		cv::imwrite(outputDirPath + "\\stylized_frames\\" + padLeft(std::to_string(i), 4, '0') + ".png", alphaBlendResult);
		cv::imwrite(outputDirPath + "\\gpos_frames\\" + padLeft(std::to_string(i), 4, '0') + ".png", targetPosGuide);
		cv::imwrite(outputDirPath + "\\gapp_frames\\" + padLeft(std::to_string(i), 4, '0') + ".png", targetAppGuide);
		faceMask.convertTo(faceMask, CV_8UC3, 255.0);
		cv::imwrite(outputDirPath + "\\facemask_frames\\" + padLeft(std::to_string(i), 4, '0') + ".png", faceMask);
		cv::Mat targetBase, targetDetail;
		createInputImages_LVLS2(frame, targetBase, targetDetail);
		cv::imwrite(outputDirPath + "\\base_frames\\" + std::to_string(i + 1000) + "_base.png", targetBase);
		cv::imwrite(outputDirPath + "\\detail_frames\\" + std::to_string(i + 1000) + "_detail.png", targetDetail);
		//Window::imgShow("Result", alphaBlendResult);

		i++;
	}
	//*/
	/*
	// --- CORRECT AN INPUT VIDEO (RATIO, SIZE, ORIENTATION) -------------------------
	//int cnt = 2188;
	while (true) {
		cap >> frame;
		if (frame.empty()) {
			std::cout << "No more frames." << std::endl;
			break;
		}

		//frame = cv::imread(root + "\\input_frames\\0" + std::to_string(cnt++) + "_rt.bmp");
		//frame = frame(cv::Rect(25, 0, frame.cols / 2 - 50, frame.rows)); // crop to fit 4:3 ratio
		//cv::resize(frame, frame, cv::Size(styleImg.cols, styleImg.rows)); // resize to fit style size

		frame = frame(cv::Rect(240, 0, width - 480, height)); // crop to fit 4:3 ratio
		cv::resize(frame, frame, cv::Size(styleImg.rows, styleImg.cols)); // resize to fit style size
		if (frontCamera)
			cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); // from landscape to portrait - COUNTERCLOCKWISE front camera, CLOCKWISE back camera
		else
			cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
		//Window::imgShow("frame", frame);

		//cv::imwrite(outputDirPath + "\\frames\\" + std::to_string(i++) + ".png", frame);
		out << frame;

		//if (cnt == 2739) break;
	}
	*/
	/*
	// --- DRAW LANDMARKS INTO A VIDEO ----------------------------------------------
	while (true) {
		cap >> frame;
		if (frame.empty()) {
			std::cout << "No more frames." << std::endl;
			break;
		}

		if (i % 2 == 1) // skip odd frames
		{
			i++; 
			continue;
		}  

		frame = frame(cv::Rect(240, 0, width - 480, height)); // crop to fit 4:3 ratio
		cv::resize(frame, frame, cv::Size(styleImg.rows, styleImg.cols)); // resize to fit style size
		if (frontCamera)
			cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); // from landscape to portrait - COUNTERCLOCKWISE front camera, CLOCKWISE back camera
		else
			cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
		//Window::imgShow("frame", frame);

		//faceDetector.prepareImage(frame, frame);
		TimeMeasure tm;
		bool success = faceDetector.detectFacemarks(frame, faceDetResult);
		std::cout << "Landmarks: " << tm.elapsed_milliseconds() << " ms" << std::endl;
		if (!success) {
			std::cout << "Face detection failed." << std::endl;
			system("pause");
			return 1;
		}

		targetLandmarks = faceDetResult.second;

		if (targetLandmarks[1].y - targetLandmarks[0].y < 30) continue; // skip bad detections

		CartesianCoordinateSystem::drawRainbowLandmarks(frame, targetLandmarks);
		//cv::imwrite(outputDirPath + "\\frames\\" + std::to_string(i) + ".png", frame);
		out << frame;
		i++;
	}
	*/
	/*
	// --- 3-FRAME SIDE BY SIDE VIDEO --- edit Size (cols * 3) in VideoWriter!!!!!!!
	while (true) {
		cap >> frame;
		if (frame.empty()) {
			std::cout << "No more frames." << std::endl;
			break;
		}

		if (i % 2 == 1) // skip odd frames
		{
			i++; 
			continue;
		}

		
		//frame = frame(cv::Rect(240, 0, width - 480, height)); // crop to fit 4:3 ratio
		//cv::resize(frame, frame, cv::Size(styleImg.rows, styleImg.cols)); // resize to fit style size
		//if (frontCamera)
		//	cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); // from landscape to portrait - COUNTERCLOCKWISE front camera, CLOCKWISE back camera
		//else
		//	cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
		
		//Window::imgShow("lm", frame);
		//frame = cv::imread(outputDirPath + "\\frames\\0.png");
		
		//faceDetector.prepareImage(frame, frame);
		//TimeMeasure tm;
		//bool success = faceDetector.detectFacemarks(frame, faceDetResult);
		//std::cout << "Landmarks: " << tm.elapsed_milliseconds() << " ms" << std::endl;
		//if (!success) {
		//	std::cout << "Face detection failed." << std::endl;
		//	system("pause");
		//	return 1;
		//}
		//targetLandmarks = faceDetResult.second;
		
		//if (targetLandmarks[1].y - targetLandmarks[0].y < 30) continue;
		
		std::string lmPath = root + "\\landmarks\\" + videoName.substr(0, videoName.find_last_of(".")) + "\\" + padLeft(std::to_string(i), 4, '0') + ".txt";
		std::ifstream targetLandmarkFile(lmPath);
		std::string targetLandmarkStr((std::istreambuf_iterator<char>(targetLandmarkFile)), std::istreambuf_iterator<char>());
		targetLandmarkFile.close();
		targetLandmarks = getLandmarkPointsFromString(targetLandmarkStr.c_str());

		//targetLandmarks = CartesianCoordinateSystem::recomputePoints180Rotation(targetLandmarks, cv::Size(frame.cols, frame.rows));
		//std::ofstream outFile(lmPath);
		//for (cv::Point2i point : targetLandmarks)
		//	outFile << point.x << " " << point.y << std::endl;
		//outFile.close();
		
		//CartesianCoordinateSystem::drawRainbowLandmarks(frame, targetLandmarks);
		//Window::imgShow("lm", frame);

		
		cv::Mat faceMask = getSkinMask(frame, targetLandmarks);

		// Generate guidance channels
		cv::Mat stylePosGuide = getGradient(frame.cols, frame.rows, false); // G_pos
		cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
		cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
		cv::Mat targetAppGuide = getAppGuide(frame, false); // G_app
		targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

		// StyleBlit
		cv::Mat stylizedImg;
		if (NNF_patchsize > 0)
			stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize);
		else
			stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows));
		
		// Alpha blending
		cv::Mat alphaBlendResult;
		if (transparentBG)
			alphaBlendResult = alphaBlendTransparentBG(stylizedImg, faceMask);
		else
			alphaBlendResult = alphaBlendFG_BG(stylizedImg, frame, faceMask, 25.0);

		cv::Mat concatFrame;
		cv::hconcat(frame, alphaBlendResult, concatFrame);
		cv::hconcat(concatFrame, styleImg, concatFrame);
		//Window::imgShow("Result", concatFrame);

		// Save frame to video
		out << concatFrame;

		//cv::imwrite(outputDirPath + "\\frames\\" + std::to_string(i) + ".png", frame);
		
		i++;
	}
	*/


	system("pause");
	return 0;
}


// ############################################################################
// #				STYLIZE A SET OF IMAGES IN A DIRECTORY					  #
// ############################################################################


int xxxmain()
{
	// *** PARAMETERS TO SET ***********************************************************************************************************
	const std::string root = "C:\\Users\\Aneta\\Desktop\\faces_test_subset";
	//const std::string root = std::filesystem::current_path().string();

	const std::string styleName = "watercolorgirl.png";					// name of a style image from dir root/styles
	const int NNF_patchsize = 3;							// voting patch size (0 for no voting)
	const bool transparentBG = false;						// choice of background (true = transparent bg, false = target image bg)
	const bool faceStylization = true;
	const bool hairStylization = false;						
	// *********************************************************************************************************************************

	std::string styleNameNoExtension = styleName.substr(0, styleName.find_last_of("."));
	std::string outputName = "out_" + styleNameNoExtension + "_" + std::to_string(NNF_patchsize) + "smnnf" 
		+ (faceStylization ? "_face" : "") + (hairStylization ? "_hair" : "") + ".png";   // output file name with extension
	
	
	// --- READ STYLE FILES --------------------------------------------------
	cv::Mat styleImg = cv::imread(root + "\\styles\\" + styleName);
	cv::Mat styleHairMask = cv::imread(root + "\\styles\\" + styleNameNoExtension + "_hmask.png");
	std::ifstream styleLandmarkFile(root + "\\styles\\" + styleNameNoExtension + "_lm.txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	cv::Mat lookUpCube = loadLookUpCube(root + "\\styles\\" + styleNameNoExtension + "_lut.bytes");
	// -----------------------------------------------------------------------
	std::vector<cv::Point2i> styleFaceLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());
	std::vector<cv::Point2i> styleHairContourPoints, styleHairLandmarks;
	styleHairContourPoints = CartesianCoordinateSystem::getContourPoints(styleHairMask);
	styleHairLandmarks = CartesianCoordinateSystem::getContourPointsSubset(styleHairContourPoints, 50, styleImg);
	
	//for (int i = 0; i < styleHairLandmarks.size(); i++)
	//	cv::circle(styleHairMask, styleHairLandmarks[i], 5, cv::Scalar(0, 255, 0), 2);
	//Window::imgShow("shm", styleHairMask);
	


	//FacemarkDetector facemarkDetector("facemark_models\\lbpcascade_frontalface.xml", 1.4, 3, "facemark_models\\lbf_landmarks.yaml");
	//facemarkDetector.prepareImage(styleImg, styleImg);
	//cv::resize(styleImg, styleImg, cv::Size(styleImg.cols/2, styleImg.rows/2));

	//std::pair<cv::Rect, std::vector<cv::Point2i>> faceDetResult; // face and its landmarks
	/*
	TimeMeasure tm;
	bool success = facemarkDetector.detectFacemarks(styleImg, faceDetResult);
	std::vector<cv::Point2i> landmarks = faceDetResult.second;
	std::cout << "Landmarks: " << tm.elapsed_milliseconds() << " ms" << std::endl;

	//tm.reset();
	//success = facemarkDetector.detectFacemarks(styleImg, faceDetResult);
	//landmarks = faceDetResult.second;
	//std::cout << "Landmarks: " << tm.elapsed_milliseconds() << " ms" << std::endl;

	Window::imgShow("Style", styleImg);
	system("pause");
	return 0;
	*/



	for (const auto& entryI : std::filesystem::directory_iterator(root)) // for each dir in root
	{
		int inputStrPos;
		if (entryI.is_directory() && (inputStrPos = entryI.path().filename().string().find("input")) != std::string::npos) // only dirs containing "input" in their names
		{
			std::string outputDirName = entryI.path().filename().string().replace(inputStrPos, 5, "output") + "_" + styleNameNoExtension;
			makeDir(root + "\\" + outputDirName);

			std::vector<std::filesystem::directory_entry> entryJvector;
			for (const std::filesystem::directory_entry& entryJ : std::filesystem::directory_iterator(root + "\\" + entryI.path().filename().string())) // for each dir in root/input
			{
				entryJvector.push_back(entryJ);
			}

			//#pragma omp parallel for
			for (int i = 0; i < entryJvector.size(); i++) // for each dir in root/input
			{
				const std::filesystem::directory_entry& entryJ = entryJvector[i];
				
				makeDir(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string());

				// --- READ TARGET FILES --------------------------------------------------
				cv::Mat targetImg = cv::imread(entryJ.path().string() + "\\!input.png");
				cv::Mat targetFaceMask = cv::imread(entryJ.path().string() + "\\!face.png");
				cv::Mat targetHairMask = cv::imread(entryJ.path().string() + "\\!hair.png");
				cv::Mat targetPortraitMask = cv::imread(entryJ.path().string() + "\\!portrait.png");
				std::ifstream targetLandmarkFile(entryJ.path().string() + "\\!face.txt");
				std::string targetLandmarkStr((std::istreambuf_iterator<char>(targetLandmarkFile)), std::istreambuf_iterator<char>());
				targetLandmarkFile.close();
				// -----------------------------------------------------------------------
				std::vector<cv::Point2i> targetFaceLandmarks = getLandmarkPointsFromString(targetLandmarkStr.c_str());
				std::vector<cv::Point2i> targetHairContourPoints, targetHairLandmarks;
				targetHairContourPoints = CartesianCoordinateSystem::getContourPoints(targetHairMask);
				targetHairLandmarks = CartesianCoordinateSystem::getContourPointsSubset(targetHairContourPoints, 50, targetImg);
				//for (int i = 0; i < targetHairLandmarks.size(); i++)
				//	cv::circle(targetHairMask, targetHairLandmarks[i], 5, cv::Scalar(0, 255, 0), 2);
				//Window::imgShow("thm", targetHairMask);
				//CartesianCoordinateSystem::drawRainbowLandmarks(styleHairMask, styleHairLandmarks);
				//Window::imgShow("shm", styleHairMask);
				//CartesianCoordinateSystem::drawRainbowLandmarks(targetHairMask, targetHairLandmarks);
				//Window::imgShow("thm", targetHairMask);

				if (styleHairLandmarks.size() != targetHairLandmarks.size())
					std::cout << "ERROR: Number of style's and target's hair points are not equal. Input: " << entryJ.path().filename().string() << std::endl;


				// Merge face and hair landmarks - in case of face and hair simultaneous stylization
				//std::vector<cv::Point2i> stylePortraitLandmarks = styleFaceLandmarks;
				//stylePortraitLandmarks.insert(stylePortraitLandmarks.end(), styleHairLandmarks.begin(), styleHairLandmarks.end());
				//std::vector<cv::Point2i> targetPortraitLandmarks = targetFaceLandmarks;
				//targetPortraitLandmarks.insert(targetPortraitLandmarks.end(), targetHairLandmarks.begin(), targetHairLandmarks.end());


				//facemarkDetector.prepareImage(targetImg, targetImg);
				//bool success = facemarkDetector.detectFacemarks(targetImg, faceDetResult);
				//std::vector<cv::Point2i> targetLandmarks = faceDetResult.second;

				//Window::imgShow("img", targetImg);

				//cv::Mat faceMask = getSkinMask(targetImg, targetLandmarks);
				//cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\skin_mask.png", faceMask);
				//Window::imgShow("faceMask", faceMask);

				//CartesianCoordinateSystem::drawRainbowLandmarks(targetImg, targetLandmarks, entryJ.path().string() + "\\!lm.png"); // TODO: copy target first!!
				//CartesianCoordinateSystem::drawRainbowLandmarks(styleImg, styleLandmarks, root + "\\styles\\" + styleName + "_lm.png");

				// Generate guidance channels
				cv::Mat stylePosGuide = getGradient(targetImg.cols, targetImg.rows, false); // G_pos
				cv::Mat targetFacePosGuide = MLSDeformation(stylePosGuide, styleFaceLandmarks, targetFaceLandmarks); // G_pos for face
				cv::Mat targetHairPosGuide = MLSDeformation(stylePosGuide, styleHairLandmarks, targetHairLandmarks); // G_pos for hair
				cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
				cv::Mat targetAppGuide = getAppGuide(targetImg, false); // G_app
				targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

				//cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\gpos_face.png", targetFacePosGuide);
				//cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\gapp.png", targetAppGuide);

				//cv::Mat targetBase, targetDetail;
				//createInputImages_LVLS2(targetImg, targetBase, targetDetail);
				//cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\base.png", targetBase);
				//cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\detail.png", targetDetail);

				//cv::Mat lookUpCube = getLookUpCube(stylePosGuide, styleAppGuide); // 3D table that maps Pos_Red, Pos_Green, App values to coordinates u, v in style
				//saveLookUpCube(lookUpCube, root + "\\styles\\" + styleName + "_lut.bytes");

				
				// StyleBlit - FACE
				cv::Mat stylizedImg_FACE;
				if (faceStylization)
				{
					if (NNF_patchsize > 0)
						stylizedImg_FACE = styleBlit_voting(stylePosGuide, targetFacePosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), NNF_patchsize);
					else
						stylizedImg_FACE = styleBlit(stylePosGuide, targetFacePosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows));
				}

				// StyleBlit - HAIR
				cv::Mat stylizedImg_HAIR;
				if (hairStylization)
				{
					if (NNF_patchsize > 0)
						stylizedImg_HAIR = styleBlit_voting(stylePosGuide, targetHairPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), NNF_patchsize);
					else
						stylizedImg_HAIR = styleBlit(stylePosGuide, targetHairPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows));
				}

				if (faceStylization && hairStylization)
				{
					cv::Mat alphaBlendHair2Face, alphaBlendResult;
					// Alpha blending - HAIR to FACE
					alphaBlendHair2Face = alphaBlendFG_BG(stylizedImg_HAIR, stylizedImg_FACE, targetHairMask, 25.0);
					// Alpha blending - PORTRAIT to original image
					if (transparentBG)
						alphaBlendResult = alphaBlendTransparentBG(alphaBlendHair2Face, targetPortraitMask);
					else
						alphaBlendResult = alphaBlendFG_BG(alphaBlendHair2Face, targetImg, targetPortraitMask, 25.0);
					
					cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\" + outputName, alphaBlendResult);
				}
				else if (faceStylization && !hairStylization)
				{
					// Alpha blending - FACE to original image
					cv::Mat alphaBlendFace = alphaBlendFG_BG(stylizedImg_FACE, targetImg, targetFaceMask, 25.0);
					
					cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\" + outputName, alphaBlendFace);
				}
				else if (!faceStylization && hairStylization)
				{
					// Alpha blending - HAIR to original image
					cv::Mat alphaBlendHair = alphaBlendFG_BG(stylizedImg_HAIR, targetImg, targetHairMask, 25.0);
					
					cv::imwrite(root + "\\" + outputDirName + "\\" + entryJ.path().filename().string() + "\\" + outputName, alphaBlendHair);
				}
				else
				{
					std::cout << "No stylization was performed based on set parameters." << std::endl;
					system("pause");
					return 1;
				}
			}
		}
	}

	system("pause");
	return 0;
}


// ############################################################################
// #							SINGLE IMAGE TEST							  #
// ############################################################################
int xxmain()
{
	// *** PARAMETERS TO SET ***********************************************************************************************************
	//const std::string root = "C:\\Users\\Aneta\\Desktop\\faces_test_subset";
	//const std::string root = std::filesystem::current_path().string();
	std::string rawPath = "..\\app\\src\\main\\res\\raw\\";
	std::string drawablePath = "..\\app\\src\\main\\res\\drawable\\";

	const std::string styleName = "style_het.png";		// name of a style PNG from drawablePath
	const std::string targetName = "target1.png";				// name of a target PNG
	const bool loadTargetLandmarks = false;						// true = landmarks are loaded from file "lm_<targetName>.txt", false = DLIB detector is called 
	const int NNF_patchsize = 3;								// voting patch size (0 for no voting)
	const bool transparentBG = false;							// choice of background (true = transparent bg, false = target image bg)
	// *********************************************************************************************************************************
	
	std::string styleNameWithoutExtensionAndPrefix = styleName.substr(0, styleName.find_last_of(".")).replace(styleName.find("style_"), 6, "");
	std::string targetNameWithoutExtension = targetName.substr(0, targetName.find_last_of("."));

	std::string outputName = targetNameWithoutExtension + "_" + styleNameWithoutExtensionAndPrefix + "_"
		+ (NNF_patchsize > 0 ? std::to_string(NNF_patchsize) + "x" + std::to_string(NNF_patchsize) + "_voting" : "0_voting") + ".png";   // output file name with extension

	// --- READ STYLE FILES -----------------
	cv::Mat styleImg = cv::imread(drawablePath + styleName);
	std::ifstream styleLandmarkFile(rawPath + "lm_" + styleNameWithoutExtensionAndPrefix + ".txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());
	cv::Mat lookUpCube = loadLookUpCube(rawPath + "lut_" + styleNameWithoutExtensionAndPrefix + ".bytes");
	
	// --- READ TARGET FILES ----------------
	cv::Mat targetImg = cv::imread(drawablePath + targetName);
	std::vector<cv::Point2i> targetLandmarks;
	if (loadTargetLandmarks)
	{
		std::ifstream targetFile(rawPath + "lm_" + targetNameWithoutExtension + ".txt");
		std::string targetLandmarkStr((std::istreambuf_iterator<char>(targetFile)), std::istreambuf_iterator<char>());
		targetFile.close();
		targetLandmarks = getLandmarkPointsFromString(targetLandmarkStr.c_str());
	}
	// --------------------------------------
	
	std::pair<cv::Rect, std::vector<cv::Point2i>> detectionResult; // face and its landmarks
	if (!loadTargetLandmarks)
	{
		DlibDetector dlibDetector("facemark_models\\shape_predictor_68_face_landmarks.dat");
		TimeMeasure t_detection;
		dlibDetector.detectFacemarks(targetImg, detectionResult);
		std::cout << "Landmarks detection time: " << t_detection.elapsed_milliseconds() << " ms" << std::endl;
		targetLandmarks = detectionResult.second;
		//CartesianCoordinateSystem::drawLandmarks(targetImg, targetLandmarks);
		//cv::imwrite("TESTS\\lm.png", targetImg);
	}
	

	//TimeMeasure tm;
	cv::Mat stylePosGuide = getGradient(targetImg.cols, targetImg.rows, false); // G_pos
	//cv::imwrite("TESTS\\stylePosGuide.png", stylePosGuide);
	
	TimeMeasure t_targetPosGuide;
	cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
	std::cout << "targetPosGuide time: " << t_targetPosGuide.elapsed_milliseconds() << " ms" << std::endl;
	//cv::imwrite("TESTS\\targetPosGuide.png", targetPosGuide);
	
	TimeMeasure t_styleAppGuide;
	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
	std::cout << "styleAppGuide time: " << t_styleAppGuide.elapsed_milliseconds() << " ms" << std::endl;
	//cv::imwrite("TESTS\\styleAppGuide.png", styleAppGuide);
	
	TimeMeasure t_targetAppGuide;
	cv::Mat targetAppGuide = getAppGuide(targetImg, false); // G_app
	std::cout << "targetAppGuide time: " << t_targetAppGuide.elapsed_milliseconds() << " ms" << std::endl;
	//cv::imwrite("TESTS\\targetAppGuide.png", targetAppGuide);
	
	TimeMeasure t_targetAppGuide_hist;
	targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);
	std::cout << "targetAppGuide_hist time: " << t_targetAppGuide_hist.elapsed_milliseconds() << " ms" << std::endl;
	//cv::imwrite("TESTS\\targetAppGuide2.png", targetAppGuide);


	//lookUpCube = getLookUpCube(stylePosGuide, styleAppGuide); // 3D table that maps Pos_Red, Pos_Green, App values to coordinates u, v in style
	//saveLookUpCube(lookUpCube, rawPath + "lut_watercolorgirl.bytes");

	TimeMeasure t_styleBlit;
	cv::Mat stylizedImg, stylizedImgNoApp;
	if (NNF_patchsize > 0)
	{
		stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), NNF_patchsize);
		stylizedImgNoApp = styleBlit_voting(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), NNF_patchsize, 20, 10, 0);
	}
	else
		stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows));
	
	std::cout << "StyleBlit time: " << t_styleBlit.elapsed_milliseconds() << " ms" << std::endl;

	//Window::imgShow("Result", stylizedImg);
	//Window::imgShow("ResultNoApp", stylizedImgNoApp);
	//cv::waitKey(0);

	TimeMeasure timer;
	cv::Mat faceMask = getSkinMask(targetImg, targetLandmarks);
	std::cout << "GetFaceMask: " << timer.elapsed_milliseconds() << " ms" << std::endl;
	timer.reset();
	//cv::Mat alphaBlendResult = alphaBlendFG_BG(stylizedImg, targetImg, faceMask, 25.0f);
	cv::Mat alphaBlendResult = alphaBlendFG_BG(stylizedImg, stylizedImgNoApp, faceMask, 25.0f);
	std::cout << "AlphaBlend: " << timer.elapsed_milliseconds() << " ms" << std::endl;

	Window::imgShow("alphaBlend", alphaBlendResult);

	cv::imwrite("TESTS\\" + outputName, alphaBlendResult);
	
	cv::waitKey(0);

	return 0;
}


int main() {
	const std::string root = "C:\\Work\\TestDataset\\Videos";
	//const std::string root = std::filesystem::current_path().string();

	std::string styleName = "watercolorgirl.png";			// name of a style image from dir root/styles with extension
	std::string videoName = "target6.mp4";					// name of a target video with extension
	const int NNF_patchsize = 1;							// voting patch size (0 for no voting)
	const bool transparentBG = false;						// choice of background (true = transparent bg, false = target image bg)
	//const bool frontCamera = true;							// camera facing
	// *********************************************************************************************************************************

	std::string styleNameNoExtension = styleName.substr(0, styleName.find_last_of("."));

	// --- READ STYLE FILES --------------------------------------------------
	cv::Mat styleImg = cv::imread(root + "\\styles\\" + styleName);
	std::ifstream styleLandmarkFile(root + "\\styles\\" + styleNameNoExtension + "_lm.txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	cv::Mat lookUpCube = loadLookUpCube(root + "\\styles\\" + styleNameNoExtension + "_lut.bytes");
	// -----------------------------------------------------------------------
	cv::Mat stylePosGuide = getGradient(styleImg.cols, styleImg.rows, false); // G_pos

	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());

	// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
	styleLandmarks.push_back(cv::Point2i(0, styleImg.rows)); // left bottom corner
	styleLandmarks.push_back(cv::Point2i(styleImg.cols, styleImg.rows)); // right bottom corner

	// Open a video file
	cv::VideoCapture cap(root + "\\" + videoName);
	if (!cap.isOpened()) {
		std::cout << "Unable to open the video." << std::endl;
		system("pause");
		return 1;
	}

	// Get the width/height and the FPS of the video
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	double FPS = cap.get(cv::CAP_PROP_FPS) / 2;

	// Create output directory
	std::string outputDirPath = root + "\\out_" + videoName.substr(0, videoName.find_last_of(".")) + "\\" + styleNameNoExtension + "_" + std::to_string(NNF_patchsize) + "nnf_" + std::to_string((int)std::ceil(FPS)) + "fps";
	makeDir(outputDirPath);

	// Open a video file for writing (the MP4V codec works on OS X and Windows)
	cv::VideoWriter out(outputDirPath + "\\result.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), FPS, cv::Size(styleImg.cols, styleImg.rows));
	if (!out.isOpened()) {
		std::cout << "Error! Unable to open video file for output." << std::endl;
		system("pause");
		return 1;
	}

	// Load facemark models
	//FacemarkDetector faceDetector("facemark_models\\lbpcascade_frontalface.xml", 1.1, 3, "facemark_models\\lbf_landmarks.yaml");
	DlibDetector faceDetector("facemark_models\\shape_predictor_68_face_landmarks.dat");
	std::pair<cv::Rect, std::vector<cv::Point2i>> faceDetResult; // face and its landmarks
	std::vector<cv::Point2i> targetLandmarks;

	cv::Mat frame;
	int i = 0;

	// *********************************************************************************************************************************
	// OPENGL INITIALIZATION
	if (!FB_OpenGL::init()) {
		std::cout << "Something is wrong!" << std::endl;
		return -1;
	}

	FB_OpenGL::Shader debug_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex.vert", "..\\app\\src\\main\\opengl\\shader\\pass_tex.frag");
	FB_OpenGL::Shader styleblit_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex.vert", "..\\app\\src\\main\\opengl\\shader\\styleblit_main.frag");
	debug_shader.init();
	styleblit_shader.init();

	FB_OpenGL::FullScreenQuad quad = FB_OpenGL::FullScreenQuad(&debug_shader);
	FB_OpenGL::StyblitRenderer styleblit_main = FB_OpenGL::StyblitRenderer(&styleblit_shader);
	styleblit_main.setWidthHeight( styleImg.cols, styleImg.rows);

	//GLuint quad_texture = FB_OpenGL::makeTexture(styleImg);
	//quad.setTextureID( &quad_texture );

	// ************************ OPENGL VARIABLES ******************
	GLuint stylePosGuide_texture = 0;
	GLuint targetPosGuide_texture = 0;
	GLuint styleAppGuide_texture = 0;
	GLuint targetAppGuide_texture = 0;
	GLuint styleImg_texture = 0;
	GLuint lookUpTableTexture = FB_OpenGL::make3DTexture(lookUpCube);

	// GLuint frame_as_texture = 0;

	// Generate jitter table
	cv::Mat gaussian_noise = cv::Mat::zeros(styleImg.rows, styleImg.cols, CV_8UC2);
	cv::randu(gaussian_noise, 0, 255);
	
	GLuint jitter_table_texture = FB_OpenGL::makeJitterTable(gaussian_noise.clone());
	styleblit_main.setJitterTable(&jitter_table_texture);

	while (true) {
		cap >> frame;
		if (frame.empty()) {
			std::cout << "No more frames." << std::endl;
			break;
		}

		if (i % 2 == 1) // skip odd frames when we want the half FPS (not in case of target12!!!)
		{
			i++;
			continue;
		}

		faceDetector.detectFacemarks(frame, faceDetResult);
		targetLandmarks = faceDetResult.second;

		alignTargetToStyle(frame, targetLandmarks, styleLandmarks);

		targetLandmarks.push_back(cv::Point2i(0, frame.rows)); // left bottom corner
		targetLandmarks.push_back(cv::Point2i(frame.cols, frame.rows)); // right bottom corner

		cv::Mat faceMask = getSkinMask(frame, targetLandmarks);

		// Generate target guidance channels
		cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
		cv::Mat targetAppGuide = getAppGuide(frame, false); // G_app
		targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

		// StyleBlit
		/*cv::Mat stylizedImg, stylizedImgNoApp;
		if (NNF_patchsize > 0)
		{
			stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize);
			stylizedImgNoApp = styleBlit_voting(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize, 20, 10, 0);
		}
		else
			stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows));

		// Alpha blending
		cv::Mat alphaBlendResult;
		if (transparentBG)
			alphaBlendResult = alphaBlendTransparentBG(stylizedImg, faceMask);
		else
			//alphaBlendResult = alphaBlendFG_BG(stylizedImg, frame, faceMask, 25.0);
			alphaBlendResult = alphaBlendFG_BG(stylizedImg, stylizedImgNoApp, faceMask, 25.0);

		//cv::circle(alphaBlendResult, targetLandmarks[30], radius, cv::Scalar(0, 255, 0), 3);
		//CartesianCoordinateSystem::drawLandmarks(alphaBlendResult, targetLandmarks);
		//cv::circle(alphaBlendResult, getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), 5, cv::Scalar(0, 0, 255), 3); // circle center
		// Save frame to video
		out << alphaBlendResult;*/

		// Window::imgShow("Result", alphaBlendResult);

		i++;

		/*if (i > 2) {
			FB_OpenGL::updateTexture(frame_as_texture, alphaBlendResult);
		}
		else {
			frame_as_texture = FB_OpenGL::makeTexture(alphaBlendResult.clone());
			quad.setTextureID(&frame_as_texture);
		}*/

		if (i > 2) {
			FB_OpenGL::updateTexture(stylePosGuide_texture, stylePosGuide.clone()); // Not needed, just for debug now.
			FB_OpenGL::updateTexture(targetPosGuide_texture, targetPosGuide.clone());
			FB_OpenGL::updateTexture(styleAppGuide_texture, styleAppGuide.clone()); // Not needed, just for debug now.
			FB_OpenGL::updateTexture(targetAppGuide_texture, targetAppGuide.clone());
			FB_OpenGL::updateTexture(styleImg_texture, styleImg.clone()); // Not needed, just for debug now.
		}
		else {
			// std::cout << "stylepos" << std::endl;
			stylePosGuide_texture = FB_OpenGL::makeTexture(stylePosGuide.clone());
			// std::cout << "targetpos" << std::endl;
			targetPosGuide_texture = FB_OpenGL::makeTexture(targetPosGuide.clone());
			// std::cout << "styleapp" << std::endl;
			styleAppGuide_texture = FB_OpenGL::makeTexture(styleAppGuide.clone());
			// std::cout << "targetapp" << std::endl;
			targetAppGuide_texture = FB_OpenGL::makeTexture(targetAppGuide.clone());
			// std::cout << "styleimg" << std::endl;
			styleImg_texture = FB_OpenGL::makeTexture(styleImg.clone());
			styleblit_main.setTextures(&stylePosGuide_texture, &targetPosGuide_texture, &styleAppGuide_texture, &targetAppGuide_texture, &styleImg_texture, &lookUpTableTexture);
		}

		glClearColor(1.0f, 0.1f, 0.1f, 1.0f);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		styleblit_main.draw();

		SDL_GL_SwapWindow(globalOpenglData.mainWindow);
	}
	
	return 0;
}



/*

+--------+----+----+----+----+------+------+------+------+
|        | C1 | C2 | C3 | C4 | C(5) | C(6) | C(7) | C(8) |
+--------+----+----+----+----+------+------+------+------+
| CV_8U  |  0 |  8 | 16 | 24 |   32 |   40 |   48 |   56 |
| CV_8S  |  1 |  9 | 17 | 25 |   33 |   41 |   49 |   57 |
| CV_16U |  2 | 10 | 18 | 26 |   34 |   42 |   50 |   58 |
| CV_16S |  3 | 11 | 19 | 27 |   35 |   43 |   51 |   59 |
| CV_32S |  4 | 12 | 20 | 28 |   36 |   44 |   52 |   60 |
| CV_32F |  5 | 13 | 21 | 29 |   37 |   45 |   53 |   61 |
| CV_64F |  6 | 14 | 22 | 30 |   38 |   46 |   54 |   62 |
+--------+----+----+----+----+------+------+------+------+

*/