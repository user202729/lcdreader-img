#include"Grid.h"

#include<iostream>
#include<array>
#include<cassert>

#include<opencv2/imgproc.hpp>
#include<opencv2/calib3d.hpp>

Grid::Grid():corners(4),anchor_src(4),transform_cached(),binarize_cached(){
	setGridSize(31,48);
}

void Grid::setCorner(int index,cv::Point point){
	corners[index]=point;
	binarize_cached=false;
	transform_cached=false;
}

void Grid::addAnchor(cv::Point2d src){
	computeTransform();

	anchor_src.push_back(src);
	std::array<cv::Point2d,1> arr{src};
	cv::perspectiveTransform(arr,arr,transform.inv());
	corners.push_back(arr[0]);
	// assume the transformation is correct after this operation
}

void Grid::setGridSize(int a,int b){
	assert(a>0);
	assert(b>0);
	maxA=a;maxB=b;

	anchor_src[0]={   0,   0};
	anchor_src[1]={   0,maxA};
	anchor_src[2]={maxB,   0};
	anchor_src[3]={maxB,maxA};

	binarize_cached=false;
	transform_cached=false;

	data.create(a,b,CV_8S);
	data_manual=cv::Mat::ones(a,b,CV_8S)*-1;
}

void Grid::setImage(cv::Mat const image_){
	binarize_cached=false;
	image=image_;
}

void Grid::drawBox(cv::Mat image){
	cv::line(image,corners[0],corners[1],cv::Scalar(0,0,0));
	cv::line(image,corners[0],corners[2],cv::Scalar(0,0,0));
	cv::line(image,corners[3],corners[1],cv::Scalar(0,0,0));
	cv::line(image,corners[3],corners[2],cv::Scalar(0,0,0));
}

void Grid::drawAnchorInput(cv::Mat image){
	for(auto p:corners)
		cv::circle(image,p,3,cv::Scalar(255,255,0),-1 /* filled */);
}

void Grid::drawAnchorTransformed(cv::Mat image){
	assert(image.rows%maxA==0);
	assert(image.cols%maxB==0);
	int const factor=image.rows/maxA;
	assert(factor==image.cols/maxB);

	int const radius=std::min(factor,3);
	for(unsigned i=4;i<anchor_src.size();++i)
		cv::circle(image,anchor_src[i]*factor,
				radius,cv::Scalar(255,255,0),-1);
}

void Grid::drawGrid(cv::Mat image){
	cv::Scalar color(0,0,0);
	float factor;
	factor=(float)image.rows/maxA;
	for(int a=1;a<maxA;++a)
		cv::line(image,{0,int(a*factor)},{image.cols,int(a*factor)},color);
	factor=(float)image.cols/maxB;
	for(int b=1;b<maxB;++b)
		cv::line(image,{int(b*factor),0},{int(b*factor),image.rows},color);
}

cv::Mat Grid::extractScreen(double zoom_factor){
	computeTransform();
	assert(zoom_factor>0);

	auto transform_scaled=transform.clone();
	for(int r=0;r<2;++r)
	for(int c=0;c<3;++c)
		transform_scaled.at<double>(r,c)*=zoom_factor;

	cv::Mat result;
	int h=int(maxA*zoom_factor);
	int w=int(maxB*zoom_factor);
	cv::warpPerspective(image,result,transform_scaled,{w,h});
	return result;
}

void Grid::computeTransform(){
	if(transform_cached)
		return;

	transform=cv::findHomography(corners,anchor_src);
	transform_cached=true;
}

cv::Mat Grid::drawPreview(cv::Mat const image,double alpha){
	assert(alpha>=0&&alpha<=1);
	assert(binarize_cached);

	cv::Mat dest(maxA,maxB,CV_8UC3);
	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b)
		dest.at<cv::Vec3b>(a,b)=getData(a,b)?cv::Vec3b(255,0,0):cv::Vec3b(255,255,255);
	cv::resize(dest,dest,image.size(),0,0,cv::INTER_NEAREST);

	cv::addWeighted(dest,alpha,image,1-alpha,0,dest);
	return dest;
}

void Grid::binarize(){
	if(binarize_cached)
		return;

	assert(data.rows==maxA);
	assert(data.cols==maxB);
	data=cv::Mat::zeros(maxA,maxB,data.type());

	std::cout<<"Temporarily unimplemented, sorry\n";

	binarize_cached=true;
}

/*
void Grid::changeEdgeThreshold(int delta){
	if(edge_threshold+delta<0||edge_threshold+delta>255)
		return;
	edge_threshold+=delta;
	binarize_cached=false;
}
*/

void Grid::setDataManual(int a,int b,int8_t value){
	assert(0<=a&&a<maxA);
	assert(0<=b&&b<maxB);
	data_manual.at<int8_t>(a,b)=value;
}

signed char Grid::getData(int a,int b)const{
	assert(binarize_cached);
	auto d=data_manual.at<int8_t>(a,b);
	if(d<0)
		d=data.at<int8_t>(a,b);
	return d;
}
