#include<iostream>
#include"win32.h"
#include <assert.h>

const int WINDOW_HEIGHT = 640;
const int WINDOW_WIDTH = 960;

const UINT BPP = 32; //bit-per-pixel

//设备无关位图不支持透明通道？32位大小是为了便于寻址？
//屏幕内存中颜色存储顺序为argb
#define RGBA32BIT8888(r,g,b,a) ((b & 0xff) | ((g & 0xff) << 8) | ((r & 0xff) << 16) | ((a & 0xff) << 24))

//异步获取按键状态
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

inline void drawPoint(int x, int y, UINT32* framebuffer)
{
	//裁剪，根据窗口起始位置（0，0）和宽高，针对每一像素，速度慢
	if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
	{
		framebuffer[x + y * WINDOW_WIDTH] = RGBA32BIT8888(255, 255, 0, 0);
	}
}

//矩形区域裁剪，速度较快
void blitClipped(int x, int y, int width, int height, UINT32* imgbuffer, UINT32* framebuffer)
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

void drawLine_bresenham(int x0, int y0, int x1, int y1, UINT32* framebuffer)
{

}

void clearBuffer(UINT32* framebuffer)
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

	clearBuffer((UINT32*)framebuffer);

	//测试逐像素裁剪
	for (int i = 0; i < 50; i++)
	{
		drawPoint(i + WINDOW_WIDTH - 30, 100, (UINT32*)framebuffer);
	}

	//测试矩形裁剪和透明色
	int imgWidth = 100;
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
	blitClipped(-30, -70, imgWidth, imgHeight, imgbuffer, (UINT32*)framebuffer);
	
	//start to render
	int num_frames = 0;
	float print_time = platform_get_time();
	while (!window->is_close)
	{
		float curr_time = platform_get_time();

		window->dispatchMsg();

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

	//free memory
	window->destroy();

	return 0;
}