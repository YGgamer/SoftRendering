#pragma once
#include <Windows.h>
//#include "../core/maths.h"

//typedef struct mouse
//{
//	//orbit
//	vec2 orbit_pos;
//	vec2 orbit_delta;
//	//first-person view (diabled now)
//	vec2 fv_pos;
//	vec2 fv_delta;
//	//wheel
//	float wheel_delta;
//}mouse_t;

class MyWindow
{
public:
	HWND h_window;//窗口句柄
	HDC mem_dc;//与窗口兼容的内存DC
	HBITMAP bm_old;//内存DC原本的位图句柄，在释放时要先选会旧的
	HBITMAP bm_dib;//设备无关位图句柄，供内存DC使用
	HFONT hf_old;//内存DC原本的字体句柄，在释放时要先选会旧的
	HFONT hf_cur;//新建的内存DC字体对象
	unsigned char* window_fb;//设备无关位图的图像内存空间，我们实际操作的是这块内存
	int width;//窗口宽度
	int height;//窗口高度
	char keys[512];//按键状态
	char buttons[2];//鼠标状态，left button—0， right button—1
	bool is_close;//是否关闭窗口
	//mouse_t mouse_info;

	//创建窗口，每个类只能有一个窗口被创建
	bool create(int width, int height, const char* title);
	//销毁窗口
	bool destroy();
	//显示图像信息到屏幕
	void display();
	//处理windows消息
	void dispatchMsg();
	//vec2 get_mouse_pos();

	~MyWindow();
	MyWindow(const MyWindow&) = delete;
	MyWindow& operator=(const MyWindow&) = delete;
	static MyWindow* get_instance() {
		static MyWindow instance;
		return &instance;
	}
private:
	MyWindow();
};

extern MyWindow* window;

float platform_get_time(void);