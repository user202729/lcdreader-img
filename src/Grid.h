#pragma once

#include<array>
#include<cassert>
#include<vector>
#include<SDL.h>

#include"Point.h"

struct Grid{
	SDL_Point getCorner(int index) const {return corners[index];}
	void setCorner(int index,SDL_Point);

	int getMaxA() const{return maxA;}
	int getMaxB() const{return maxB;}

	/// Bilinear interpolation: [0..maxA] * [0..maxB] -> quadrilateral formed by corners
	/// matrix (a points down, b points right) --P--> SDL (x points right, y points down)
	Point P(double a,double b);
	Point P(Point ab){return P(ab.x,ab.y);};

	/// Inverse transformation of P
	Point invP(Point p);

	void draw(SDL_Renderer* renderer);
	void draw_preview(SDL_Renderer* renderer,int preview_alpha);

	/// Get pixel values from the SDL_Surface.
	void compute_pxval(SDL_Surface*);

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
	std::array<SDL_Point,4> corners; // ul,dl,ur,dr
	int maxA=12,maxB=56;

	std::vector<std::vector<Uint8>> pxval;
	bool pxval_cached=true;

	std::vector<std::vector<Uint8>> medColor;
	bool med_color_cached=true;

	std::vector<std::vector<std::vector<SDL_Point>>> ptListFull,ptlist;
	bool ptlist_cached=true;

	/// Binarized value correspond to the pixels, in row-major order.
	/// Light (high value): 0, dark (low value): 1.
	std::vector<signed char> data,data_manual;
	int edge_threshold=13;
	bool binarize_cached=true;
};
