
#include "gl\glew.h"			// 包含最新的gl.h,glu.h库
//#include <GL/glut.h>			// 包含OpenGL实用库
#include <stdio.h>			// 标准输入/输出库的头文件
#include <math.h>			// 数学库函数
#include "gl\glaux.h"			// GLaux库的头文件
#include "gl\glext.h"
#include <olectl.h>
#include <gl\glut.h>

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default


bool light;
bool lp;
bool Tab;

#define GL_FOG_COORDINATE_SOURCE_EXT	0x8450					// 从GLEXT.H得到的值
#define GL_FOG_COORDINATE_EXT		0x8451

typedef void (APIENTRY * PFNGLFOGCOORDFEXTPROC) (GLfloat coord);		// 声明雾坐标函数的原形

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
#define	MAX_PARTICLES	1000				// 定义最大的粒子数
typedef struct								// 创建粒子数据结构
{
	bool	active;							// 是否激活
	float	life;							// 粒子生命
	float	fade;							// 衰减速度
	float	r;								// 红色值
	float	g;								// 绿色值
	float	b;								// 蓝色值
	float	x;								// X 位置
	float	y;								// Y 位置
	float	z;								// Z 位置
	float	xi;								// X 方向速度
	float	yi;								// Y 方向速度
	float	zi;								// Z 方向速度

	float	xg;								// X 方向重力加速度
	float	yg;								// Y 方向重力加速度
	float	zg;								// Z 方向重力加速度
}
particles;									// 粒子数据结构
particles particle[MAX_PARTICLES];			// 保存1000个粒子的数组

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

	for (int loop=0;loop<MAX_PARTICLES;loop++)						//初始化所有的粒子
	{
		particle[loop].active=true;								// 使所有的粒子为激活状态
		particle[loop].life=1.0f;								// 所有的粒子生命值为最大
		particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// 随机生成衰减速率
		particle[loop].r=1.0f;	// 粒子的红色颜色
		particle[loop].g=0.3f;	// 粒子的绿色颜色
		particle[loop].b=0.2f;	// 粒子的蓝色颜色
		particle[loop].xi=float((rand()%50)-26.0f)*10.0f;		// 随机生成X轴方向速度
		particle[loop].yi=float((rand()%50)-25.0f)*10.0f;		// 随机生成Y轴方向速度
		particle[loop].zi=float((rand()%50)-25.0f)*10.0f;		// 随机生成Z轴方向速度
		particle[loop].xg=0.0f;									// 设置X轴方向加速度为0
		particle[loop].yg=0.8f;								// 设置Y轴方向加速度为-0.8
		particle[loop].zg=0.0f;									// 设置Z轴方向加速度为0
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

	for (int loop=0;loop<MAX_PARTICLES;loop++)					// 循环所有的粒子
	{
		if (particle[loop].active)					// 如果粒子为激活的
		{
			float x=particle[loop].x;				// 返回X轴的位置
			float y=particle[loop].y;				// 返回Y轴的位置
			float z=particle[loop].z;			// 返回Z轴的位置
			// 设置粒子颜色
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glColor4f(particle[loop].r,particle[loop].g,particle[loop].b,particle[loop].life);
			glRotatef(loop,0.0f,1.0f,0.0f);
			glBegin(GL_TRIANGLE_STRIP);				// 绘制三角形带
				glNormal3f(0.0f,0.0f,1.0);
				glTexCoord2d(1,1); glVertex3f(x+0.5f,y+0.5f,z); 
				glTexCoord2d(0,1); glVertex3f(x-0.5f,y+0.5f,z); 
				glTexCoord2d(1,0); glVertex3f(x+0.5f,y-0.5f,z); 
				glTexCoord2d(0,0); glVertex3f(x-0.5f,y-0.5f,z); 
			glEnd();
			particle[loop].x+=particle[loop].xi/(slowdown*1000);		// 更新X坐标的位置
			particle[loop].y+=particle[loop].yi/(slowdown*1000);		// 更新Y坐标的位置
			particle[loop].z+=particle[loop].zi/(slowdown*1000);		// 更新Z坐标的位置
			particle[loop].xi-=particle[loop].xg;						// 更新X轴方向速度大小
			particle[loop].yi-=particle[loop].yg*8;						// 更新Y轴方向速度大小
			particle[loop].zi-=particle[loop].zg;						// 更新Z轴方向速度大小
			particle[loop].life-=particle[loop].fade;					// 减少粒子的生命值
			if (particle[loop].life<0.0f)								// 如果粒子生命值小于0
			{
				particle[loop].life=1.0f;								// 产生一个新的粒子
				particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// 随机生成衰减速率
				particle[loop].x=0.0f;									// 新粒子出现在屏幕的中央
				particle[loop].y=0.0f;					
				particle[loop].z=0.0f;				
				particle[loop].xi=float((rand()%60)-32.0f);		// 随机生成粒子速度
				particle[loop].yi=float((rand()%60)-30.0f);	
				particle[loop].zi=float((rand()%60)-30.0f);	
				particle[loop].r=1.0f;						// 设置粒子颜色
				particle[loop].g=0.3f;			
				particle[loop].b=0.2f;			
			}
			if (keys[VK_TAB])						
			{
				particle[loop].x=0.0f;					
				particle[loop].y=0.0f;					
				particle[loop].z=0.0f;					
				particle[loop].xi=float((rand()%50)-26.0f)*10.0f;		// 随机生成速度
				particle[loop].yi=float((rand()%50)-25.0f)*10.0f;	
				particle[loop].zi=float((rand()%50)-25.0f)*10.0f;	
			}							
		}
	}
	return TRUE;										// 一切 OK
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