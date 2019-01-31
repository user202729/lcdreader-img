#pragma once

#include<SDL.h>

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
inline Point operator*(double f,Point p){return p*f;}

