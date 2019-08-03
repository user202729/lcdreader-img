#include"VideoStabilize.hpp"

#include<iostream>
#include<cassert>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/calib3d.hpp>
#include<opencv2/video.hpp>
#include"Grid.hpp"

extern Grid grid;
extern cv::Mat image;

int _cap_frame=0; // temp workaround

namespace stab{

	cv::Mat ref_image;
	std::vector<cv::Point2d> ref_corners;
	std::vector<cv::Point2f> ref_features;

	void setRefImage(cv::Mat image_){
		ref_image=image_;
		ref_corners=grid.getCorners();

		// compute ref_features to track
		cv::Mat_<uint8_t> mask(ref_image.rows,ref_image.cols,(uint8_t)255);
		{ // only take features outside the changing (digits) area
			std::vector<cv::Point2f> corners{
				{-1,-1},
				{-1,(float)grid.getMaxA()+1},
				{(float)grid.getMaxB()+1,(float)grid.getMaxA()+1},
				{(float)grid.getMaxB()+1,-1}
			};
			grid.reverseTransform(corners);
			cv::fillConvexPoly(mask,
					std::vector<cv::Point>(begin(corners),end(corners)),
					cv::Scalar(0));
		}

		cv::Mat gray;
		cv::cvtColor(ref_image,gray,cv::COLOR_BGR2GRAY);
		cv::goodFeaturesToTrack(gray,ref_features,
				15, // maxCorners
				0.5, // qualityLevel
				30, // minDistance
				mask);

		std::cerr<<"Detected "<<ref_features.size()<<" corners\n";


		if(0){ // DEBUG: Show the detected features
			cv::Mat i1=ref_image.clone();
			for(auto p:ref_features)
				cv::circle(i1,p,3,cv::Scalar(0,0,255),-1);
			cv::imshow("I1",i1);

			while(cv::waitKey(0)!='q');
		}
	}

	cv::Mat next_image;
	std::vector<cv::Point2f> next_features;
	std::vector<cv::Point2d> next_corners;
	std::vector<uint8_t> status;

	double computeGridSpacing(){
		std::array<cv::Point2d,3> pts{{
			{0,0},
			{0,1},
			{1,0}
		}};
		grid.reverseTransform(pts);
		return std::min(
				cv::norm(pts[0]-pts[1]),
				cv::norm(pts[0]-pts[2]));
	}

	bool largeMovement(std::vector<cv::Point2f> const& a,std::vector<cv::Point2f> const& b,double threshold){
		assert(a.size()==b.size());
		for(unsigned i=0;i<a.size();++i)
			if(cv::norm(a[i]-b[i])>threshold){
				return true;
			}

		return false;
	}

	void computeNextImageFeatures(){
		/// Returns true if there's a large movement (so it's necessary to
		//recompute the grid coordinate)

		assert(!ref_features.empty());
		cv::calcOpticalFlowPyrLK(ref_image,next_image,ref_features,
				next_features,status,cv::noArray());
		for(auto x:status)
			if(!x){
				std::cerr<<"Can't find optical flow. cap frame = "<<
					_cap_frame<<'\n';
				assert(false);
			}
	}

	void computeNextImageCorners(){
		// assume next image features are computed

		// TODO which transformation to use?
		cv::Mat T=cv::estimateAffinePartial2D(ref_features,next_features);
		if(T.empty()){
			std::cerr<<"Can't find rigid transform. cap frame = "<<
				_cap_frame<<'\n';
			ref_image=cv::Mat();
			ref_features.clear();
			return;
		}

		cv::transform(ref_corners,next_corners,T);
	}

	void setGridCornersToNextImage(){
		assert(grid.getCorners().size()==next_corners.size());
		for(unsigned i=0;i<next_corners.size();++i)
			grid.setCorner(i,next_corners[i]);
	}

	void setNextImageAsRef(){
		ref_corners=std::move(next_corners);
		ref_image=std::move(next_image);
		ref_features=std::move(next_features);
	}
}
