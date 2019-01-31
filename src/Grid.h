#pragma once

#include<array>
#include<cassert>
#include<vector>

#include<opencv2/core.hpp>

struct Grid{
	cv::Point getCorner(int index) const {return corners[index];}
	void setCorner(int index,cv::Point);

	int getMaxA() const{return maxA;}
	int getMaxB() const{return maxB;}

	/// Bilinear interpolation: [0..maxA] * [0..maxB] -> quadrilateral formed by corners
	/// matrix (a points down, b points right) --P--> (x points right, y points down)
	cv::Point2d P(double a,double b);
	cv::Point2d P(cv::Point2d ab){return P(ab.x,ab.y);};

	/// Inverse transformation of P
	cv::Point2d invP(cv::Point2d p);

	/// Draw the grid (horizontal/vertical black lines) on the image.
	void draw(cv::Mat image);

	/// Draw a preview of the resulting binarization on the image.
	void draw_preview(cv::Mat image);

	/// Same as above, but with transparency and does not modify input.
	cv::Mat draw_preview(cv::Mat const image,double alpha);

	/// Get pixel values from the cv::Mat.
	void compute_pxval(cv::Mat);

	/// Compute the median color in each cell.
	void computeMedColor();

	void computePtList();

	/// Compute (data) based on (medColor) and (edge_threshold).
	void binarize();
	void changeEdgeThreshold(int delta);
	signed char getData(int i) const{
		assert(binarize_cached);
		return data_manual[i]>=0?data_manual[i]:data[i];
	}
	signed char getData(int a,int b) const{ return getData(a*maxB+b); }

	void setDataManual(int index,char value);
	void setDataManual(int a,int b,char value){setDataManual(a*maxB+b,value);}

private:
	std::array<cv::Point,4> corners; // ul,dl,ur,dr
	int maxA=12,maxB=56;

	std::vector<std::vector<uint8_t>> pxval;
	bool pxval_cached=true;

	std::vector<std::vector<uint8_t>> medColor;
	bool med_color_cached=true;

	std::vector<std::vector<std::vector<cv::Point>>> ptlist;
	bool ptlist_cached=true;

	/// Binarized value correspond to the pixels, in row-major order.
	/// Light (high value): 0, dark (low value): 1.
	std::vector<signed char> data,data_manual;
	int edge_threshold=13;
	bool binarize_cached=true;
};
