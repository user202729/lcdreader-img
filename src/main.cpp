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
int const zoom_factor=15;


int preview_alpha=35;

bool preview=false;
bool image_only=false;
void render(){
	if(image_only){
		cv::imshow(window_name,image);
	}else{
		cv::Mat i1;
		image.copyTo(i1);
		grid.drawBox(i1);
		cv::imshow(window_name,i1);
	}

	cv::Mat i1=grid.extractScreen(zoom_factor);
	if(!image_only){
		cv::cvtColor(i1,i1,cv::COLOR_BGR2GRAY);
		if(preview){
			cv::adaptiveThreshold(i1,i1,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,
					cv::THRESH_BINARY,zoom_factor*5,2);
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

				/*
			case '9':
				grid.changeEdgeThreshold(-1);
				render();
				break;

			case '0':
				grid.changeEdgeThreshold(1);
				render();
				break;
				*/

			case 't':
				/// Show only image - toggle
				image_only^=true;
				render();
				break;

			case 'p':
				/// Preview (average color) - toggle
				preview^=true;
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
				std::ofstream out("out.txt",std::ios::binary);
				while(capture.read(image)){
					cv::cvtColor(image,image,cv::COLOR_BGR2GRAY);
					grid.setImage(image);
					cv::Mat screen=grid.extractScreen(1);
					assert(screen.rows==grid.getMaxA());
					assert(screen.cols==grid.getMaxB());
					for(int a=0;a<screen.rows;++a)
					for(int b=0;b<screen.cols;++b)
						out<<(unsigned char)screen.at<uint8_t>(a,b);
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
				grid.setImage(image);
				render();
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
