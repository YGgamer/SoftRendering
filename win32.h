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
	HWND h_window;//���ھ��
	HDC mem_dc;//�봰�ڼ��ݵ��ڴ�DC
	HBITMAP bm_old;//�ڴ�DCԭ����λͼ��������ͷ�ʱҪ��ѡ��ɵ�
	HBITMAP bm_dib;//�豸�޹�λͼ��������ڴ�DCʹ��
	HFONT hf_old;//�ڴ�DCԭ���������������ͷ�ʱҪ��ѡ��ɵ�
	HFONT hf_cur;//�½����ڴ�DC�������
	unsigned char* window_fb;//�豸�޹�λͼ��ͼ���ڴ�ռ䣬����ʵ�ʲ�����������ڴ�
	int width;//���ڿ��
	int height;//���ڸ߶�
	char keys[512];//����״̬
	char buttons[2];//���״̬��left button��0�� right button��1
	bool is_close;//�Ƿ�رմ���
	//mouse_t mouse_info;

	//�������ڣ�ÿ����ֻ����һ�����ڱ�����
	bool create(int width, int height, const char* title);
	//���ٴ���
	bool destroy();
	//��ʾͼ����Ϣ����Ļ
	void display();
	//����windows��Ϣ
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