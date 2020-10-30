#ifndef WINDOW_HELPER_H
#define WINDOW_HELPER_H

#include <opencv2/highgui.hpp>


class Window
{
public:
	static void imgShow(const std::string& windowTitle, const cv::Mat& image)
	{
		cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
		cv::resizeWindow(windowTitle, image.cols, image.rows);
		imshow(windowTitle, image);
		cv::waitKey(1);
	}

	static void imgShow(const std::string& windowTitle, const cv::Mat& image, int windowWidth, int windowHeight)
	{
		cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
		cv::resizeWindow(windowTitle, windowWidth, windowHeight);
		imshow(windowTitle, image);
		cv::waitKey(1);
	}

	static void imgShow(const std::string& windowTitle, const cv::Mat& image, int windowWidth, int windowHeight, int moveX, int moveY)
	{
		cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
		cv::resizeWindow(windowTitle, windowWidth, windowHeight);
		cv::moveWindow(windowTitle, moveX, moveY);
		imshow(windowTitle, image);
		cv::waitKey(1);
	}
};

#endif //WINDOW_HELPER_H