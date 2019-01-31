#include<iostream>
#include<fstream>
#include<vector>
#include<array>
#include<algorithm>
#include<cstdlib>
#include<cassert>
#include<SDL.h>
#include<SDL_image.h>

#include"Point.h"
#include"Grid.h"

SDL_Renderer* renderer;
SDL_Texture* image;
Grid grid;

void draw_image(){
	SDL_RenderCopy(renderer,image,nullptr,nullptr);
}

int preview_alpha=35;

bool preview=false;
bool image_only=false;
void render(){
	draw_image();
	if(!image_only){
		if(preview)
			grid.draw_preview(renderer,preview_alpha);
		else
			grid.draw(renderer);
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

	grid.compute_pxval(image_surface);

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

	int heldCorner=-1; // [0..4[, or -1. For manipulating the grid.corners with mouse.

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
			for(unsigned i=0;i<4;++i){
				int dx=grid.getCorner(i).x-mouse.x;
				int dy=grid.getCorner(i).y-mouse.y;
				if(dx*dx+dy*dy<=20*20){
					heldCorner=i;
					grid.setCorner(i,mouse);
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
			grid.setCorner(heldCorner,mouse);
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
				grid.setCorner(cornerIndex,{x,y});
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
				grid.changeEdgeThreshold(-1);
				render();
				break;

			case SDLK_0:
				grid.changeEdgeThreshold(1);
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

			case SDLK_z:
			case SDLK_x:
			{
				/// Manually change the pixel at the cursor to 0/1
				int x,y;
				SDL_GetMouseState(&x,&y);
				Point point=grid.invP({(double)x,(double)y});
				auto a=(int)std::floor(point.x),b=(int)std::floor(point.y);
				if(a<0||a>=grid.getMaxA()||b<0||b>=grid.getMaxB())
					break;
				grid.setDataManual(a,b,event.key.keysym.sym==SDLK_x?1:0);
				render();
				break;
			}

			case SDLK_o:
				/// Output
			{
				std::ofstream out("out.txt");

				int i=0;
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
			break;
		}
	}

	if(save_config){
		std::ofstream config_f("config.txt");
		for(int i=0;i<4;++i)
			config_f<<grid.getCorner(i).x<<' '<<grid.getCorner(i).y<<'\n';
	}

	SDL_DestroyTexture(image);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	return 0;
}
