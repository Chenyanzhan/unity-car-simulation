#include "navigation.h"
#include <QtWidgets/QApplication>
#include <fstream>
#include "opengl.h"

int main(int argc, char *argv[])
{
	/* ��console�����Ϣ */
	/*AllocConsole();
	FILE *stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);*/
	/* ����qt�������л��������򿪴��� */
    QApplication a(argc, argv);
	/* ���������� */
    Navigation w;
	/* opengl���� */
	/*myOpenGl opengl_win;*/
	/* ��������� */
	//w.ConnectOpengl(&opengl_win);		//����opengl����
    w.show();			//��������ʾ
    return a.exec();
}
