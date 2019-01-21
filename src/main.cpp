#include<iostream>
#include<fstream>
#include<vector>
#include<array>
#include<algorithm>
#include<cstdlib>
#include<cassert>
#include<SDL.h>
#include<SDL_image.h>

struct Point{
	double x,y;
	Point(){}
	Point(double x,double y):x(x),y(y){}
	Point(SDL_Point p):x(p.x),y(p.y){}
	Point operator+(Point p)const{return {x+p.x,y+p.y};}
	Point operator-(Point p)const{return {x-p.x,y-p.y};}
	Point operator-()const{return {-x,-y};}
	Point operator*(double f)const{return {x*f,y*f};}
	Point operator/(double f)const{return *this*(1/f);}
	double sqrlen()const{return x*x+y*y;}
};
Point operator*(double f,Point p){return p*f;}

int maxA=12,maxB=56;
std::array<SDL_Point,4> corners; // ul,dl,ur,dr

/// Bilinear interpolation: [0..maxA] * [0..maxB] -> quadrilateral formed by corners
/// matrix (a points down, b points right) --P--> SDL (x points right, y points down)
Point P(double a,double b){
	Point
		u = (Point(corners[1]) - corners[0]) * a / maxA + corners[0],
		d = (Point(corners[3]) - corners[2]) * a / maxA + corners[2];
	return u + (d - u) * b / maxB;
}
Point P(Point ab) {return P(ab.x,ab.y);}

/// Inverse transformation of P
Point invP(Point p,int iter=5){
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

std::vector<std::vector<Uint8>> pxval;
void compute_pxval(SDL_Surface* image_surface){
	int const width=image_surface->w;
	int const height=image_surface->h;

	std::cout<<"format="<<SDL_GetPixelFormatName(image_surface->format->format)<<'\n';
	if(image_surface->format->BytesPerPixel!=4||
			image_surface->pitch!=width*4){
		std::cerr<<"Unsupported image format\n";
		return;
	}

	SDL_LockSurface(image_surface);
	pxval.resize(height);
	auto pixel_ptr=(Uint32*)image_surface->pixels;
	for(int row=0;row<height;++row){
		pxval[row].resize(width);
		for(int col=0;col<width;++col,++pixel_ptr){
			Uint8 r,g,b;
			SDL_GetRGB(*pixel_ptr,image_surface->format,&r,&g,&b);
			pxval[row][col]=(r+g+b)/3;
			// *pixel_ptr=SDL_MapRGB(image_surface->format,x,x,x);
		}
	}
	SDL_UnlockSurface(image_surface);
}

SDL_Renderer* renderer;
SDL_Texture* image;

void draw_image(){
	SDL_RenderCopy(renderer,image,nullptr,nullptr);
}

void draw_grid(){
	SDL_SetRenderDrawColor(renderer,0,0,0,SDL_ALPHA_OPAQUE);
	for (int b = 0; b <= maxB; ++b) { // draw vertical lines
		auto p1=P(0,b),p2=P(maxA,b);
		SDL_RenderDrawLine(renderer,p1.x,p1.y,p2.x,p2.y);
	}
	for (int a = 0; a <= maxA; ++a) { // draw horizontal lines
		auto p1=P(a,0),p2=P(a,maxB);
		SDL_RenderDrawLine(renderer,p1.x,p1.y,p2.x,p2.y);
	}
}

std::vector<std::vector<std::vector<SDL_Point>>> ptListFull,ptlist;
void computePtList(){
	double constexpr min_border=0.2,max_border=0.8;

	ptListFull.assign(maxA,std::vector<std::vector<SDL_Point>>(maxB));
	ptlist.assign(maxA,std::vector<std::vector<SDL_Point>>(maxB));

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
		Point point=invP({(double)x,(double)y});
		auto a=(int)std::floor(point.x),b=(int)std::floor(point.y);
		if(a<0||a>=maxA||b<0||b>=maxB)
			continue;
		ptListFull[a][b].push_back({x,y});
		if(
				point.x-a<min_border||point.x-a>max_border||
				point.y-b<min_border||point.y-b>max_border)
			continue;
		ptlist[a][b].push_back({x,y});
	}
}

std::vector<std::vector<Uint8>> medColor;
/// Compute the median color in each cell. Return the uncertainty value.
void computeMedColor(){
	computePtList();

	std::array<unsigned,256> cnt;
	medColor.resize(maxA);
	for(int a=0;a<maxA;++a){
		medColor[a].resize(maxB);
		for(int b=0;b<maxB;++b){
			std::fill(cnt.begin(),cnt.end(),0);
			int sum_pxv=0;
			Uint8 min_pxv=255,max_pxv=0;
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

			// Bias towards high value (white). A black cell must be black, while a
			// white cell may be partially black because of shadow.
			auto x=ptlist[a][b].size()*4/5; 
			for(unsigned r=0;;++r){
				if(cnt[r]>x){
					medColor[a][b]=r;
					break;
				}
				x-=cnt[r];
			}
		}
	}
}

/// Binarized value correspond to the pixels, in row-major order.
/// Light (high value): 0, dark (low value): 1.
std::vector<signed char> data;
int edge_threshold=13;

// Helper function for binarize.
// f is invoked with coordinates of cells with chessboard distance
// from (a,b) <= 2 (including the cell (a,b) itself)
template<class Fn>
void for_near_cells(int a,int b,Fn const& f){
	for(int a1=std::max(0,a-2);a1<a2+3&&a1<maxA;++a1)
	for(int b1=std::max(0,b-2);b1<b2+3&&b1<maxB;++b1)
		f(a1,b1);
}

// Helper function for binarize.
void dfs_data(int a,int b){
	for(int a1=std::max(0,a-2);a1<a2+3&&a1<maxA;++a1)
	for(int b1=std::max(0,b-2);b1<b2+3&&b1<maxB;++b1){
		if(pxval[a1][b1]<0){
			pxval[a1][b1]=pxval[a][b];
			dfs_data(a1,b1);
		}
	}
}

/// Compute (data) based on (medColor) and (edge_threshold).
void binarize(){
	computeMedColor();

	data.assign(maxA*maxB,-1);

	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b){
		int pxv=pxval[a][b];
		for(int a1=std::max(0,a-2);a1<a2+3&&a1<maxA;++a1)
		for(int b1=std::max(0,b-2);b1<b2+3&&b1<maxB;++b1){
			int pxv1=pxv1[a1][b1];
			if(pxv>pxv1+edge_threshold){
				data[a*maxB+b]=0;
				goto determined;
			}else if(pxv<pxv1-edge_threshold){
				data[a*maxB+b]=1;
				goto determined;
			}
		}
determined:;
	}

	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b){
		if(data[a*maxB+b]>=0)
			dfs_data(data);
	}
}

bool preview_binary=true;
int preview_alpha=35;
void draw_preview(){
	if(preview_binary)
		binarize();
	else
		computeMedColor();

	SDL_RenderCopy(renderer,image,nullptr,nullptr);
	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b){
		if(preview_binary){
			if(data[a*maxB+b])
				SDL_SetRenderDrawColor(renderer,0,0,255,preview_alpha); // blue
			else
				SDL_SetRenderDrawColor(renderer,255,255,255,preview_alpha); // white
			SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
		}else{
			Uint8 x=medColor[a][b];
			SDL_SetRenderDrawColor(renderer,x,x,x,SDL_ALPHA_OPAQUE);
			SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
		}
		auto const& ptl=ptListFull[a][b];
		SDL_RenderDrawPoints(renderer,ptl.data(),ptl.size());
	}
}

bool preview=false;
bool image_only=false;
void render(){
	draw_image();
	if(!image_only){
		if(preview)
			draw_preview();
		else
			draw_grid();
	}
	SDL_RenderPresent(renderer);
}

int main(){
	std::atexit(SDL_Quit);
	if(SDL_Init(SDL_INIT_VIDEO)!=0){
		std::cerr<<"SDL_Init failed: "<<SDL_GetError()<<'\n';
		return 1;
	}

	int imgFlags=IMG_INIT_PNG;
	std::atexit(IMG_Quit);
	if((IMG_Init(imgFlags)&imgFlags)!=imgFlags){
		std::cerr<<"IMG_Init failed: "<<IMG_GetError()<<'\n';
		return 1;
	}

	SDL_Surface *image_surface=IMG_Load("in.png");
	if(!image_surface){
		std::cerr<<"IMG_Load failed: "<<IMG_GetError()<<'\n';
		return 1;
	}

	int width=image_surface->w;
	int height=image_surface->h;

	compute_pxval(image_surface);

	SDL_Window* window=SDL_CreateWindow(
		"lcdreader-img",
		SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
		width,height,
		SDL_WINDOW_SHOWN
	);
	if (!window){
		std::cerr<<"SDL_CreateWindow failed: "<<SDL_GetError()<<'\n';
		return 1;
	}

	renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	if (!renderer){
		std::cerr<<"SDL_CreateRenderer failed: "<<SDL_GetError()<<'\n';
		return 1;
	}

	image=SDL_CreateTextureFromSurface(renderer,image_surface);
	SDL_FreeSurface(image_surface);
	if(!image){
		std::cerr<<"SDL_CreateTextureFromSurface failed: "<<SDL_GetError()<<'\n';
		return 1;
	}

	bool save_config;
	{
		std::ifstream config_f("config.txt");
		save_config=(bool)config_f;
		if(config_f){
			for(auto& p:corners)
				config_f>>p.x>>p.y;
		}
		if(!config_f // empty file or read failed
				|| std::any_of(corners.begin(),corners.end(),[=](SDL_Point p){
					return p.x<0||p.x>width||p.y<0||p.y>height;
					}) // invalid coordinate
				){
			corners[0]={0,0};
			corners[1]={0,height};
			corners[2]={width,0};
			corners[3]={width,height};
		}
		config_f.close();
	}

	render();

	int heldCorner=-1; // [0..4[, or -1. For manipulating the corners with mouse.

	bool running=true;
	while(running){
		SDL_Event event;
		if (!SDL_WaitEvent(&event)){
			std::cerr<<"SDL_WaitEvent failed: "<<SDL_GetError()<<'\n';
			break;
		}

		switch (event.type)
		{
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				running=false;
				break;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
		{
			if(heldCorner>=0)
				break;
			SDL_Point mouse{event.button.x,event.button.y};
			for(unsigned i=0;i<corners.size();++i){
				int dx=corners[i].x-mouse.x;
				int dy=corners[i].y-mouse.y;
				if(dx*dx+dy*dy<=20*20){
					heldCorner=i;
					corners[i]=mouse;
					render();
					break;
				}
			}
			break;
		}

		case SDL_MOUSEBUTTONUP:
			heldCorner=-1;
			break;

		case SDL_MOUSEMOTION:
		{
			if(heldCorner<0)break;
			SDL_Point mouse{event.button.x,event.button.y};
			corners[heldCorner]=mouse;
			render();
			break;
		}

		case SDL_KEYDOWN:
			switch(event.key.keysym.sym){
			{
				/// Move corner
				int cornerIndex;

			case SDLK_w:
				cornerIndex=0; goto move_corner;
			case SDLK_s:
				cornerIndex=1; goto move_corner;
			case SDLK_e:
				cornerIndex=2; goto move_corner;
			case SDLK_d:
				cornerIndex=3; goto move_corner;
move_corner:
				int x,y;
				SDL_GetMouseState(&x,&y);
				corners[cornerIndex]={x,y};
				render();
				break;
			}

			/*
			case SDLK_r:
				/// Refresh (probably not useful)
				render();
				break;
				*/

			case SDLK_q:
				/// Quit
				running=false;
				break;

			case SDLK_a:
				/// Automatically adjust
				std::cout<<"unimplemented\n";
				break;

			case SDLK_7:
				if(preview_alpha==0)break;
				--preview_alpha;
				std::cout<<"a="<<preview_alpha<<'\n';
				render();
				break;

			case SDLK_8:
				if(preview_alpha==255)break;
				++preview_alpha;
				std::cout<<"a="<<preview_alpha<<'\n';
				render();
				break;

			case SDLK_9:
				if(edge_threshold==0)break;
				--edge_threshold;
				std::cout<<"t="<<edge_threshold<<'\n';
				render();
				break;

			case SDLK_0:
				if(edge_threshold==255)break;
				++edge_threshold;
				std::cout<<"t="<<edge_threshold<<'\n';
				render();
				break;

			case SDLK_t:
				/// Show only image - toggle
				image_only^=true;
				render();
				break;

			case SDLK_p:
				/// Preview (average color) - toggle
				preview^=true;
				render();
				break;

			case SDLK_b:
				/// Preview (binarization) - toggle
				preview=true; // otherwise this has no effect...
				preview_binary^=true;
				render();
				break;

			case SDLK_o:
				/// Output
			{
				std::ofstream out("out.txt");

				int i=0;
				for(int row=0;row<maxA;++row){
					for(int col=0;col<maxB;++col)
						out<<(data[i++]?'1':'0');
					out<<'\n';
				}
				out.close();
				break;
			}
			}
			break;
		}
	}

	if(save_config){
		std::ofstream config_f("config.txt");
		for(auto& p:corners)
			config_f<<p.x<<' '<<p.y<<'\n';
	}

	SDL_DestroyTexture(image);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	return 0;
}
