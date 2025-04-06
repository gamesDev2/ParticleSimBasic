#pragma once

#ifndef _PARTICLE_H_
#define _PARTICLE_H_
#include <cmath>

struct vec2d
{
	float x;
	float y;
};

struct particle
{
	vec2d pos;
	vec2d prePos;

	vec2d vel;

	float mass;
	float charge;

	particle* prev;
	particle* next;

	float density = 1;
};



double dist(vec2d p1, vec2d p2);

#endif

double dist(vec2d p1, vec2d p2)
{

	return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
}