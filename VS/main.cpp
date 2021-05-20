#include "time_helper.h"
#include "window_helper.h"
#include "cartesian_coordinate_system_helper.h"
#include "common_utils.h"
#include "dlib_detector.h"
#include "style_transfer.h"
#include "imgwarp_mls_similarity.h"
#include "imgwarp_mls_rigid.h"
#include "imgwarp_piecewiseaffine.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <opencv2/core.hpp>




// ############################################################################
// #							VIDEO STYLIZATION							  #
// ############################################################################
bool getStylizedVideo(const std::string& rawPath, const std::string& drawablePath, const std::string& targetPath, const std::string& targetName, const std::string& styleName, const bool stylizeBG, const int NNF_patchsize)
{

	std::string styleNameWithoutExtensionAndPrefix = styleName.substr(0, styleName.find_last_of(".")).replace(styleName.find("style_"), 6, "");
	std::string targetNameWithoutExtension = targetName.substr(0, targetName.find_last_of("."));

	std::string outputName = targetNameWithoutExtension + "_" + styleNameWithoutExtensionAndPrefix + "_"
		+ (NNF_patchsize > 0 ? std::to_string(NNF_patchsize) + "x" + std::to_string(NNF_patchsize) + "_voting" : "0_voting")
		+ (stylizeBG ? "_bg" : "")
		+ ".mp4";   // output file name with extension

	// --- READ STYLE FILES ---------------------------
	cv::Mat styleImg = cv::imread(drawablePath + styleName);
	if (styleImg.empty()) {
		Log_e("FACEBLIT", "Failed to read style image '" + drawablePath + styleName + "'");
		return false;
	}
	std::ifstream styleLandmarkFile(rawPath + "lm_" + styleNameWithoutExtensionAndPrefix + ".txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());
	cv::Mat lookUpCube = loadLookUpCube(rawPath + "lut_" + styleNameWithoutExtensionAndPrefix + ".bytes");

	// --- GENERATE STYLE GUIDING CHANNELS ------------
	cv::Mat stylePosGuide = getGradient(styleImg.cols, styleImg.rows, false); // G_pos
	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app

	if (stylizeBG) {
		// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
		styleLandmarks.push_back(cv::Point2i(0, styleImg.rows)); // left bottom corner
		styleLandmarks.push_back(cv::Point2i(styleImg.cols, styleImg.rows)); // right bottom corner
	}

	// Open a video file
	cv::VideoCapture cap(targetPath + targetName);
	if (!cap.isOpened()) {
		Log_e("FACEBLIT", "Unable to open the input video.");
		return false;
	}

	// Get the width/height and the FPS of the video
	int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	double FPS = cap.get(cv::CAP_PROP_FPS) / 2;

	// Open a video file for writing (the MP4V codec works on OS X and Windows)
	cv::VideoWriter out("TESTS\\" + outputName, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), FPS, cv::Size(styleImg.cols, styleImg.rows));
	if (!out.isOpened()) {
		Log_e("FACEBLIT", "Unable to open the output video.");
		return false;
	}

	// Load facemark models
	DlibDetector faceDetector("facemark_models\\shape_predictor_68_face_landmarks.dat");
	std::pair<cv::Rect, std::vector<cv::Point2i>> faceDetResult; // face and its landmarks
	std::vector<cv::Point2i> targetLandmarks;

	Log_i("FACEBLIT", "Video stylization in progress...");

	cv::Mat frame;
	int i = 0, j = 0;
	while (true)
	{
		cap >> frame;
		if (frame.empty())
		{
			Log_i("FACEBLIT", "Done."); // no more frames
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
		*/

		bool success = faceDetector.detectFacemarks(frame, faceDetResult);
		if (!success)
		{
			Log_i("FACEBLIT", "Face detection failed, continue to the next frame.");
			i++;
			continue;
		}

		targetLandmarks = faceDetResult.second;

		/* // Load landmarks from file
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


		//alignTargetToStyle(frame, targetLandmarks, styleLandmarks);


		if (stylizeBG) {
			// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
			targetLandmarks.push_back(cv::Point2i(0, frame.rows)); // left bottom corner
			targetLandmarks.push_back(cv::Point2i(frame.cols, frame.rows)); // right bottom corner
		}


		// --- GENERATE TARGET GUIDING CHANNELS -------------------------
		cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
		cv::Mat targetAppGuide = getAppGuide(frame, false); // G_app
		targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

		// --- STYLE TRANSFER --------------------------------------------
		cv::Rect2i headAreaRect = CartesianCoordinateSystem::getHeadAreaRect(targetLandmarks, cv::Size(frame.cols, frame.rows));

		cv::Mat stylizedImg, stylizedImgNoApp;
		if (NNF_patchsize > 0)
		{
			stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, headAreaRect, NNF_patchsize);
			if (stylizeBG)
				stylizedImgNoApp = styleBlit_voting(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), NNF_patchsize, 10, 10, 0);
		}
		else
		{
			stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, headAreaRect);
			if (stylizeBG)
				stylizedImgNoApp = styleBlit(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows), 10, 10, 0);
		}

		// --- FACE MASK -----------------------------------
		cv::Mat targetFaceMask = getSkinMask(frame, targetLandmarks);

		// --- ALPHA BLEND ---------------------------------
		cv::Mat alphaBlendResult;
		if (stylizeBG)
			alphaBlendResult = alphaBlendFG_BG(stylizedImg, stylizedImgNoApp, targetFaceMask, 25.0); // blend the face into the stylized image without appearance
		else
			alphaBlendResult = alphaBlendFG_BG(stylizedImg, frame, targetFaceMask, 25.0); // blend the face into the original target image

		Window::imgShow("Result", alphaBlendResult);

		// Save frame to video
		out << alphaBlendResult;

		i++;
		j++;
	}

	return true;
}



// ############################################################################
// #						SINGLE IMAGE STYLIZATION						  #
// ############################################################################
bool getStylizedImage(const std::string& rawPath, const std::string& drawablePath, const std::string& targetPath, const std::string& targetName, const std::string& styleName, const bool stylizeBG, const int NNF_patchsize)
{

	std::string styleNameWithoutExtensionAndPrefix = styleName.substr(0, styleName.find_last_of(".")).replace(styleName.find("style_"), 6, "");
	std::string targetNameWithoutExtension = targetName.substr(0, targetName.find_last_of("."));

	std::string outputName = targetNameWithoutExtension + "_" + styleNameWithoutExtensionAndPrefix + "_"
		+ (NNF_patchsize > 0 ? std::to_string(NNF_patchsize) + "x" + std::to_string(NNF_patchsize) + "_voting" : "0_voting")
		+ (stylizeBG ? "_bg" : "")
		+ ".png";   // output file name with extension

	// --- READ STYLE FILES -----------------
	cv::Mat styleImg = cv::imread(drawablePath + styleName);
	if (styleImg.empty()) {
		Log_e("FACEBLIT", "Failed to read style image '" + drawablePath + styleName + "'");
		return false;
	}
	std::ifstream styleLandmarkFile(rawPath + "lm_" + styleNameWithoutExtensionAndPrefix + ".txt");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());
	cv::Mat lookUpCube = loadLookUpCube(rawPath + "lut_" + styleNameWithoutExtensionAndPrefix + ".bytes");

	// --- DETECT LANDMARKS IN TARGET ----------------
	cv::Mat targetImg = cv::imread(targetPath + targetName);
	if (styleImg.empty()) {
		Log_e("FACEBLIT", "Failed to read target image '" + targetPath + targetName + "'");
		return false;
	}
	std::vector<cv::Point2i> targetLandmarks;
	std::pair<cv::Rect, std::vector<cv::Point2i>> detectionResult; // face rectangle and landmarks
	DlibDetector dlibDetector("facemark_models\\shape_predictor_68_face_landmarks.dat"); // load model

	TimeMeasure t_detection;
	bool success = dlibDetector.detectFacemarks(targetImg, detectionResult);
	if (!success)
	{
		Log_e("FACEBLIT", "Face detection failed.");
		return false;
	}
	Log_i("FACEBLIT", "LM detection time: " + std::to_string(t_detection.elapsed_milliseconds()) + " ms");

	targetLandmarks = detectionResult.second;
	//CartesianCoordinateSystem::drawLandmarks(targetImg, targetLandmarks);
	//cv::imwrite("TESTS\\lm.png", targetImg);

	// --- GENERATE GUIDING CHANNELS ------------------
	cv::Mat stylePosGuide = getGradient(targetImg.cols, targetImg.rows, false); // style's G_pos

	TimeMeasure t_targetPosGuide;
	cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // target's G_pos
	Log_i("FACEBLIT", "T G_pos time: " + std::to_string(t_targetPosGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_styleAppGuide;
	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // style's G_app
	Log_i("FACEBLIT", "S G_app time: " + std::to_string(t_styleAppGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_targetAppGuide;
	cv::Mat targetAppGuide = getAppGuide(targetImg, false); // target's G_app
	Log_i("FACEBLIT", "T G_app time: " + std::to_string(t_targetAppGuide.elapsed_milliseconds()) + " ms");

	TimeMeasure t_targetAppGuide_hist;
	targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);
	Log_i("FACEBLIT", "T G_app hist. matching time: " + std::to_string(t_targetAppGuide_hist.elapsed_milliseconds()) + " ms");

	// --- GENERATE LOOKUP TABLE FOR A NEW STYLE OR LOWER RESOLUTION -----------
	/* // Generate style resources for lower resolution
	cv::resize(styleImg, styleImg, cv::Size(480, 640));
	cv::Mat styleGpos = getGradient(480, 640, false); // G_pos
	cv::Mat styleGapp = getAppGuide(styleImg, true); // G_app
	cv::imwrite(drawablePath + "style_" + styleNameWithoutExtensionAndPrefix + "_480x640.png", styleImg);
	for (int i = 0; i < styleLandmarks.size(); i++)
		styleLandmarks[i] *= 0.625;
	CartesianCoordinateSystem::drawLandmarks(styleImg, styleLandmarks);
	Window::imgShow("lm", styleImg);
	CartesianCoordinateSystem::savePointsIntoFile(styleLandmarks, rawPath + "lm_" + styleNameWithoutExtensionAndPrefix + "_480x640.txt");

	TimeMeasure t_lut;
	lookUpCube = getLookUpCube(styleGpos, styleGapp); // 3D table that maps Pos_Red, Pos_Green, App values to coordinates u, v in style
	Log_i("FACEBLIT", "Lookup table time: " + std::to_string(t_lut.elapsed_milliseconds()) + " ms");
	saveLookUpCube(lookUpCube, rawPath + "lut_" + styleNameWithoutExtensionAndPrefix + "_480x640.bytes");
	*/

	// --- STYLE TRANSFER --------------------------------------------
	cv::Rect2i headAreaRect = CartesianCoordinateSystem::getHeadAreaRect(targetLandmarks, cv::Size(targetImg.cols, targetImg.rows));

	TimeMeasure t_styleBlit;
	cv::Mat stylizedImg, stylizedImgNoApp;
	if (NNF_patchsize > 0)
	{
		stylizedImg = styleBlit_voting(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, headAreaRect, NNF_patchsize);
		if (stylizeBG)
			stylizedImgNoApp = styleBlit_voting(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), NNF_patchsize, 10, 10, 0);
	}
	else
	{
		stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, headAreaRect);
		if (stylizeBG)
			stylizedImgNoApp = styleBlit(stylePosGuide, targetPosGuide, cv::Mat(), cv::Mat(), lookUpCube, styleImg, cv::Rect2i(0, 0, targetImg.cols, targetImg.rows), 10, 10, 0);
	}
	Log_i("FACEBLIT", "Style transfer time: " + std::to_string(t_styleBlit.elapsed_milliseconds()) + " ms");

	// --- FACE MASK -----------------------------------
	TimeMeasure timer;
	cv::Mat targetFaceMask = getSkinMask(targetImg, targetLandmarks);
	Log_i("FACEBLIT", "T face mask time: " + std::to_string(timer.elapsed_milliseconds()) + " ms");
	timer.reset();

	// --- ALPHA BLEND ---------------------------------
	TimeMeasure t_blend;
	cv::Mat alphaBlendResult;
	if (stylizeBG)
		alphaBlendResult = alphaBlendFG_BG(stylizedImg, stylizedImgNoApp, targetFaceMask, 25.0); // blend the face into the stylized image without appearance
	else
		alphaBlendResult = alphaBlendFG_BG(stylizedImg, targetImg, targetFaceMask, 25.0); // blend the face into the original target image
	Log_i("FACEBLIT", "Alpha blend time: " + std::to_string(t_blend.elapsed_milliseconds()) + " ms");

	Window::imgShow("Result", alphaBlendResult);
	cv::imwrite("TESTS\\" + outputName, alphaBlendResult);

	return true;
}

// ##################################################################################
// #		Add new style and create its resources (lookup table, landmarks)		#
// ##################################################################################

bool addNewStyle(const std::string& inputPath, const std::string& rawPath = "..\\app\\src\\main\\res\\raw\\", const std::string& drawablePath = "..\\app\\src\\main\\res\\drawable\\")
{
	// Load dlib model for landmark detection
	std::pair<cv::Rect, std::vector<cv::Point2i>> detectionResult; // face rectangle and landmarks
	DlibDetector dlibDetector("facemark_models\\shape_predictor_68_face_landmarks.dat"); // load model

	// Get a style name
	int startIdx = inputPath.find_last_of("\\") != std::string::npos ? inputPath.find_last_of("\\") + 1 : 0;
	std::string styleNameWithoutExtension = inputPath.substr(startIdx, inputPath.find_last_of(".") - startIdx);
	styleNameWithoutExtension.erase(std::remove(styleNameWithoutExtension.begin(), styleNameWithoutExtension.end(), '_'), styleNameWithoutExtension.end());
	styleNameWithoutExtension.erase(std::remove(styleNameWithoutExtension.begin(), styleNameWithoutExtension.end(), '-'), styleNameWithoutExtension.end());
	styleNameWithoutExtension.erase(std::remove(styleNameWithoutExtension.begin(), styleNameWithoutExtension.end(), ' '), styleNameWithoutExtension.end());
	std::transform(styleNameWithoutExtension.begin(), styleNameWithoutExtension.end(), styleNameWithoutExtension.begin(), ::tolower);

	// --- Original resolution for the desktop testing ----------------------------------------
	cv::Mat styleImg = cv::imread(inputPath);
	if (styleImg.empty()) {
		Log_e("FACEBLIT", "Failed to read style image '" + inputPath + "'");
		return false;
	}
	if (styleImg.cols != 768 || styleImg.rows != 1024) {
		Log_e("FACEBLIT", "Wrong resolution! Please provide the image in resolution 768x1024 (width x height).");
		return false;
	}
	cv::imwrite(drawablePath + "style_" + styleNameWithoutExtension + ".png", styleImg);
	cv::Mat styleGpos = getGradient(styleImg.cols, styleImg.rows, false); // G_pos
	cv::Mat styleGapp = getAppGuide(styleImg, true); // G_app

	Log_i("FACEBLIT", "Generating lookup table for original resolution...this may take a while.");
	cv::Mat lookUpCube = getLookUpCube(styleGpos, styleGapp); // 3D table that maps Pos_Red, Pos_Green, App values to coordinates u, v in style
	saveLookUpCube(lookUpCube, rawPath + "lut_" + styleNameWithoutExtension + ".bytes");

	bool success = dlibDetector.detectFacemarks(styleImg, detectionResult);
	if (!success) {
		Log_e("FACEBLIT", "Landmark detection failed.");
		return false;
	}
	std::vector<cv::Point2i> landmarks = detectionResult.second;
	cv::Mat copyImg = styleImg.clone();
	CartesianCoordinateSystem::drawLandmarks(copyImg, landmarks);
	Window::imgShow("landmarks", copyImg);
	Log_i("FACEBLIT", "Check the precision of drawn landmarks in the window and press any key in that window to continue...");
	cv::waitKey(0);

	CartesianCoordinateSystem::savePointsIntoFile(landmarks, rawPath + "lm_" + styleNameWithoutExtension + ".txt");

	// --- Lower resolution for the Android app ------------------------------------------------
	cv::resize(styleImg, styleImg, cv::Size(480, 640));
	cv::imwrite(drawablePath + "style_" + styleNameWithoutExtension + "_480x640.png", styleImg);
	styleGpos = getGradient(styleImg.cols, styleImg.rows, false); // G_pos
	styleGapp = getAppGuide(styleImg, true); // G_app
	
	Log_i("FACEBLIT", "Generating lookup table for lower resolution...this may take a while.");
	lookUpCube = getLookUpCube(styleGpos, styleGapp); // 3D table that maps Pos_Red, Pos_Green, App values to coordinates u, v in style
	saveLookUpCube(lookUpCube, rawPath + "lut_" + styleNameWithoutExtension + "_480x640.bytes");

	for (int i = 0; i < landmarks.size(); i++)
		landmarks[i] *= 0.625;
	//copyImg = styleImg.clone();
	//CartesianCoordinateSystem::drawLandmarks(copyImg, landmarks);
	//Window::imgShow("landmarks", copyImg);
	
	CartesianCoordinateSystem::savePointsIntoFile(landmarks, rawPath + "lm_" + styleNameWithoutExtension + "_480x640.txt");

	// --- Save thumbnail for Android app -------------------------------------------------------
	cv::resize(styleImg, styleImg, cv::Size(300, 400));
	cv::imwrite(drawablePath + "recycler_view_" + styleNameWithoutExtension + ".jpg", styleImg);

	Log_i("FACEBLIT", "--------------------------------------------------------------");
	Log_i("FACEBLIT", "Double check landmark positions and fix them manually if needed. Style's landmarks have to be as precise as possible!");
	Log_i("FACEBLIT", "--------------------------------------------------------------");
	Log_i("FACEBLIT", "Copy the following code into the switch block of ResourceHelper.java file:");
	std::cout << std::endl;
	std::cout << "\tcase \"" << styleNameWithoutExtension << "\":" << std::endl;
	std::cout << "\t\tid_img = R.drawable.style_" << styleNameWithoutExtension << "_480x640;" << std::endl;
	std::cout << "\t\tid_lm = R.raw.lm_" << styleNameWithoutExtension << "_480x640;" << std::endl;
	std::cout << "\t\tid_lut = R.raw.lut_" << styleNameWithoutExtension << "_480x640;" << std::endl;
	std::cout << "\t\tbreak;" << std::endl;
	std::cout << std::endl;

	return true;
}



int main(int argc, char* argv[])
{
	const std::string rawPath = "..\\app\\src\\main\\res\\raw\\";			// path to styles landmarks and lookup tables
	const std::string drawablePath = "..\\app\\src\\main\\res\\drawable\\";	// path to style exemplars

	// *** PARAMETERS TO SET ***********************************************************************************************************
	const std::string targetPath = "TESTS\\";					// path to target images and videos
	const std::string targetName = "target2.png";				// name of a target PNG image or MP4 video with extension from drawablePath
	const std::string styleName = "style_watercolorgirl.png";	// name of a style PNG with extension from drawablePath
	const bool stylizeBG = false;								// true - stylized face with appearace is blended into the stylized target without appearance, false - stylized face is blended into the original target
	const int NNF_patchsize = 3;								// voting patch size (0 for no voting), ideal is 3 or 5; it has to be an ODD NUMBER!!!
	// *********************************************************************************************************************************

	bool success = false;

	
	// ADD A NEW STYLE (DETECT LANDMARKS & GENERATE LOOKUP TABLE)
	/*
	success = addNewStyle("C:\\Users\\Aneta\\Pictures\\styles\\abstract.png");
	if (!success) {
		Log_e("FACEBLIT", "Adding of the new style failed.");
		system("pause");
		return -1;
	}
	else {
		Log_i("FACEBLIT", "The new style added successfully.");
		system("pause");
		return 0;
	}
	*/


	if (targetName.find(".png") != std::string::npos)
		success = getStylizedImage(rawPath, drawablePath, targetPath, targetName, styleName, stylizeBG, NNF_patchsize); // Result is saved into "VS/TESTS"

	if (targetName.find(".mp4") != std::string::npos)
		success = getStylizedVideo(rawPath, drawablePath, targetPath, targetName, styleName, stylizeBG, NNF_patchsize); // Result is saved into "VS/TESTS"

	if (!success) {
		Log_e("FACEBLIT", "Stylization failed.");
		system("pause");
		return -1;
	}

	Log_i("FACEBLIT", "Stylization succeeded. See the result in 'VS/TESTS'");
	system("pause");
	return 0;
}







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

cv::Mat cropWidth(const cv::Mat& image, const int dstWidth, const int centerX)
{
	int x = std::max(0, centerX - dstWidth / 2);
	if (x + dstWidth > image.cols)
		x = image.cols - dstWidth;

	return cv::Mat(image, cv::Rect(x, 0, dstWidth, image.rows));
}

cv::Mat extendHeight(const cv::Mat& image, const int dstHeight)
{
	cv::Mat result = cv::Mat(dstHeight, image.cols, image.type());
	int totalRowsToAdd = dstHeight - image.rows;
	cv::Mat firstRow = image.row(0);
	cv::Mat lastRow = image.row(image.rows - 1);

	// duplicate the first row (totalRowsToAdd / 2)-times
	for (int i = 0; i < totalRowsToAdd / 2; i++)
		firstRow.copyTo(result.row(i));

	// copy the original image under the duplicated first rows
	image.copyTo(result(cv::Rect(0, totalRowsToAdd / 2, image.cols, image.rows)));

	// duplicate the last row (totalRowsToAdd / 2)-times
	for (int i = totalRowsToAdd / 2 + image.rows; i < result.rows; i++)
		lastRow.copyTo(result.row(i));

	return result;
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