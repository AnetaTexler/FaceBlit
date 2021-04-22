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
#include <opencv2/core.hpp>

#include <opengl_main.hpp>




int xxxmain() {
	// Don't forget to change this path before you start.
	const std::string root = "C:\\Work\\TestDataset\\Videos";
	const std::string drawablePath = "..\\app\\src\\main\\res\\drawable\\";
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
	cv::Mat styleImgBody_full = cv::imread(drawablePath + styleNameNoExtension + "_body.png", cv::IMREAD_UNCHANGED);
	cv::Mat styleImgExternalMask = cv::imread(drawablePath + styleNameNoExtension + "_mask.png");
	cv::Mat styleImgHairMask = cv::imread(drawablePath + styleNameNoExtension + "_hair_mask.png");

	// Removing hair from the head mask.
	cv::subtract(styleImgExternalMask, styleImgHairMask, styleImgExternalMask);

	//cv::Mat styleImgExternalMask = cv::imread(root + "\\styles\\" + styleNameNoExtension + "_mask_mock.png");
	cv::Mat styleImgBackground = cv::imread(drawablePath + styleNameNoExtension + "_bg.png");
	std::string styleLandmarkStr((std::istreambuf_iterator<char>(styleLandmarkFile)), std::istreambuf_iterator<char>());
	styleLandmarkFile.close();
	cv::Mat lookUpCube = loadLookUpCube(root + "\\styles\\" + styleNameNoExtension + "_lut.bytes");
	// -----------------------------------------------------------------------
	cv::Mat stylePosGuide = getGradient(styleImg.cols, styleImg.rows, false); // G_pos

	cv::Mat styleAppGuide = getAppGuide(styleImg, true); // G_app
	std::vector<cv::Point2i> styleLandmarks = getLandmarkPointsFromString(styleLandmarkStr.c_str());

	// Circle
	int radius = (styleLandmarks[45].x - styleLandmarks[36].x) * 0.7f; // distance of outer eye corners + 80%
	//styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 45.0));
	styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 90.0));
	//styleLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 135.0));
	// Add 2 landmarks into the bottom corners - to prevent the body moving during MLS deformation
	//styleLandmarks.push_back(cv::Point2i(0, styleImg.rows)); // left bottom corner
	//styleLandmarks.push_back(cv::Point2i(styleImg.cols, styleImg.rows)); // right bottom corner

	float x_max = 0.0f;
	float y_max = 0.0f;
	float x_min = 1.0f;
	float y_min = 1.0f;

	for (int k = 60; k < styleLandmarks.size(); ++k) {
		float x = float(styleLandmarks[k].x) / styleImg.cols;
		float y = -1.0f * (float(styleLandmarks[k].y) / styleImg.rows) + 1.0f;

		if (x_max < x) x_max = x;
		if (x_min > x) x_min = x;
		if (y_max < y) y_max = y;
		if (y_min > y) y_min = y;
	}

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
	cv::VideoWriter out(outputDirPath + "\\result.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), FPS, cv::Size(styleImg.cols*3, styleImg.rows));
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

	FB_OpenGL::Shader debug_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex_coords.vert", "..\\app\\src\\main\\opengl\\shader\\pass_tex.frag");
	FB_OpenGL::Shader styleblit_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex.vert", "..\\app\\src\\main\\opengl\\shader\\styleblit_main.frag");
	FB_OpenGL::Shader blending_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex.vert", "..\\app\\src\\main\\opengl\\shader\\styleblit_blend.frag");
	FB_OpenGL::Shader mixing_shader = FB_OpenGL::Shader("..\\app\\src\\main\\opengl\\shader\\pass_tex.vert", "..\\app\\src\\main\\opengl\\shader\\blending.frag");
	debug_shader.init();
	styleblit_shader.init();
	blending_shader.init();
	mixing_shader.init();

	FB_OpenGL::StyblitBlender quad = FB_OpenGL::StyblitBlender(&blending_shader);
	FB_OpenGL::StyblitRenderer styleblit_main = FB_OpenGL::StyblitRenderer(&styleblit_shader);
	FB_OpenGL::Grid grid = FB_OpenGL::Grid(32, 32, 0.0f, 1.0f, 0.0f, 0.5f, &debug_shader); // FIXNUTÉ NATVRDO!
	FB_OpenGL::Grid grid_hair = FB_OpenGL::Grid(32, 32, 0.1f, 0.9f, 0.2f, 0.98f, &debug_shader); // FIXNUTÉ NATVRDO!
	FB_OpenGL::Blending mixer = FB_OpenGL::Blending(&mixing_shader);

	styleblit_main.setWidthHeight( styleImg.cols, styleImg.rows);
	quad.setWidthHeight(styleImg.cols, styleImg.rows);
	mixer.setWidthHeight(styleImg.cols, styleImg.rows);

	//GLuint quad_texture = FB_OpenGL::makeTexture(styleImg);
	//quad.setTextureID( &quad_texture );

	// ************************ OPENGL VARIABLES ******************
	GLuint stylePosGuide_texture = 0;
	GLuint targetPosGuide_texture = 0;
	GLuint styleAppGuide_texture = 0;
	GLuint targetAppGuide_texture = 0;
	GLuint styleImg_texture = 0;
	GLuint lookUpTableTexture = FB_OpenGL::make3DTexture(lookUpCube);
	GLuint faceMask_texture = 0;
	GLuint bodyMask_texture = 0;
	GLuint body_texture = 0;
	GLuint externalMask_texture = 0;
	GLuint background_texture = 0;
	GLuint hairmask_texture = 0;

	// Setup framebuffer for styleblit to make it render to a texture which we can further process.
	GLuint styleblit_frame_buffer, styleblit_render_buffer_depth_stencil, styleblit_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, styleblit_frame_buffer, styleblit_render_buffer_depth_stencil, styleblit_tex_color_buffer);
	GLuint blending_frame_buffer, blending_render_buffer_depth_stencil, blending_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, blending_frame_buffer, blending_render_buffer_depth_stencil, blending_tex_color_buffer);
	GLuint body_frame_buffer, body_render_buffer_depth_stencil, body_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, body_frame_buffer, body_render_buffer_depth_stencil, body_tex_color_buffer);
	GLuint bodymask_frame_buffer, bodymask_render_buffer_depth_stencil, bodymask_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, bodymask_frame_buffer, bodymask_render_buffer_depth_stencil, bodymask_tex_color_buffer);
	GLuint facemask_frame_buffer, facemask_render_buffer_depth_stencil, facemask_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, facemask_frame_buffer, facemask_render_buffer_depth_stencil, facemask_tex_color_buffer);
	GLuint hair_frame_buffer, hair_render_buffer_depth_stencil, hair_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, hair_frame_buffer, hair_render_buffer_depth_stencil, hair_tex_color_buffer);
	GLuint hairmask_frame_buffer, hairmask_render_buffer_depth_stencil, hairmask_tex_color_buffer;
	FB_OpenGL::makeFrameBuffer(globalOpenglData.w_width, globalOpenglData.w_height, hairmask_frame_buffer, hairmask_render_buffer_depth_stencil, hairmask_tex_color_buffer);


	quad.setTextures(&styleblit_tex_color_buffer, &styleImg_texture);
	// GLuint frame_as_texture = 0;

	// Generate jitter table
	cv::Mat gaussian_noise = cv::Mat::zeros(styleImg.rows, styleImg.cols, CV_8UC2);
	cv::randu(gaussian_noise, 0, 255);
	
	GLuint jitter_table_texture = FB_OpenGL::makeJitterTable(gaussian_noise.clone());
	styleblit_main.setJitterTable(&jitter_table_texture);

	TimeMeasure tm;
	tm.reset();

	std::vector<int> landmarkControlPointsIDs;
	std::vector<int> hairControlPointsIDs;

	for (int k = 0; k < 32; ++k) {
		grid.vertexDeformations[k].fixed = true;
	}

	// Currently used landmarks are the bottom left and bottom right corners, three points of the notional circle, and the mouth facial landmarks.
	for (int k = 0; k < styleLandmarks.size(); ++k) {
	//for (auto landmark : styleLandmarks) {
		cv::Point2i landmark = styleLandmarks[k];
		int cp = grid.getNearestControlPointID( 2.0f * (float(landmark.x) / float(styleImg.cols)) - 1.0f, -1.0f * (2.0f * (float(landmark.y) / float(styleImg.rows)) - 1.0f));
		// grid.vertexDeformations[cp].fixed = true;
		landmarkControlPointsIDs.push_back(cp);
	}

	std::vector<int> selected_landmark_ids;
	selected_landmark_ids.push_back(30); // nose tip
	selected_landmark_ids.push_back(60); // lip left corner
	selected_landmark_ids.push_back(64); // lip right corner
	selected_landmark_ids.push_back(68); // throat

	std::vector<cv::Point2i> hair_markers;
	/*hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 45.0));
	hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 90.0));
	hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(styleLandmarks.begin() + 48, styleLandmarks.begin() + 67)), radius, 135.0));*/
	std::vector<cv::Point2i> toAverage;
	toAverage.push_back(styleLandmarks[36]);
	toAverage.push_back(styleLandmarks[45]);
	radius = (styleLandmarks[45].x - styleLandmarks[36].x) * 1.0f; 
	hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 270.0));
	hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 315.0));
	hair_markers.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 225.0));

	hair_markers.push_back(styleLandmarks[0]);
	hair_markers.push_back(styleLandmarks[1]);
	hair_markers.push_back(styleLandmarks[2]);
	hair_markers.push_back(styleLandmarks[3]);
	hair_markers.push_back(styleLandmarks[4]);
	hair_markers.push_back(styleLandmarks[5]);

	hair_markers.push_back(styleLandmarks[11]);
	hair_markers.push_back(styleLandmarks[12]);
	hair_markers.push_back(styleLandmarks[13]);
	hair_markers.push_back(styleLandmarks[14]);
	hair_markers.push_back(styleLandmarks[15]);
	hair_markers.push_back(styleLandmarks[16]);

		// Currently used landmarks are the bottom left and bottom right corners, three points of the notional circle, and the mouth facial landmarks.
	for (int k = 0; k < hair_markers.size(); ++k) {
		//for (auto landmark : styleLandmarks) {
		cv::Point2i landmark = hair_markers[k];
		int cp = grid_hair.getNearestControlPointID(2.0f * (float(landmark.x) / float(styleImg.cols)) - 1.0f, -1.0f * (2.0f * (float(landmark.y) / float(styleImg.rows)) - 1.0f));
		// grid.vertexDeformations[cp].fixed = true;
		hairControlPointsIDs.push_back(cp);
	}
	// Image processing

	cv::Mat bodyChannels[4];
	cv::split(styleImgBody_full, bodyChannels);
	
	cv::Mat styleImgBody;
	cv::Mat styleImgBodyMask;
	cv::cvtColor(styleImgBody_full, styleImgBody, cv::COLOR_BGRA2BGR);
	cv::cvtColor(bodyChannels[3], styleImgBodyMask, cv::COLOR_GRAY2BGR, 3);

	/*Window::imgShow("Result", styleImgBodyMask);
	cv::waitKey(0);*/

	/*std::vector<cv::Mat> mask_vector;
	mask_vector.push_back(bodyChannels[3]);
	mask_vector.push_back(bodyChannels[3]);
	mask_vector.push_back(bodyChannels[3]);

	cv::merge(mask_vector, styleImgBodyMask);*/
	// Window::imgShow("Result", styleImgBody);


	/*int bot_left = grid.getNearestControlPointID(-1.0f,-1.0f); 
	int bot_right = grid.getNearestControlPointID(1.0f, -1.0f);
	grid.vertexDeformations[bot_left].fixed = true;
	grid.vertexDeformations[bot_right].fixed = true;
	grid.vertexDeformations[150].fixed = true; // Debug.*/

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
		std::vector<cv::Point2i> targetLandmarks_temp = targetLandmarks;
		targetLandmarks = faceDetResult.second;
		// alignTargetToStyle(frame, targetLandmarks, styleLandmarks);


		
		// Add 3 landmarks on the bottom of the notional circle
		radius = (targetLandmarks[45].x - targetLandmarks[36].x) * 1.0f; // distance of outer eye corners + 80%
		//targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 45.0));
		targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 90.0));
		//targetLandmarks.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(std::vector<cv::Point2i>(targetLandmarks.begin() + 48, targetLandmarks.begin() + 67)), radius, 135.0));
		
		if (i > 1) {
			targetLandmarks[targetLandmarks.size() - 1].x = (targetLandmarks[targetLandmarks.size() - 1].x + targetLandmarks_temp[targetLandmarks.size() - 1].x) / 2;
			targetLandmarks[targetLandmarks.size() - 1].y = (targetLandmarks[targetLandmarks.size() - 1].y + targetLandmarks_temp[targetLandmarks.size() - 1].y) / 2;
		}

		//targetLandmarks.push_back(cv::Point2i(0, frame.rows)); // left bottom corner
		//targetLandmarks.push_back(cv::Point2i(frame.cols, frame.rows)); // right bottom corner

		//std::cout << landmarkControlPointsIDs.size() << " " << targetLandmarks.size() << std::endl;
		//for (int k = 60; k < landmarkControlPointsIDs.size(); ++k) {
		for (int k : selected_landmark_ids) {
			int cp = landmarkControlPointsIDs[k];
			grid.vertexDeformations[cp].fixed = true;
			grid.vertexDeformations[cp].x = 2.0f * (float(targetLandmarks[k].x) / float(frame.cols)) - 1.0f;
			grid.vertexDeformations[cp].y = -1.0 * (2.0f * (float(targetLandmarks[k].y) / float(frame.rows)) - 1.0f);
		}

		//hair_markers.clear();
		std::vector<cv::Point2i> hair_markers_target;
		toAverage.clear();
		toAverage.push_back(targetLandmarks[36]);
		toAverage.push_back(targetLandmarks[45]);
		radius = (targetLandmarks[45].x - targetLandmarks[36].x) * 0.9f;

		float angle_of_eyes = cv::fastAtan2(targetLandmarks[45].x - targetLandmarks[36].x, targetLandmarks[45].y - targetLandmarks[36].y);
		// std::cout << angle_of_eyes - 90.0f << std::endl;
		//(targetLandmarks[45].x - targetLandmarks[36].x)/(targetLandmarks[45].y - targetLandmarks[36].y)

		hair_markers_target.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 270.0 - angle_of_eyes + 90.0f));
		hair_markers_target.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 315.0 - angle_of_eyes + 90.0f));
		hair_markers_target.push_back(CartesianCoordinateSystem::getPointLyingOnCircle(CartesianCoordinateSystem::getAveragePoint(toAverage), radius, 225.0 - angle_of_eyes + 90.0f));


		hair_markers_target.push_back(targetLandmarks[0]);
		hair_markers_target.push_back(targetLandmarks[1]);
		hair_markers_target.push_back(targetLandmarks[2]);
		hair_markers_target.push_back(targetLandmarks[3]);
		hair_markers_target.push_back(targetLandmarks[4]);
		hair_markers_target.push_back(targetLandmarks[5]);

		hair_markers_target.push_back(targetLandmarks[11]);
		hair_markers_target.push_back(targetLandmarks[12]);
		hair_markers_target.push_back(targetLandmarks[13]);
		hair_markers_target.push_back(targetLandmarks[14]);
		hair_markers_target.push_back(targetLandmarks[15]);
		hair_markers_target.push_back(targetLandmarks[16]);

		for (int k = 0; k < hair_markers_target.size(); ++k) {
			int cp = hairControlPointsIDs[k];
			grid_hair.vertexDeformations[cp].fixed = true;
			grid_hair.vertexDeformations[cp].x = 2.0f * (float(hair_markers_target[k].x) / float(frame.cols)) - 1.0f;
			grid_hair.vertexDeformations[cp].y = -1.0 * (2.0f * (float(hair_markers_target[k].y) / float(frame.rows)) - 1.0f);
		}

		cv::Mat faceMask = getSkinMask(frame, targetLandmarks);

		// Generate target guidance channels
		cv::Mat targetPosGuide = MLSDeformation(stylePosGuide, styleLandmarks, targetLandmarks); // G_pos
		cv::Mat targetAppGuide = getAppGuide(frame, false); // G_app
		targetAppGuide = grayHistMatching(targetAppGuide, styleAppGuide);

		cv::Mat stylizedImg = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImg, cv::Rect2i(0, 0, frame.cols, frame.rows));
		cv::Mat stylizedMaskCPU = styleBlit(stylePosGuide, targetPosGuide, styleAppGuide, targetAppGuide, lookUpCube, styleImgExternalMask, cv::Rect2i(0, 0, frame.cols, frame.rows));

		i++;

		// std::cout << i << std::endl;

		// Make the Opencv Mat images into OpenGL accepted textures. On the 1st frame create the textures from scratch, after that we only need to update them to save memory.
		if (i > 2) {
			FB_OpenGL::updateTexture(targetPosGuide_texture, targetPosGuide.clone());
			FB_OpenGL::updateTexture(targetAppGuide_texture, targetAppGuide.clone());
			FB_OpenGL::updateTexture(faceMask_texture, faceMask.clone());
		}
		else {
			stylePosGuide_texture = FB_OpenGL::makeTexture(stylePosGuide.clone());
			targetPosGuide_texture = FB_OpenGL::makeTexture(targetPosGuide.clone());
			styleAppGuide_texture = FB_OpenGL::makeTexture(styleAppGuide.clone());
			targetAppGuide_texture = FB_OpenGL::makeTexture(targetAppGuide.clone());
			styleImg_texture = FB_OpenGL::makeTexture(styleImg.clone());
			styleblit_main.setTextures(&stylePosGuide_texture, &targetPosGuide_texture, &styleAppGuide_texture, &targetAppGuide_texture, &styleImg_texture, &lookUpTableTexture);
			faceMask_texture = FB_OpenGL::makeTexture(faceMask.clone());
			bodyMask_texture = FB_OpenGL::makeTexture(styleImgBodyMask.clone());
			body_texture = FB_OpenGL::makeTexture(styleImgBody.clone());
			externalMask_texture = FB_OpenGL::makeTexture(styleImgExternalMask.clone());
			background_texture = FB_OpenGL::makeTexture(styleImgBackground.clone());
			hairmask_texture = FB_OpenGL::makeTexture(styleImgHairMask.clone());

		}
		FB_OpenGL::updateTexture(blending_tex_color_buffer, stylizedImg.clone());
		FB_OpenGL::updateTexture(facemask_tex_color_buffer, stylizedMaskCPU.clone());

		// Bind the StyleBlit framebuffer, which will make all operations render in a texture.
		/*glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, styleblit_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		styleblit_main.draw();

		// ********************************************************************************************************************
		// ***************************************** STYLEBLIT NNF ************************************************************

		// Re-bind the default framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, blending_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		quad.setTextures(&styleblit_tex_color_buffer, &styleImg_texture);
		quad.draw();

		// *************************************************************************************************************************
		// ***************************************** STYLEBLIT BLENDING ************************************************************
		// Re-bind the default framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, facemask_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		quad.setTextures(&styleblit_tex_color_buffer, &externalMask_texture);
		quad.draw();*/


		// ***********************************************************************************************************************
		// ***************************************** BODY DEFORMATION ************************************************************
		// Re-bind the default framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, body_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// grid.vertexDeformations[150].x = sin(tm.elapsed_milliseconds() / 1000.0f);


		grid.setTextureID(&body_texture);
		grid.deformGrid(200);
		grid.draw(); // Draw grid with deformations.


		// ****************************************************************************************************************************
		// ***************************************** BODY DEFORMATION MASK ************************************************************
		glBindFramebuffer(GL_FRAMEBUFFER, bodymask_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// grid.vertexDeformations[150].x = sin(tm.elapsed_milliseconds() / 1000.0f);


		grid.setTextureID(&bodyMask_texture);
		grid.draw(); // Draw grid with deformations.

		
		// ***********************************************************************************************************************
		// ***************************************** HAIR DEFORMATION ************************************************************
		glBindFramebuffer(GL_FRAMEBUFFER, hair_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// grid.vertexDeformations[150].x = sin(tm.elapsed_milliseconds() / 1000.0f);

		grid_hair.setTextureID(&styleImg_texture);
		grid_hair.deformGrid(300);
		grid_hair.draw(); // Draw grid with deformations.

		// ****************************************************************************************************************************
		// ***************************************** HAIR DEFORMATION MASK ************************************************************
		glBindFramebuffer(GL_FRAMEBUFFER, hairmask_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// grid.vertexDeformations[150].x = sin(tm.elapsed_milliseconds() / 1000.0f);

		grid_hair.setTextureID(&hairmask_texture);
		grid_hair.draw(); // Draw grid with deformations.


		// ***************************************************************************************************************************************
		// ***************************************** FINAL MIXING AND PRINT TO SCREEN ************************************************************
		// Re-bind the default framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDepthFunc(GL_ALWAYS);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mixer.draw(body_tex_color_buffer, blending_tex_color_buffer, bodymask_tex_color_buffer, facemask_tex_color_buffer, background_texture, faceMask_texture, hair_tex_color_buffer, hairmask_tex_color_buffer);


		// ******************************************************************************************************************************************
		// ***************************************** FINAL MIXING AND PRINT INTO TEXTURE ************************************************************
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, styleblit_frame_buffer);
		glDepthFunc(GL_ALWAYS);
		glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mixer.draw(body_tex_color_buffer, blending_tex_color_buffer, bodymask_tex_color_buffer, facemask_tex_color_buffer, background_texture, faceMask_texture, hair_tex_color_buffer, hairmask_tex_color_buffer);

		SDL_GL_SwapWindow(globalOpenglData.mainWindow);

		// Uncomment if you want to save the output NNF. Current implementation is leaking a bit, so be careful.
		//if ( i == 99) {
			cv::Mat out_image = FB_OpenGL::get_ocv_img_from_gl_img(styleblit_tex_color_buffer);
			cv::Mat out_image_concat;
		std::vector<cv::Point2i> selected_markers;
		for (auto landmark_id : selected_landmark_ids) {
			selected_markers.push_back(targetLandmarks[landmark_id]);
		}
			//CartesianCoordinateSystem::drawRainbowLandmarks(frame, hair_markers_target);
			cv::hconcat(styleImg, out_image, out_image_concat);
			cv::hconcat(out_image_concat, frame, out_image_concat);
			out << out_image_concat;
			Window::imgShow("Result", out_image_concat);
			//cv::imwrite(root + "\\styles\\" + std::to_string(i) + "_body_stylized.png", out_image);
		//}

	}
	
	return 0;
}