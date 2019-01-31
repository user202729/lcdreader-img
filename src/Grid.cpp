#include"Grid.h"

#include<iostream>
#include<cassert>

void Grid::setCorner(int index,SDL_Point point){
	corners[index]=point;
	med_color_cached=false;
	ptlist_cached=false;
	binarize_cached=false;
}

Point Grid::P(double a,double b){
	Point
		u = (Point(corners[1]) - corners[0]) * a / maxA + corners[0],
		d = (Point(corners[3]) - corners[2]) * a / maxA + corners[2];
	return u + (d - u) * b / maxB;
}

Point Grid::invP(Point p){
	int constexpr iter=5;

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

void Grid::compute_pxval(SDL_Surface* image_surface){
	med_color_cached=false;
	binarize_cached=false;

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

	pxval_cached=true;
}

void Grid::draw(SDL_Renderer* renderer){
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

void Grid::draw_preview(SDL_Renderer* renderer,int preview_alpha){
	binarize();
	computePtList();

	for(int a=0;a<maxA;++a)
	for(int b=0;b<maxB;++b){
		if(data_manual[a*maxB+b]>=0?data_manual[a*maxB+b]:data[a*maxB+b])
			SDL_SetRenderDrawColor(renderer,0,0,255,preview_alpha); // blue
		else
			SDL_SetRenderDrawColor(renderer,255,255,255,preview_alpha); // white
		SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);

		auto const& ptl=ptListFull[a][b];
		SDL_RenderDrawPoints(renderer,ptl.data(),ptl.size());
	}
}

void Grid::computePtList(){
	if(ptlist_cached)
		return;

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

	ptlist_cached=true;
}

void Grid::computeMedColor(){
	if(med_color_cached)
		return;

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

	med_color_cached=true;
}

void Grid::binarize(){
	if(binarize_cached)
		return;

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
	if(data_manual.size()!=data.size())
		data_manual.assign(data.size(),-1);
	/*
	std::sort(adjacentPairs.begin(),adjacentPairs.end(),[](DiffPair x,DiffPair y){
			return x.delta>y.delta;});
			*/

	for(DiffPair p:adjacentPairs)
	if(p.delta>=edge_threshold){
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

	binarize_cached=true;
}

void Grid::changeEdgeThreshold(int delta){
	if(edge_threshold+delta<0||edge_threshold+delta>255)
		return;
	edge_threshold+=delta;
	binarize_cached=false;
}

void Grid::setDataManual(int index,char value){
	data_manual[index]=value;
}
