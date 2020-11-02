#ifndef STYLE_CACHE_65856225556
#define STYLE_CACHE_65856225556

#include <opencv2/core.hpp>
#include <vector>

//class FacemarkDetector; // Forward Declaration instead of #include FacemarkDetector.h
class DlibDetector;


class StyleCache
{
    public:
        std::vector<cv::Point2i> styleLandmarks;
        cv::Mat styleImg;
        cv::Mat stylePosGuide;
        cv::Mat styleAppGuide;
        cv::Mat lookUpCube;
        //FacemarkDetector* facemarkDetector; // from openCV
        DlibDetector* dlibDetector;

    public:
        static StyleCache& getInstance()
        {
            static StyleCache instance;   
            return instance;
        }
    private:
        StyleCache() {}

    public:
        StyleCache(StyleCache const&)      = delete;
        void operator=(StyleCache const&)  = delete;
};

#endif //STYLE_CACHE_65856225556
