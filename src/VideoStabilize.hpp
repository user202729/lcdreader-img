#include<opencv2/core.hpp>
#include<vector>

namespace stab{
	extern cv::Mat ref_image;
	extern std::vector<cv::Point2d> ref_corners;
	extern std::vector<cv::Point2f> ref_features;

	extern cv::Mat next_image;
	extern std::vector<cv::Point2f> next_features;
	extern std::vector<cv::Point2d> next_corners;
	extern std::vector<uint8_t> status;

	void setRefImage(cv::Mat image_); // use current grid corners
	double computeGridSpacing(); // estimation, assuming it's approximately rectangular
	bool largeMovement(std::vector<cv::Point2f> const& a,std::vector<cv::Point2f> const& b,double threshold);
	void computeNextImageFeatures();
	void computeNextImageCorners();
	void setGridCornersToNextImage();
	void setNextImageAsRef();
}
