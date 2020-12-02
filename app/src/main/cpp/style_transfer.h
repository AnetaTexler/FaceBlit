//
// Created by Aneta on 3/3/2019.
//

#ifndef STYLECAM_STYLIZEIMAGE_H
#define STYLECAM_STYLIZEIMAGE_H

#include <opencv2/core.hpp>

unsigned char* stylize(const char* modelPath, const char* styleLandmarkStr, unsigned char* cubeData, unsigned char* styleData, unsigned char* targetData, int width, int height, bool stylizeFaceOnly);
std::vector<cv::Point2i> getLandmarkPointsFromString(const char* landmarks);
void alignTargetToStyle(cv::Mat& targetImg, std::vector<cv::Point2i>& targetLandmarks, const std::vector<cv::Point2i>& styleLandmarks);
cv::Mat getGradient(int width, int height, bool drawGrid);
cv::Mat MLSDeformation(const cv::Mat& gradientImg, const std::vector<cv::Point2i> styleLandmarks, const std::vector<cv::Point2i> targetLandmarks);
cv::Mat getAppGuide(const cv::Mat& image, bool stretchHist);
cv::Mat getSkinMask(const cv::Mat& image, const std::vector<cv::Point2i>& landmarks);
cv::Mat alphaBlendFG_BG(cv::Mat foreground, cv::Mat background, cv::Mat alpha, float sigma);
cv::Mat grayHistMatching(cv::Mat I, cv::Mat R);
cv::Mat cumSum(cv::Mat & src);
cv::Mat& scanImageAndReduceC(cv::Mat& image, const unsigned char* const table);
cv::Mat getLookUpCube(const cv::Mat& stylePosGuide, const cv::Mat& styleAppGuide, const int lambdaPos = 10, const int lambdaApp = 2);
void saveLookUpCube(const cv::Mat& lookUpCube, const std::string& path);
cv::Mat loadLookUpCube(const std::string& path);
cv::Mat styleBlit(const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, const cv::Mat& lookUpCube, const cv::Mat& styleImg, const cv::Rect2i stylizationRangeRect, const int threshold = 50, const int lambdaPos = 10, const int lambdaApp = 2);
int getError(int stylePosPixelG, int stylePosPixelR, int targetPosPixelG, int targetPosPixelR, uchar styleAppPixel, uchar targetAppPixel, const int LAMBDA_POS, const int LAMBDA_APP);
void boxSeedGrow(int rowT, int colT, cv::Point2i styleSeedPoint, int boxSize, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, cv::Mat& resultImg, const cv::Mat& styleImg, cv::Mat1i& coveredPixels, int chunkNumber, const int threshold);
void DFSSeedGrow(cv::Point2i targetSeedPoint, cv::Point2i styleSeedPoint, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, cv::Mat& resultImg, const cv::Mat& styleImg, cv::Mat1i& coveredPixels, int chunkNumber, const int threshold, const int lambdaPos, const int lambdaApp);
void visualizeChunks(const cv::Mat1i& coveredPixels);
void chunkStatistics(const cv::Mat1i& coveredPixels);
bool pixelOutOfImageRange(cv::Point2i stylePixel, cv::Point2i targetPixel, const cv::Mat& image);
bool pixelIsNotCovered(cv::Point2i pixel, const cv::Mat1i& coveredPixels);

cv::Mat2i denoiseNNF(const cv::Mat2i& noisyNNF, const int patchsizeNNF);
cv::Mat votingOnRGB(const cv::Mat& style, const cv::Mat2i& NNF, const int patchsizeNNF);
cv::Mat votingOnNNF(const cv::Mat& style, const cv::Mat2i& NNF, const int patchsizeNNF);
void DFSSeedGrow_voting(cv::Point2i targetSeedPoint, cv::Point2i styleSeedPoint, const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, cv::Mat2i& NNF,/* const cv::Mat& styleImg,*/ cv::Mat1i& coveredPixels, int chunkNumber, const int threshold, const int lambdaPos, const int lambdaApp);
cv::Mat styleBlit_voting(const cv::Mat& stylePosGuide, const cv::Mat& targetPosGuide, const cv::Mat& styleAppGuide, const cv::Mat& targetAppGuide, const cv::Mat& lookUpCube, const cv::Mat& styleImg, const cv::Rect2i stylizationRangeRect, const int NNF_patchsize, const int threshold = 50, const int lambdaPos = 10, const int lambdaApp = 2);

#endif //STYLECAM_STYLIZEIMAGE_H
