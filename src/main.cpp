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

#include"Grid.hpp"

cv::Mat image;
Grid grid;

char const* const window_name="lcdreader-img";
char const* const outwin_name="output";
int zoom_factor=15;


int preview_alpha=35;

bool preview=false;
bool image_only=false;
cv::Point out_cursor; // used to set data with Z/X

void render(){
	if(image_only){
		cv::imshow(window_name,image);
	}else{
		cv::Mat i1;
		image.copyTo(i1);
		grid.drawBox(i1);
		grid.drawAnchorInput(i1);
		cv::imshow(window_name,i1);
	}

	cv::Mat i1=grid.extractScreen(zoom_factor);
	if(!image_only){
		if(preview){
			grid.binarize();
			i1=grid.drawPreview(i1,preview_alpha/255.);
		}else{
			grid.drawAnchorTransformed(i1);
			grid.drawGrid(i1);
		}

		// draw out_cursor
		cv::Scalar const border_color(0,0,255); // red
		cv::Point const topleft=out_cursor*zoom_factor,
			v1{0,zoom_factor},v2{zoom_factor,0};
		cv::line(i1,topleft,topleft+v1,border_color);
		cv::line(i1,topleft,topleft+v2,border_color);
		cv::line(i1,topleft+v1,topleft+v1+v2,border_color);
		cv::line(i1,topleft+v2,topleft+v1+v2,border_color);
	}
	cv::imshow(outwin_name,i1);
}

void setOutCursor(cv::Point p){
	if(p==out_cursor)return;
	out_cursor=p;
	if(!image_only)
		render();
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
			auto const& corners=grid.getCorners();
			for(unsigned i=0;i<corners.size();++i){
				if(cv::norm(corners[i]-mouse)<=20){
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

void outwinMouseCallback(int event,int x,int y,int,void*){
	switch(event){
		case cv::EVENT_LBUTTONUP:
			// add anchor point
			std::cerr<<"add "<<std::round((double)x/zoom_factor)<<' '<<std::round((double)y/zoom_factor)<<'\n';
			grid.addAnchor({std::round((double)x/zoom_factor),
					std::round((double)y/zoom_factor)});
			if(!image_only)
				render();
			break;

		case cv::EVENT_RBUTTONUP:
			break;

		case cv::EVENT_MOUSEMOVE:
		{
			cv::Point p(x/zoom_factor,y/zoom_factor);
			assert(0<=p.x&&0<=p.y);
			if(p.x<grid.getMaxB()&&p.y<grid.getMaxA()){
				setOutCursor(p);
			}
			break;
		}
	}
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
		grid.setGridSize(std::stoi(args.at("h")),std::stoi(iter->second));
	}

	iter=args.find("f");
	if(iter==args.end())
		image=cv::imread("in.png");
	else
		image=cv::imread(iter->second);
	if(!image.data){
		std::cerr<<"imread failed\n";
		return 1;
	}

	iter=args.find("zoom");
	if(iter!=args.end())
		zoom_factor=std::stoi(iter->second);

	int width =image.cols;
	int height=image.rows;

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
			auto p=grid.getCorners()[i];
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

			case 'u':
			{
				char key=cv::waitKey(0);
				if(key=='0'||key=='1'){
					grid.setUndistort(key=='1');
					render();
				}
				break;
			}

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

			{ /// Manually change the pixel at the cursor to 0/1
				char data;
			case 'z':
				data=0;
				goto change_pixel;
			case 'x':
				data=1;
				goto change_pixel;

change_pixel:
				grid.setDataManual(out_cursor.y,out_cursor.x,data);
				render();
				break;
			}

			case 'k':
				/// Move out cursor
				if(out_cursor.y>0)
					setOutCursor({out_cursor.x,out_cursor.y-1});
				break;
			case 'j':
				if(out_cursor.y<grid.getMaxA()-1)
					setOutCursor({out_cursor.x,out_cursor.y+1});
				break;
			case 'h':
				if(out_cursor.x>0)
					setOutCursor({out_cursor.x-1,out_cursor.y});
				break;
			case 'l':
				if(out_cursor.x<grid.getMaxB()-1)
					setOutCursor({out_cursor.x+1,out_cursor.y});
				break;

			case 'o':
				/// Output
			{
				std::ofstream out("out.txt");

				int i=0;
				grid.binarize();
				for(int row=0;row<grid.getMaxA();++row){
					for(int col=0;col<grid.getMaxB();++col){
						out<<(grid.getData(row,col)?'1':'0');
						++i;
					}
					out<<'\n';
				}
				out.close();
				break;
			}
		}
	}
break_outer:

	if(save_config){
		std::ofstream config_f("config.txt");
		for(auto p:grid.getCorners())
			config_f<<p.x<<' '<<p.y<<'\n';
	}

	return 0;
}
