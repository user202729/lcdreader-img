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
#include<opencv2/calib3d.hpp>
#include<opencv2/video.hpp>
#include<opencv2/videoio.hpp>

#include"Grid.hpp"
#include"DigitRecognition.hpp"
#include"VideoStabilize.hpp"



enum class StabilizationMethod{
	//Step, // removed
	None_,
	StepToFirst // calc optical flow first frame <-> nth frame, avoid drift?
};
auto constexpr stabilization_method=StabilizationMethod::None_;


cv::Mat image;
Grid grid;

char const* const window_name="lcdreader-img";
char const* const outwin_name="output";
int zoom_factor=15;
double image_zoom=1;
double border=0;


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

	cv::Mat i1=grid.extractScreen(zoom_factor, border);
	if(!image_only){
		switch(preview_mode){
			case Preview::NONE:
				grid.drawAnchorTransformed(i1, border);
				grid.drawGrid(i1, border);
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
		cv::Point const topleft{(cv::Point2d(out_cursor)+cv::Point2d(border,border))*zoom_factor},
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
int lastMovedCorner=-1;
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
				lastMovedCorner=heldCorner=best_i;
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
	{ o output | /tmp/out.txt | Output file path. Used in some functions. }
	{ zoom   | 15     | Zoom factor (pixel width) of output image. }
	{ border | 0      | Border (experimental). Some features might work incorrectly with border!=0. }
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
	border=args.get<double>("border");

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
		}else{
			std::cout<<"Configuration file not found. Create a new file? (y/n) ";
			std::string line;
			while(true){
				if(not std::getline(std::cin, line)){
					std::cerr<<"Cannot read from stdin\n";
					return 0;
				}
				switch(line[0]){ // safe, line is null-terminated
					case 'y': case 'Y':
						save_config=true; break;
					case 'n': case 'N':
						break;
					default:
						std::cout<<"(y/n) ";
						continue;
				}
				break;
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


	auto const do_save_config=[&]{
		std::ofstream config_f(config_file_name);
		for(auto p:grid.getCorners())
			config_f<<p.x<<' '<<p.y<<'\n';
	};

	render();

	cv::setMouseCallback(window_name,mouseCallback,nullptr);
	cv::setMouseCallback(outwin_name,outwinMouseCallback,nullptr);

	bool playing=args.get<bool>("p");

#if USE_X11
	Display* dpy = XOpenDisplay(NULL);
	std::array<KeyCode, 2> shiftKeyCodes{{
		XKeysymToKeycode(dpy, XK_Shift_L),
		XKeysymToKeycode(dpy, XK_Shift_R)
	}};
#endif


	while(true){
		char key=cv::waitKey(playing
				?std::max(1,int(1000/cap.get(cv::CAP_PROP_FPS)))
				:0);

#if USE_X11
		if('a'<=key and key<='z'){
			char keys_return[32];
			XQueryKeymap( dpy, keys_return );
			if(std::any_of(begin(shiftKeyCodes), end(shiftKeyCodes),[&](KeyCode keyCode){
				return keys_return[ keyCode>>3 ] & ( 1<<(keyCode&7) );
			}))
				key=key-'a'+'A';
		}
#endif
		switch(key){
			case 'S':
			{ // video stabilize
				static int refFrame=-1;
				switch(cv::waitKey()){
					case 's': // set
						stab::setRefImage(image);
						refFrame=cap.get(cv::CAP_PROP_POS_FRAMES);
						break;

					case 'g': // go to ref image
						if(refFrame<0){
							std::cerr<<"refFrame not set\n";
							break;
						}

						cap.set(cv::CAP_PROP_POS_FRAMES,refFrame-1);
						cap>>image;
						// fallthrough

					case 'r': // reset grid corners
						for(unsigned i=0;i<stab::ref_corners.size();++i)
							grid.setCorner(i,stab::ref_corners[i]);
						render();
						break;

					case 'a': // apply
						if(refFrame<0){
							std::cerr<<"refFrame not set\n";
							break;
						}
						save_config=false; // for safety

						stab::next_image=image;
						stab::computeNextImageFeatures();
						stab::computeNextImageCorners();
						stab::setGridCornersToNextImage();
						render();
						break;
				}
				break;
			}

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

			case 'N': // advance backward
				{
					int frame=(int)cap.get(cv::CAP_PROP_POS_FRAMES);
					if(frame!=1){
						cap.set(cv::CAP_PROP_POS_FRAMES,frame-2);
						goto read_frame;
					}
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
			case 'w':
				lastMovedCorner=0; goto move_corner;
			case 's':
				lastMovedCorner=1; goto move_corner;
			case 'e':
				lastMovedCorner=2; goto move_corner;
			case 'd':
				lastMovedCorner=3; goto move_corner;
move_corner:
				grid.setCorner(lastMovedCorner,mouse);
				render();
				break;

				int dx, dy;
			case 'Y':
				dx=-1; dy=0; goto move_corner_1;
			case 'U':
				dx=0; dy=1; goto move_corner_1;
			case 'I':
				dx=0; dy=-1; goto move_corner_1;
			case 'O':
				dx=1; dy=0; goto move_corner_1;
move_corner_1:
				if(lastMovedCorner>=0){
					cv::Point2d const point=grid.getCorners()[lastMovedCorner];
					grid.setCorner(lastMovedCorner, {point.x+dx/image_zoom, point.y+dy/image_zoom});
					render();
				}else{
					std::cout<<"No corner moved\n";
				}
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
				{
					auto [string, score]=grid.recognizeDigitsTemplateMatching();
					std::cout<<"String="<<string<<". Is it correct? (Y/n) ";
					std::string line; std::getline(std::cin, line);
					if(line=="y" or line==""){
						for(double step=1; step>=0.01; step/=2){
							while(true){
								for(int corner=0; corner<4; ++corner){
									auto const oldValue=grid.getCorners()[corner];
									double dx=step, dy=0;
									for(int _=0; _<4; ++_, std::tie(dx, dy)=std::pair(dy,-dx)){
										grid.setCorner(corner, {oldValue.x+dx, oldValue.y+dy});
										auto [string1, score1]=grid.recognizeDigitsTemplateMatching();
										if(string1==string and score1<score){
											score=score1;
											goto continue_outer;
										}
									}
									grid.setCorner(corner, oldValue);
								}
								break;
continue_outer:;
							}
						}
						std::cout<<"Adjusted\n";
						render();
					}else{
						std::cout<<"Adjust the grid manually until the recognized string is correct\n";
					}
				}
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
				std::ofstream out(args.get<std::string>("o"));

				std::clock_t start=std::clock();
				int n_done=0;
				int last_quotient=0;
				int const REPORT_INTERVAL=2*CLOCKS_PER_SEC;
				auto const total_process=int(cap.get(cv::CAP_PROP_FRAME_COUNT)-cap.get(cv::CAP_PROP_POS_FRAMES));

				int64_t last_ns=-1;

				// note: this function must be called with the most recently-read image from `cap` for correct timestamp output
				auto const processImage=[&](cv::Mat image_){
					/// Assume grid corners are correct w.r.t. image_.
					grid.setImage(image=image_);
					auto digits=grid.recognizeDigits();

					auto const ns=int64_t(cap.get(cv::CAP_PROP_POS_MSEC)*1e6);
					if(last_ns>=ns){
						std::cerr<<"OpenCV's seek is broken.\n"
							"See https://github.com/opencv/opencv/issues/4890 for more details.\n"
							"Workaround: reencode the video with `ffmpeg -i <file> -bf 0 <output>`.\n";
						assert(false);
					}
					last_ns=ns;
					out<<ns<<' ';

					out<<digits<<std::endl;

					++n_done;
					auto current_diff=std::clock()-start;
					if(current_diff/REPORT_INTERVAL!=last_quotient){
						render();
						cv::waitKey(std::max(1,int(1000/cap.get(cv::CAP_PROP_FPS))));

						last_quotient=current_diff/REPORT_INTERVAL;
						double elapsed=current_diff/(double)CLOCKS_PER_SEC;
						std::cerr<<
							"Elapsed: "<<elapsed<<", "
							"current ans: "<<digits<<", "
							"processed: "<<n_done<<"/"<<total_process<<", "
							"ETA: "<<elapsed*double(total_process-n_done)/n_done<<
							'/'<<elapsed*(double)total_process/n_done<<'\n';
					}
				};

				// ===============


				int const STEP=500;
				// if there is little or no motion between frame X and X+STEP
				// it's assumed that there's no motion in all the middle frames

				if(stabilization_method==StabilizationMethod::StepToFirst){
					if(save_config){
						do_save_config();
						save_config=false;
					}

					stab::setRefImage(image);

					double const minGridSpacing=stab::computeGridSpacing();
					std::cerr<<"minGridSpacing="<<minGridSpacing<<'\n';

					auto prev_image=stab::ref_image;
					auto prev_features=stab::ref_features;

					// DEBUG
					auto ref_image_sum=cv::sum(stab::ref_image);
					auto ref_features_sum=cv::sum(stab::ref_features);


					while(true){
						assert(ref_image_sum==cv::sum(stab::ref_image));
						assert(ref_features_sum==cv::sum(stab::ref_features));

						auto pos=cap.get(cv::CAP_PROP_POS_FRAMES);
						cap.set(cv::CAP_PROP_POS_FRAMES,pos+STEP-1);
						bool read_success=cap.read(stab::next_image);
						cap.set(cv::CAP_PROP_POS_FRAMES,pos);
						if(read_success&&(
									stab::computeNextImageFeatures(),
									!stab::largeMovement(prev_features,stab::next_features,minGridSpacing/3)
									)){
							// there exists STEP frames with no large movement
							// process frames without recompute grid coordinate


							cv::Mat tmp;
							for(int i=0;i<STEP;++i){
								bool success=cap.read(tmp);
								assert(success);
								processImage(tmp);
							}

							stab::computeNextImageCorners();
							stab::setGridCornersToNextImage();

							/*
							double norm=cv::norm(tmp,stab::next_image,cv::NORM_L1);
							if(!(
										// tmp.type()==stab::next_image.type()&&
										// tmp.size==stab::next_image.size&&
										// std::equal(tmp.begin<uchar>(),tmp.end<uchar>(),
										// 	stab::next_image.begin<uchar>())
										norm<=1e-10
										)){
								std::cerr<<"norm="<<norm<<'\n';
								cv::imshow("I1",prev_image);
								cv::imshow("I2",stab::next_image);
								while(cv::waitKey()!='q');
								// NOTE: frame 105412 or so: norm~=1e5, images look identical???
							}
							*/
						}else{
							// recompute grid coordinate for each frame
							if(0&&read_success){ // DEBUG:Display difference
								std::cerr<<"Large movement - cap frame = "<<pos
									<<" (/STEP="<<(int)pos/STEP<<")\n";

								double const THRES=minGridSpacing/3;

								cv::Mat i1=prev_image.clone();
								cv::Mat i2=stab::next_image.clone();
								for(unsigned i=0;i<prev_features.size();++i){
									cv::Scalar color=
										cv::norm(prev_features[i]-stab::next_features[i])>THRES?
										cv::Scalar(255,0,0): // blue = too far
										cv::Scalar(0,0,255);
									cv::circle(i1,prev_features[i],3,color,-1);
									cv::circle(i2,stab::next_features[i],3,color,-1);
								}

								cv::imshow("I1",i1);
								cv::imshow("I2",i2);

								while(cv::waitKey()!='q');
							}

							for(int i=0;i<STEP;++i){
								if(!cap.read(stab::next_image))
									goto break_outer;

								stab::computeNextImageFeatures();
								stab::computeNextImageCorners();
								stab::setGridCornersToNextImage();
								processImage(stab::next_image);
							}
						}
						prev_image=stab::next_image;
						prev_features=stab::next_features;
					}

				}else if(stabilization_method==StabilizationMethod::None_){

					while(cap.read(stab::next_image)){
						processImage(stab::next_image);
					}
					goto break_outer;

				}else{
					std::cerr<<"Method not supported?\n";
					assert(false);
				}

			}
		}
	}
break_outer:

	if(save_config){
		do_save_config();
	}

#if USE_X11
	XCloseDisplay(dpy);
#endif

	return 0;
}
