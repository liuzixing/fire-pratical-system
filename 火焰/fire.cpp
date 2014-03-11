
#include "gl\glew.h"			// �������µ�gl.h,glu.h��
//#include <GL/glut.h>			// ����OpenGLʵ�ÿ�
#include <stdio.h>			// ��׼����/������ͷ�ļ�
#include <math.h>			// ��ѧ�⺯��
#include "gl\glaux.h"			// GLaux���ͷ�ļ�
#include "gl\glext.h"
#include <olectl.h>
#include <gl\glut.h>

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default


bool light;
bool lp;
bool Tab;

#define GL_FOG_COORDINATE_SOURCE_EXT	0x8450					// ��GLEXT.H�õ���ֵ
#define GL_FOG_COORDINATE_EXT		0x8451

typedef void (APIENTRY * PFNGLFOGCOORDFEXTPROC) (GLfloat coord);		// ���������꺯����ԭ��

PFNGLFOGCOORDFEXTPROC glFogCoordfEXT = NULL;

//GLfloat xspeed;
//GLfloat yspeed;

const float piover180 = 0.0174532925f;
float heading;
float xpos;
float zpos;
float eyePosition[3];

GLfloat yrot;
GLfloat walkbias = 0;
GLfloat walkbiasangle = 0;
GLfloat lookupdown = 0.0f;
GLfloat camz = 0.0f;
GLfloat	fogColor[4] = {0.5f, 0.5f, 0.5f, 0.0f};

GLfloat LightAmbient[] = {0.5f, 0.5f, 0.5f, 1.0f};
GLfloat LightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat LightPosition[] = {0.0f,0.0f,2.0f,1.0f};

GLuint texture[2];
#define	MAX_PARTICLES	1000				// ��������������
typedef struct								// �����������ݽṹ
{
	bool	active;							// �Ƿ񼤻�
	float	life;							// ��������
	float	fade;							// ˥���ٶ�
	float	r;								// ��ɫֵ
	float	g;								// ��ɫֵ
	float	b;								// ��ɫֵ
	float	x;								// X λ��
	float	y;								// Y λ��
	float	z;								// Z λ��
	float	xi;								// X �����ٶ�
	float	yi;								// Y �����ٶ�
	float	zi;								// Z �����ٶ�

	float	xg;								// X �����������ٶ�
	float	yg;								// Y �����������ٶ�
	float	zg;								// Z �����������ٶ�
}
particles;									// �������ݽṹ
particles particle[MAX_PARTICLES];			// ����1000�����ӵ�����

float slowdown=2.0f;

void myDisplay(void){
	glClear(GL_COLOR_BUFFER_BIT);
	glRectf(-0.5,-0.5,0.5,0.5);
	glFlush();
}
AUX_RGBImageRec *LoadBMP(char *Filename)
{
	FILE *File = NULL;
	if(!Filename)
	{
		return NULL;
	}
	File = fopen(Filename, "r");
	if(File)
	{
		fclose(File);
		return auxDIBImageLoad((LPCWSTR)Filename);
	}
	return NULL;
}
int LoadGLTextures()
{
	int Status = FALSE;
	AUX_RGBImageRec *TextureImage[2];
	memset(TextureImage,0,sizeof(void *)*1);
	if (TextureImage[0]=LoadBMP("particle.bmp"))
	{
		Status=TRUE;				

		glGenTextures(1, &texture[0]);
		//Mipmaps
		glBindTexture(GL_TEXTURE_2D,texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);	
	}


	//if (TextureImage[1]=LoadBMP("wall.bmp"))
	//{
	//	Status=TRUE;				

	//	glGenTextures(1, &texture[1]);
	//	//Mipmaps
	//	glBindTexture(GL_TEXTURE_2D,texture[1]);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[1]->sizeX, TextureImage[1]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[1]->data);	
	//}

	return Status;
}

void reshape(int w,int h)
{
	glViewport(0,0,(GLsizei)w,(GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (float)w/(float)h, 1.0, 300.0);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}
int myinit(GLvoid){
	if(!LoadGLTextures())
		return FALSE;

	//if(!Extension_Init())
	//	return FALSE;

	glEnable(GL_TEXTURE_2D);

	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);

	for (int loop=0;loop<MAX_PARTICLES;loop++)						//��ʼ�����е�����
	{
		particle[loop].active=true;								// ʹ���е�����Ϊ����״̬
		particle[loop].life=1.0f;								// ���е���������ֵΪ���
		particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// �������˥������
		particle[loop].r=1.0f;	// ���ӵĺ�ɫ��ɫ
		particle[loop].g=0.3f;	// ���ӵ���ɫ��ɫ
		particle[loop].b=0.2f;	// ���ӵ���ɫ��ɫ
		particle[loop].xi=float((rand()%50)-26.0f)*10.0f;		// �������X�᷽���ٶ�
		particle[loop].yi=float((rand()%50)-25.0f)*10.0f;		// �������Y�᷽���ٶ�
		particle[loop].zi=float((rand()%50)-25.0f)*10.0f;		// �������Z�᷽���ٶ�
		particle[loop].xg=0.0f;									// ����X�᷽����ٶ�Ϊ0
		particle[loop].yg=0.8f;								// ����Y�᷽����ٶ�Ϊ-0.8
		particle[loop].zg=0.0f;									// ����Z�᷽����ٶ�Ϊ0
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);

	return TRUE;										// Initialization Went OK
}
int reshape(GLvoid)									// Here's Where We Do All The Drawing
{	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	glLoadIdentity();

	//AnjustViewByMouse();

	//eyePosition[0] = 5.0f*cos(angle_y);
	//eyePosition[1] = 5.0f*sin(angle_x);
	//eyePosition[2] = 5.0f*sin(angle_y);
	gluLookAt( eyePosition[0], 
		eyePosition[1],
		eyePosition[2],
		0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	//DrawCube();

	for (int loop=0;loop<MAX_PARTICLES;loop++)					// ѭ�����е�����
	{
		if (particle[loop].active)					// �������Ϊ�����
		{
			float x=particle[loop].x;				// ����X���λ��
			float y=particle[loop].y;				// ����Y���λ��
			float z=particle[loop].z;			// ����Z���λ��
			// ����������ɫ
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glColor4f(particle[loop].r,particle[loop].g,particle[loop].b,particle[loop].life);
			glRotatef(loop,0.0f,1.0f,0.0f);
			glBegin(GL_TRIANGLE_STRIP);				// ���������δ�
				glNormal3f(0.0f,0.0f,1.0);
				glTexCoord2d(1,1); glVertex3f(x+0.5f,y+0.5f,z); 
				glTexCoord2d(0,1); glVertex3f(x-0.5f,y+0.5f,z); 
				glTexCoord2d(1,0); glVertex3f(x+0.5f,y-0.5f,z); 
				glTexCoord2d(0,0); glVertex3f(x-0.5f,y-0.5f,z); 
			glEnd();
			particle[loop].x+=particle[loop].xi/(slowdown*1000);		// ����X�����λ��
			particle[loop].y+=particle[loop].yi/(slowdown*1000);		// ����Y�����λ��
			particle[loop].z+=particle[loop].zi/(slowdown*1000);		// ����Z�����λ��
			particle[loop].xi-=particle[loop].xg;						// ����X�᷽���ٶȴ�С
			particle[loop].yi-=particle[loop].yg*8;						// ����Y�᷽���ٶȴ�С
			particle[loop].zi-=particle[loop].zg;						// ����Z�᷽���ٶȴ�С
			particle[loop].life-=particle[loop].fade;					// �������ӵ�����ֵ
			if (particle[loop].life<0.0f)								// �����������ֵС��0
			{
				particle[loop].life=1.0f;								// ����һ���µ�����
				particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// �������˥������
				particle[loop].x=0.0f;									// �����ӳ�������Ļ������
				particle[loop].y=0.0f;					
				particle[loop].z=0.0f;				
				particle[loop].xi=float((rand()%60)-32.0f);		// ������������ٶ�
				particle[loop].yi=float((rand()%60)-30.0f);	
				particle[loop].zi=float((rand()%60)-30.0f);	
				particle[loop].r=1.0f;						// ����������ɫ
				particle[loop].g=0.3f;			
				particle[loop].b=0.2f;			
			}
			if (keys[VK_TAB])						
			{
				particle[loop].x=0.0f;					
				particle[loop].y=0.0f;					
				particle[loop].z=0.0f;					
				particle[loop].xi=float((rand()%50)-26.0f)*10.0f;		// ��������ٶ�
				particle[loop].yi=float((rand()%50)-25.0f)*10.0f;	
				particle[loop].zi=float((rand()%50)-25.0f)*10.0f;	
			}							
		}
	}
	return TRUE;										// һ�� OK
}

int main(int argc,char *argv[]){
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_SINGLE);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(700,700);
	glutCreateWindow("fire");
	glutDisplayFunc(&myDisplay);
	glutReshapeFunc(reshape);
	//glutIdleFunc(animate);

	myinit();
	glutMainLoop();
	return 0;
}