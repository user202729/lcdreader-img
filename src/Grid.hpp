#pragma once

#include<vector>
#include<cassert>
#include<vector>

#include<opencv2/core.hpp>

struct Grid{
	Grid();

	// Also used for anchor points.
	std::vector<cv::Point> const& getCorners() const {return corners;}
	void setCorner(int index,cv::Point);

	// Add an anchor point.
	void addAnchor(cv::Point2d src);

	int getMaxA() const{return maxA;}
	int getMaxB() const{return maxB;}
	void setGridSize(int a,int b);

	/// Draw the bounding box. Not very accurate w.r.t. homographic transformation.
	void drawBox(cv::Mat image);

	/// Draw anchor points (including corners) on the input (not transformed) image.
	void drawAnchorInput(cv::Mat image);

	/// Draw anchor points (excluding corners) on the transformed image
	void drawAnchorTransformed(cv::Mat image);

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

	/** Set if camera distortion/undistortion should be used. Default false.
	 * In degenerate cases, the computed camera matrix may be wrong.
	 * Pinhole camera model is assumed.
	 */
	void setUndistort(bool);

private:
	std::vector<cv::Point> corners; // ul,dl,ur,dr, anchor dests
	std::vector<cv::Point> anchor_src; // 4 first items = corners
	int maxA,maxB;

	bool use_distort;
	// if std::isnan(camera_matrix(0,0)) then camera_matrix should not be used to
	// distort or undistort anything (treat those operations as no-op)
	cv::Matx33d transform,camera_matrix;
	cv::Mat dist_coeffs;
	bool transform_cached;
	void computeTransform();

	/// Binarized value correspond to the pixels, in row-major order.
	/// Light (high value): 0, dark (low value): 1.
	cv::Mat data,data_manual;
	bool binarize_cached;

	cv::Mat image;
};