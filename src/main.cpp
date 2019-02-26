#include<iostream>
#include<fstream>
#include<vector>
#include<array>
#include<map>
#include<string>
#include<algorithm>
#include<cstdlib>
#include<cassert>

#include<opencv2/core.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>

#include"Grid.h"

cv::Mat image;
Grid grid;

char const* const window_name="lcdreader-img";
char const* const outwin_name="output";
char const* const histwin_name="history";
int const zoom_factor=5;


int preview_alpha=35;

int disp_mode=3,n_mode=7;

cv::Mat hist=cv::Mat::zeros(700,12,CV_8UC3);
int hist_row=0;

int constexpr sign(int x){
	return (x>0)-(x<0);
}

cv::Mat computeAvg(cv::Mat i1,int zoom_factor=5,int border=1){
	if(i1.empty())
		i1=grid.extractScreen(zoom_factor);
	assert(i1.rows==zoom_factor*grid.getMaxA()&&i1.cols==zoom_factor*grid.getMaxB());
	assert(i1.type()==CV_8UC3);

	cv::Mat avg(grid.getMaxA(),grid.getMaxB(),CV_8UC3);
	for(int a=0;a<grid.getMaxA();++a)
	for(int b=0;b<grid.getMaxB();++b){
		cv::Vec3i sum(0,0,0);
		for(int x=border;x<zoom_factor-border;++x)
		for(int y=border;y<zoom_factor-border;++y)
			sum+=i1.at<cv::Vec3b>(a*zoom_factor+x,b*zoom_factor+y);

		avg.at<cv::Vec3b>(a,b)=sum/((zoom_factor-2*border)*(zoom_factor-2*border));
	}
	return avg;
}

void render(){
	cv::Mat i2;
	image.copyTo(i2);
	grid.drawBox(i2);
	cv::imshow(window_name,i2);

	cv::Mat i1=grid.extractScreen(zoom_factor);
	int constexpr border=1;
	switch(disp_mode){
		case 0:
			// nothing
			break;
		case 1:
			// grid
			grid.drawGrid(i1);
			break;
		case 2:
			{
			// border
			for(int a=0;a<i1.rows;a+=zoom_factor)
			for(int b=0;b<i1.cols;b+=zoom_factor)
				for(int x=0;x<border;++x)
				for(int y=border;y<zoom_factor;++y)
					i1.at<cv::Vec3b>(a+x,b+y)=
					i1.at<cv::Vec3b>(a+zoom_factor-1-y,b+x)=
					i1.at<cv::Vec3b>(a+zoom_factor-1-x,b+zoom_factor-1-y)=
					i1.at<cv::Vec3b>(a+y,b+zoom_factor-1-x)=
					cv::Vec3b(0,0,0);
			break;
			}
		case 3:
		case 4:
		case 5:
		case 6:
			{
			// border & avg
			cv::Mat avg=computeAvg(i1);

			for(int a=0;a<i1.rows;a+=zoom_factor)
			for(int b=0;b<i1.cols;b+=zoom_factor){
				for(int x=0;x<border;++x)
				for(int y=border;y<zoom_factor;++y)
					i1.at<cv::Vec3b>(a+x,b+y)=
					i1.at<cv::Vec3b>(a+zoom_factor-1-y,b+x)=
					i1.at<cv::Vec3b>(a+zoom_factor-1-x,b+zoom_factor-1-y)=
					i1.at<cv::Vec3b>(a+y,b+zoom_factor-1-x)=
					cv::Vec3b(0,0,0);

				auto avg_ab=avg.at<cv::Vec3b>(a/zoom_factor,b/zoom_factor);
				for(int x=0;x<zoom_factor;++x)
				for(int y=0;y<zoom_factor;++y)
					i1.at<cv::Vec3b>(a+x,b+y)=avg_ab;

				if(a==zoom_factor*10&&b<zoom_factor*12)
					hist.at<cv::Vec3b>(hist_row,b/zoom_factor)=avg_ab;
			}

			if(disp_mode>=4)
				cv::cvtColor(i1,i1,cv::COLOR_BGR2GRAY);

			cv::Mat h1,h2;
			switch(disp_mode){
				case 3:
					cv::resize(hist,h1,{},8,1,cv::INTER_NEAREST);
					break;
				case 4:
					cv::cvtColor(hist,h2,cv::COLOR_BGR2GRAY);
					cv::resize(h2,h1,{},8,1,cv::INTER_NEAREST);
					break;
				case 5:
				case 6:
					cv::cvtColor(hist,h1,cv::COLOR_BGR2GRAY);
					h2.create(h1.rows,h1.cols,CV_8UC1);
					for(int c=0;c<h1.cols;++c){
						int last_r=-1,last_delta=0;
						for(int r=0;r<h2.rows;++r){
							int delta=h1.at<uint8_t>((r+4+h1.rows)%h1.rows,c)
								-h1.at<uint8_t>((r-4)%h1.rows,c);
							if(std::abs(delta)<20)
								delta=0;

							// eliminate lines adjacent to other, larger lines
							// with the same sign
							if(delta!=0){
								if(last_r<0){
									last_r=r;
								}else{
									if(sign(delta)==sign(last_delta)){
										if(std::abs(delta)<std::abs(last_delta))
											delta=0;
										else{
											h2.at<uint8_t>(last_r,c)=127;
											last_r=r;
											last_delta=delta;
										}
									}else{
										last_r=r;
										last_delta=delta;
									}
								}
							}

							h2.at<uint8_t>(r,c)=std::min(255,std::max(0,delta+127));
						}

						// accumulated sum -> binarized image
						if(disp_mode==6){
							int cur=0;
							for(int r=0;r<h2.rows;++r){
								if(h2.at<uint8_t>(r,c)>127){
									cur=1;
								}else if(h2.at<uint8_t>(r,c)<127){
									if(cur==0){
										for(int r1=r;r1-->0;)
											h2.at<uint8_t>(r1,c)=255;
									}
									cur=0;
								}
								h2.at<uint8_t>(r,c)=255*cur;
							}
						}
					}
					cv::resize(h2,h1,{},8,1,cv::INTER_NEAREST);
					break;
			}
			cv::imshow(histwin_name,h1);
			break;
			}
	}

	cv::imshow(outwin_name,i1);
}

int heldCorner=-1; // [0..4[, or -1. For manipulating the grid.corners with mouse.
cv::Point mouse;
void mouseCallback(int event,int x,int y,int,void*){
	mouse={x,y};
	switch(event){
		case cv::EVENT_LBUTTONDOWN:
		{
			if(heldCorner>=0)
				break;
			for(unsigned i=0;i<4;++i){
				if(cv::norm(grid.getCorner(i)-mouse)<=20){
					heldCorner=i;
					grid.setCorner(i,mouse);
					render();
					break;
				}
			}
			break;
		}

		case cv::EVENT_LBUTTONUP:
			heldCorner=-1;
			break;

		case cv::EVENT_MOUSEMOVE:
		{
			if(heldCorner<0)break;
			grid.setCorner(heldCorner,mouse);
			render();
			break;
		}
	}
}

cv::Point outwinMouse;
void outwinMouseCallback(int,int x,int y,int,void*){
	outwinMouse={x,y};
}

int main(int argc,char** argv){
	std::map<std::string,std::string> args;
	for(int i=1;i<argc;++i){
		std::string arg=argv[i];
		auto index=arg.find('=');
		if(index==std::string::npos){
			std::cerr<<"Invalid arguments\n";
			return 1;
		}
		args[arg.substr(0,index)]=arg.substr(index+1);
	}

	auto iter=args.find("w");
	if(iter!=args.end()){
		grid.setGridSize(std::stoi(iter->second),std::stoi(args.at("h")));
	}

	iter=args.find("f");
	if(iter==args.end()){
		std::cerr<<"No file name\n";
		return 1;
	}

	cv::VideoCapture capture(iter->second);
	if(!capture.isOpened()){
		std::cerr<<"Can't open video\n";
		return 1;
	}

	auto
		width  = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH),
		height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT),
		nframe = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	std::cout<<"Number of frames: "<<nframe<<'\n';

	iter=args.find("s");
	if(iter!=args.end()){
		int nreadframe=std::stoi(iter->second);
		capture.set(cv::CAP_PROP_POS_FRAMES,nreadframe);
	}

	if(!capture.read(image)){
		std::cerr<<"Read past file end\n";
		return 1;
	}

	grid.setImage(image);

	cv::namedWindow(window_name,cv::WINDOW_AUTOSIZE);
	cv::namedWindow(outwin_name,cv::WINDOW_AUTOSIZE);
	cv::namedWindow(histwin_name,cv::WINDOW_AUTOSIZE);

	bool save_config;
	{
		std::ifstream config_f("config.txt");
		save_config=(bool)config_f;
		if(config_f){
			for(int i=0;i<4;++i){
				int x,y;
				config_f>>x>>y;
				grid.setCorner(i,{x,y});
			}
		}

		bool set_corners_to_default=!config_f; // empty file or read failed
		for(int i=0;i<4&&!set_corners_to_default;++i){
			auto p=grid.getCorner(i);
			if(p.x<0||p.x>width||p.y<0||p.y>height) // invalid coordinate
				set_corners_to_default=true;
		}
		if(set_corners_to_default){
			grid.setCorner(0,{0,0});
			grid.setCorner(1,{0,height});
			grid.setCorner(2,{width,0});
			grid.setCorner(3,{width,height});
		}
		config_f.close();
	}

	render();

	cv::setMouseCallback(window_name,mouseCallback,nullptr);
	cv::setMouseCallback(outwin_name,outwinMouseCallback,nullptr);

	while(true){
		char key=cv::waitKey(0);
		switch(key){
			{ /// Move corner
				int cornerIndex;

			case 'w':
				cornerIndex=0; goto move_corner;
			case 's':
				cornerIndex=1; goto move_corner;
			case 'e':
				cornerIndex=2; goto move_corner;
			case 'd':
				cornerIndex=3; goto move_corner;
move_corner:
				grid.setCorner(cornerIndex,mouse);
				render();
				break;
			}

			/*
			case 'r':
				/// Refresh (probably not useful)
				render();
				break;
				*/

			case 'q':
				/// Quit
				goto break_outer;

			case 'a':
				/// Automatically adjust
				std::cout<<"unimplemented\n";
				break;

			case '7':
				if(preview_alpha==0)break;
				--preview_alpha;
				std::cout<<"a="<<preview_alpha<<'\n';
				render();
				break;

			case '8':
				if(preview_alpha==255)break;
				++preview_alpha;
				std::cout<<"a="<<preview_alpha<<'\n';
				render();
				break;

			case '1':
				/// Change display mode
				disp_mode=(disp_mode+n_mode-1)%n_mode;
				render();
				break;

			case '2':
				/// Change display mode
				disp_mode=(disp_mode+1)%n_mode;
				render();
				break;

			case 'f':
				/// Show current frame index
				std::cout<<"Current frame: "<<(int)capture.get(cv::CAP_PROP_POS_FRAMES)<<'\n';
				break;

			{ /// Manually change the pixel at the cursor to 0/1
				char data;
			case 'z':
				data=0;
				goto change_pixel;
			case 'x':
				data=1;
				goto change_pixel;

change_pixel:
				grid.setDataManual(
						int(outwinMouse.y/zoom_factor),
						int(outwinMouse.x/zoom_factor),
						data);
				render();
				break;
			}

			case 'o':
				/// Output
			{
				std::ofstream out("time.txt",std::ios::binary);
				double fps=capture.get(cv::CAP_PROP_FPS);
				while(capture.read(image)){
					if(fps!=capture.get(cv::CAP_PROP_FPS)){
						std::cerr<<"variable fps: "<<fps<<"!="<<
							capture.get(cv::CAP_PROP_FPS)<<'\n';
						assert(false);
					}

					/* print position in nanosecond as 8-byte little-endian int
					auto ns=int64_t(capture.get(cv::CAP_PROP_POS_MSEC)*1e6);
					for(int i=0;i<8;++i,ns>>=8)
						out<<(unsigned char)ns;
						*/

					grid.setImage(image);
					cv::Mat avg=computeAvg({});
					cv::cvtColor(avg,avg,cv::COLOR_BGR2GRAY);
					assert(avg.rows==grid.getMaxA());
					assert(avg.cols==grid.getMaxB());
					assert(avg.type()==CV_8U);
					
					cv::Mat i1;
					cv::resize(avg,i1,{},5,5,cv::INTER_NEAREST);
					cv::imshow(outwin_name,i1);
					cv::waitKey(1);

					for(int a=0;a<avg.rows;++a)
					for(int b=0;b<avg.cols;++b){
						out<<(unsigned char)avg.at<uint8_t>(a,b);
					}
				}
				out.close();
				goto break_outer;
			}

			// set frame (seek)
			case 'H':
				capture.set(cv::CAP_PROP_POS_FRAMES,std::max(
						0.,
						capture.get(cv::CAP_PROP_POS_FRAMES)-100));
				goto read_frame;

			case 'L':
				capture.set(cv::CAP_PROP_POS_FRAMES,std::min(
						capture.get(cv::CAP_PROP_FRAME_COUNT)-1,
						capture.get(cv::CAP_PROP_POS_FRAMES)+100));
				goto read_frame;

			case 'g':
				std::cout<<"Frame number: ";
				{
					int frame;std::cin>>frame;
					capture.set(cv::CAP_PROP_POS_FRAMES,frame);
				}
				goto read_frame;

			case 'n':
			read_frame:
				if(!capture.read(image)){
					std::cout<<"End of file\n";
					break;
				}
				hist_row=(hist_row+1)%hist.rows;
				grid.setImage(image);
				render();
				break;

			case 'N':
				for(int _=100;_-->0;){
					if(!capture.read(image)){
						std::cout<<"End of file\n";
						break;
					}
					hist_row=(hist_row+1)%hist.rows;
					grid.setImage(image);
					render();
				}
				break;
		}
	}
break_outer:

	if(save_config){
		std::ofstream config_f("config.txt");
		for(int i=0;i<4;++i)
			config_f<<grid.getCorner(i).x<<' '<<grid.getCorner(i).y<<'\n';
	}

	return 0;
}
