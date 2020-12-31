#include<iostream>
#include"win32.h"
#include"geometry.h"
#include"model.h"
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

	//version 5, 去除浮点数，乘以 2dx
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
			//drawPoint(y, x, color);//不调用drawPoint函数而直接写代码的原因是，debug模式下inline关键字没有生效，导致帧率太低
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

bool compare(Vec2i a, Vec2i b)
{
	return a.y < b.y;   //升序排列，如果改为return a>b，则为降序

}

inline float getDelta(Vec2i p0, Vec2i p1)
{
	if (p1.y != p0.y) {
		return (p1.x - p0.x) / (float)(p1.y - p0.y);
	}
	return 0.0;
}

void drawTriangle(Vec2i p0, Vec2i p1, Vec2i p2, UINT32 color, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	//从上到下排序
	Vec2i array[3] = { p0, p1, p2 };
	std::sort(array, array + 3, compare);
	
	//计算 delta x/ delta y 的值，即斜率分之一（y每变化1 x的变化量）
	//也可以用斜率和截距算，但是循环里就多了一次乘法
	float dxy1 = getDelta(array[0], array[1]);
	float dxy2 = getDelta(array[1], array[2]);
	float dxy3 = getDelta(array[2], array[0]);
	
	//注意是排好序后才计算delta的，所以下面确定初始点的逻辑才成立
	//如果上方不是平底则肯定是第一个点，是的话为第二个点(是平底的话直接会走下半部分的绘制，delta是根据第二个点和第三个点计算的)
	float tempX1 = array[0].y == array[1].y ? array[1].x : array[0].x;
	//这里肯定是第一个点，因为delta是根据第一个点和第三个点计算的
	float tempX2 = array[0].x;
	for (int y = array[0].y; y < array[2].y; y++)
	{
		//这里加0.5更合理，过半要画在下一个点上
		int x0 = tempX1 + 0.5;
		int x1 = tempX2 + 0.5;
		if (x0 > x1) {
			std::swap(x0, x1);
		}
		for (int x = x0; x <= x1; x++) {
			if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
			{
				framebuffer[x + y * WINDOW_WIDTH] = color;
			}
		}
		//过分界线后线段变化，更正斜率
		tempX1 += (y >= array[1].y ? dxy2 : dxy1);
		tempX2 += dxy3;
	}
}

void flip_y(int width = WINDOW_WIDTH, int height = WINDOW_HEIGHT, UINT32* framebuffer = (UINT32*)window->window_fb)
{
	for (int y = 0; y < WINDOW_HEIGHT / 2; y++)
	{
		for (int x = 0; x < WINDOW_WIDTH; x++)
		{
			std::swap(framebuffer[x + y * WINDOW_WIDTH], framebuffer[x + (WINDOW_HEIGHT - y - 1) * WINDOW_WIDTH]);
		}
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

	srand(time(NULL));
	Model* model = new Model("obj/african_head.obj");
	/*for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec2i screen_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f world_coords = model->vert(face[j]);
			screen_coords[j] = Vec2i((world_coords.x + 1.) * WINDOW_WIDTH / 2., (world_coords.y + 1.) * WINDOW_HEIGHT / 2.);
		}
		drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2], RGBA32BIT8888(rand() % 255, rand() % 255, rand() % 255, 0));
	}
	flip_y();*/
	
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
		
		Vec2i pos[3] = { Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80) };
		Vec2i pos1[3] = { Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180) };
		Vec2i pos2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
		Vec2i pos3[3] = { Vec2i(300, 300), Vec2i(720, 600), Vec2i(500, 300) };
		Vec2i pos4[3] = { Vec2i(600, 300), Vec2i(500, 100), Vec2i(800, 300) };
		for (int i = 0; i < 1000; i++) {
			drawTriangle(pos[0], pos[1], pos[2], RGBA32BIT8888(255, 255, 0, 0));
			drawTriangle(pos1[0], pos1[1], pos1[2], RGBA32BIT8888(255, 255, 255, 0));
			drawTriangle(pos2[0], pos2[1], pos2[2], RGBA32BIT8888(255, 0, 0, 0));
			drawTriangle(pos3[0], pos3[1], pos3[2], RGBA32BIT8888(0, 255, 0, 0));
			drawTriangle(pos4[0], pos4[1], pos4[2], RGBA32BIT8888(0, 255, 0, 0));
		}

		//display image
		window->display();

		//calculate FPS
		num_frames += 1;
		if (curr_time - print_time >= 1) {
			int sum_millis = (int)((curr_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;
			printf("fps: %3d, avg: %3d ms\n", num_frames, avg_millis);
			num_frames = 0;
			print_time = curr_time;
		}
	}

	delete model;

	//free memory
	window->destroy();

	return 0;
}