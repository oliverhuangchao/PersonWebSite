/*
	final project:       advanced interactive and fisheye function
	CP SC 404/604           Chao Huang        12/04/2013
	email:chaoh@g.clmeson.edu

	
	this function will give user a wonderful experiment to interactive with the program. And same time, user can also see some detail of fisheye function.

Press 'b' or 'B' to return to the original value; 
Press 'Q' or 'q' or ESC to quit;
Press 's' or 'W' to save 
Press F1 to "F1mode"
	{
	1.press 'l' or 'L' to move a line
	2.press 'p' or 'P' to drag a point
	3.press 'm' or 'M' to move the how shape
	}	
	use mouse to see the result
press F2 to "F2mode"
	1.use mouse to see the result of fisheye.
	2.press UP and DOWN to increse and decrese the value of focal lens of the picture.
	3.Press 'm' or 'M' to active the bilinar function.

	This is the main function 
	It includes the main.cpp MyMethod.h MyMethod.cpp
	and Matrix fold which contribute by Dr. House.

	To compile on linux:
	make clean
	make
		
*/
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <cstdio>
#include <string.h>
#include <sys/time.h>
#include <ctime>
#include <cmath>
#include <OpenImageIO/imageio.h>
#include <algorithm>
#include "MyMethod.h"
#include "mycode/Matrix.h"

OIIO_NAMESPACE_USING
/******************define some gloabal valuables*******************/
const Matrix3x3 IdentyMatirx(1,0,0,0,1,0,0,0,1);
const float my_PI=3.1415926;

int Catch_Size=75;
int CurrentLine;
int click_above=20;
int xres;
int yres;
int new_xres,new_yres;
int CurrentPoint_X,CurrentPoint_Y;
int input_channelnum;
int ori_channelnum;
int output_channelnum;
int Input_number;
int windowWidth;
int windowHeight;
int NearestPosition;
int a_new[4];
int b_new[4];
int a_drawline[4];
int b_drawline[4];
int m_max,m_min,n_max,n_min;
int window2,window1;
int Show_Interval=0;
int a_start,b_start;
int a_end,b_end;
int a_min,b_min;

unsigned char* pixels;
unsigned char* smallerpixels;
unsigned char* OriData;
unsigned char* alpha_pixels;
unsigned char* Fisheye_pixels;
unsigned char* fishwholepixels;

char** Input_Name;
float windowAspect= (float)windowHeight / (float)windowWidth;
float translate_distance[2]={0.0,0.0};
float Len_focus=1;//refer to the lens focus
float Current_radius=1;
float k_width=2;
float k_height=2;
string savefilename;


bool IsTranslate=false;
bool IsClick=false;
bool IsDrawLine=false;
bool F1mode=false;
bool F2mode=false;
bool IsPartChanged=false;
bool IsBilinear=false;
bool IsDragPoint=false;
bool IsMoveWhole=false;
bool IsDragline=false;
bool IsMoveLine=false;

Matrix matrix_8x8;
Vector vector_8x1;
Vector result_8x1;
Matrix3x3 OutMatrix(IdentyMatirx);

using namespace std;
/*------------fisheye function-------------------*/
void FishEyeFunction(int cx,int cy )
{
	int width=2*Catch_Size;
	int height=2*Catch_Size;
	int m,n;
	float x,y,z;
	float u,v;
	float r,phi,theta;
	float length;


	for (int i=0;i<height;i++)
	{
		for (int j=0;j<width;j++)
		{
			x=2.0*(float)j/width-1;
			y=2.0*(float)i/height-1;
			z=sqrt(x*x+y*y);
			length=Current_radius*asin(z/Current_radius);
			phi=atan2(y,x);
			u=cx+round(length*width*cos(phi));
			v=cy+round(length*height*sin(phi));
			
			m=i*4*width+j*4;
			n=v*4*new_xres+u*4;
			if (x*x+y*y<1 && u>0+a_min && u<new_xres && v>0+b_min && v<new_yres)
			{
				Fisheye_pixels[m+0]=smallerpixels[n+0];
				Fisheye_pixels[m+1]=smallerpixels[n+1];
				Fisheye_pixels[m+2]=smallerpixels[n+2];
				Fisheye_pixels[m+3]=255;
			}
			else
			{
				Fisheye_pixels[m+0]=0;
				Fisheye_pixels[m+1]=0;
				Fisheye_pixels[m+2]=0;
				Fisheye_pixels[m+3]=0;
			}
		}
	}
	SetSmallerpixels();

	return;
}
/*-------------set the fisheye  pixels into window1------------*/
void SetSmallerpixels()
{
	int m,n;
	int iii=0;
	int jjj=0;
	for (int i=0;i<new_yres;i++)
	{
		for (int j=0;j<new_xres;j++)
		{
			m=i*new_xres*4+j*4;
			if (i>=CurrentPoint_Y-Catch_Size && 
				j>=CurrentPoint_X-Catch_Size && 
				i<CurrentPoint_Y+Catch_Size && 
				j<CurrentPoint_X+Catch_Size)
			{
				n=iii*2*Catch_Size*4+jjj*4;
				fishwholepixels[m+0]=Fisheye_pixels[n+0];
				fishwholepixels[m+1]=Fisheye_pixels[n+1];
				fishwholepixels[m+2]=Fisheye_pixels[n+2];
				fishwholepixels[m+3]=Fisheye_pixels[n+3];
				jjj++;
				if(jjj==2*Catch_Size)
				{
					iii++;
					jjj=0;
				}
			}
			else
			{
				fishwholepixels[m+0]=smallerpixels[m+0];
				fishwholepixels[m+1]=smallerpixels[m+1];
				fishwholepixels[m+2]=smallerpixels[m+2];
				fishwholepixels[m+3]=smallerpixels[m+3];
			}

		}
	}
	return;
}
/*--------cut down function if the data is out of bound---------*/
int  BroadLimit(int x,int q,int y)
{
	if(x>y) 
		return y;
	if(x<q)
		return q;
	return x;
}
/*-------------- find the max----------------------------------*/
int findmax(int a,int b,int c, int d)
{
	if(a>=b && a>=c && a>=d) return a;
	if(b>=a && b>=c && b>=d) return b;
	if(c>=a && c>=b && c>=d) return c;
	if(d>=a && d>=b && d>=c) return d;
}
/*-------------- find the min----------------------------------*/
int findmin(int a,int b,int c, int d)
{
	if(a<=b && a<=c && a<=d) return a;
	if(b<=a && b<=c && b<=d) return b;
	if(c<=a && c<=b && c<=d) return c;
	if(d<=a && d<=b && d<=c) return d;
}
/*-------------- change the picture's size in windows 2 --------------*/
void ChangeSize(float k1,float k2)
{
	int  testnum=0;
	int m,n;
	int nxres=k1*2*Catch_Size;
	int nyres=k2*2*Catch_Size;
	unsigned char * tempdata=new unsigned char [nxres*nyres*4];

	if (!IsBilinear)
	{
	std::cout<<"is not bilinear"<<endl;
	int i,j;
	for (int i1=0;i1<nyres;i1++)
	{
		for (int j1=0;j1<nxres;j1++)
		{
			i=floor(float(i1)/k2);
			j=floor(float(j1)/k1);
			n=i1*nxres*4+j1*4;
			m=i*2*Catch_Size*4+j*4;
			tempdata[n+0]=Fisheye_pixels[m+0];
			tempdata[n+1]=Fisheye_pixels[m+1];
			tempdata[n+2]=Fisheye_pixels[m+2];
			tempdata[n+3]=Fisheye_pixels[m+3];
		}
	}
	}
	else
	{
		std::cout<<"is bilinear"<<endl;
		float x1,y1;
		int width=2*Catch_Size;
		int height=2*Catch_Size;
		for (int i=0;i<nyres;i++)
		{
		for (int j=0;j<nxres;j++)
		{

			x1=(float)j/k2;
			y1=(float)i/k1;	
			if (round(x1)<0) x1=0;
			if (round(x1)>=nxres) x1=nxres-1;
			if (round(y1)<0) y1=0;
			if (round(y1)>=nyres) y1=nyres-1;
				
			int x_up,x_down,y_up,y_down;
			int n1,n2,n3,n4;
			x_up=ceil(x1);
			y_up=ceil(y1);
			x_down=floor(x1);
			y_down=floor(y1);

			x_up=BroadLimit(x_up,0,nxres);
			x_down=BroadLimit(x_down,0,nxres);
			y_up=BroadLimit(y_up,0,nyres);
			y_down=BroadLimit(y_down,0,nyres);

			float a,b,c,d;
			a=x1-x_down;//a
			b=y1-y_down;//b
			c=1-a;
			d=1-b;
			m=i*width*k1*4+j*4;
			n1=y_down*width*4+x_down*4;
			n2=y_up*width*4+x_down*4;
			n3=y_down*width*4+x_up*4;
			n4=y_up*width*4+x_up*4;

			tempdata[m+0]=c*(d*Fisheye_pixels[n1+0]+b*Fisheye_pixels[n2+0])+a*(d*Fisheye_pixels[n3+0]+b*Fisheye_pixels[n4+0]);
			tempdata[m+1]=c*(d*Fisheye_pixels[n1+1]+b*Fisheye_pixels[n2+1])+a*(d*Fisheye_pixels[n3+1]+b*Fisheye_pixels[n4+1]);
			tempdata[m+2]=c*(d*Fisheye_pixels[n1+2]+b*Fisheye_pixels[n2+2])+a*(d*Fisheye_pixels[n3+2]+b*Fisheye_pixels[n4+2]);
			tempdata[m+3]=c*(d*Fisheye_pixels[n1+3]+b*Fisheye_pixels[n2+3])+a*(d*Fisheye_pixels[n3+3]+b*Fisheye_pixels[n4+3]);
		}
		}
	}


	delete []Fisheye_pixels;
	Fisheye_pixels=new unsigned char [nxres*nyres*4]; 
	for (int i=0;i<nyres;i++)
	{
		for (int j=0;j<nxres;j++)
		{
			m=i*nxres*4+j*4;
			Fisheye_pixels[m+0]=tempdata[m+0];
			Fisheye_pixels[m+1]=tempdata[m+1];
			Fisheye_pixels[m+2]=tempdata[m+2];
			Fisheye_pixels[m+3]=tempdata[m+3];
		}
	}
	delete []tempdata;
	return;
}

	
/*-------find the current area with xres and yres as input-------*/
void FindtheArea(int x,int y)
{
	int a_x,b_x,c_x,d_x;
	int a_y,b_y,c_y,d_y;
	a_x=x;a_y=0;
	b_x=0;b_y=0;
	c_x=0;c_y=y;
	d_x=x;d_y=y;
	CaculateXY(a_x,a_y);
	CaculateXY(b_x,b_y);
	CaculateXY(c_x,c_y);
	CaculateXY(d_x,d_y);
	new_xres=findmax(a_x,b_x,c_x,d_x)-findmin(a_x,b_x,c_x,d_x);
	new_yres=findmax(a_y,b_y,c_y,d_y)-findmin(a_y,b_y,c_y,d_y);
	return;
}
/*------- mapping from ori to now-----------*/
void CaculateXY(int &x, int &y)
{
	Vector3d ori_point(x,y,1);
	Vector3d after_point(OutMatrix*ori_point);
	x=after_point.x/after_point.z;
	y=after_point.y/after_point.z;
	return;
}

/*--------mapping from now to ori-----------------*/
void InvCaculateXY(int &x,int &y)
{
	Vector3d ori_point(x,y,1);
	Vector3d after_point(OutMatrix.inv()*ori_point);
	x=after_point.x/after_point.z;
	y=after_point.y/after_point.z;
	return;
}

/*-----given the matrix M and get invese mapping-----*/
void Mapping(void)
{
	FindtheArea(xres,yres);
	if (IsTranslate)
	{
		new_xres=new_xres+abs(translate_distance[0]);
		new_yres=new_yres+abs(translate_distance[1]);
	}
	int testnum=0;
	cout<<"new size is: "<<new_xres<<'*'<<new_yres<<endl;
	delete []smallerpixels;
	smallerpixels =new unsigned char[new_xres*new_yres*4];
	int m,n,x,y;
	for (int i=0;i<new_yres;i++)
	{
		for (int j=0;j<new_xres;j++)
		{
			m=i*new_xres*input_channelnum+j*input_channelnum;
			x=j;y=i;
			InvCaculateXY(x,y);
			if(x<0||y<0||x>xres||y>yres) 
			{
				smallerpixels[m+0]=0;
				smallerpixels[m+2]=0;
				smallerpixels[m+1]=0;
				smallerpixels[m+3]=255;
				testnum++;
			}
			else
			{
				n=y*xres*input_channelnum+x*input_channelnum;
				smallerpixels[m+0]=pixels[n+0];
				smallerpixels[m+1]=pixels[n+1];
				smallerpixels[m+2]=pixels[n+2];
				smallerpixels[m+3]=255;
			}
		}
	}
	IsPartChanged=true;
	return;
}

//---------------return to the original value---------------------
void ReturnOri(void)
{
	int m;
	
	delete []smallerpixels;	
	smallerpixels = new unsigned char[xres*yres*4];
	new_xres=xres;
	new_yres=yres;
	for (int i=0;i<yres;i++)
	{
		for (int j=0;j<xres;j++)
		{
			m=i*xres*input_channelnum+j*input_channelnum;
			smallerpixels[m+0]=OriData[m+0];
			smallerpixels[m+1]=OriData[m+1];
			smallerpixels[m+2]=OriData[m+2];
			smallerpixels[m+3]=OriData[m+3];
		}
	}
	
	IsPartChanged=false;
	F1mode=false;
	F2mode=false;
	IsBilinear=false;

	return;
}


/*----get the pixture upsidedown so it looks well------*/
void Upsidedown(unsigned char *filedata)
{
	int a=xres*4;
	int b;
	unsigned char *tempP;
	tempP= new unsigned char[xres*yres*4];
	for (int i=0;i<yres;i++)
	{
		for (int j=0;j<a;j++)
			{
				*(tempP+i*a+j)=*(filedata+(yres-i-1)*a+j);
			}
	}
	for (int i=0;i<yres;i++)
	{
		for (int j=0;j<a;j++)
			{
				b=i*a+j;
				*(pixels+b)=*(tempP+b);
			}
	}
	delete []tempP;
}
/*-----------new window to show the result----------------*/
void display2()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2i(0,0);//necessary!!!!!
	if (F2mode)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawPixels(2*k_width*Catch_Size,2*k_height*Catch_Size,GL_RGBA,GL_UNSIGNED_BYTE,Fisheye_pixels);
		glDisable(GL_BLEND);
	}
	glutSwapBuffers();
	return;
}

//------------------dispaly the pixture------------------
void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2i(0,0);//necessary!!!!!
	if (!IsPartChanged && !F2mode)
	{
		std::cout<<"original pixture"<<endl;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawPixels(xres,yres,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
		glDisable(GL_BLEND);
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawPixels(new_xres,new_yres,GL_RGBA,GL_UNSIGNED_BYTE,smallerpixels);
		glDisable(GL_BLEND);
	}
	if (F2mode&&!F1mode)//show fishwhole pixel on the first screen when pressing F2
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawPixels(new_xres,new_yres,GL_RGBA,GL_UNSIGNED_BYTE,fishwholepixels);
		glDisable(GL_BLEND);
	}
	//draw a line
	if(F1mode&&IsDrawLine)//green line
	{
		glColor3f(0,1,0);
		glLineWidth(5.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2f((float)a_drawline[0],(float)b_drawline[0]);
		glVertex2f((float)a_drawline[1],(float)b_drawline[1]);
		glVertex2f((float)a_drawline[2],(float)b_drawline[2]);
		glVertex2f((float)a_drawline[3],(float)b_drawline[3]);
		glEnd();
	}
	if(F1mode)
	{
		// red line
		m_max=findmax(a_new[0],a_new[1],a_new[2],a_new[3]);
		m_min=findmin(a_new[0],a_new[1],a_new[2],a_new[3]);
		n_max=findmax(b_new[0],b_new[1],b_new[2],b_new[3]);
		n_min=findmin(b_new[0],b_new[1],b_new[2],b_new[3]);

		glColor3f(1,0,0);
		glLineWidth(5.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2f((float)m_min,(float)n_min);
		glVertex2f((float)m_min,(float)n_max);
		glVertex2f((float)m_max,(float)n_max);
		glVertex2f((float)m_max,(float)n_min);
		glEnd();
	}
	if(F2mode&&IsClick)
	{
		glColor3f(1,0,0);
		glLineWidth(3.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2f((float)CurrentPoint_X-Catch_Size,(float)CurrentPoint_Y-Catch_Size);
		glVertex2f((float)CurrentPoint_X-Catch_Size,(float)CurrentPoint_Y+Catch_Size);
		glVertex2f((float)CurrentPoint_X+Catch_Size,(float)CurrentPoint_Y+Catch_Size);
		glVertex2f((float)CurrentPoint_X+Catch_Size,(float)CurrentPoint_Y-Catch_Size);
		glEnd();
	}

	glutSwapBuffers();
	return;
}

/*-----------------reshape the picture-------------------*/
void reshape( int width, int height )
{	
/*	if (width < xres || height < yres) 
	{
		glViewport(0,0,width,height);		
		glPixelZoom(float(width)/xres,float(height)/yres);
	} else {
	   glPixelZoom(float(width)/xres,float(height)/yres);
	}*/
//	glMatrixMode( GL_PROJECTION );
//	glLoadIdentity();
//	gluOrtho2D(0,width,0,height);
	windowHeight=height;// change the current window size
	windowWidth=width;
	return;
}




/*************try to save what we see***************/
bool Saving(string savefilename)
{
  	int WinWidth=new_xres;
	int WinHeight=new_yres;
	int m,n;
	ImageOutput *out = ImageOutput::create(savefilename);
  	if (!out) 
   	{
		std::cout<<OpenImageIO::geterror()<<endl;
   		return false;
   	}
	int scanlinesize; 
  	if (output_channelnum==3)
	{
		ImageSpec spec(WinWidth,WinHeight,output_channelnum,TypeDesc::UINT8);out->open (savefilename, spec); 
		scanlinesize = WinWidth * output_channelnum * sizeof(smallerpixels[0]);
		unsigned char * tempdata= new unsigned char[output_channelnum*new_xres*new_yres];
		for (int i=0;i<new_yres;i++)
		{
			for (int j=0;j<new_xres;j++)
			{
			m=i*new_xres*4+j*4;
			n=i*new_xres*3+j*3;
			if (F1mode)
			{
				tempdata[n+0]=smallerpixels[m+0];
				tempdata[n+1]=smallerpixels[m+1];
				tempdata[n+2]=smallerpixels[m+2];
			}
			else if (F2mode)
			{
				tempdata[n+0]=fishwholepixels[m+0];
				tempdata[n+1]=fishwholepixels[m+1];
				tempdata[n+2]=fishwholepixels[m+2];
			}
			else
			{
				tempdata[n+0]=pixels[m+0];
				tempdata[n+1]=pixels[m+1];
				tempdata[n+2]=pixels[m+2];
			}
			}
		}

		if(!out->write_image (TypeDesc::UINT8,	(unsigned char*)tempdata+(WinHeight-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride))
		{
			std::cout<< out->geterror()<<endl;
			return false;
		}
		delete []tempdata;
	}
	else if (output_channelnum==4)
	{
		int m,n;
		ImageSpec spec(WinWidth,WinHeight,output_channelnum,TypeDesc::UINT8);
		out->open (savefilename, spec); 
		scanlinesize = WinWidth * output_channelnum * sizeof(smallerpixels[0]);
		if(F1mode)
		{
			if(!out->write_image (TypeDesc::UINT8,	(unsigned char*)smallerpixels+(WinHeight-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride))
			{
				std::cout<< out->geterror()<<endl;
				return false;
			}
		}
		else if (F2mode)
		{
			if(!out->write_image (TypeDesc::UINT8,	(unsigned char*)fishwholepixels+(WinHeight-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride))
			{
				std::cout<< out->geterror()<<endl;
				return false;
			}
		}
		else
		{
			if(!out->write_image (TypeDesc::UINT8,	(unsigned char*)pixels+(WinHeight-1)*scanlinesize, AutoStride, -scanlinesize, AutoStride))
			{
				std::cout<< out->geterror()<<endl;
				return false;
			}
		}
	}
 
  	if(!out->close ())
	{
		std::cout<<out->geterror()<<endl;
	}
	delete out; 
	return true;
}

/*--------------ge the big matrix for invert mapping-----------*/
void Matrix_init(void)
{
	matrix_8x8.setsize(8,8);
	int x_ori[4]={0,0,xres,xres};
	int y_ori[4]={0,yres,yres,0};

	matrix_8x8[0][0]=x_ori[0];
	matrix_8x8[0][1]=y_ori[0];
	matrix_8x8[0][2]=1;
	matrix_8x8[0][3]=0;
	matrix_8x8[0][4]=0;
	matrix_8x8[0][5]=0;
	matrix_8x8[0][6]=-1*a_new[0]*x_ori[0];
	matrix_8x8[0][7]=-1*a_new[0]*y_ori[0];
	
	matrix_8x8[1][0]=0;
	matrix_8x8[1][1]=0;
	matrix_8x8[1][2]=0;
	matrix_8x8[1][3]=x_ori[0];
	matrix_8x8[1][4]=y_ori[0];
	matrix_8x8[1][5]=1;
	matrix_8x8[1][6]=-1*b_new[0]*x_ori[0];
	matrix_8x8[1][7]=-1*b_new[0]*y_ori[0];

	matrix_8x8[2][0]=x_ori[1];
	matrix_8x8[2][1]=y_ori[1];
	matrix_8x8[2][2]=1;
	matrix_8x8[2][3]=0;
	matrix_8x8[2][4]=0;
	matrix_8x8[2][5]=0;
	matrix_8x8[2][6]=-1*a_new[1]*x_ori[1];
	matrix_8x8[2][7]=-1*a_new[1]*y_ori[1];
	
	matrix_8x8[3][0]=0;
	matrix_8x8[3][1]=0;
	matrix_8x8[3][2]=0;
	matrix_8x8[3][3]=x_ori[1];
	matrix_8x8[3][4]=y_ori[1];
	matrix_8x8[3][5]=1;
	matrix_8x8[3][6]=-1*b_new[1]*x_ori[1];
	matrix_8x8[3][7]=-1*b_new[1]*y_ori[1];

	matrix_8x8[4][0]=x_ori[2];
	matrix_8x8[4][1]=y_ori[2];
	matrix_8x8[4][2]=1;
	matrix_8x8[4][3]=0;
	matrix_8x8[4][4]=0;
	matrix_8x8[4][5]=0;
	matrix_8x8[4][6]=-1*a_new[2]*x_ori[2];
	matrix_8x8[4][7]=-1*a_new[2]*y_ori[2];
	
	matrix_8x8[5][0]=0;
	matrix_8x8[5][1]=0;
	matrix_8x8[5][2]=0;
	matrix_8x8[5][3]=x_ori[2];
	matrix_8x8[5][4]=y_ori[2];
	matrix_8x8[5][5]=1;
	matrix_8x8[5][6]=-1*b_new[2]*x_ori[2];
	matrix_8x8[5][7]=-1*b_new[2]*y_ori[2];

	matrix_8x8[6][0]=x_ori[3];
	matrix_8x8[6][1]=y_ori[3];
	matrix_8x8[6][2]=1;
	matrix_8x8[6][3]=0;
	matrix_8x8[6][4]=0;
	matrix_8x8[6][5]=0;
	matrix_8x8[6][6]=-1*a_new[3]*x_ori[3];
	matrix_8x8[6][7]=-1*a_new[3]*y_ori[3];
	
	matrix_8x8[7][0]=0;
	matrix_8x8[7][1]=0;
	matrix_8x8[7][2]=0;
	matrix_8x8[7][3]=x_ori[3];
	matrix_8x8[7][4]=y_ori[3];
	matrix_8x8[7][5]=1;
	matrix_8x8[7][6]=-1*b_new[3]*x_ori[3];
	matrix_8x8[7][7]=-1*b_new[3]*y_ori[3];
	//matrix_8x8.print();
	
	vector_8x1.setsize(8);
	vector_8x1[0]=a_new[0];vector_8x1[1]=b_new[0];
	vector_8x1[2]=a_new[1];vector_8x1[3]=b_new[1];
	vector_8x1[4]=a_new[2];vector_8x1[5]=b_new[2];
	vector_8x1[6]=a_new[3];vector_8x1[7]=b_new[3];
	//vector_8x1.print();cout<<endl;
	
	result_8x1.setsize(8);
	result_8x1.set(matrix_8x8.inv()*vector_8x1);
	//result_8x1.print();cout<<endl;
}
/*---------calculate the vertical distance------------*/
int Cal_Vertical(int x,int y,int a0, int a1,int b0,int b1)
{
	float a,b,c,d;
	a=sqrt(pow((x-a0),2)+pow((y-b0),2));
	b=sqrt(pow((x-a1),2)+pow((y-b1),2));
	d=sqrt(pow((a0-a1),2)+pow((b0-b1),2));
	c=a*a-pow((a*a+d*d-b*b)/2/d,2);
	return round(sqrt(c));
}
/*-----------find out the nearest line ---------*/
int NearestLine(int x,int y)
{
	int a,b,c,d;
	a=Cal_Vertical(x,y,a_new[0],a_new[1],b_new[0],b_new[1]);
	b=Cal_Vertical(x,y,a_new[1],a_new[2],b_new[1],b_new[2]);
	c=Cal_Vertical(x,y,a_new[2],a_new[3],b_new[2],b_new[3]);
	d=Cal_Vertical(x,y,a_new[3],a_new[0],b_new[3],b_new[0]);

	int e;
	int f=1;
	e=findmin(a,b,c,d);
	if (e==a && a<click_above*2) return 1;
	if (e==b && b<click_above*2) return 2;
	if (e==c && c<click_above*2) return 3;
	if (e==d && d<click_above*2) return 4;

	return 0;
}
/*-----give the current position of each point if move a line----- */
void LineMove(int a,int b)
{	
	int a_cross=a_end-a_start;
	int b_cross=-1*(b_end-b_start);
	a_new[a]=a_new[a]+a_cross;
	b_new[a]=b_new[a]+b_cross;
	a_new[b]=a_new[b]+a_cross;
	b_new[b]=b_new[b]+b_cross;
}
/*-----------F1 mode function-------------------*/
void F1_mode_mousefunc(int button,int state,int x,int y)
{
	if (button==GLUT_LEFT_BUTTON && state==GLUT_DOWN)
	{
		IsClick=true;
		NearestPoint(x,windowHeight-y);
		a_start=x;
		b_start=y;
		IsDrawLine=true;
		CurrentLine=NearestLine(x,windowHeight-y);
	
	}
	if (button==GLUT_LEFT_BUTTON && state==GLUT_UP)
	{
		if (IsClick)
		{
			if (IsDragPoint && NearestPosition<5)//drag point to the position
			{
				a_new[NearestPosition-1]=x;
				b_new[NearestPosition-1]=windowHeight-y;
			}
			else if (IsMoveWhole)//move the whole picture
			{
				a_end=x;
				b_end=y;
				int a_across=a_end-a_start;
				int b_across=-1*(b_end-b_start);
				for (int i=0;i<4;i++)
				{
					a_new[i]=a_new[i]+a_across;
					b_new[i]=b_new[i]+b_across;
				}
			}
			else if (IsMoveLine)
			{
				a_end=x;
				b_end=y;
				switch (CurrentLine)
				{
					case 1:
						LineMove(0,1);
						break;
					case 2:
						LineMove(1,2);
						break;
					case 3:
						LineMove(2,3);
						break;
					case 4:
						LineMove(3,0);
						break;
					default:
						break;
				}
			}
			Matrix_init();
			if(a_new[0]>0||b_new[0]>0)
			{
				IsTranslate=true;
				translate_distance[0]=a_new[0];
				translate_distance[1]=b_new[0];
			}
			for (int i=0;i<3;i++)
			{
				for (int j=0;j<3;j++)
				{
					if(i==2&&j==2)
					break;
					OutMatrix[i][j]=result_8x1[3*i+j];
				}
			}
	//		OutMatrix.print();
			Mapping();
			a_min=findmin(a_new[0],a_new[1],a_new[2],a_new[3]);
			b_min=findmin(b_new[0],b_new[1],b_new[2],b_new[3]);

			All_redisplay();
		}
		IsClick=false;
		IsDrawLine=false;
	}

return ;
}
/*---------------------F2 mode function---------------*/
void F2_mode_mousefunc(int button,int state, int x,int y)
{
	if (button==GLUT_LEFT_BUTTON && state==GLUT_DOWN)
	{
		CurrentPoint_X=x;
		CurrentPoint_Y=windowHeight-y;
		IsClick=true;
		IsDrawLine=true;
	}
	if (button==GLUT_LEFT_BUTTON && state==GLUT_UP)
	{
		IsClick=false;
		IsDrawLine=false;
	}
	glutSetWindow(window1);
	glutPostRedisplay();
	return;
}


/*-----------mouse click function-----------------*/
void mousefunc(int button,int state,int x,int y)
{
	if (F1mode)
	{
		F1_mode_mousefunc(button,state,x,y);
	}
	if (F2mode)
	{
		F2_mode_mousefunc(button,state,x,y);
	}
}


/*--------if move a line, draw its outline-----*/
void LineDraw(int a,int x,int y)
{
	a_drawline[a]=a_new[a]+x;
	b_drawline[a]=b_new[a]+y;
}


/*--------mouse move function------------------------*/
void mousemove(int x,int y)
{
	//every five time, show the changed picture;
	if (Show_Interval==5)
		Show_Interval=0;
	else
		Show_Interval++;
	
	if (F1mode)
	{
		if (IsDragPoint && IsDrawLine && IsClick && NearestPosition!=5)
		{
			a_drawline[NearestPosition-1]=x;

			b_drawline[NearestPosition-1]=windowHeight-y;
			glutPostRedisplay();
		}
		if (IsMoveWhole && IsDrawLine && IsClick)
		{
			for (int i=0;i<4;i++)
			{
				a_drawline[i]=a_new[i]+x-a_start;
				b_drawline[i]=b_new[i]+b_start-y;
			}
			glutPostRedisplay();
		}
		if (IsMoveLine )
		{
			//a_end=x;
			//b_end=y;
			int xx=x-a_start;
			int yy=b_start-y;
			switch (CurrentLine)
			{
				case 1:
					LineDraw(0,xx,yy);
					LineDraw(1,xx,yy);
					break;
				case 2:
					LineDraw(1,xx,yy);
					LineDraw(2,xx,yy);
					break;
				case 3:
					LineDraw(2,xx,yy);
					LineDraw(3,xx,yy);
					break;
				case 4:
					LineDraw(3,xx,yy);
					LineDraw(0,xx,yy);
					break;
				default:
					break;
			}
			glutPostRedisplay();
		}
	}
	if (F2mode&&IsClick)
	{
		CurrentPoint_X=x;
		CurrentPoint_Y=windowHeight-y;
		CurrentPoint_X=BroadLimit(CurrentPoint_X,Catch_Size+a_min,new_xres-Catch_Size);
		CurrentPoint_Y=BroadLimit(CurrentPoint_Y,Catch_Size+b_min,new_yres-Catch_Size);
		//cout<<CurrentPoint_X<<'\t'<<CurrentPoint_Y<<endl;
		if (Show_Interval==0)
			Mode2_move_func(CurrentPoint_X,CurrentPoint_Y);
	//glutPostRedisplay();
	}

	return;
}
/*------------F2 mouse function if mouse move------------*/
void Mode2_move_func(int x,int y)
{
	int m,n;
	FishEyeFunction(x,y);
	ChangeSize(k_width,k_height);
	All_redisplay();
	return;
}

/*------get the distance from now to the current corner-----*/
int  GetDistance(int x,int y,int a, int b)
{
	return (x-a)*(x-a)+(y-b)*(y-b);
}


/*----------Find the nearest corner--------------------*/
void NearestPoint(int x,int y)
{

	NearestPosition=5;
	int m, m1,m2,m3,m4;
	m1=GetDistance(x,y,a_new[0],b_new[0]);
	m2=GetDistance(x,y,a_new[1],b_new[1]);
	m3=GetDistance(x,y,a_new[2],b_new[2]);
	m4=GetDistance(x,y,a_new[3],b_new[3]);

	m=findmin(m1,m2,m3,m4);
	if (m<pow(click_above,2))
	{
		if (m==m1) NearestPosition=1;
		if (m==m2) NearestPosition=2;
		if (m==m3) NearestPosition=3;
		if (m==m4) NearestPosition=4;
	}
	else
	{
		if (IsDragPoint)
		std::cout<<"You should drag one of the four corner of this picture!!!"<<endl;
	}
}

/*------special key call back-----------*/
void specialkey(int key,int x,int y)
{
	if (key==GLUT_KEY_F1)
	{
		F1mode=true;
		F2mode=false;
		IsPartChanged=true;
		cout<<"F1 MODE!!!!"<<endl;
	}
	if (key==GLUT_KEY_F2)
	{
		F2mode=true;
		F1mode=false;
		cout<<"F2 MODE!!!!"<<endl;
		Fisheye_pixels=new unsigned char [2*Catch_Size*2*Catch_Size*4];
		fishwholepixels=new unsigned char [new_xres*new_yres*4];
	}
	if(key==GLUT_KEY_UP)
	{
		Len_focus+=0.1;
		Current_radius=(Len_focus*Len_focus+1)/2/Len_focus;
		cout<<"The length of len is: "<<Len_focus<<endl;
	}
	if(key==GLUT_KEY_DOWN)
	{
		Len_focus-=0.1;
		cout<<"The length of len is: "<<Len_focus<<endl;
		Current_radius=(Len_focus*Len_focus+1)/2/Len_focus;
	}
}
/*-----------------keyboard function-------------------*/
void keyboardfunc(unsigned char key,int x, int y)
{
   switch ( key ) 
   {
	case 'q'://when press q or Q or ESC then exit directly
	case 'Q':
	case  0x1b:
		exit(0);
		break;
	case 's':
	case 'S':
		if (Input_number>1)
		{
			Saving(savefilename);
			cout<<"Saving Successfully!!!"<<endl;
		}
		else
		{
			cout<<"You should input the save filename!"<<endl;
		}
		break;
	case 'b':
	case 'B':
		ReturnOri();
		cout<<"successfully back!!!"<<endl;
		All_redisplay();
		PointInit();

		break;
	case 'm':
	case 'M':
		IsBilinear=true;
		IsMoveWhole=true;
		IsDragPoint=false;
		IsMoveLine=false;
		break;
	case 'p':
	case 'P':
		IsDragPoint=true;
		IsMoveWhole=false;
		IsMoveLine=false;
		IsBilinear=false;
		break;
	case 'l':
	case 'L':
		IsMoveLine=true;
		IsDragPoint=false;
		IsMoveWhole=false;
		IsBilinear=false;
		break;
	default:
	   	break;
   }
   return ;
}
/*--------------display all the windows----------*/
void All_redisplay()
{
	glutSetWindow(window1);
	glutPostRedisplay();
	glutSetWindow(window2);
	glutPostRedisplay();
	return;
}

/*-----------------Initialize the first windows-------------------*/
void init (void)
{
	glClearColor(0.0,0.0,0.0,0.0);//backcolor is white
	glClear(GL_COLOR_BUFFER_BIT);//clear the color buffer
	glClear(GL_DEPTH_BUFFER_BIT);//clear the depth buffer
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);//ortho to the current size 
	return;
}
/*-----------------Initialize the second windows-------------------*/
void init2 (void)
{
	glClearColor(0.0,0.0,0.0,0.0);//backcolor is white
	glClear(GL_COLOR_BUFFER_BIT);//clear the color buffer
	glClear(GL_DEPTH_BUFFER_BIT);//clear the depth buffer
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);//ortho to the current size 
	return;
}

/*-----------------Open the picture-------------------*/
bool OpenImage(string outfilename)
{ 
	int m,n;
	ImageInput *in = ImageInput::create(outfilename); //open an input file
  	if (!in) 
	 {
      std::cout<<OpenImageIO::geterror()<<endl;
      exit(0);
      return false;
    }
  	ImageSpec spec;
	in->open (outfilename, spec);//check the detail of this file
	xres=spec.width;
	yres=spec.height;
	input_channelnum=spec.nchannels;
  	int pixel_length;
  	std::cout<<"Width = "<<xres<<endl;
  	std::cout<<"Height = "<<yres<<endl;
  	std::cout<<"Color Channel = "<<input_channelnum<<endl;
  	pixel_length=xres*yres*input_channelnum;
  	unsigned char* tempdata=new unsigned char[pixel_length];
	if(!in->read_image (TypeDesc::UINT8,tempdata))//read from the file
  	{
  		std::cout<<in->geterror()<<endl;
  	} 
	if (!in->close())
	{
		std::cout<<in->geterror()<<endl;
	}
	delete in;

	alpha_pixels=new unsigned char[xres*yres];
	pixels = new unsigned char [xres*yres*4];
	if (input_channelnum==4)
	{
		ori_channelnum=4;
		for (int i=0;i<yres;i++)
		{
			for (int j=0;j<xres;j++)
			{
				m=i*xres*input_channelnum+j*input_channelnum;
				n=i*xres*4+j*4;
				pixels[m+0]=tempdata[n+0];
				pixels[m+1]=tempdata[n+1];
				pixels[m+2]=tempdata[n+2];
				pixels[m+3]=tempdata[n+3];
				alpha_pixels[i*xres+j]=tempdata[n+3];
			}
		}
		
	}
	else if (input_channelnum==3)
	{
		ori_channelnum=3;
		input_channelnum=4;
		for (int i=0;i<yres;i++)
		{
			for (int j=0;j<xres;j++)
			{
				m=i*xres*4+j*4;
				n=i*xres*3+j*3;
				pixels[m+0]=tempdata[n+0];
				pixels[m+1]=tempdata[n+1];
				pixels[m+2]=tempdata[n+2];
				pixels[m+3]=255;
				alpha_pixels[xres*i+j]=255;
			}
		}
	}
	delete [] tempdata;	
	Upsidedown(pixels);
	OriData=new unsigned char[xres*yres*4];
	for (int i=0;i<yres;i++)
	{
		for (int j=0;j<xres;j++)
		{
			m=i*xres*input_channelnum+j*input_channelnum;
			OriData[m+0]=pixels[m+0];
			OriData[m+1]=pixels[m+1];
			OriData[m+2]=pixels[m+2];
			OriData[m+3]=alpha_pixels[xres*i+j];
		}
	}
	PointInit();
	smallerpixels=new unsigned char[new_xres*new_yres*input_channelnum];
	for (int i=0;i<new_xres*new_yres*input_channelnum;i++)
	{
		smallerpixels[i]=pixels[i];		
	}

	return true;
	
}


/*------initialize the point at the beginning ------*/
void PointInit(void)
{
	a_new[0]=0;
	a_new[1]=0;
	a_new[2]=xres;
	a_new[3]=xres;

	b_new[0]=0;
	b_new[1]=yres;
	b_new[2]=yres;
	b_new[3]=0;
	
	a_drawline[0]=0;
	a_drawline[1]=0;
	a_drawline[2]=xres;
	a_drawline[3]=xres;

	b_drawline[0]=0;
	b_drawline[1]=yres;
	b_drawline[2]=yres;
	b_drawline[3]=0;
	
	new_xres=xres;
	new_yres=yres;

}

/*-----------------input mistake-------------------*/
void ErrorExit(int argc)
{
	if (argc==1) //if there is no input
	{
		std::cout<<"No imagefile input"<<endl;
		exit (0);
	}
	if (argc==4)//if there is too much input
	{
     	std::cout<<"Too many input names!!!"<<endl;
		exit (0);        	 	
    } 	 
	return;
}

/*--------change the upper case to lower case-----*/
char Lowcasefunc(char x)
{
	if (x>64&&x<91)
		return x-32;
	else 
		return x;
}

