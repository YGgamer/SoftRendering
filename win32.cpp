#include"win32.h"
#include <assert.h>

MyWindow* window = MyWindow::get_instance();

static LRESULT CALLBACK msg_callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE://应用程序应该被关闭时发出，如果你愿意，可以向用户提出是否真的要退出
		window->is_close = 1;
		break;
	case WM_DESTROY://窗口正在关闭时发出，窗口已经从视觉上被删除，不意味着应用程序结束了，因为它可以在没有窗口的条件下继续运行
		PostQuitMessage(0);//希望结束应用程序，则发出WM_QUIT
		break;
	case WM_QUIT://应用程序结束发出，一般可以看成进程被kill掉的情况。正常情况下响应不到这里，除非调用SendMessage直接发送该事件
		MessageBox(NULL, "end", "title", MB_OK);
		break;
	case WM_KEYDOWN:
		window->keys[wParam & 511] = 1;
		break;
	case WM_KEYUP:
		window->keys[wParam & 511] = 0;
		break;
	case WM_LBUTTONDOWN:
		//window->mouse_info.orbit_pos = get_mouse_pos();
		window->buttons[0] = 1; break;
	case WM_LBUTTONUP:
		window->buttons[0] = 0;
		break;
	case WM_RBUTTONDOWN:
		//window->mouse_info.fv_pos = get_mouse_pos();
		window->buttons[1] = 1;
		break;
	case WM_RBUTTONUP:
		window->buttons[1] = 0;
		break;
	case WM_MOUSEWHEEL:
		//window->mouse_info.wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;

	default: return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

//vec2 get_mouse_pos()
//{
//	POINT point;
//	GetCursorPos(&point);
//	ScreenToClient(window->h_window, &point);
//	return vec2((float)point.x, (float)point.y);
//}


MyWindow::MyWindow()
{
	memset(this, 0, sizeof(MyWindow));
}

MyWindow::~MyWindow()
{
	this->destroy();
}

bool MyWindow::create(int width, int height, const char* title)
{
	if (this->h_window != NULL) return false;

	memset(this, 0, sizeof(MyWindow));

	//注册窗口类
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_BYTEALIGNCLIENT;							//窗口风格
	wc.lpfnWndProc = (WNDPROC)msg_callback;				//回调函数
	wc.cbClsExtra = 0;										//紧跟在窗口类尾部的一块额外空间，不用则设为0
	wc.cbWndExtra = 0;										//紧跟在窗口实例尾部的一块额外空间，不用则设为0
	wc.hInstance = GetModuleHandle(NULL);					//当前实例句柄
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);				//任务栏图标
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//光标样式
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//背景样式
	wc.lpszMenuName = NULL;									//菜单
	wc.lpszClassName = "SoftRendering_window";					//该窗口类的名字
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	ATOM atom = RegisterClassEx(&wc); //注册窗口类
	assert(atom != 0);

	//创建窗口
	this->h_window = CreateWindowEx(NULL, "SoftRendering_window", title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	assert(this->h_window != NULL);

	//获得兼容性DC
	HDC hDC = GetDC(this->h_window);
	this->mem_dc = CreateCompatibleDC(hDC);
	ReleaseDC(this->h_window, hDC);

	//初始化字体
	LOGFONT logfont;
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfHeight = 30; //设置字体的大小
	this->hf_cur = CreateFontIndirect(&logfont);
	assert(this->hf_cur != NULL);
	this->hf_old = (HFONT)SelectObject(this->mem_dc, this->hf_cur);
	SetTextColor(this->mem_dc, RGB(255, 255, 255));
	//SetBkMode(this->mem_dc, TRANSPARENT);
	SetBkColor(this->mem_dc, RGB(0, 0, 0));

	//初始化位图格式
	BITMAPINFOHEADER bi;
	memset(&bi, 0, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;   //正数，左下角为起始点，从下向上，负数，左上角为起点，从上向下
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = width * height * 4;

	//创建设备无关位图句柄
	LPVOID ptr; //就是void *
	this->bm_dib = CreateDIBSection(this->mem_dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &ptr, 0, 0);
	assert(this->bm_dib != NULL);
	//把新创建的位图句柄写入mem_dc
	this->bm_old = (HBITMAP)SelectObject(this->mem_dc, this->bm_dib);
	this->window_fb = (unsigned char*)ptr;

	//记录窗口宽高
	this->width = width;
	this->height = height;

	//调整窗口客户区为我们想要的大小
	int wx, wy, sx, sy;
	RECT rect = { 0, 0, width - 1, height - 1 };//一个矩形范围 左上右下
	AdjustWindowRectEx(&rect, GetWindowLong(this->h_window, GWL_STYLE), GetMenu(this->h_window) != NULL, GetWindowLong(this->h_window, GWL_EXSTYLE));
	wx = rect.right - rect.left;//整个窗口的宽
	wy = rect.bottom - rect.top;//整个窗口的高
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2; // GetSystemMetrics(SM_CXSCREEN)获取你屏幕的分辨率
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2; //我的是1920*1080， 这里是为了算出中心位置
	if (sy < 0) sy = 0;

	SetWindowPos(this->h_window, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(this->h_window);
	ShowWindow(this->h_window, SW_NORMAL);

	//消息循环
	this->dispatchMsg();

	//初始化keys, window_fb全为0
	memset(this->window_fb, 0, sizeof(UCHAR) * width * height * 4);
	memset(this->keys, 0, sizeof(char) * 512);

	return true;
}

bool MyWindow::destroy()
{
	if (this->mem_dc)
	{
		if (this->bm_old)
		{
			SelectObject(this->mem_dc, this->bm_old);//写入原来的bitmap，才能释放DC！
			this->bm_old = NULL;
		}
		if (this->bm_dib)
		{
			DeleteObject(this->bm_dib);
			this->bm_dib = NULL;
		}
		if (this->hf_old)
		{
			SelectObject(this->mem_dc, this->hf_old);//写入原来的bitmap，才能释放DC！
			this->hf_old = NULL;
		}
		if (this->hf_cur)
		{
			DeleteObject(this->hf_cur);
			this->hf_cur = NULL;
		}
		DeleteDC(this->mem_dc);
		this->mem_dc = NULL;
	}
	if (this->h_window)
	{
		//CloseWindow(this->h_window);//最小化指定的窗口，但并不销毁该窗口
		DestroyWindow(this->h_window);
		this->h_window = NULL;
	}
	
	return true;
}

void MyWindow::display()
{
	HDC hDC = GetDC(this->h_window);
	TextOut(this->mem_dc, 20, 20,
		"Control:hold left buttion to rotate, right button to pan",
		strlen("Control:hold left buttion to rotate, right button to pan"));
	//目标矩形的左上角(x,y), 宽度，高度，上下文指针
	BitBlt(hDC, 0, 0, this->width, this->height, this->mem_dc, 0, 0, SRCCOPY);
	ReleaseDC(this->h_window, hDC);
}

void MyWindow::dispatchMsg()
{
	MSG msg;
	while (1)
	{
		//Peek不阻塞，Get会阻塞，PM_NOREMOVE表示如果有消息不处理（留给接下来的Get处理）
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break; //没消息就溜，确定有消息再用Get
		if (!GetMessage(&msg, NULL, 0, 0)) break;

		TranslateMessage(&msg);	 //转换消息 虚拟按键->字符
		DispatchMessage(&msg); //传送消息给回调
	}
}


/* misc platform functions */

static double get_native_time(void) {
	static double period = -1;
	LARGE_INTEGER counter;
	if (period < 0) {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		period = 1 / (double)frequency.QuadPart;
	}
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * period;
}

float platform_get_time(void) {
	static double initial = -1;
	if (initial < 0) {
		initial = get_native_time();
	}
	return (float)(get_native_time() - initial);
}