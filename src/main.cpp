#include<iostream>
#include<fstream>
#include<vector>
#include<array>
#include<map>
#include<string>
#include<algorithm>
#include<cstdlib>
#include<cassert>
#include<ctime>

#if USE_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

#include<opencv2/core.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/videoio.hpp>

#include"Grid.hpp"
#include"DigitRecognition.hpp"

cv::Mat image;
Grid grid;

char const* const window_name="lcdreader-img";
char const* const outwin_name="output";
int zoom_factor=15;
double image_zoom=1;


int preview_alpha=35;

enum class Preview{
	NONE,BLOCK,EDGES,
	BINARIZE,
	EDGES_BINARIZE,
	N_MODES
};
Preview preview_mode=Preview::NONE;
bool image_only=false;
cv::Point out_cursor; // used to set data with Z/X

void render(){
	{
		cv::Mat i1;
		cv::resize(image,i1,{},image_zoom,image_zoom);
		if(!image_only){
			grid.drawBox(i1);
			grid.drawAnchorInput(i1);
		}
		cv::imshow(window_name,i1);
	}

	cv::Mat i1=grid.extractScreen(zoom_factor);
	if(!image_only){
		switch(preview_mode){
			case Preview::NONE:
				grid.drawAnchorTransformed(i1);
				grid.drawGrid(i1);
				break;
			case Preview::BLOCK:
				grid.binarize();
				i1=grid.drawPreview(i1,preview_alpha/255.);
				break;
			case Preview::EDGES:
				grid.binarize();
				grid.drawPreviewEdges(i1);
				break;
			case Preview::BINARIZE:
				cv::cvtColor(i1,i1,cv::COLOR_BGR2GRAY);
				cv::adaptiveThreshold(i1,i1,255,
						cv::ADAPTIVE_THRESH_GAUSSIAN_C,
						cv::THRESH_BINARY,
						zoom_factor*9,5);
				break;
			case Preview::EDGES_BINARIZE:
				cv::cvtColor(i1,i1,cv::COLOR_BGR2GRAY);
				cv::adaptiveThreshold(i1,i1,255,
						cv::ADAPTIVE_THRESH_GAUSSIAN_C,
						cv::THRESH_BINARY,
						zoom_factor*9,5);
				grid.binarize();
				cv::cvtColor(i1,i1,cv::COLOR_GRAY2BGR);
				grid.drawPreviewEdges(i1);
				break;
			default:
				assert(false);
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
cv::Point2d mouse;
void mouseCallback(int event,int x,int y,int,void*){
	mouse={(double)x/image_zoom,(double)y/image_zoom};
	switch(event){
		case cv::EVENT_LBUTTONDOWN:
		{
			if(heldCorner>=0)
				break;
			auto const& corners=grid.getCorners();

			int best_i=-1;
			double ans=20; // only consider points with distance <= this
			for(unsigned i=0;i<corners.size();++i){
				double ans_i=cv::norm(corners[i]-mouse);
				if(ans_i<=ans){
					best_i=i;
					ans=ans_i;
				}
			}
			if(best_i>=0){
				heldCorner=best_i;
				grid.setCorner(best_i,mouse);
				render();
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
	cv::CommandLineParser args(argc,argv,R"(
	{ help ? |        | Print help message.                        }
	{ w      | 10     | Grid width in pixels.                      }
	{ h      | 10     | Grid height in pixels.                     }
	{ f      | in.png | Image or video file name/URL.              }
	{ zoom   | 15     | Zoom factor (pixel width) of output image. }
	{ inzoom | 1.0    | Zoom factor of input image.                }
	{ s skip | 1      | Number of frames to skip.                  }
	{ p play | false  | Auto-play the video on start.              }
	{ conf   | config.txt | Config file name.                      }
	)");

	if(args.has("help")){
		args.printMessage();
		return 0;
	}

	grid.setGridSize(args.get<int>("h"),args.get<int>("w"));

	cv::VideoCapture cap(args.get<std::string>("f"));
	if(!cap.isOpened()){
		std::cerr<<"Failed to open\n";
		return 1;
	}

	cap.set(cv::CAP_PROP_POS_FRAMES,args.get<unsigned>("s"));

	if(!(cap.read(image))){
		std::cerr<<"Failed to read image\n";
		return 1;
	}

	zoom_factor=args.get<int>("zoom");
	image_zoom=args.get<double>("inzoom");

	int width =image.cols;
	int height=image.rows;

	grid.setImage(image);

	cv::namedWindow(window_name,cv::WINDOW_AUTOSIZE);
	cv::namedWindow(outwin_name,cv::WINDOW_AUTOSIZE);

	std::string const config_file_name=args.get<std::string>("conf");
	bool save_config;
	{
		std::ifstream config_f(config_file_name);
		save_config=(bool)config_f;
		if(config_f){
			for(int i=0;i<4;++i){
				double x,y;
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
			grid.setCorner(1,{0,(double)height});
			grid.setCorner(2,{(double)width,0});
			grid.setCorner(3,{(double)width,(double)height});
		}
		config_f.close();
	}

	render();

	cv::setMouseCallback(window_name,mouseCallback,nullptr);
	cv::setMouseCallback(outwin_name,outwinMouseCallback,nullptr);

	bool playing=args.get<bool>("p");

#if USE_X11
	Display* dpy = XOpenDisplay(NULL);
	KeyCode const shift_key_code = XKeysymToKeycode( dpy, XK_Shift_L );
#endif


	while(true){
		char key=cv::waitKey(playing
				?std::max(1,int(1000/cap.get(cv::CAP_PROP_FPS)))
				:0);

#if USE_X11
		if('a'<=key and key<='z'){
			char keys_return[32];
			XQueryKeymap( dpy, keys_return );
			if( keys_return[ shift_key_code>>3 ] & ( 1<<(shift_key_code&7) ) ){
				key=key-'a'+'A';
			}
		}
#endif
		switch(key){
			case 'H': // set frame (seek)
				cap.set(cv::CAP_PROP_POS_FRAMES,std::max(
						0.,
						cap.get(cv::CAP_PROP_POS_FRAMES)-100));
				goto read_frame;

			case 'L':
				cap.set(cv::CAP_PROP_POS_FRAMES,std::min(
						cap.get(cv::CAP_PROP_FRAME_COUNT)-1,
						cap.get(cv::CAP_PROP_POS_FRAMES)+100));
				goto read_frame;

			case 'g':
				std::cout<<"Current frame number: "<<cap.get(cv::CAP_PROP_POS_FRAMES)<<
					"/"<<cap.get(cv::CAP_PROP_FRAME_COUNT)<<
					"; New frame number (-1 to exit): ";
				{
					int frame;
					if(std::cin>>frame&&frame!=-1){
						cap.set(cv::CAP_PROP_POS_FRAMES,frame);
						goto read_frame;
					}else
						break;
				}

			case -1:
			case 'n':
			{
read_frame:
				cv::Mat nextframe;
				if(cap.read(nextframe)){
					grid.setImage(image=nextframe);
					render();
				}else{
					std::cerr<<"Failed to read next frame\n";
					playing=false;
				}
				break;
			}

			case ' ':
				playing=!playing;
				break;

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

			case 'r':
			{
				char key=cv::waitKey(0);
				switch(key){
					case 'f':
						/// Refresh (probably not useful)
						render();
						break;
					case 'c':
					{
						/// Recognize digits
						std::cout<<grid.recognizeDigits()<<'\n';
						break;
					}
				}
				break;
			}

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
				preview_mode=Preview(((int)preview_mode+1)%(int)Preview::N_MODES);
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

			case 'c':
			{
				/// Change data manual by digit pattern
				std::cout<<"Enter 4 hexadecimal characters\n";
				std::string digits(4,' ');
				std::size_t i=0;
				while(true){
					if(i==digits.size()){
						cv::Mat_<uint8_t> m=getDigitMat(digits);
						for(int a=0;a<m.rows;++a)
						for(int b=0;b<m.cols;++b)
							grid.setDataManual(a,b,m(a,b));
						if(preview_mode!=Preview::NONE)
							render();
						break;
					}

					auto& digit=digits[i];
					digit=cv::waitKey(0);
					if('a'<=digit&&digit<='f')
						digit+='A'-'a';
					auto index=(unsigned)indexOf(digit);
					if(index>=NDIGIT||name[index]!=digit){
						std::cout<<"Invalid letter "<<digit<<'\n';
						// Delete data manual
						for(int a=0;a<grid.getMaxA();++a)
						for(int b=0;b<grid.getMaxB();++b)
							grid.setDataManual(a,b,-1);
						if(preview_mode!=Preview::NONE)
							render();
						break;
					}
					++i;
				}
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

				std::clock_t start=std::clock();
				int n_done=0;
				int last_quotient=0;
				int const REPORT_INTERVAL=2*CLOCKS_PER_SEC;
				auto const total_process=int(cap.get(cv::CAP_PROP_FRAME_COUNT)-cap.get(cv::CAP_PROP_POS_FRAMES));

				while(cap.read(image)){
					grid.setImage(image);
					out<<grid.recognizeDigits()<<'\n';
					++n_done;

					auto current_diff=std::clock()-start;
					if(current_diff/REPORT_INTERVAL!=last_quotient){
						last_quotient=current_diff/REPORT_INTERVAL;
						double elapsed=current_diff/(double)CLOCKS_PER_SEC;
						std::cerr<<
							"Elapsed: "<<elapsed<<", "
							"processed: "<<n_done<<"/"<<total_process<<", "
							"ETA: "<<elapsed*double(total_process-n_done)/n_done<<
							'/'<<elapsed*(double)total_process/n_done<<'\n';
					}
				}
				goto break_outer;
			}
		}
	}
break_outer:

	if(save_config){
		std::ofstream config_f(config_file_name);
		for(auto p:grid.getCorners())
			config_f<<p.x<<' '<<p.y<<'\n';
	}

#if USE_X11
	XCloseDisplay(dpy);
#endif

	return 0;
}
