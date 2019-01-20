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
Point invP(Point p) {
	double x = p.x, y = p.y,
	   x00 = corners[0].x, x01 = corners[1].x, x10 = corners[2].x, x11 = corners[3].x,
	   y00 = corners[0].y, y01 = corners[1].y, y10 = corners[2].y, y11 = corners[3].y;

	// substition 1 ( see. derivation )
	double dx0 = x01 - x00;
	double dx1 = x11 - x10;
	double dy0 = y01 - y00;
	double dy1 = y11 - y10;

	// substitution 2 ( see. derivation )
	double x00x = x00 - x;
	double xd   = x10 - x00;
	double dxd  = dx1 - dx0;
	double y00y = y00 - y;
	double yd   = y10 - y00;
	double dyd  = dy1 - dy0;

	// solution of quadratic equations
	double c =   x00x*yd - y00y*xd;
	double b =   dx0*yd  + dyd*x00x - dy0*xd - dxd*y00y;
	double a =   dx0*dyd - dy0*dxd;
	double D2 = b*b - 4*a*c;
	double D  = std::sqrt( D2 );
	double u1 = (-b + D)/(2*a);
	double u2 = (-b - D)/(2*a);

	double u=std::abs(u1-0.5)<std::abs(u2-0.5)?u1:u2;

	// backsubstitution of "u" to obtain "v"
	double v;

	double denom_x = xd + u*dxd;
	double denom_y = yd + u*dyd;
	if( std::abs(denom_x)>std::abs(denom_y) ){  v = -( x00x + u*dx0 )/denom_x;  }else{  v = -( y00y + u*dy0 )/denom_y;  }
	return {u*maxA, v*maxB};
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
	double constexpr min_border=0.3,max_border=0.7;

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
double computeMedColor(){
	computePtList();

	std::array<unsigned,256> cnt;
	double uncertainty=0;
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
			double mean_pxv=sum_pxv/(double)ptlist[a][b].size();
			uncertainty+=std::min(max_pxv-mean_pxv,mean_pxv-min_pxv);

			if(ptlist[a][b].size()==0){
				std::cout<<"empty cell\n";
				return -1;
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

	std::cout<<"u="<<uncertainty<<'\n';
	return uncertainty;
}

/// Binarized value correspond to the pixels, in row-major order.
/// Light (high value): 0, dark (low value): 1.
std::vector<signed char> data;
int edge_threshold=13;

/// Compute (data) based on (medColor) and (edge_threshold).
void binarize(){
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
	/*
	std::sort(adjacentPairs.begin(),adjacentPairs.end(),[](DiffPair x,DiffPair y){
			return x.delta>y.delta;});
			*/

	bool valid=true;
	for(DiffPair p:adjacentPairs)
	if(p.delta>=edge_threshold){
		if(data[p.a]==0||data[p.b]==1)
			valid=false;
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

	if(!valid)
		std::cout<<"invalid!\n";
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
void render(){
	draw_image();
	if(preview)
		draw_preview();
	else
		draw_grid();
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

	corners[0]={0,0};
	corners[1]={0,height};
	corners[2]={width,0};
	corners[3]={width,height};

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
				std::cout<<"m "<<cornerIndex<<'\n';
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

	SDL_DestroyTexture(image);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	return 0;
}
