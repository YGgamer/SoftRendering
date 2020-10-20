#include<iostream>
#include"win32.h"
#include <assert.h>

int main()
{
	int width = 600, height = 600;
	unsigned char* framebuffer;
	framebuffer = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 4);
	assert(framebuffer != NULL);
	memset(framebuffer, 0, sizeof(unsigned char) * width * height * 4);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int index = (i * width + j) * 4;

			framebuffer[index + 2] = 80;
			framebuffer[index + 1] = 56;
			framebuffer[index] = 56;
		}
	}

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if (i == j)
			{
				int index = (i * width + j) * 4;

				framebuffer[index + 2] = 255;
				framebuffer[index + 1] = 255;
				framebuffer[index] = 255;
			}
		}
	}
	
	window_init(width, height, "MyWindow");
	while (!window->is_close)
	{
		//display image
		window_draw(framebuffer);
		msg_dispatch();
	}
	system("pause");
	return 0;
}