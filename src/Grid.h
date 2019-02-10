#pragma once

#include<array>
#include<cassert>
#include<vector>

#include<opencv2/core.hpp>

struct Grid{
	Grid();

	cv::Point getCorner(int index) const {return corners[index];}
	void setCorner(int index,cv::Point);

	int getMaxA() const{return maxA;}
	int getMaxB() const{return maxB;}
	void setGridSize(int a,int b);

	// Draw the bounding box. Not very accurate w.r.t. homographic transformation.
	void drawBox(cv::Mat image);

	/// Same as above, but with transparency (draw onto another image) and
	/// zoom factor.
	cv::Mat drawPreview(cv::Mat const image,double alpha);

	/// Draw horizontal/vertical grid lines, excluding borders.
	void drawGrid(cv::Mat image);

	/// Use warpPerspective to extract the screen region.
	cv::Mat extractScreen(double zoom_factor);

	/// Set the image used by binarize.
	/// Note that if the image is modified, setImage must be called again.
	void setImage(cv::Mat const);

	/// Compute (data).
	void binarize();

	/*
	 * This is currently unused, because of a different binarization algorithm.
	 *
	 * void changeEdgeThreshold(int delta);
	 */

	signed char getData(int a,int b)const;

	void setDataManual(int a,int b,int8_t value);

private:
	std::array<cv::Point,4> corners; // ul,dl,ur,dr
	int maxA,maxB;

	cv::Mat transform;
	bool transform_cached;
	void computeTransform();

	/// Binarized value correspond to the pixels, in row-major order.
	/// Light (high value): 0, dark (low value): 1.
	cv::Mat data,data_manual;
	bool binarize_cached;

	cv::Mat image;
};
