#ifndef TIME_HELPER_16892876612
#define TIME_HELPER_16892876612

#include <iostream>
#include <chrono>

/**
* \class Timer
*
* \brief Utility for time measure
*/
class TimeMeasure
{
public:

	/**
	* \brief Create and start timer
	*/
	TimeMeasure() : beg_(std::chrono::high_resolution_clock::now()) {}

	/**
	* \brief Reset timer
	*/
	void reset()
	{
		beg_ = std::chrono::high_resolution_clock::now();
	}

	/**
	* \brief Get elapsed milliseconds.
	* By calling this method timer is neither reseted nor stopped.
	*/
	long long elapsed_milliseconds() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beg_).count();
	}

	/**
	* \brief Get FPS.
	* Call in each frame to get FPS.
	* By calling this method, timer is reseted.
	*/
	float getFPS()
	{
		long long dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beg_).count();
		this->reset();
		return 1000.0f / dt;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> beg_;
};

#endif //TIME_HELPER_16892876612