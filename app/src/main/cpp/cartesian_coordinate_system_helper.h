#ifndef CARTESIAN_COORDINATE_SYSTEM_HELPER_H
#define CARTESIAN_COORDINATE_SYSTEM_HELPER_H

#include "window_helper.h"
#include "common_utils.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <fstream>


#define PI acos(-1.0)


class CartesianCoordinateSystem
{
public:
	
	static cv::Point2i getAveragePoint(const std::vector<cv::Point2i>& points)
	{
		int sumX = 0;
		int sumY = 0;

		for (cv::Point2i point : points)
		{
			sumX += point.x;
			sumY += point.y;
		}

		return cv::Point2i(sumX / points.size(), sumY / points.size());
	}


	static void clampPointInsideImage(cv::Point& point, const cv::Size2i& size)
	{
		if (point.x < 0) point.x = 0;
		if (point.y < 0) point.y = 0;
		if (point.x > size.width) point.x = size.width;
		if (point.y > size.height) point.y = size.height;

		if (point.x < 0 || point.y < 0 || point.x > size.width || point.y > size.height) 
			Log_i("FACEBLIT", "A landmark [" + std::to_string(point.x) + ", " + std::to_string(point.y) + "] was clamped!!!");
	}


	static void clampPointInsideImage(int& x, int& y, const cv::Size2i& size)
	{
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > size.width) x = size.width;
		if (y > size.height) y = size.height;

		if (x < 0 || y < 0 || x > size.width || y > size.height)
			Log_i("FACEBLIT", "A landmark [" + std::to_string(x) + ", " + std::to_string(y) + "] was clamped!!!");
	}


	// Premise: 0 and 360 degrees lies at the positive part of the x axe (moving clockwise)
	static cv::Point2i getPointLyingOnCircle(const cv::Point2i& center, const int& radius, double theta, const cv::Size2i& size = cv::Size2i(768, 1024))
	{
		theta = theta * PI / 180; // convert degrees into radians

		int x = center.x + radius * cos(theta);
		int y = center.y + radius * sin(theta);
		
		clampPointInsideImage(x, y, size); // secure the coordinates to be inside the image

		return cv::Point2i(x, y);
	}


	static std::vector<cv::Point2i> recomputePointsDueToScale(const std::vector<cv::Point2i>& points, const float& scaleRatio, const cv::Point2i& origin, const cv::Size& dstSize)
	{
		std::vector<cv::Point2i> resultPoints;
		for (cv::Point2i point : points)
		{
			int x = round((point.x - origin.x) * scaleRatio + origin.x);
			int y = round((point.y - origin.y) * scaleRatio + origin.y);
			resultPoints.push_back(cv::Point2i(x, y));

			clampPointInsideImage(resultPoints[resultPoints.size() - 1], dstSize); // secure the coordinates to be inside the image
		}
		return resultPoints;
	}


	static std::vector<cv::Point2i> recomputePointsDueToTranslation(const std::vector<cv::Point2i>& points, const cv::Point2i& shift, const cv::Size& size)
	{
		std::vector<cv::Point2i> resultPoints;
		for (cv::Point2i point : points)
		{
			resultPoints.push_back(point + shift);

			clampPointInsideImage(resultPoints[resultPoints.size() - 1], size); // secure the coordinates to be inside the image
		}
		return resultPoints;
	}


	static std::vector<cv::Point2i> recomputePoints180Rotation(const std::vector<cv::Point2i>& points, const cv::Size size)
	{
		std::vector<cv::Point2i> resultPoints(68);
		for (int i = 0; i < 68; i++)
		{
			if (i >= 0 && i < 17)
				resultPoints[16 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 17 && i < 27) // eyebrows
				resultPoints[43 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 27 && i < 31) // nose
				resultPoints[i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 31 && i < 36) // nose
				resultPoints[66 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if ((i >= 36 && i < 40) || (i >= 42 && i < 46)) // eyes
				resultPoints[81 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if ((i >= 40 && i < 42) || (i >= 46 && i < 48)) // eyes
				resultPoints[87 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 48 && i < 55) // mouth
				resultPoints[102 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 55 && i < 60) // mouth
				resultPoints[114 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else if (i >= 60 && i < 65) // mouth
				resultPoints[124 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
			else // mouth
				resultPoints[132 - i] = cv::Point2i(size.width - points[i].x, size.height - points[i].y);
		}
		return resultPoints;
	}


	static cv::Rect2i getHeadAreaRect(const std::vector<cv::Point2i>& faceLandmarks, const cv::Size imgSize)
	{
		cv::Rect2i rect;
		int width = faceLandmarks[16].x - faceLandmarks[0].x;
		int higherY = faceLandmarks[0].y < faceLandmarks[16].y ? faceLandmarks[0].y : faceLandmarks[16].y;
		int height = faceLandmarks[8].y - (higherY - width / 2);
		rect.x = std::max(faceLandmarks[0].x - (width * 0.25), 0.0);
		rect.y = std::max((higherY - width / 2) - (height * 0.25), 0.0);
		rect.width = std::min(width * 1.5, (double)imgSize.width);
		rect.height = std::min(height * 1.5, (double)imgSize.height);

		return rect;
	}


	static std::vector<cv::Point> getContourPoints(cv::Mat& image, bool drawPoints = false)
	{
		cv::Mat imageCopy = image.clone();
		cv::cvtColor(image, imageCopy, cv::COLOR_BGR2GRAY);
		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(imageCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
		std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2) { // find the biggest contour (descending sort - the biggest is contours[0]) 
			return contourArea(c1, false) > contourArea(c2, false);
		});
		//cv::drawContours(hairMask, contours, -1, cv::Scalar(0, 255, 0), 3);
		if (drawPoints)
		{
			for (int i = 0; i < contours[0].size(); i += 20)
				cv::circle(image, contours[0][i], 4, cv::Scalar(0, 255, 0), 2);

			Window::imgShow("image", image);
		}

		return contours[0];
	}


	static float euclideanDistance(const cv::Point p1, const cv::Point p2)
	{
		return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
	}


	static std::vector<std::pair<cv::Point, float>> getDistancesOfEachPointFromOrigin(const std::vector<cv::Point>& points)
	{
		float cumDistance = 0;
		std::vector<std::pair<cv::Point, float>> result;

		for (int i = 0; i < points.size(); i++)
		{
			result.push_back(std::pair<cv::Point, float>(points[i], cumDistance));
			if (i + 1 < points.size())
				cumDistance += euclideanDistance(points[i], points[i + 1]);
		}

		return result;
	}


	static cv::Point interpolate(const cv::Point& A, const cv::Point& B, const float& distBI)
	{
		float distAB = euclideanDistance(A, B); // distance between the current point A and the next point B)
		float distAI = distAB - distBI; // distance between the A and the new point I, that will be interpolated between A and B
		int xI, yI;

		if (B.x > A.x)
			xI = floor(A.x + (distAI / distAB) * (B.x - A.x));
		else
			xI = ceil(B.x + (distBI / distAB) * (A.x - B.x));

		if (B.y > A.y)
			yI = floor(A.y + (distAI / distAB) * (B.y - A.y));
		else
			yI = ceil(B.y + (distBI / distAB) * (A.y - B.y));

		return cv::Point(xI, yI);
	}


	static std::vector<cv::Point> getContourPointsSubset(std::vector<cv::Point>& points, const int subsetSize, const cv::Mat& image)
	{
		points.push_back(points[0]);
		std::vector<std::pair<cv::Point, float>> pointsAndDistancesFromOrigin = getDistancesOfEachPointFromOrigin(points);
		std::vector<cv::Point> newPoints;
		float distanceBetweenNewPoints = pointsAndDistancesFromOrigin[pointsAndDistancesFromOrigin.size() - 1].second / subsetSize;
		float currDistance;
		std::pair<cv::Point, float> oldA = pointsAndDistancesFromOrigin[0];
		std::pair<cv::Point, float> oldB = pointsAndDistancesFromOrigin[1];
		int oldIndex = 0;

		for (int i = 0; i < subsetSize; i++)
		{
			currDistance = distanceBetweenNewPoints * i;
			while (currDistance < oldA.second || currDistance > oldB.second) // currDistance is not between points A, B -> shift A, B
			{
				oldA = pointsAndDistancesFromOrigin[++oldIndex];
				oldB = pointsAndDistancesFromOrigin[oldIndex + 1];
			}
			newPoints.push_back(interpolate(oldA.first, oldB.first, oldB.second - currDistance));
		}

		return newPoints;
	}


	static void drawRainbowLandmarks(cv::Mat& targetImg, const std::vector<cv::Point2i>& targetLandmarks, const std::string shape = "circles", const std::string path = "")
	{
		double r = 255, g = -25, b = 0;
		for (int i = 0; i < targetLandmarks.size(); i++)
		{
			if (i < 11) // 11x
			{
				g += 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(0, g, 255), 2); // g = 0, 25, 50, 75, 100, 125, 150, 175, 200, 225, 250
			}

			if (i >= 11 && i < 21) // 10x
			{
				r -= 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(0, 255, r), 2); // r = 230, 205, 180, 155, 130, 105, 80, 55, 30, 5
			}

			if (i >= 21 && i < 31) // 10x
			{
				b += 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(b, 255, 0), 2); // b = 25, 50, 75, 100, 125, 150, 175, 200, 225, 250 
			}

			if (i >= 31 && i < 41) // 10x
			{
				g -= 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(255, g, 0), 2); // g = 230, 205, 180, 155, 130, 105, 80, 55, 30, 5
			}

			if (i >= 41 && i < 51) // 10x
			{
				r += 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(255, 0, r), 2); // r = 30, 55, 80, 105, 130, 155, 180, 205, 230, 255
			}

			if (i >= 51 && i < 61) // 10x
			{
				b -= 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(b, 0, 255), 2); // b = 225, 200, 175, 150, 125, 100, 75, 50, 25, 0
			}

			if (i >= 61)
			{
				g += 25;
				cv::circle(targetImg, targetLandmarks[i], 4, cv::Scalar(0, g, 255), 2); // g = 30, 55, 80, 105, 130, 155, 180
			}

		}
		if (!path.empty())
			cv::imwrite(path, targetImg);
	}


	static void drawLandmarks(cv::Mat& targetImg, const std::vector<cv::Point2i>& targetLandmarks, const cv::Scalar& color = cv::Scalar(0, 255, 0), const std::string shape = "circles", const std::string path = "")
	{
		if (shape == "circles")
		{
			for (int i = 0; i < targetLandmarks.size(); i++)
				cv::circle(targetImg, targetLandmarks[i], 4, color, 2);
		}
		else if (shape == "lines")
		{
			for (int i = 0; i < targetLandmarks.size() - 1; i++)
			{
				if (i != 16 && i != 21 && i != 26 && i != 35 && i != 41 && i != 47 && i != 59) // skip lines between indices 16-17, 21-22, 26-27, 35-36, 41-42, 47-48, 59-60
					cv::line(targetImg, targetLandmarks[i], targetLandmarks[i + 1], color, 2);
			}
			// additional lines between indices 30-35, 36-41, 42-47, 48-59, 60-67
			cv::line(targetImg, targetLandmarks[30], targetLandmarks[35], color, 2);
			cv::line(targetImg, targetLandmarks[36], targetLandmarks[41], color, 2);
			cv::line(targetImg, targetLandmarks[42], targetLandmarks[47], color, 2);
			cv::line(targetImg, targetLandmarks[48], targetLandmarks[59], color, 2);
			cv::line(targetImg, targetLandmarks[60], targetLandmarks[67], color, 2);
		}

		if (!path.empty())
			cv::imwrite(path, targetImg);
	}


	static void savePointsIntoFile(const std::vector<cv::Point>& points, const std::string path)
	{
		std::ofstream file(path);
		if (file.is_open())
		{
			for (cv::Point point : points)
				file << std::to_string(point.x) << " " << std::to_string(point.y) << std::endl;
			file.close();
		}
		else 
			Log_e("FACEBLIT", "Unable to open file to write points.");
	}

};

#endif //CARTESIAN_COORDINATE_SYSTEM_HELPER_H