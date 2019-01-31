#include"Grid.h"

#include<iostream>
#include<cassert>

#include<opencv2/imgproc.hpp>

void Grid::setCorner(int index,cv::Point point){
	corners[index]=point;
	med_color_cached=false;
	ptlist_cached=false;
	binarize_cached=false;
}

cv::Point2d Grid::P(double a,double b){
	cv::Point2d
		u = cv::Point2d(corners[1] - corners[0]) * a / maxA + cv::Point2d(corners[0]),
		d = cv::Point2d(corners[3] - corners[2]) * a / maxA + cv::Point2d(corners[2]);
	return u + (d - u) * b / maxB;
}

cv::Point2d Grid::invP(cv::Point2d p){
	int constexpr iter=5;

	double const px = p.x, py = p.y,
	   p1x = corners[0].x, p2x = corners[1].x, p3x = corners[3].x, p4x = corners[2].x,
	   p1y = corners[0].y, p2y = corners[1].y, p3y = corners[3].y, p4y = corners[2].y;

	// https://stackoverflow.com/a/18332009
	double ss = 0.5, tt = 0.5;
	for(int k=0;k<iter;++k){
		double r1 = p1x*(1-ss)*(1-tt) + p2x*ss*(1-tt) + p3x*ss*tt + p4x*(1-ss)*tt - px;
		double r2 = p1y*(1-ss)*(1-tt) + p2y*ss*(1-tt) + p3y*ss*tt + p4y*(1-ss)*tt - py;

		double J11 = -p1x*(1-tt) + p2x*(1-tt) + p3x*tt - p4x*tt;
		double J21 = -p1y*(1-tt) + p2y*(1-tt) + p3y*tt - p4y*tt;
		double J12 = -p1x*(1-ss) - p2x*ss + p3x*ss + p4x*(1-ss);
		double J22 = -p1y*(1-ss) - p2y*ss + p3y*ss + p4y*(1-ss);

		double inv_detJ = 1/(J11*J22 - J12*J21);

		ss = ss - inv_detJ*(J22*r1 - J12*r2);
		tt = tt - inv_detJ*(-J21*r1 + J11*r2);
	}

	return {ss*maxA,tt*maxB};
}

void Grid::compute_pxval(cv::Mat const image){
	med_color_cached=false;
	binarize_cached=false;

	pxval.resize(image.rows);
	for(int row=0;row<image.rows;++row){
		pxval[row].resize(image.cols);
		for(int col=0;col<image.cols;++col){
			auto color=image.at<cv::Vec3b>(row,col);
			pxval[row][col]=(color[0]+color[1]+color[2])/3;
		}
	}

	pxval_cached=true;
}

void Grid::draw(cv::Mat image){
	for (int b = 0; b <= maxB; ++b) { // draw vertical lines
		auto p1=P(0,b),p2=P(maxA,b);
		cv::line(image,p1,p2,cv::Scalar(0,0,0));
	}
	for (int a = 0; a <= maxA; ++a) { // draw horizontal lines
		auto p1=P(a,0),p2=P(a,maxB);
		cv::line(image,p1,p2,cv::Scalar(0,0,0));
	}
}

void Grid::draw_preview(cv::Mat image){
	binarize();
	computePtList();

	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b){
		cv::Scalar color;
		if(data_manual[a*maxB+b]>=0?data_manual[a*maxB+b]:data[a*maxB+b])
			color={255,0,0}; // blue
		else
			color={255,255,255}; // white

		cv::Point pt[4]={P(a,b),P(a+1,b),P(a+1,b+1),P(a,b+1)};
		cv::Point const* pts[1]={pt};
		int npts[1]={4};
		cv::fillPoly(image,pts,npts,1,color);
	}
}

cv::Mat Grid::draw_preview(cv::Mat const image,double alpha){
	assert(alpha>=0&&alpha<=1);

	cv::Mat dest;
	image.copyTo(dest);
	draw_preview(dest);

	cv::addWeighted(dest,alpha,image,1-alpha,0,dest);
	return dest;
}

void Grid::computePtList(){
	if(ptlist_cached)
		return;

	double constexpr min_border=0.3,max_border=0.7;

	ptlist.assign(maxA,std::vector<std::vector<cv::Point>>(maxB));

	int min_x=corners[0].x,max_x=corners[0].x;
	int min_y=corners[0].y,max_y=corners[0].y;
	for(unsigned i=1;i<corners.size();++i){
		min_x=std::min(min_x,corners[i].x);
		max_x=std::max(max_x,corners[i].x);
		min_y=std::min(min_y,corners[i].y);
		max_y=std::max(max_y,corners[i].y);
	}

	for(int x=min_x;x<max_x;++x)
	for(int y=min_y;y<max_y;++y){
		cv::Point2d point=invP({(double)x,(double)y});
		auto a=(int)std::floor(point.x),b=(int)std::floor(point.y);
		if(a<0||a>=maxA||b<0||b>=maxB)
			continue;
		if(
				point.x-a<min_border||point.x-a>max_border||
				point.y-b<min_border||point.y-b>max_border)
			continue;
		ptlist[a][b].push_back({x,y});
	}

	ptlist_cached=true;
}

void Grid::computeMedColor(){
	if(med_color_cached)
		return;

	computePtList();

	std::array<unsigned,256> cnt;
	medColor.resize(maxA);
	for(int a=0;a<maxA;++a){
		medColor[a].resize(maxB);
		for(int b=0;b<maxB;++b){
			std::fill(cnt.begin(),cnt.end(),0);
			int sum_pxv=0;
			uint8_t min_pxv=255,max_pxv=0;
			for(auto point:ptlist[a][b]){
				auto pxv=pxval[point.y][point.x];
				sum_pxv+=pxv;
				min_pxv=std::min(min_pxv,pxv);
				max_pxv=std::max(max_pxv,pxv);
				++cnt[pxv];
			}

			if(ptlist[a][b].size()==0){
				std::cout<<"empty cell\n";
				return;
			}

			auto x=ptlist[a][b].size()/2;
			for(unsigned r=0;;++r){
				if(cnt[r]>x){
					medColor[a][b]=r;
					break;
				}
				x-=cnt[r];
			}
		}
	}

	med_color_cached=true;
}

void Grid::binarize(){
	if(binarize_cached)
		return;

	computeMedColor();

	struct DiffPair{
		// a is 1 (darker), b is 0, their difference in color is delta
		int a,b,delta;
	};
	std::vector<DiffPair> adjacentPairs;
	for(int row=1;row<maxA;++row)
	for(int col=0;col<maxB;++col){
		int x1=row*maxB+col;
		int v1=medColor[row][col];
		int x2=(row-1)*maxB+col;
		int v2=medColor[row-1][col];
		if(v1>v2)
			adjacentPairs.push_back({x2,x1,v1-v2});
		else
			adjacentPairs.push_back({x1,x2,v2-v1});
	}
	for(int row=0;row<maxA;++row)
	for(int col=1;col<maxB;++col){
		int x1=row*maxB+col;
		int v1=medColor[row][col];
		int x2=row*maxB+(col-1);
		int v2=medColor[row][col-1];
		if(v1>v2)
			adjacentPairs.push_back({x2,x1,v1-v2});
		else
			adjacentPairs.push_back({x1,x2,v2-v1});
	}

	data.assign(maxA*maxB,-1);
	if(data_manual.size()!=data.size())
		data_manual.assign(data.size(),-1);
	/*
	std::sort(adjacentPairs.begin(),adjacentPairs.end(),[](DiffPair x,DiffPair y){
			return x.delta>y.delta;});
			*/

	for(DiffPair p:adjacentPairs)
	if(p.delta>=edge_threshold){
		data[p.a]=1;
		data[p.b]=0;
	}

	auto fill=[&](int a,int b){
		if(data[a]>=0&&data[b]<0)
			data[b]=data[a];
	};

	for(int row=1;row<maxA;++row)
		for(int col=0;col<maxB;++col)
			fill((row-1)*maxB+col,row*maxB+col);

	for(int row=maxA-1;row!=0;--row)
		for(int col=0;col<maxB;++col)
			fill(row*maxB+col,(row-1)*maxB+col);

	for(int row=0;row<maxA;++row){
		for(int col=maxB-1;col!=0;--col)
			fill(row*maxB+col,row*maxB+(col-1));
		for(int col=1;col<maxB;++col)
			fill(row*maxB+(col-1),row*maxB+col);
	}

	binarize_cached=true;
}

void Grid::changeEdgeThreshold(int delta){
	if(edge_threshold+delta<0||edge_threshold+delta>255)
		return;
	edge_threshold+=delta;
	binarize_cached=false;
}

void Grid::setDataManual(int index,char value){
	data_manual[index]=value;
}
