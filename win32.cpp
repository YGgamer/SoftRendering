#include"win32.h"
#include <assert.h>

MyWindow* window = MyWindow::get_instance();

static LRESULT CALLBACK msg_callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE://Ӧ�ó���Ӧ�ñ��ر�ʱ�����������Ը�⣬�������û�����Ƿ����Ҫ�˳�
		window->is_close = 1;
		break;
	case WM_DESTROY://�������ڹر�ʱ�����������Ѿ����Ӿ��ϱ�ɾ��������ζ��Ӧ�ó�������ˣ���Ϊ��������û�д��ڵ������¼�������
		PostQuitMessage(0);//ϣ������Ӧ�ó����򷢳�WM_QUIT
		break;
	case WM_QUIT://Ӧ�ó������������һ����Կ��ɽ��̱�kill��������������������Ӧ����������ǵ���SendMessageֱ�ӷ��͸��¼�
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

	//ע�ᴰ����
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_BYTEALIGNCLIENT;							//���ڷ��
	wc.lpfnWndProc = (WNDPROC)msg_callback;				//�ص�����
	wc.cbClsExtra = 0;										//�����ڴ�����β����һ�����ռ䣬��������Ϊ0
	wc.cbWndExtra = 0;										//�����ڴ���ʵ��β����һ�����ռ䣬��������Ϊ0
	wc.hInstance = GetModuleHandle(NULL);					//��ǰʵ�����
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);				//������ͼ��
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//�����ʽ
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//������ʽ
	wc.lpszMenuName = NULL;									//�˵�
	wc.lpszClassName = "SoftRendering_window";					//�ô����������
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	ATOM atom = RegisterClassEx(&wc); //ע�ᴰ����
	assert(atom != 0);

	//��������
	this->h_window = CreateWindowEx(NULL, "SoftRendering_window", title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	assert(this->h_window != NULL);

	//��ü�����DC
	HDC hDC = GetDC(this->h_window);
	this->mem_dc = CreateCompatibleDC(hDC);
	ReleaseDC(this->h_window, hDC);

	//��ʼ������
	LOGFONT logfont;
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfHeight = 30; //��������Ĵ�С
	this->hf_cur = CreateFontIndirect(&logfont);
	assert(this->hf_cur != NULL);
	this->hf_old = (HFONT)SelectObject(this->mem_dc, this->hf_cur);
	SetTextColor(this->mem_dc, RGB(255, 255, 255));
	//SetBkMode(this->mem_dc, TRANSPARENT);
	SetBkColor(this->mem_dc, RGB(0, 0, 0));

	//��ʼ��λͼ��ʽ
	BITMAPINFOHEADER bi;
	memset(&bi, 0, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;   //���������½�Ϊ��ʼ�㣬�������ϣ����������Ͻ�Ϊ��㣬��������
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = width * height * 4;

	//�����豸�޹�λͼ���
	LPVOID ptr; //����void *
	this->bm_dib = CreateDIBSection(this->mem_dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &ptr, 0, 0);
	assert(this->bm_dib != NULL);
	//���´�����λͼ���д��mem_dc
	this->bm_old = (HBITMAP)SelectObject(this->mem_dc, this->bm_dib);
	this->window_fb = (unsigned char*)ptr;

	//��¼���ڿ��
	this->width = width;
	this->height = height;

	//�������ڿͻ���Ϊ������Ҫ�Ĵ�С
	int wx, wy, sx, sy;
	RECT rect = { 0, 0, width - 1, height - 1 };//һ�����η�Χ ��������
	AdjustWindowRectEx(&rect, GetWindowLong(this->h_window, GWL_STYLE), GetMenu(this->h_window) != NULL, GetWindowLong(this->h_window, GWL_EXSTYLE));
	wx = rect.right - rect.left;//�������ڵĿ�
	wy = rect.bottom - rect.top;//�������ڵĸ�
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2; // GetSystemMetrics(SM_CXSCREEN)��ȡ����Ļ�ķֱ���
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2; //�ҵ���1920*1080�� ������Ϊ���������λ��
	if (sy < 0) sy = 0;

	SetWindowPos(this->h_window, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(this->h_window);
	ShowWindow(this->h_window, SW_NORMAL);

	//��Ϣѭ��
	this->dispatchMsg();

	//��ʼ��keys, window_fbȫΪ0
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
			SelectObject(this->mem_dc, this->bm_old);//д��ԭ����bitmap�������ͷ�DC��
			this->bm_old = NULL;
		}
		if (this->bm_dib)
		{
			DeleteObject(this->bm_dib);
			this->bm_dib = NULL;
		}
		if (this->hf_old)
		{
			SelectObject(this->mem_dc, this->hf_old);//д��ԭ����bitmap�������ͷ�DC��
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
		//CloseWindow(this->h_window);//��С��ָ���Ĵ��ڣ����������ٸô���
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
	//Ŀ����ε����Ͻ�(x,y), ��ȣ��߶ȣ�������ָ��
	BitBlt(hDC, 0, 0, this->width, this->height, this->mem_dc, 0, 0, SRCCOPY);
	ReleaseDC(this->h_window, hDC);
}

void MyWindow::dispatchMsg()
{
	MSG msg;
	while (1)
	{
		//Peek��������Get��������PM_NOREMOVE��ʾ�������Ϣ������������������Get����
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break; //û��Ϣ���ȷ������Ϣ����Get
		if (!GetMessage(&msg, NULL, 0, 0)) break;

		TranslateMessage(&msg);	 //ת����Ϣ ���ⰴ��->�ַ�
		DispatchMessage(&msg); //������Ϣ���ص�
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