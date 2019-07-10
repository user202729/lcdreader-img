#include"Grid.hpp"

#include<iostream>
#include<array>
#include<cassert>
#include<cmath>

#include<opencv2/imgproc.hpp>
#include<opencv2/calib3d.hpp>

Grid::Grid():corners(4),anchor_src(4),use_distort(false),
	transform_cached(),binarize_cached(){
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

	if(!std::isnan(camera_matrix(0,0))){
		// reverse of undistortPoints(arr,arr) -- https://stackoverflow.com/a/35016615
		cv::undistortPoints(arr,arr,camera_matrix,cv::Mat());
		std::array<cv::Point3d,1> arr3d;
		cv::convertPointsToHomogeneous(arr,arr3d);
		cv::projectPoints(arr3d,cv::Matx31d{},cv::Matx31d{},
				camera_matrix,dist_coeffs,arr);
	}

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

	cv::Mat result;
	if(std::isnan(camera_matrix(0,0)))
		result=image.clone();
	else
		cv::undistort(image,result,camera_matrix,dist_coeffs);

	auto transform_scaled(transform);
	for(int r=0;r<2;++r)
	for(int c=0;c<3;++c)
		transform_scaled(r,c)*=zoom_factor;

	cv::warpPerspective(result,result,transform_scaled,
			{int(maxB*zoom_factor),int(maxA*zoom_factor)});
	return result;
}

void Grid::computeTransform(){
	if(transform_cached)
		return;

	std::vector<cv::Point2f> corners_(begin(corners),end(corners));

	if(use_distort){
		std::vector<cv::Point3f> anchor_src_3d(begin(anchor_src),end(anchor_src));

		std::vector<cv::Mat> rvecs,tvecs; // TODO use those
		(void)cv::calibrateCamera(
				std::vector<std::vector<cv::Point3f>>{anchor_src_3d},
				std::vector<std::vector<cv::Point2f>>{corners_},
				image.size(),
				camera_matrix,dist_coeffs,rvecs,tvecs);

		std::vector<cv::Point2f> corners_undistorted;
		if(std::isnan(camera_matrix(0,0)))
			corners_undistorted=corners_;
		else
			cv::undistortPoints(corners_,corners_,camera_matrix,dist_coeffs,
					cv::noArray(),camera_matrix);
	}else{
		camera_matrix(0,0)=std::nan("");
	}

	transform=cv::findHomography(corners_,anchor_src);

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

	int const factor=7; // %2 != 0
	cv::Mat img=extractScreen(factor);
	cv::cvtColor(img,img,cv::COLOR_BGR2GRAY);
	cv::adaptiveThreshold(img,img,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,
			cv::THRESH_BINARY,factor*9,2);
	cv::dilate(img,img,cv::Mat());
	cv::erode(img,img,cv::Mat());

	cv::resize(img,img,{},1./factor,1./factor,cv::INTER_AREA);
	cv::threshold(img,data,
			127,
			255,cv::THRESH_BINARY_INV);

	assert(data.rows==maxA);
	assert(data.cols==maxB);
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

void Grid::setUndistort(bool value){
	use_distort=value;
	binarize_cached=false;
	transform_cached=false;
}
