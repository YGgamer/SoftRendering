#pragma once

struct Vec2i
{
	Vec2i(int x = 0, int y = 0)
	{
		this->x = x;
		this->y = y;
	}
	int x;
	int y;
};

struct vec2
{
	vec2(float x, float y)
	{
		this->x = x;
		this->y = y;
	}
	float x;
	float y;
};

struct vec3
{
	float x;
	float y;
	float z;
};