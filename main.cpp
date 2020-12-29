#include<iostream>
#include"win32.h"
#include"myMath.h"
#include <assert.h>
#include <algorithm>

const int WINDOW_HEIGHT = 640;
const int WINDOW_WIDTH = 960;

const UINT BPP = 32; //bit-per-pixel

//设备无关位图不支持透明通道？32位大小是为了便于寻址？
//屏幕内存中颜色存储顺序为argb
#define RGBA32BIT8888(r,g,b,a) ((b & 0xff) | ((g & 0xff) << 8) | ((r & 0xff) << 16) | ((a & 0xff) << 24))

//异步获取按键状态
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

int swap1, swap2, total1, total2, draw1, draw2;

inline void drawPoint(int x, int y, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//裁剪，根据窗口起始位置（0，0）和宽高，针对每一像素，速度慢
	if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
	{
		framebuffer[x + y * WINDOW_WIDTH] = color;
	}
}

//矩形区域裁剪，速度较快
void blitClipped(int x, int y, int width, int height, UINT32* imgbuffer, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//矩形左上和右下角位置
	int x1 = x;
	int y1 = y;
	int x2 = x + width - 1;
	int y2 = y + height - 1;

	//矩形或者屏幕的右下角坐标都是(width - 1, height -1)，注意条件中什么时候该加等号
	//首先确定区域是否完全不可见
	//注意，这里若窗口宽高定义为UINT类型，则当x、y为负值时判断会不通过
	if (x2 < 0 || x1 >= WINDOW_WIDTH || y2 < 0 || y1 >= WINDOW_HEIGHT)	return;

	//确定可见区域
	if (x1 < 0)	x1 = 0;
	if (y1 < 0)	y1 = 0;
	if (x2 >= WINDOW_WIDTH) x2 = WINDOW_WIDTH - 1;
	if (y2 >= WINDOW_HEIGHT) y2 = WINDOW_HEIGHT - 1;

	//计算x和y的偏移量
	int x_offset = x1 - x;
	int y_offset = y1 - y;

	//定位指针
	framebuffer += (x1 + y1 * WINDOW_WIDTH);
	imgbuffer += (x_offset + y_offset * width);

	//计算要复制内容的宽高
	int copyWidth = x2 - x1 + 1;
	int copyHeight = y2 - y1 + 1;
	UINT pixel;
	UINT transColor = RGBA32BIT8888(0, 255, 0, 0);//注意这里透明设为了0，和下面的透明色对比逻辑有关，因为移位补零
	for (int index_y = 0; index_y < copyHeight; index_y++)
	{
		for (int index_x = 0; index_x < copyWidth; index_x++)
		{
			pixel = imgbuffer[index_x];
			//过滤透明色，左移再右移去除透明通道的干扰
			if ((pixel << 8 >> 8 ) != transColor)
			{
				framebuffer[index_x] = pixel;
			}
		}
		//移动指针到下一行
		framebuffer += WINDOW_WIDTH;
		imgbuffer += width;
	}
}

void drawLine_bresenham(Vec2i p0, Vec2i p1, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	int dx, dy, dx2, dy2, x_inc, y_inc, error;
	framebuffer += (p0.x + p0.y * WINDOW_WIDTH);
	
	dx = p1.x - p0.x;
	dy = p1.y - p0.y;

	if (dx >= 0)
	{
		x_inc = 1;
	}
	else
	{
		x_inc = -1;
		dx = -dx;
	}

	if (dy >= 0)
	{
		y_inc = WINDOW_WIDTH;
	}
	else
	{
		y_inc = -WINDOW_WIDTH;
		dy = -dy;
	}

	dx2 = dx << 1;
	dy2 = dy << 1;

	if (dx > dy)
	{
		error = dy2 - dx;

		for (int i = 0; i <= dx; i++)
		{
			*framebuffer = color;

			if (error >= 0)
			{
				error -= dx2;
				framebuffer += y_inc;
			}

			error += dy2;
			framebuffer += x_inc;
		}
	}
	else
	{
		error = dx2 - dy;

		for (int i = 0; i <= dy; i++)
		{
			*framebuffer = color;

			if (error >= 0)
			{
				error -= dy2;
				framebuffer += x_inc;
			}

			error += dx2;
			framebuffer += y_inc;
		}
	}
}

inline void drawLine(Vec2i p0, Vec2i p1, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//version 1, 点的数量数固定的，过少画出分离的点，过多浪费性能
	/*for (float t = 0.0; t < 1.0; t += 0.01) {
		int x = x0 + (x1 - x0) * t;
		int y = y0 + (y1 - y0) * t;
		drawPoint(x, y, color);
	}*/

	//version 2, 需要x1大于x0，且若斜率太大则画出的是分离的点
	//for (int x = x0; x <= x1; x++) {
	//	float t = (x - x0) / (float)(x1 - x0);
	//	int y = y0 * (1.0 - t) + y1 * t;//= y0 - y0 * t + y1 * t = y0 + (y1 - y0) * t
	//	drawPoint(x, y, color);
	//}

	//version 3, 解决版本2的两个问题，但效率低，计算t时有大量除法
	//bool steep = false;
	//if (std::abs(x0 - x1) < std::abs(y0 - y1)) { // if the line is steep, we transpose the image 
	//	std::swap(x0, y0);
	//	std::swap(x1, y1);
	//	steep = true;
	//}
	//if (x0 > x1) { // make it left-to-right
	//	std::swap(x0, x1);
	//	std::swap(y0, y1);
	//}
	//for (int x = x0; x <= x1; x++) {
	//	float t = (x - x0) / (float)(x1 - x0);
	//	int y = y0 * (1. - t) + y1 * t;
	//	if (steep) {
	//		drawPoint(y, x, color); // if transposed, de-transpose 
	//	}
	//	else {
	//		drawPoint(x, y, color);
	//	}
	//}

	//version 4，增加误差项，减少除法运算
	/*bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	float derror = std::abs(dy / float(dx));
	float error = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep) {
			drawPoint(y, x, color);
		}
		else {
			drawPoint(x, y, color);
		}
		error += derror;
		if (error > 0.5) {
			y += (y1 > y0 ? 1 : -1);
			error -= 1.0;
		}
	}*/

	//version 5, 去除浮点数
	bool steep = false;
	if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
		std::swap(p0.x, p0.y);
		std::swap(p1.x, p1.y);
		steep = true;
	}
	if (p0.x > p1.x) {
		std::swap(p0.x, p1.x);
		std::swap(p0.y, p1.y);
	}
	int dx = p1.x - p0.x;
	int dy = p1.y - p0.y;
	int derror2 = std::abs(dy) << 1;
	int error2 = 0;
	int y = p0.y;
	int yOffset = p1.y > p0.y ? 1 : -1;
	int dx2 = dx << 1;
	for (int x = p0.x; x <= p1.x; x++) {
		if (steep) {
			//drawPoint(y, x, color);
			if (y >= 0 && y < WINDOW_WIDTH && x >= 0 && x < WINDOW_HEIGHT)
			{
				framebuffer[y + x * WINDOW_WIDTH] = color;
			}
		}
		else {
			//drawPoint(x, y, color);
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
			}
		}
		error2 += derror2;
		if (error2 > dx) {
			y += yOffset;
			error2 -= dx2;
		}
	}
}

void getNum(Vec2i p0, Vec2i p1, float& slope, float& b)
{
	if (p1.x != p0.x) {
		slope = (p1.y - p0.y) / (float)(p1.x - p0.x);
		b = p0.y - slope * p0.x;
	}
}

bool compare(Vec2i a, Vec2i b)
{
	return a.x < b.x;   //升序排列，如果改为return a>b，则为降序

}

bool compare2(Vec2i a, Vec2i b)
{
	return a.y < b.y;   //升序排列，如果改为return a>b，则为降序

}

void drawTriangle(Vec2i p0, Vec2i p1, Vec2i p2, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//从左到右排序
	Vec2i array[3] = {p0, p1, p2};
	std::sort(array, array + 3, compare);

	//先计算斜率和截距以确定直线方程
	float slope1, slope2, slope3, b1, b2, b3;
	getNum(array[0], array[1], slope1, b1);
	getNum(array[1], array[2], slope2, b2);
	getNum(array[2], array[0], slope3, b3);
	
	//画左侧
	for (int x = array[0].x; x < array[1].x; x++)
	{
		int y0 = slope1 * x + b1;
		int y1 = slope3 * x + b3;
		if (y0 > y1) {
			std::swap(y0, y1);
			swap1++;
		}
		for (int y = y0; y <= y1; y++) {
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
				draw1++;
			}
		}
		total1++;
	}
	//画右侧（含中间线段）
	for (int x = array[1].x; x <= array[2].x; x++)
	{
		int y0 = slope2 * x + b2;
		int y1 = slope3 * x + b3;
		if (y0 > y1) {
			std::swap(y0, y1);
			swap1++;
		}
		for (int y = y0; y <= y1; y++) {
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
				draw1++;
			}
		}
		total1++;
	}
}

void drawTriangle2(Vec2i p0, Vec2i p1, Vec2i p2, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//从上到下排序
	Vec2i array[3] = { p0, p1, p2 };
	std::sort(array, array + 3, compare2);
	
	//计算 delatas x/ deltas y 的值，即斜率分之一（y每变化1 x的变化量）
	float dxy1 = (array[1].x - array[0].x) / (float)(array[1].y - array[0].y);
	float dxy2 = (array[2].x - array[1].x) / (float)(array[2].y - array[1].y);
	float dxy3 = (array[0].x - array[2].x) / (float)(array[0].y - array[2].y);

	//画上方
	float tempX1 = array[0].x;
	float tempX2 = array[0].x;
	for (int y = array[0].y; y < array[1].y; y++)
	{
		int x0 = tempX1;
		int x1 = tempX2;
		if (x0 > x1) {
			std::swap(x0, x1);
			swap2++;
		}
		for (int x = x0; x <= x1; x++) {
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
				draw1++;
			}
		}
		tempX1 += dxy1;
		tempX2 += dxy3;
		total2++;
	}
	//画下方（含中间线段）
	tempX1 = array[1].x;
	//tempX2 = array[2].x;//同一条直线，可以继续接着上方的运算
	for (int y = array[1].y; y <= array[2].y; y++)
	{
		int x0 = tempX1;
		int x1 = tempX2;
		if (x0 > x1) {
			std::swap(x0, x1);
			swap2++;
		}
		for (int x = x0; x <= x1; x++) {
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
				draw1++;
			}
		}
		tempX1 += dxy2;
		tempX2 += dxy3;
		total2++;
	}
}

void clearBuffer(UINT32* framebuffer = (UINT32*)window->window_fb)
{
	for (int x = 0; x < WINDOW_WIDTH; x++)
	{
		for (int y = 0; y < WINDOW_HEIGHT; y++)
		{
			framebuffer[x + y * WINDOW_WIDTH] = RGBA32BIT8888(56, 56, 80, 0);
		}
	}
}

int main()
{	
	window->create(WINDOW_WIDTH, WINDOW_HEIGHT, "MyWindow");
	UCHAR* framebuffer = window->window_fb;
	assert(framebuffer != NULL);

	clearBuffer();

	//测试逐像素裁剪
	/*for (int i = 0; i < 50; i++)
	{
		drawPoint(i + WINDOW_WIDTH - 30, 100, RGBA32BIT8888(255, 255, 0, 0));
	}*/

	//测试矩形裁剪和透明色
	/*int imgWidth = 100;
	int imgHeight = 100;
	UINT32* imgbuffer = (UINT32*)malloc(sizeof(UINT32) * imgWidth * imgHeight);
	assert(imgbuffer != NULL);
	for (int y = 0; y < imgHeight; y++)
	{
		for (int x = 0; x < imgWidth; x++)
		{
			if (x == y)
			{
				imgbuffer[x + y * imgWidth] = RGBA32BIT8888(0, 255, 0, 0);
			}
			else
			{
				imgbuffer[x + y * imgWidth] = RGBA32BIT8888(255, 0, 0, 0);
			}
		}
	}
	blitClipped(-30, -70, imgWidth, imgHeight, imgbuffer);*/

	//drawLine_bresenham(vec2i(100, 100), vec2i(300, 300), RGBA32BIT8888(255, 0, 0, 0));
	
	//start to render
	int num_frames = 0;
	float print_time = platform_get_time();
	while (!window->is_close)
	{
		float curr_time = platform_get_time();

		window->dispatchMsg();

		//测试画线段函数效率
		/*Vec2i pos[6] = { Vec2i(13, 25), Vec2i(80, 45), Vec2i(20, 13), Vec2i(40, 80), Vec2i(80, 40), Vec2i(13, 20) };
		for (int i = 0; i < 10000; i++) {
			drawLine(pos[0], pos[1], RGBA32BIT8888(255, 255, 255, 0));
			drawLine(pos[2], pos[3], RGBA32BIT8888(255, 255, 0, 0));
			drawLine(pos[4], pos[5], RGBA32BIT8888(255, 255, 0, 0));
		}*/

		swap1 = swap2 = total1 = total2 = draw1 = draw2 = 0;
		Vec2i pos[3] = { Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80) };
		Vec2i pos1[3] = { Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180) };
		Vec2i pos2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
		for (int i = 0; i < 1000; i++) {
			drawTriangle(pos[0], pos[1], pos[2], RGBA32BIT8888(255, 255, 0, 0));
			drawTriangle(pos1[0], pos1[1], pos1[2], RGBA32BIT8888(255, 255, 255, 0));
			drawTriangle(pos2[0], pos2[1], pos2[2], RGBA32BIT8888(255, 0, 0, 0));
		}

		//display image
		window->display();

		//calculate FPS
		num_frames += 1;
		if (curr_time - print_time >= 1) {
			int sum_millis = (int)((curr_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;
			printf("fps: %3d, avg: %3d ms\n", num_frames, avg_millis);
			printf("swap1: %3d, total1: %3d, draw1: %3d\n", swap1, total1, draw1);
			printf("swap2: %3d, total2: %3d, draw1: %3d\n", swap2, total2, draw2);
			num_frames = 0;
			print_time = curr_time;
		}
	}

	//free memory
	window->destroy();

	return 0;
}