#include "navigation.h"
#include <time.h>
#include <fstream>
#include <iostream>
#include <direct.h>



#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define buffer_size 1024

enum WinUpdateCmd
{
	ConnectStateChange = 0,			/* ����״̬�ı� */
	GetMesFromServer,				/* �ӷ�������ȡ�������� */
	SendDataToServer,				/* �������ݸ������� */
	ShowMap2d,						/* ��ʾ��ͼ */
	UpdateLadar16,					/* ����16���״����� */
};

/*-------- ��־λ���� --------*/
bool usingLadar16 = true;			//ʹ��16���״�

/*-------- �ؼ�ӳ�� --------*/
struct {
	QLabel *IPLabel;
	QLabel *PortLabel;
}Controls;

/*-------- socket�������� --------*/
SOCKET sockClient;
sockaddr_in addrServer = {};

/* �������в������� */
const char* client_ip = "127.0.0.1";		//������ip��ַ
const unsigned int client_port = 8081;		//�������˿ں�
bool connect_flag = false;					//���������ӱ�־λ
const string file_type = ".loc";			//�ļ����ͺ�׺
const string file_path = "../../../__buffer__/__loc__/";
const string file_path16 = "../../../__buffer__/__loc16__/";

/*-------- �������в��� --------*/
//���յ�����Ϣ
typedef struct {
	queue<string> mes_queue;			//��Ϣ��ʾ���У�������ʾ
	queue<string> t_queue;				//ʱ����У�����Ϣ��ʾ����ͬ��
	queue<string> mes_pro;				//��Ϣ������У�����Ϣ��������Ϣ��ʾ�ֿ�����
}MesRece_t;		
static MesRece_t MesRece;
//���͵���Ϣ
typedef struct {
	vector<string> stack;				//��Ϣջ��
	vector<string> t_stack;				//ʱ��ջ��������Ϣջ��ͬ��
}MesSend_t;		
static MesSend_t MesSend;
queue<int> cmd_queue;					//�����������
//��ͼ
int Map2d_h = 500, Map2d_w = 500;
Mat Map2d(Map2d_h, Map2d_w, CV_8UC3);
float Map2dRuler = 0.3;					//һ�����ص��ڶ�����
//��ͼ��ɫ��Ϣ
Scalar MapColor_wall(0, 0, 0);
Scalar MapColor_ground(255, 255, 255);
Scalar MapColor_map(127, 127, 127);
//λ����Ϣ
typedef struct {
	//������Ϣ
	float x, y, z;					//λ����Ϣ
	float speed;					//�ٶ���Ϣ
	float angle;					//��ת�Ƕ�
	//�����״���Ϣ
	float hit_x, hit_y, hit_z;		//ɨ�赽�ĵ��λ��
	//16�߼����״���Ϣ
	float hit_x16[16], hit_y16[16], hit_z16[16];		//ɨ�赽�ĵ��λ��
}locData_t;
static locData_t locData;

/*-------- �������� --------*/
DWORD WINAPI ClientThread(LPVOID lpParam);			//�ͻ����������߳�
DWORD WINAPI ClientDataRecThread(LPVOID lpParam);	//�ͻ������ݽ������߳�
QString str2qstr(const string str);
void QClear(queue<string>& q);						//��ն���
void GetlocData(string path);						//��ȡlocData
void Map2dInit();									//��ʼ����ͼ
void DrawMap2d();									//���Ƶ�ͼ
QImage cvMat2QImage(const cv::Mat& mat);

/* Navigation���캯�� */
Navigation::Navigation(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	/* ���ÿؼ����� */
	ui.ServerIp->setText(QString(client_ip));
	char port_str[10];
	sprintf(port_str, "%d", client_port);
	ui.ServerPort->setText(QString(port_str));
	/* ����socket�ͻ������� */
	WSADATA wsaDate = {};
	WSAStartup(MAKEWORD(2, 2), &wsaDate);
	sockClient = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	addrServer.sin_family = PF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr(client_ip);
	addrServer.sin_port = htons(client_port);
	if (::connect(sockClient, (SOCKADDR*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR) {
		;
	}
	else {
		connect_flag = true;			//�������ӱ�־λ
		cmd_queue.push(ConnectStateChange);		//������Ϣ�źţ���ʾ�����Ѿ�����
	}

	/* ��ʼ����ͼ������ͼ��ʼ��Ϊ��ɫ */
	Map2dInit();

	/* ���������������߳� */
	Mainloop *t = new Mainloop();
	QObject::connect(t, SIGNAL(WinUpdateSig(int)), this, SLOT(WinUpdate(int)));
	t->start();

	/* �����ͻ��������̣߳���Ҫ������Ϣ���� */
	HANDLE clientThread;
	DWORD clientThreadId;
	clientThread = CreateThread(NULL, NULL, ClientThread, NULL, 0, &clientThreadId);

	/* �����ͻ�����Ϣ�ӽ������߳� */
	HANDLE clientDataRecdThread;
	DWORD clientDataRecThreadId;
	clientDataRecdThread = CreateThread(NULL, NULL, ClientDataRecThread, NULL, 0, &clientDataRecThreadId);

	/* ��ʼ������ָ�� */
	cmd_queue.push(ShowMap2d);		//��Ĭ����ʾ��ʼ��ͼ
}

/* ������²ۺ��� */
void Navigation::WinUpdate(int argv) {
	
	static int rece_index = 0, send_index = 0;
	if (argv == ConnectStateChange) {
		/* ������ӳɹ������޸Ŀؼ���ɫ */
		ui.ServerIp->setStyleSheet(QString::fromUtf8("background-color: rgba(0, 255, 0, 100);"));
		ui.ServerPort->setStyleSheet(QString::fromUtf8("background-color: rgba(0, 255, 0, 100);"));
	}
	else if (argv == GetMesFromServer) {
		/* �����Ϣ���е����дӷ������л�ȡ����Ϣ */
		if (MesRece.mes_queue.empty())
			return;
		if (!ui.ServerRecDataShowCheck->isChecked())
			return;
		/* ��ʾһ����Ϣ */
		ui.ServerDataRecShow->append(str2qstr(MesRece.t_queue.front() + MesRece.mes_queue.front()));
		/* ��Ϣ���� */
		MesRece.mes_queue.pop();
		/* �����Ϣ���࣬�������� */
		rece_index++;
		if (rece_index > 1000) {
			ui.ServerDataRecShow->clear();
			rece_index = 0;
		}
	}
	else if (argv == SendDataToServer) {
		/* ��ʾ���͸�����������Ϣ */
		if (!ui.ServerRecDataShowCheck->isChecked())
			return;
		/* ��ʾ���һ����Ϣ */
		ui.ServerDataSendShow->append(str2qstr(MesSend.t_stack[MesSend.t_stack.size() - 1] + MesSend.stack[MesSend.stack.size() - 1]));
		/* �����Ϣ���࣬�������� */
		send_index++;
		if (send_index > 1000) {
			ui.ServerDataSendShow->clear();
			send_index = 0;
		}
	}
	else if (argv == ShowMap2d) {
		/* ����ͼ��mat����תΪqpixmap */
		QPixmap imgmap = QPixmap::fromImage(cvMat2QImage(Map2d));
		/* ��ʾ��ͼ */
		ui.MapShow->setPixmap(imgmap);
	}
	else if (argv == UpdateLadar16) {
		/* ����16���״� */
		this->opengl_win->GetLadarData(locData.hit_x16, locData.hit_y16, locData.hit_z16);
		this->opengl_win->update();
	}
}

/* ����opengli���� */
void Navigation::ConnectOpengl(myOpenGl *win) {
	this->opengl_win = win;
	this->opengl_win->show();
}


/*-------------------------------------------------- ���̴߳��� --------------------------------------------------*/
Mainloop::Mainloop(QObject *parent) :
	QThread(parent)
{
}

void Mainloop::run()
{
	/* �����������߳� */
	while (true)
	{
		if (!cmd_queue.empty()) {
			/* ���������û��ִ���꣬��ֱ�ӷ��� */
			emit WinUpdateSig(cmd_queue.front());
			cmd_queue.pop();
		}
		if (!MesRece.mes_pro.empty()) {
			/* �����Ϣ���ն���������Ϣδ���� */
			int ret;
			//��ȡ�ļ���
			string filename = MesRece.mes_pro.front();
			//�ж��ļ������Ƿ���ȷ
			ret = filename.find(file_type);
			if (ret != -1) {
				/* �����ǰ�ļ����ǿ��Դ�����ļ������ȡ�ļ����� */
				string path;
				if (!usingLadar16)
					path = file_path + filename;
				else
					path = file_path16 + filename;
				/* ��ȡlocData */
				GetlocData(path);
				/* ����ͼƬ */
				DrawMap2d();
			}
			//��Ϣ������
			MesRece.mes_pro.pop();
		}
	}
}

DWORD WINAPI ClientThread(LPVOID lpParam) {
	/* �ͻ������߳�������ѭ�� */
	while (1) {
		if (!connect_flag) {
			/* ���û�����ӳɹ���������������ӷ����� */
			if (::connect(sockClient, (SOCKADDR*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
				;
			else {
				connect_flag = true;			//�������ӱ�־λ
				cmd_queue.push(ConnectStateChange);		//������Ϣ�źţ���ʾ�����Ѿ�����
			}
			Sleep(1000);
		}
		else {
			///* ������������ӳɹ��� */
			//string str = "navigation tcp connect test";
			///* ������Ϣ */
			//::send(sockClient, str.c_str(), str.length(), 0);
			//MesSend.stack.push_back(str);
			////��ȡϵͳʱ��  
			//time_t now_time = time(NULL);
			////��ȡ����ʱ��  
			//tm*  t_tm = localtime(&now_time);
			//MesSend.t_stack.push_back(asctime(t_tm));
			////�������߳��Ѿ�������Ϣ����������
			//cmd_queue.push(SendDataToServer);
			Sleep(500);
		}
	}
}

DWORD WINAPI ClientDataRecThread(LPVOID lpParam) {
	/* �ͻ�����Ϣ�����߳� */
	while (1) {
		if (connect_flag) {
			/* ��������Ϸ����� */
			char buf[buffer_size];
			int len = ::recv(sockClient, buf, buffer_size, NULL);
			string str = buf;
			//��ȡϵͳʱ��  
			time_t now_time = time(NULL);
			//��ȡ����ʱ��  
			tm*  t_tm = localtime(&now_time);
			//����Ϣ�������
			if (MesRece.mes_queue.size() > 1000) {
				QClear(MesRece.mes_queue);
				QClear(MesRece.t_queue);
			}
			MesRece.mes_queue.push(str);
			MesRece.mes_pro.push(str);
			MesRece.t_queue.push(asctime(t_tm));
			cmd_queue.push(GetMesFromServer);		//���������֪��������Ϣ�Ѿ����ڶ�������
		}
	}
}

/*-------------------------------------------------- ���ߺ��� --------------------------------------------------*/
QString str2qstr(const string str)
{
	return QString::fromLocal8Bit(str.data());
}

void QClear(queue<string>& q) {
	/* ��ն��У��ұ���STL������׼ */
	queue<string> empty;
	swap(empty, q);
}

/* ��ȡlodData */
void GetlocData(string path) {
	const char a = ',';		//���ݼ�ķָ���
	int b, e;
	/* �����ļ������� */
	ifstream fi(path);
	/* ���������ж�ȡ���� */
	string readbuf;
	if (!usingLadar16) {
		/* ��ȡ��һ�����ݣ�Ϊx, y, z */
		fi >> readbuf;
		b = readbuf.find(a);
		e = readbuf.rfind(a);
		locData.x = atof(readbuf.substr(0, b).c_str());
		locData.y = atof(readbuf.substr(b + 1, e - b - 1).c_str());
		locData.z = atof(readbuf.substr(e + 1, readbuf.length() - e).c_str());
		/* ��ȡ�ڶ������ݣ�Ϊhit_x, hit_y, hit_z */
		fi >> readbuf;
		b = readbuf.find(a);
		e = readbuf.rfind(a);
		locData.hit_x = atof(readbuf.substr(0, b).c_str());
		locData.hit_y = atof(readbuf.substr(b + 1, e - b - 1).c_str());
		locData.hit_z = atof(readbuf.substr(e + 1, readbuf.length() - e).c_str());
	}
	else {
		/* ��ȡ��һ�����ݣ�Ϊx, y, z */
		fi >> readbuf;
		b = readbuf.find(a);
		e = readbuf.rfind(a);
		locData.x = atof(readbuf.substr(0, b).c_str());
		locData.y = atof(readbuf.substr(b + 1, e - b - 1).c_str());
		locData.z = atof(readbuf.substr(e + 1, readbuf.length() - e).c_str());
		/* ��ȡ�ڶ������ݣ�Ϊhit_x, hit_y, hit_z */
		for (int i = 0; i < 16; i++) {
			fi >> readbuf;
			b = readbuf.find(a);
			e = readbuf.rfind(a);
			locData.hit_x16[i] = atof(readbuf.substr(0, b).c_str());
			locData.hit_y16[i] = atof(readbuf.substr(b + 1, e - b - 1).c_str());
			locData.hit_z16[i] = atof(readbuf.substr(e + 1, readbuf.length() - e).c_str());
		}
		locData.hit_x = locData.hit_x16[0];
		locData.hit_y = locData.hit_y16[0];
		locData.hit_z = locData.hit_z16[0];
		/* ��������ʾ���Ը���16���״������� */
		/*cmd_queue.push(UpdateLadar16);*/
	}
	/* �ر��ļ� */
	fi.close();
}

/* ��ʵ��λ��ӳ�䵽��ͼ�� */
vector<int> Position2Map(const int in_x, const int in_z) {
	vector<int> res;

	int x, z;
	/* λ��ӳ�� */
	x = int(Map2d_w / 2 - in_x / Map2dRuler);
	z = int(Map2d_h / 2 - in_z / Map2dRuler);
	/* ��ֵ���� */
	x = x > Map2d_w ? Map2d_w : x;
	z = z > Map2d_h ? Map2d_h : z;
	x = x < 0 ? 0 : x;
	z = z < 0 ? 0 : z;

	res.push_back(x);
	res.push_back(z);

	return res;
}

/* ���Ƶ�ͼ */
#define maxLadarDis 100		//�״�������
void DrawMap2d() {
	/* ����߽�����λ�� */
	int hit2d_x, hit2d_z, max2d_x, max2d_z;
	int x, z;
	if (locData.hit_x == 0 && locData.hit_y == 0 && locData.hit_z == 0) {
		//��������Ϊ0����˵��û����ײ
		hit2d_x = Map2d_w;
		hit2d_z = Map2d_h;
		max2d_x = Map2d_w;
		max2d_z = Map2d_h;
		return;
	}
	else {
		/* �������ײ������ײ��ӳ�䵽��ͼ�� */
		vector<int> p;
		/* ��hit����ӳ�䵽��ͼ������ */
		p = Position2Map(locData.hit_x, locData.hit_z);
		hit2d_x = p[0];
		hit2d_z = p[1];
		/* ������λ��ӳ�䵽��ͼ������ */
		p = Position2Map(locData.x, locData.z);
		x = p[0];
		z = p[1];
	}
	/* ��������㵽�߽�İ�ɫֱ��(����ʹ·��) */
	cv::Point p0 = cv::Point(hit2d_x, hit2d_z);
	cv::Point p1 = cv::Point(x, z);
	cv::line(Map2d, p0, p1, MapColor_ground, 3);
	/* ���Ʊ߽�(ǽ��) */
	cv::Point hit = cv::Point(hit2d_x, hit2d_z);
	cv::line(Map2d, hit, hit, MapColor_wall , 3);
	/* ��������ʾ������ʾ��ͼ�� */
	cmd_queue.push(ShowMap2d);
}

QImage cvMat2QImage(const cv::Mat& mat)
{
	// 8-bits unsigned, NO. OF CHANNELS = 1
	if (mat.type() == CV_8UC1)
	{
		QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
		// Set the color table (used to translate colour indexes to qRgb values)
		image.setColorCount(256);
		for (int i = 0; i < 256; i++)
		{
			image.setColor(i, qRgb(i, i, i));
		}
		// Copy input Mat
		uchar *pSrc = mat.data;
		for (int row = 0; row < mat.rows; row++)
		{
			uchar *pDest = image.scanLine(row);
			memcpy(pDest, pSrc, mat.cols);
			pSrc += mat.step;
		}
		return image;
	}
	// 8-bits unsigned, NO. OF CHANNELS = 3
	else if (mat.type() == CV_8UC3)
	{
		// Copy input Mat
		const uchar *pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}


	return QImage();
}

void Map2dInit() {
	vector<Mat> src;
	src.push_back(Mat::ones(Map2d_w, Map2d_h, CV_8UC1) * 127);
	src.push_back(Mat::ones(Map2d_w, Map2d_h, CV_8UC1) * 127);
	src.push_back(Mat::ones(Map2d_w, Map2d_h, CV_8UC1) * 127);
	merge(src, Map2d);
}
