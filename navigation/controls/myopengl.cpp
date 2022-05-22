#include "myopengl.h"
#include "malloc.h"

using namespace std;
using namespace cv;

/*-------- �̶����� --------*/
#define dis_max 10
#define resolution 100			//��ͼ�ֱ���

/*-------- �洢���� --------*/
unsigned char Map3d[resolution][resolution][resolution] = {};

/*-------- �������в��� --------*/
float *map_show;
/* VBO��VAO���� */
unsigned int VBO, VAO;
/* ���ݶ��� */
float buffer[] = {
	0.5, 0.5, 0
};
/*-------- �������� --------*/


myOpenGl::myOpenGl(QWidget *parent)
	: QOpenGLWidget(parent)
{

}


/* ��ȡ����16���״�ɨ������ */
void myOpenGl::GetLadarData(float * x, float * y, float * z)
{
	for (int i = 0; i < 16; i++) {
		if ((*(x + i) <= dis_max && *(x + i) >= -dis_max) &&
			(*(y + i) <= dis_max && *(y + i) >= -dis_max) &&
			(*(z + i) <= dis_max && *(z + i) >= -dis_max)) {
			/* �޸ĵ�ͼ���� */
			Map3d[resolution / 2 - int(resolution * (*(x + i)) / dis_max)][resolution / 2 - int(resolution * (*(y + i)) / dis_max)][resolution / 2 - int(resolution * (*(z + i)) / dis_max)] = 1;
		}
	}
}

/* opengl��ʼ������ */
void myOpenGl::initializeGL()
{
	/* ����һ��ؼ����ڴ洢��Ҫ��ʾ�ĵ� */
	map_show = (float*)malloc(resolution * resolution * resolution * 3 * sizeof(float));
	/* ��ʼ��ָ�룬������ָ��ָ���Կ��ṩ�ĺ��� */
	initializeOpenGLFunctions();
	/* ����VBO��VAO���󣬲�����ID */
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	/* ��VBO��VAO���� */
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	/* �����������鵽�������������� */
	glBufferData(GL_ARRAY_BUFFER, sizeof(map_show) , map_show, GL_STREAM_DRAW);
	/* ��֪�Կ���ν��������������ֵ,�����ö�������ָ�� */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	/* ����VAO����ĵ�һ������ֵ */
	glEnableVertexAttribArray(0);

	/* link shaders */
	bool success;
	ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, "./shaders/Vertex.txt");
	ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, "./shaders/Fragment.txt");
	success = ShaderProgram.link();
	if (!success) {
		printf("LINK ERR: %s", ShaderProgram.log());
	}
	/* ����ɫ�� */
	ShaderProgram.bind();

	/* ���ͼ�񻺳��� */
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	/* �ͷ�������ڴ� */
	free(map_show);
}

/* opengl�������ô�С */
void myOpenGl::resizeGL(int w, int h)
{

}

/* opengl���ƺ��� */
void myOpenGl::paintGL()
{
	/* ����һ��ؼ����ڴ洢��Ҫ��ʾ�ĵ� */
	map_show = (float*)malloc(resolution * resolution * resolution * 3 * sizeof(float));
	/* �ҵ���Ҫ��ʾ�ĵ� */
	unsigned int index = 0;
	for (int i = 0; i < resolution; i++)
		for (int j = 0; j < resolution; j++)
			for (int k = 0; k < resolution; k++)
				if (Map3d[i][j][k] == 1) {
					map_show[index * 3 + 0] = ((float)(resolution / 2 - i) / resolution);
					map_show[index * 3 + 1] = ((float)(resolution / 2 - j) / resolution);
					map_show[index * 3 + 2] = ((float)(resolution / 2 - k) / resolution);
					index++;
				}
	printf("index: %d\n", index);
	if (index == 0)
		glBufferData(GL_ARRAY_BUFFER, sizeof(map_show), map_show, GL_STREAM_DRAW);
	else
		glBufferData(GL_ARRAY_BUFFER, index * sizeof(float), map_show, GL_STREAM_DRAW);
	glDrawArrays(GL_POINTS, 0, 16);
	/* �ͷ�������ڴ� */
	free(map_show);
}

