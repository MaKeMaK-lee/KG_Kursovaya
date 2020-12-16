#include "Render.h"

#include <windows.h>
#include <iostream>
#include <string>  
#include <vector>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

#include <chrono>
#include <functional>
#include <thread>

/*

<<������ ��� ���� �����_���� ����������.>>

<<������ ����� ���� �������� ��������.>>

<< ������������������������ >>

<<� ����� �� ������ ������.>>

*/
#define ROOM1_WALLX  200
#define ROOM1_WALLY	 200
#define ROOM1_WALLZ	 50
#define ROOM1_WALLXX -200
#define ROOM1_WALLYY -200
#define ROOM1_WALLZZ 0
#define ROOM2_WALLX	 300
#define ROOM2_WALLY	 200
#define ROOM2_WALLZ	 50
#define ROOM2_WALLXX 200
#define ROOM2_WALLYY -200
#define ROOM2_WALLZZ 0

#define WISPMAXCOUNT 128 //��� ��������� ���������� ����� �������� ��� � �������� (��� +1, ��� ������)
#define POP glPopMatrix()
#define PUSH glPushMatrix()
#define TICKMS 50
#define TICKS 0.05
#define MANSCALEONSCENE 1
#define AMASSBOXRAD 0.5
#define PICKUPRANGE 9

void DrawBox(float, float, float);

inline double GetRandomDouble(long min = 0, long max = 100)
{
	min *= 1000;
	max *= 1000;
	double r = (double)(((long)rand() % (max - min)) + min) / 1000;
	return r;
}

inline double GetR(double m1[3], double m2[3]) {
	return sqrt((m1[0] - m2[0]) * (m1[0] - m2[0]) + (m1[1] - m2[1]) * (m1[1] - m2[1]) + (m1[2] - m2[2]) * (m1[2] - m2[2]));
}

inline double f(double p1, double p2, double p3, double p4, double t)
{
	return p1 * (1 - t) * (1 - t) * (1 - t) + 3 * p2 * t * (1 - t) * (1 - t) + 3 * p3 * t * t * (1 - t) + p4 * t * t * t;
}

void SetBkColor()//���� ������� � ������� � ����������� �� ��� ����
{
	//====== ����������� ����� �� ��� ����������
	GLclampf red = 8 / 255.f,
		green = 0 / 255.f,
		blue = 32 / 255.f;
	//====== ��������� ����� ���� (��������) ����
	glClearColor(red, green, blue, 1.f);
	//====== ���������������� ��������
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool ispaused = false;
short CameraaMode = 0, CameraMode_b = 0, X_CD = 0, H_CD = 0;

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;
bool showHelps = true;

//ObjFile* model;

Shader s[11];  //��������� ��� ������ ��������

ObjFile objModel, monkey, monik, m, man_stepLeft, man_shortstepLeft, man_stepRight, man_shortstepRight, man_Stay, Wisp1, Fence1, Fence1R;
Texture monkeyTex, monikTex, mTex, manTex, grassTex, woodTex1, bamboonot, woodTex2, woodTex3, brickTex1, potolokTex1;

//������ ���������� ����
int mouseX = 0, mouseY = 0;

float offsetX = 0, offsetY = 0;
float zoom = 10000;
long Time = 0;
int tick_o = 0;
int tick_n = 0;

//����������
double ACCELMUL = 1;
double SPEEDMUL = 10;
double GRAVITY = -9.8;
double RUNNIGMUL = 1.5;
short MAXVADIMAMASS = 10;
float LIGHTLEVEL = 0.5;

class Wisp {
public:
	short type;
	short room;
	bool inman;
	int Timer;
	double pos[3];
	double speedv[3];
	double accelv[3];
	double maxspeed;

	float color[3];
	float ma[3];
	float md[3];
	float ms[4];

	inline Wisp(const double position[3] = 0, const double colorRGB[3] = 0, const double _maxspeed = 100) {
		if (position == 0)
		{
			pos[0] = 0;
			pos[1] = 0;
			pos[2] = 0;
		}
		else {
			pos[0] = position[0];
			pos[1] = position[1];
			pos[2] = position[2];
		}
		if (colorRGB == 0)
		{
			color[0] = 0.5;
			color[1] = 0.5;
			color[2] = 0.5;
		}
		else {
			color[0] = colorRGB[0];
			color[1] = colorRGB[1];
			color[2] = colorRGB[2];
		}
		ma[0] = 0.2;
		ma[1] = 0.2;
		ma[2] = 0.2;
		md[0] = 0.85;
		md[1] = 0.85;
		md[2] = 0.85;
		ms[0] = 0.9;
		ms[1] = 0.9;
		ms[2] = 0.9;
		ms[3] = GetRandomDouble(37, 150);
		type = 0;
		Timer = 0;
		room = 1;
		speedv[0] = 0;
		speedv[1] = 0;
		speedv[2] = 0;
		accelv[0] = 0;
		accelv[1] = 0;
		accelv[2] = 0;
		maxspeed = _maxspeed;
		inman = false;

	}

	inline void CalculateAccel() {
		accelv[0] = rand() % (21 + 21) - 21;
		accelv[1] = rand() % (21 + 21) - 21;
		accelv[2] = rand() % (7 + 7) - 7;
	}

	inline void CalculateSpeed(short* f) {
		speedv[0] += SPEEDMUL * accelv[0] * TICKS;
		speedv[1] += SPEEDMUL * accelv[1] * TICKS;
		speedv[2] += SPEEDMUL * accelv[2] * TICKS;
		int r = rand() % 1000;
		//if (r < 5)
		//	speedv[0] *= 0.01;
		//else
		//	if (r < 10)
		//		speedv[1] *= 0.01;
		//	else
		//if (r < 3)
		//	speedv[2] *= 0.01;

		if (speedv[0] > maxspeed)
			speedv[0] = (speedv[0] > 0 ? 1 : -1) * maxspeed / 2;
		if (speedv[1] > maxspeed)
			speedv[1] = (speedv[1] > 0 ? 1 : -1) * maxspeed / 2;
		if (speedv[2] > maxspeed)
			speedv[2] = (speedv[2] > 0 ? 1 : -1) * maxspeed / 2;

		if (f[0])
			speedv[0] *= -1.1;
		if (f[1])
			speedv[1] *= -1.1;
		if (f[2])
			speedv[2] *= -1.1;
	}

	inline void CalculateSpeed() {
		speedv[0] += SPEEDMUL * accelv[0] * TICKS;
		speedv[1] += SPEEDMUL * accelv[1] * TICKS;
		speedv[2] += SPEEDMUL * accelv[2] * TICKS;
	}

	/// <summary>
	/// ������� ������� �� ������ ��������
	/// </summary>
	/// <returns>
	/// 0, ���� �� ��
	/// ������, ���
	/// 0 - ��
	/// -1 - � -
	/// +1 - � +
	/// ������ ��������� � �����������:
	/// 0 - �
	/// 1 - �
	/// 2 - Z
	/// � �� ���������� � ������� �����������.
	/// </returns>
	inline void CalculatePosition(short* f) {
		double tmpx = pos[0] + speedv[0] * TICKS;
		double tmpy = pos[1] + speedv[1] * TICKS;
		double tmpz = pos[2] + speedv[2] * TICKS;
		f[0] = 0;
		f[1] = 0;
		f[2] = 0;
		if (room == 1)
		{
			if (!(ROOM1_WALLXX < tmpx))
				f[0] = -1;
			if (!(tmpx < ROOM1_WALLX))
				f[0] = 1;
			if (!(ROOM1_WALLYY < tmpy))
				f[1] = -1;
			if (!(tmpy < ROOM1_WALLY))
				f[1] = 1;
			if (!(ROOM1_WALLZZ < tmpz))
				f[2] = -1;
			if (!(tmpz < ROOM1_WALLZ))
				f[2] = 1;
		}
		if (room == 2)
		{
			if (!(ROOM2_WALLXX < tmpx))
				f[0] = -1;
			if (!(tmpx < ROOM2_WALLX))
				f[0] = 1;
			if (!(ROOM2_WALLYY < tmpy))
				f[1] = -1;
			if (!(tmpy < ROOM2_WALLY))
				f[1] = 1;
			if (!(ROOM2_WALLZZ < tmpz))
				f[2] = -1;
			if (!(tmpz < ROOM2_WALLZ))
				f[2] = 1;
		}
		if (!f[0] && !f[1] && !f[2]) {
			pos[0] = tmpx;
			pos[1] = tmpy;
			pos[2] = tmpz;
			if (f[0])
				speedv[0] *= 0.3;
			if (f[1])
				speedv[1] *= 0.3;
			if (f[2])
				speedv[2] *= 0.3;
			f[0] = 0;
			f[1] = 0;
			f[2] = 0;
		}
	}

	inline void Draw();

};
std::vector<Wisp> WispList;

class Man {
public:
	double MovementSpeed;
	double JumpSpeed;
	double mass;
	double pos[3];
	short moveMode;
	double speedv[3];
	double accelv[3];
	double rotZdeg;
	bool startJumping;
	bool stay;//animation only
	bool stepRight;//animation only
	bool running;//animation only
	short changesteps;
	short amass;
	std::vector<int> PickedUpWisps;

	inline void TryToJump() {
		pos[2] <= 0 ? startJumping = true : 0;
	}

	Man(const double position[3] = 0) {
		if (position == 0)
		{
			pos[0] = 0;
			pos[1] = 0;
			pos[2] = 0;
		}
		else {
			pos[0] = position[0];
			pos[1] = position[1];
			pos[2] = position[2];
		}
		startJumping = false;
		amass = 0;
		rotZdeg = 0;
		moveMode = 0;
		stay = true;
		stepRight = true;
		changesteps = 0;
		running = false;
		mass = 55;
		MovementSpeed = 5 * 0.7;
		JumpSpeed = 7.2;
		speedv[0] = 0;
		speedv[1] = 0;
		speedv[2] = 0;
		accelv[0] = 0;
		accelv[1] = 0;
		accelv[2] = 0;
	}

	/// <summary>
	/// �������� �������������
	/// </summary>
	/// <param name="isplus">���� ��� ���������� � ��������</param>
	/// <returns>
	/// 0 - �������
	/// 1 - ������� + ��� ��������� ��������
	/// -1 - ������� - ��� 0 ��������
	/// </returns>
	short AmassUp(const bool isplus = false) {
		if ((!isplus) && amass == 0)
			return -1;
		if ((isplus) && amass == MAXVADIMAMASS)
			return 1;
		isplus ? amass++ : amass--;
		JumpSpeed *= isplus ? 0.9 : (double)10 / 9;
		switch (amass)
		{
		case 0:
			MovementSpeed = 0.7 * 5;
			break;
		case 1:
			MovementSpeed = 0.7 * 4.8;
			break;
		case 2:
			MovementSpeed = 0.7 * 4.5;
			break;
		case 3:
			MovementSpeed = 0.7 * 4.25;
			break;
		case 4:
			MovementSpeed = 0.7 * 4;
			break;
		case 5:
			MovementSpeed = 0.7 * 3.67;
			break;
		case 6:
			MovementSpeed = 0.7 * 3.33;
			break;
		case 7:
			MovementSpeed = 0.7 * 3;
			break;
		case 8:
			MovementSpeed = 0.7 * 2.5;
			break;
		case 9:
			MovementSpeed = 0.7 * 2;
			break;
		case 10:
			MovementSpeed = 0.7 * 1;
			break;
		}
		return 0;
	}

	inline void CalculateAccel() {
		if (pos[2] <= 0) {
			accelv[2] = 0;
			return;
		}
		//accelv[0] = 0;
		//accelv[1] = 0;
		accelv[2] = GRAVITY;

		//abs(accelv[0]) < 0.01 ? accelv[0] = 0 : 0;
		//abs(accelv[1]) < 0.01 ? accelv[1] = 0 : 0;
		abs(accelv[2]) < 0.01 ? accelv[2] = 0 : 0;
	}

	inline void CalculateSpeed() {
		if (startJumping)
		{
			speedv[0] = speedv[0];
			speedv[1] = speedv[1];
			speedv[2] += SPEEDMUL * (JumpSpeed + accelv[2] * TICKS);
			startJumping = false;
			return;
		}
		if (pos[2] <= 0 && moveMode == 0)
		{
			speedv[0] = 0;
			speedv[1] = 0;
			speedv[2] = 0;
			return;
		}
		if (pos[2] > 0)
		{
			speedv[0] = speedv[0];
			speedv[1] = speedv[1];
			speedv[2] += SPEEDMUL * accelv[2] * TICKS;
			return;
		}

		if (moveMode == 2)
		{
			speedv[0] = RUNNIGMUL * SPEEDMUL * MovementSpeed * -sin(rotZdeg * PI / 180);
			speedv[1] = RUNNIGMUL * SPEEDMUL * MovementSpeed * cos(rotZdeg * PI / 180);
			speedv[2] = 0;
		}
		else
		{
			speedv[0] = moveMode * SPEEDMUL * MovementSpeed * -sin(rotZdeg * PI / 180);
			speedv[1] = moveMode * SPEEDMUL * MovementSpeed * cos(rotZdeg * PI / 180);
			speedv[2] = 0;
		}
	}

	inline void CalculatePosition() {
		double tmpp0 = pos[0] + speedv[0] * TICKS;
		double tmpp1 = pos[1] + speedv[1] * TICKS;
		(pos[2] = pos[2] + speedv[2] * TICKS) < 0 ? pos[2] = 0 : 0;
		if (tmpp0 < -200)
			tmpp0 = -200;
		if (tmpp0 > 300)
			tmpp0 = 300;
		if (tmpp1 < -200)
			tmpp1 = -200;
		if (tmpp1 > 200)
			tmpp1 = 200;
		pos[0] = tmpp0;
		pos[1] = tmpp1;
	}

	inline void AnimationTick() {

		if (moveMode == 0)
			stay = true;
		else {
			stay = false;
			if (moveMode == 2)
				running = true;
			else
				running = false;
			if (changesteps++ >= (running ? 1 : 2)) {
				stepRight = !stepRight;
				changesteps = 0;
			}
		}
	}

	void Draw();


};
Man Vadim;

//����� ��� ��������� ������
class CustomCamera : public Camera
{
public:
	//��������� ������
	double camDist;
	double camDist0;
	double camDist12;
	//���� �������� ������
	double fi1, fi2;
	Vector3 prepos;

	//������� ������ �� ���������
	CustomCamera()
	{
		camDist = 70;
		camDist0 = camDist;
		camDist12 = 17;
		fi1 = 1;
		fi2 = 1;
		prepos = pos;
	}


	//������� ������� ������, ������ �� ����� ��������, ���������� �������
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
			camDist * cos(fi2) * sin(fi1),
			camDist * sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		if (CameraaMode == 1 || CameraaMode == 2) {
			Vector3 nowpos = { (Vadim.pos[0]) * MANSCALEONSCENE + pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + pos.Z() };
			gluLookAt(nowpos.X(), nowpos.Y(), nowpos.Z(),
				Vadim.pos[0] * MANSCALEONSCENE, Vadim.pos[1] * MANSCALEONSCENE, (Vadim.pos[2] + 17) * MANSCALEONSCENE,
				normal.X(), normal.Y(), normal.Z());
		}
		if (CameraaMode == 0)
			gluLookAt(pos.X(), pos.Y(), pos.Z() + 17, lookPoint.X(), lookPoint.Y(), lookPoint.Z() + 17, normal.X(), normal.Y(), normal.Z());
	}



}  camera;    //������� ������ ������

//����� ��� ��������� �����
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//��������� ������� �����
		pos = Vector3(1, 1, 3);
	}


	//������ ����� � ����� ��� ���������� �����, ���������� �������
	void  DrawLightGhismo()
	{

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);

		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale * 0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//����� �� ��������� ����� �� ����������
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//������ ���������
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale * 1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.3, 0.3, 0.3, 1 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 1 };
		GLfloat spec[] = { .7, .7, .7, 1 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// ��������� ��������� �����
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// �������������� ����������� �����
		// ������� ��������� (���������� ����)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// ��������� ������������ �����
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// ��������� ���������� ������������ �����
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //������� �������� �����

//���������� �������� ����
void mouseEvent(OpenGL* ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//������ ���� ������ ��� ������� ����� ������ ����
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01 * dx;
		camera.fi2 += -0.01 * dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0 * dx / ogl->getWidth() / zoom;
		offsetY += 1.0 * dy / ogl->getHeight() / zoom;
	}



	//������� ���� �� ���������, � ����� ��� ����
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(camera.camDist, camera.fi1, camera.fi2, POINT->x, POINT->y, 92, ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k * r.direction.X() + r.origin.X();
		y = k * r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02 * dy);
	}


}

//���������� �������� ������  ����
void mouseWheelEvent(OpenGL* ogl, int delta)
{


	float _tmpZ = delta * 0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2 * zoom * _tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01 * delta;
}

//���������� ������� ������ ����������

void keyDownEvent(OpenGL* ogl, int key)
{
	/*
	if (key == 'L')
	{
		Vadim.pos[2] += 1;
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		Vadim.pos[2] += 1;
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		Vadim.pos[2] += 1;
	}

	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	//if (key == 'Q')
	//	Time = 0;
	*/
}

void keyUpEvent(OpenGL* ogl, int key)
{
}

void Tick() {
	//-����� ��� � ��������� ������� ������
	if (OpenGL::isKeyPressed('W'))
		if (OpenGL::isKeyPressed(VK_SHIFT))
			Vadim.moveMode = 2;
		else
			Vadim.moveMode = 1;
	else
		if (OpenGL::isKeyPressed('S'))
			Vadim.moveMode = -1;
		else
			Vadim.moveMode = 0;
	if (OpenGL::isKeyPressed('A')) {
		double drotz = (double)360 / TICKMS;
		Vadim.rotZdeg += drotz;
		CameraaMode == 2 ? camera.fi1 += drotz * PI / 180 : 0;
	}
	if (OpenGL::isKeyPressed('D')) {
		double drotz = (double)360 / TICKMS;
		Vadim.rotZdeg -= drotz;
		CameraaMode == 2 ? camera.fi1 -= drotz * PI / 180 : 0;
	}
	if (OpenGL::isKeyPressed(VK_SPACE))
		Vadim.TryToJump();

	if (OpenGL::isKeyPressed('V'))
		if (CameraMode_b <= 0) {
			CameraMode_b = 5;
			++CameraaMode > 2 ? CameraaMode = 0 : 0;
			switch (CameraaMode)
			{
			case 0:
				camera.camDist12 = camera.camDist;
				camera.camDist = camera.camDist0;
				break;
			case 1:
				camera.camDist0 = camera.camDist;
				camera.camDist = camera.camDist12;
				break;
			case 2:
				break;
			}
		}

	if (OpenGL::isKeyPressed('C'))
		if (CameraaMode == 2 || CameraaMode == 1)
			camera.fi1 = (Vadim.rotZdeg - 90) * PI / 180;
	if (CameraaMode == 0)
		camera.lookPoint = { Vadim.pos[0] * MANSCALEONSCENE, Vadim.pos[1] * MANSCALEONSCENE, camera.lookPoint.Z() };
	//������ �������, �� ����� � ���������� ������))
	//if (OpenGL::isKeyPressed(VK_OEM_6) && LIGHTLEVEL < 1)
	//{
	//	LIGHTLEVEL += 0.05;
	//	if (LIGHTLEVEL > 1)
	//		LIGHTLEVEL = 1;
	//}
	//if (OpenGL::isKeyPressed(VK_OEM_4) && LIGHTLEVEL > 0)
	//{
	//	LIGHTLEVEL -= 0.05;
	//	if (LIGHTLEVEL < 0.15)
	//		LIGHTLEVEL = 0.15;
	//}
	if (OpenGL::isKeyPressed('F')) {
		double tmppos[3] = { Vadim.pos[0],Vadim.pos[1],Vadim.pos[2] + 12 };
		for (int i = 0; i < WispList.size(); i++)
		{
			if (!WispList[i].inman)
				if (GetR(tmppos, WispList[i].pos) <= PICKUPRANGE) {
					if (Vadim.AmassUp(true) == 0) {
						WispList[i].inman = true;
						Vadim.PickedUpWisps.push_back(i);
					}
					else
						break;
				}
		}
	}
	if (OpenGL::isKeyPressed('X')) {
		if (X_CD <= 0) {
			X_CD = 10;

			if (Vadim.AmassUp(false) == 0) {
				WispList[Vadim.PickedUpWisps.back()].pos[0] = Vadim.pos[0];
				WispList[Vadim.PickedUpWisps.back()].pos[1] = Vadim.pos[1];
				WispList[Vadim.PickedUpWisps.back()].pos[2] = Vadim.pos[2] + 17;
				WispList[Vadim.PickedUpWisps.back()].speedv[0] = 0;
				WispList[Vadim.PickedUpWisps.back()].speedv[1] = 0;
				WispList[Vadim.PickedUpWisps.back()].speedv[2] = 0;
				WispList[Vadim.PickedUpWisps.back()].room = Vadim.pos[0] < 200 ? 1 : 2;
				WispList[Vadim.PickedUpWisps.back()].inman = false;
				Vadim.PickedUpWisps.pop_back();
			}
		}
	}

	if (OpenGL::isKeyPressed('H')) {
		if (H_CD <= 0) {
			H_CD = 8;
			showHelps = !showHelps;
		}
	}

	//-������� ��, ����



	//-����������� ��������� ��������
	//Vadim's
	Vadim.AnimationTick();

	//-�������� ���������� ��������� �����
	//(�� ��� ������ � ������� ���� ���������� ������ ������. ������ ���-������ ��� ����������� ��� ��� �� � ���� ����, ��� ��� ����� � �� �������.)

	//����� ����� ������
	if (WispList.size() < WISPMAXCOUNT && GetRandomDouble() < 0.17)
		WispList.push_back(Wisp(Vector3((double)(rand() % (ROOM1_WALLX - ROOM1_WALLXX) + ROOM1_WALLXX), (double)(rand() % (ROOM1_WALLY - ROOM1_WALLYY) + ROOM1_WALLYY), GetRandomDouble(ROOM1_WALLZZ, ROOM1_WALLZ)).toArray(),
			Vector3(GetRandomDouble(0.1, 1), GetRandomDouble(0.1, 1), GetRandomDouble(0.1, 1)).toArray(),
			rand() % 100 + 10));;
	//��������� ������������
	short flags[3] = { 0,0,0 };
	for (int l = 0; l < WispList.size(); l++)
	{
		if (WispList[l].inman)
			continue;
		WispList[l].CalculateAccel();
		do {

			if (!(flags[0] == 0 && flags[1] == 0 && flags[2] == 0)) {
				WispList[l].CalculateSpeed(flags);
			}
			else {
				WispList[l].CalculateSpeed();
			}
			WispList[l].CalculatePosition(flags);
		} while (!(flags[0] == 0 && flags[1] == 0 && flags[2] == 0));
		//
	}

	//-����������� ������
	//-������� ��� (���, �� ������)
	X_CD > 0 ? X_CD-- : 0;
	H_CD > 0 ? H_CD-- : 0;
	CameraMode_b > 0 ? CameraMode_b-- : 0;
	Time = 0;
}

inline void DrawBox(float r, float g, float b) {
	Shader::DontUseShaders();
	s[10].UseShader();
	//������ ����
	int location = glGetUniformLocationARB(s[8].program, "ColorRGB");
	glUniform3fARB(location, r, g, b);

	double m1[] = { AMASSBOXRAD, AMASSBOXRAD, AMASSBOXRAD };
	double m2[] = { AMASSBOXRAD, -AMASSBOXRAD, AMASSBOXRAD };
	double m3[] = { -AMASSBOXRAD, -AMASSBOXRAD, AMASSBOXRAD };
	double m4[] = { -AMASSBOXRAD, AMASSBOXRAD, AMASSBOXRAD };
	double m5[] = { AMASSBOXRAD, AMASSBOXRAD, -AMASSBOXRAD };
	double m6[] = { AMASSBOXRAD, -AMASSBOXRAD, -AMASSBOXRAD };
	double m7[] = { -AMASSBOXRAD, -AMASSBOXRAD, -AMASSBOXRAD };
	double m8[] = { -AMASSBOXRAD, AMASSBOXRAD, -AMASSBOXRAD };

	double* massV[] = { m1, m2, m3, m4, m5, m6, m7, m8 };
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glVertex3dv(massV[0]);
	glVertex3dv(massV[1]);
	glVertex3dv(massV[2]);
	glVertex3dv(massV[3]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glVertex3dv(massV[0]);
	glVertex3dv(massV[1]);
	glVertex3dv(massV[5]);
	glVertex3dv(massV[4]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glVertex3dv(massV[1]);
	glVertex3dv(massV[2]);
	glVertex3dv(massV[6]);
	glVertex3dv(massV[5]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glVertex3dv(massV[2]);
	glVertex3dv(massV[3]);
	glVertex3dv(massV[7]);
	glVertex3dv(massV[6]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glVertex3dv(massV[0]);
	glVertex3dv(massV[3]);
	glVertex3dv(massV[7]);
	glVertex3dv(massV[4]);
	glEnd();
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glVertex3dv(massV[4]);
	glVertex3dv(massV[5]);
	glVertex3dv(massV[6]);
	glVertex3dv(massV[7]);
	glEnd();

	Shader::DontUseShaders();
}

//����������� ����� ������ ��������
void initRender(OpenGL* ogl)
{
	srand(time(0));

	camera.lookPoint = { camera.lookPoint.X(), camera.lookPoint.Y(), camera.lookPoint.Z() + 17 };
	//��


	//��������� �������

	//4 ����� �� �������� �������
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//��������� ������ ��������� �������
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//�������� ��������
	glEnable(GL_TEXTURE_2D);

	//������ � ���� ����������� � "������"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// ������������ �������� : �� ����� ����� ����� 1
	glEnable(GL_NORMALIZE);

	// ���������� ������������� ��� �����
	glEnable(GL_LINE_SMOOTH);


	//   ������ ��������� ���������
	//  �������� GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  ������� � ���������� �������� ���������(�� ���������), 
	//                1 - ������� � ���������� �������������� ������� ��������       
	//                �������������� ������� � ���������� ��������� ����������.    
	//  �������� GL_LIGHT_MODEL_AMBIENT - ������ ������� ���������, 
	//                �� ��������� �� ���������
	// �� ��������� (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   �������� �������� �� �����
	*/

	s[5].VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
	s[5].FshaderFileName = "shaders\\LightTex.frag"; //��� ����� ������������ �������
	s[5].LoadShaderFromFile(); //��������� ������� �� �����
	s[5].Compile(); //�����������

	s[6].VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
	s[6].FshaderFileName = "shaders\\ManyLightTex.frag"; //��� ����� ������������ �������
	s[6].LoadShaderFromFile(); //��������� ������� �� �����
	s[6].Compile(); //�����������

	s[7].VshaderFileName = "shaders\\Man.vert"; //��� ����� ���������� ������
	s[7].FshaderFileName = "shaders\\Man.frag"; //��� ����� ������������ �������
	s[7].LoadShaderFromFile(); //��������� ������� �� �����
	s[7].Compile(); //�����������

	s[8].VshaderFileName = "shaders\\Wisp.vert"; //��� ����� ���������� ������
	s[8].FshaderFileName = "shaders\\Wisp.frag"; //��� ����� ������������ �������
	s[8].LoadShaderFromFile(); //��������� ������� �� �����
	s[8].Compile(); //�����������

	s[9].VshaderFileName = "shaders\\vUniCoordonly.vert"; //��� ����� ���������� ������
	s[9].FshaderFileName = "shaders\\Fences.frag"; //��� ����� ������������ �������
	s[9].LoadShaderFromFile(); //��������� ������� �� �����
	s[9].Compile(); //�����������

	s[3].VshaderFileName = "shaders\\vUniCoordonly.vert"; //��� ����� ���������� ������
	s[3].FshaderFileName = "shaders\\SpectWall.frag"; //��� ����� ������������ �������
	s[3].LoadShaderFromFile(); //��������� ������� �� �����
	s[3].Compile(); //�����������

	s[10].VshaderFileName = "shaders\\Box.vert"; //��� ����� ���������� ������
	s[10].FshaderFileName = "shaders\\Box.frag"; //��� ����� ������������ �������
	s[10].LoadShaderFromFile(); //��������� ������� �� �����
	s[10].Compile(); //�����������


	 //��� ��� ��� ������� ������ *.obj �����, ��� ��� ��� ��������� �� ���������� � ���������� �������, 
	 // ������������ �� ����� ����������, � ������������ ������ � *.obj_m
	//loadModel("models\\lpgun6.obj_m", &objModel);

	glActiveTexture(GL_TEXTURE0);
	grassTex.loadTextureFromFile("textures\\grass.bmp");
	woodTex1.loadTextureFromFile("textures\\wood1.bmp");
	bamboonot.loadTextureFromFile("textures\\bamboonot.bmp");
	woodTex2.loadTextureFromFile("textures\\wood2.bmp");
	woodTex3.loadTextureFromFile("textures\\wood3.bmp");
	brickTex1.loadTextureFromFile("textures\\brick1.bmp");
	potolokTex1.loadTextureFromFile("textures\\potolokTex1.bmp");

	loadModel("models\\man\\man_stepLeft.obj_m", &man_stepLeft);
	manTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_stepRight.obj_m", &man_stepRight);
	//man_stepRightTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_Stay.obj_m", &man_Stay);
	//man_StayTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_shortstepLeft.obj_m", &man_shortstepLeft);
	loadModel("models\\man\\man_shortstepRight.obj_m", &man_shortstepRight);

	loadModel("models\\Wisp\\w1.obj_m", &Wisp1);

	loadModel("models\\fence\\1.obj_m", &Fence1);
	loadModel("models\\fence\\1_r90.obj_m", &Fence1R);

	tick_n = GetTickCount64();
	tick_o = tick_n;

	rec.setSize(700, 200);
	rec.setPosition(10, ogl->getHeight() - 0 - 10);
	rec.setText("�� ������ ��������� 128 ���������� �� ����� ����, � ������� ��������� ���� �\n ���� ��� ����� ������. ���������� ����� �� 10 ����������, ��� ���� �� ������ ����� ��������.\n� ��� ���� � ����� ������, ��� ������� ��� � �����������.\n\nH - ������ / �������� ������.\nWSAD, SPACE, LSHIFT - ������������, ������, ���.\nG - ������� ������\nF - ������� ��������� (�� �������� ������ ���������� ������)\nX - ��������� ���������\nV - ������� ��� (���/�� �������/�� ������� � ����.�����)\nC - ��������� ������ �� ������ ������", 0, 0, 0);

	//\n�� ������ ��������� 128 ���������� �� ����� ����, � ������� ��������� ���� � ���� ��� ����� ������.
	//\n���������� ����� �� 10 ����������, ��� ���� �� ������ ����� ��������.
	//\n� ��� ���� � ����� ������, ��� ������� ��� � �����������.
	//\n
	//\nH - ������ / �������� ������.
	//\nWSAD, SPACE, LSHIFT - ������������, ������, ���.
	//\nG - ������� ������
	//\nF - ������� ���������(��� ����� �� ������ ���� ���������� �� ���� ������)
	//\nX - ��������� ���������
	//\nV - ������� ���(��� / �� ������� / �� ������� � ����.�����)
	//\nC - ��������� ������ �� ������ ������


}

void Render(OpenGL* ogl)
{
	//������������ ������
	Vadim.CalculateAccel();
	Vadim.CalculateSpeed();
	Vadim.CalculatePosition();
	//
	SetBkColor();
	tick_o = tick_n;
	tick_n = GetTickCount64();
	Time += (tick_n - tick_o);
	if (Time >= TICKMS)
		Tick();

	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	*/

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);

	//��������������
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//��� +
#pragma region ��� +
	glActiveTexture(GL_TEXTURE0);
	////��������� ���������
	GLfloat pamb[] = { 0.1, 0.1, 0.1, 0.5 };
	GLfloat pdif[] = { 0.1, 0.1, 0.1, 0.5 };
	GLfloat pspec[] = { 0.1, 0.1, 0.1, 0.5 };
	GLfloat psh = 0.1f * 128;
	//�������
	glMaterialfv(GL_FRONT, GL_AMBIENT, pamb);
	//��������
	glMaterialfv(GL_FRONT, GL_DIFFUSE, pdif);
	//����������
	glMaterialfv(GL_FRONT, GL_SPECULAR, pspec);
	//������ �����
	glMaterialf(GL_FRONT, GL_SHININESS, psh);
	PUSH;

	grassTex.bindTexture();

	s[6].UseShader();
	//������ � ���� ������ � ��������
	int location = glGetUniformLocationARB(s[6].program, "camera");
	if (CameraaMode == 0)
		glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());
	if (CameraaMode == 1 || CameraaMode == 2)
		glUniform3fARB(location, (Vadim.pos[0]) * MANSCALEONSCENE + camera.pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + camera.pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + camera.pos.Z());
	location = glGetUniformLocationARB(s[6].program, "tex");
	glUniform1iARB(location, 0);

	//������ ��� ��� ������ ����������
	int count = 1;//���-�� ������������
	location = glGetUniformLocationARB(s[6].program, "light_pos[0]");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());
	location = glGetUniformLocationARB(s[6].program, "Ia[0]");
	glUniform3fARB(location, 0.2, 0.2, 0.2);
	location = glGetUniformLocationARB(s[6].program, "Id[0]");
	glUniform3fARB(location, 1.0, 1.0, 1.0);
	location = glGetUniformLocationARB(s[6].program, "Is[0]");
	glUniform3fARB(location, .7, .7, .7);
	location = glGetUniformLocationARB(s[6].program, "ma[0]");
	glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
	location = glGetUniformLocationARB(s[6].program, "md[0]");
	glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
	location = glGetUniformLocationARB(s[6].program, "ms[0]");
	glUniform4fARB(location, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, 25.6);

	//������ ��� ��� ��������� �������� �� ����� (�������)
	std::string strtmp1;
	int i, inmans = 0;
	for (0; count + inmans <= WispList.size(); count++) {
		i = count - 1 + inmans;
		if (WispList[i].inman) {
			inmans++;
			count--;
			continue;
		}

		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "light_pos[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].pos[0], WispList[i].pos[1], WispList[i].pos[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "Ia[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "Id[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "Is[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "ma[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].ma[0], WispList[i].ma[1], WispList[i].ma[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "md[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].md[0], WispList[i].md[1], WispList[i].md[2]);
		location = glGetUniformLocationARB(s[6].program, ((strtmp1 = "ms[") += std::to_string(count) += "]").c_str());
		glUniform4fARB(location, WispList[i].ms[0], WispList[i].ms[1], WispList[i].ms[2], WispList[i].ms[3]);

	}
	location = glGetUniformLocationARB(s[6].program, "count");
	glUniform1iARB(location, count);
	for (int i = -200; i <= 199; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(0, 0, 1);
		for (int g = -200; g <= 200; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(i, g, 0);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(i + 1, g, 0);
		}
		glEnd();
	}
	//��� � ���� 2
	//������� � ����
	woodTex2.bindTexture();
	location = glGetUniformLocationARB(s[6].program, "tex");
	glUniform1iARB(location, 0);
	for (int i = 200; i <= 299; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(0, 0, 1);
		for (int g = -200; g <= 200; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(i, g, 0);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(i + 1, g, 0);
		}
		glEnd();
	}
	//������� � ���� 2
	//������� � ����
	potolokTex1.bindTexture();
	location = glGetUniformLocationARB(s[6].program, "tex");
	glUniform1iARB(location, 0);
	for (int i = 200; i <= 299; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(0, 0, 1);
		for (int g = -200; g <= 200; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(i, g, 50);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(i + 1, g, 50);
		}
		glEnd();
	}
	//����� � ���� 2
	//������� � ����
	brickTex1.bindTexture();
	location = glGetUniformLocationARB(s[6].program, "tex");
	glUniform1iARB(location, 0);
	//����� ����� � ���� 2
	for (int i = 200; i <= 299; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(0, -1, 0);
		for (int g = 0; g <= ROOM2_WALLZ; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(i, 200, g);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(i + 1, 200, g);
		}
		glEnd();
	}
	//����� ������ � ���� 2
	for (int i = 200; i <= 299; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(0, 1, 0);
		for (int g = 0; g <= ROOM2_WALLZ; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(i, -200, g);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(i + 1, -200, g);
		}
		glEnd();
	}
	//����� ������� � ���� 2
	for (int i = -200; i <= 200; i++)
	{

		glBegin(GL_QUAD_STRIP);
		glNormal3d(-1, 0, 0);
		for (int g = 0; g <= ROOM2_WALLZ; g++)
		{
			glTexCoord2f(i * 0.1, g * 0.1);
			glVertex3d(300, i, g);
			glTexCoord2f((i + 1) * 0.1, g * 0.1);
			glVertex3d(300, i + 1, g);
		}
		glEnd();
	}
	//����� ������� � ���� 2 - ������ ������

	POP;
	Shader::DontUseShaders();
#pragma endregion

	//���������
#pragma region ���������
	PUSH;
	{
		woodTex1.bindTexture();
		s[9].UseShader();
		int location;
		//������ � ���� ������ � ��������
		location = glGetUniformLocationARB(s[9].program, "camera");
		if (CameraaMode == 0)
			glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());
		if (CameraaMode == 1 || CameraaMode == 2)
			glUniform3fARB(location, (Vadim.pos[0]) * MANSCALEONSCENE + camera.pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + camera.pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + camera.pos.Z());
		location = glGetUniformLocationARB(s[9].program, "tex");
		glUniform1iARB(location, 0);
		//������ ��� ��� ������ ����������
		int count = 1;//���-�� ������������
		location = glGetUniformLocationARB(s[9].program, "light_pos[0]");
		glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());
		location = glGetUniformLocationARB(s[9].program, "Ia[0]");
		glUniform3fARB(location, 0.7, 0.7, 0.7);
		location = glGetUniformLocationARB(s[9].program, "Id[0]");
		glUniform3fARB(location, 8.0, 8.0, 8.0);
		location = glGetUniformLocationARB(s[9].program, "Is[0]");
		glUniform3fARB(location, .7, .7, .7);
		location = glGetUniformLocationARB(s[9].program, "ma[0]");
		glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
		location = glGetUniformLocationARB(s[9].program, "md[0]");
		glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
		location = glGetUniformLocationARB(s[9].program, "ms[0]");
		glUniform4fARB(location, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, 25.6);

		//������ ��� ��� ��������� �������� �� ����� (�������)
		std::string strtmp1;
		int i, inmans = 0;
		for (0; count + inmans <= WispList.size(); count++) {
			i = count - 1 + inmans;
			if (WispList[i].inman) {
				inmans++;
				count--;
				continue;
			}

			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "light_pos[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].pos[0], WispList[i].pos[1], WispList[i].pos[2]);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "Ia[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "Id[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "Is[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "ma[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, 0.7, 0.7, 0.7);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "md[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].md[0], WispList[i].md[1], WispList[i].md[2]);
			location = glGetUniformLocationARB(s[9].program, ((strtmp1 = "ms[") += std::to_string(count) += "]").c_str());
			glUniform4fARB(location, 0.6, 0.6, 0.6, WispList[i].ms[3]);

		}
		location = glGetUniformLocationARB(s[9].program, "count");
		glUniform1iARB(location, count);

		//������ ��� ������ � ������
		location = glGetUniformLocationARB(s[9].program, "PosOffset");
		//1
		glUniform3fARB(location, -200, -200, 0);
		PUSH;
		glTranslated(-200, -200, 0);
		Fence1.DrawObj();
		POP;
		//2
		glUniform3fARB(location, -100, -200, 0);
		PUSH;
		glTranslated(-100, -200, 0);
		Fence1.DrawObj();
		POP;
		//3
		glUniform3fARB(location, 0, -200, 0);
		PUSH;
		glTranslated(0, -200, 0);
		Fence1.DrawObj();
		POP;
		//4
		glUniform3fARB(location, 100, -200, 0);
		PUSH;
		glTranslated(100, -200, 0);
		Fence1.DrawObj();
		POP;
		//21
		glUniform3fARB(location, -200, 200, 0);
		PUSH;
		glTranslated(-200, 200, 0);
		Fence1.DrawObj();
		POP;
		//22
		glUniform3fARB(location, -100, 200, 0);
		PUSH;
		glTranslated(-100, 200, 0);
		Fence1.DrawObj();
		POP;
		//23
		glUniform3fARB(location, 0, 200, 0);
		PUSH;
		glTranslated(0, 200, 0);
		Fence1.DrawObj();
		POP;
		//24
		glUniform3fARB(location, 100, 200, 0);
		PUSH;
		glTranslated(100, 200, 0);
		Fence1.DrawObj();
		POP;
		//31
		glUniform3fARB(location, -200, -200, 0);
		PUSH;
		glTranslated(-200, -200, 0);
		Fence1R.DrawObj();
		POP;
		//32
		glUniform3fARB(location, -200, -100, 0);
		PUSH;
		glTranslated(-200, -100, 0);
		Fence1R.DrawObj();
		POP;
		//33
		glUniform3fARB(location, -200, 0, 0);
		PUSH;
		glTranslated(-200, 0, 0);
		Fence1R.DrawObj();
		POP;
		//34
		glUniform3fARB(location, -200, -100, 0);
		PUSH;
		glTranslated(-200, 100, 0);
		Fence1R.DrawObj();
		POP;
	}
	POP;
#pragma endregion

	//������� 
	PUSH;
	Shader::DontUseShaders();

	manTex.bindTexture();

	Vadim.Draw();

	Shader::DontUseShaders();
	POP;

	//�����
	PUSH;
	Shader::DontUseShaders();


	for (int l = 0; l < WispList.size(); l++)
	{
		if (!WispList[l].inman)
			WispList[l].Draw();
	}

	Shader::DontUseShaders();
	POP;


	//����� ��� ���������
	PUSH;
	{
		s[3].UseShader();
		//������ � ���� ������
		int location = glGetUniformLocationARB(s[3].program, "camera");
		if (CameraaMode == 0)
			glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());
		if (CameraaMode == 1 || CameraaMode == 2)
			glUniform3fARB(location, (Vadim.pos[0]) * MANSCALEONSCENE + camera.pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + camera.pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + camera.pos.Z());
		//������ ��� ��� ������ ����������
		int count = 1;//���-�� ������������
		location = glGetUniformLocationARB(s[3].program, "light_pos[0]");
		glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());
		location = glGetUniformLocationARB(s[3].program, "Ia[0]");
		glUniform3fARB(location, 0.2, 0.2, 0.2);
		location = glGetUniformLocationARB(s[3].program, "Id[0]");
		glUniform3fARB(location, 1.0, 1.0, 1.0);
		location = glGetUniformLocationARB(s[3].program, "Is[0]");
		glUniform3fARB(location, .7, .7, .7);
		location = glGetUniformLocationARB(s[3].program, "ma[0]");
		glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
		location = glGetUniformLocationARB(s[3].program, "md[0]");
		glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
		location = glGetUniformLocationARB(s[3].program, "ms[0]");
		glUniform4fARB(location, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, 25.6);

		//������ ��� ��� ��������� �������� �� ����� (�������)
		std::string strtmp1;
		int i, inmans = 0;
		for (0; count + inmans <= WispList.size(); count++) {
			i = count - 1 + inmans;
			if (WispList[i].inman) {
				inmans++;
				count--;
				continue;
			}

			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "light_pos[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].pos[0], WispList[i].pos[1], WispList[i].pos[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "Ia[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "Id[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "Is[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "ma[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].ma[0], WispList[i].ma[1], WispList[i].ma[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "md[") += std::to_string(count) += "]").c_str());
			glUniform3fARB(location, WispList[i].md[0], WispList[i].md[1], WispList[i].md[2]);
			location = glGetUniformLocationARB(s[3].program, ((strtmp1 = "ms[") += std::to_string(count) += "]").c_str());
			glUniform4fARB(location, WispList[i].ms[0], WispList[i].ms[1], WispList[i].ms[2], WispList[i].ms[3]);

		}
		location = glGetUniformLocationARB(s[3].program, "count");
		glUniform1iARB(location, count);


		for (int i = -200; i <= 200; i++)
		{
			glBegin(GL_QUAD_STRIP);
			glNormal3d(-1, 0, 0);
			for (int g = 0; g <= ROOM2_WALLZ; g++)
			{
				glTexCoord2f(i * 0.1, g * 0.1);
				glVertex3d(200, i, g);
				glTexCoord2f((i + 1) * 0.1, g * 0.1);
				glVertex3d(200, i + 1, g);
			}
			glEnd();
		}
	}
	POP;
	PUSH;// -------------------------------------------------------------------------------- ������ �����


	//��������� ���������
	GLfloat amb[] = { 0.2, 0.2, 0.2, 1. };
	GLfloat dif[] = { 0.8, 0.8, 0.8, 1. };
	GLfloat spec[] = { 0.1, 0.1, 0.1, 1. };
	GLfloat sh = 0.1f * 128;

	//�������
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//��������
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//����������
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//������ �����
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//===================================
	//������� ��� 
	PUSH;
	Shader::DontUseShaders();

	//s[1].UseShader();
	//int l = glGetUniformLocationARB(s[1].program, "tex");
	//glUniform1iARB(l, 0);
	//glActiveTexture(GL_TEXTURE0);
	//Vadim.Draw();

	Shader::DontUseShaders();

	POP;

	POP;// ---------------------------------------------------------------------------- ��������� �������� �����

	Shader::DontUseShaders();


}   //����� ���� �������


bool gui_init = false;

//������ ���������, ��������� ����� �������� �������
void RenderGUI(OpenGL* ogl)
{

	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);


	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE0);
	if (showHelps)
		rec.Draw();



	Shader::DontUseShaders();




}

void resizeEvent(OpenGL* ogl, int newW, int newH)
{

	rec.setPosition(10, newH - 200 - 10);
}

inline void Wisp::Draw() {

	PUSH;
	glTranslated(pos[0], pos[1], pos[2]);
	//
	s[8].UseShader();
	//������ � ���� ����
	int location = glGetUniformLocationARB(s[8].program, "ColorRGB");
	glUniform3fARB(location, color[0], color[1], color[2]);
	//
	Wisp1.DrawObj();
	POP;
}

void Man::Draw() {

	PUSH;
	glTranslated(pos[0], pos[1], pos[2]);
	glRotated(rotZdeg, 0, 0, 1);
	manTex.bindTexture();
	//
	s[7].UseShader();
	//������ � ���� ������ � ��������
	int location = glGetUniformLocationARB(s[7].program, "camera");
	if (CameraaMode == 0)
		glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());
	if (CameraaMode == 1 || CameraaMode == 2)
		glUniform3fARB(location, (Vadim.pos[0]) * MANSCALEONSCENE + camera.pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + camera.pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + camera.pos.Z());
	location = glGetUniformLocationARB(s[7].program, "tex");
	glUniform1iARB(location, 0);
	location = glGetUniformLocationARB(s[7].program, "PosOffset");
	glUniform3fARB(location, pos[0], pos[1], pos[2] + (CameraaMode == 0 ? -17 : 0));
	location = glGetUniformLocationARB(s[7].program, "RotZOffset");
	glUniform1fARB(location, rotZdeg * PI / 180);

	//������ ��� ��� ������ ����������
	int count = 1;//���-�� ������������
	location = glGetUniformLocationARB(s[7].program, "light_pos[0]");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());
	location = glGetUniformLocationARB(s[7].program, "Ia[0]");
	glUniform3fARB(location, 0.2, 0.2, 0.2);
	location = glGetUniformLocationARB(s[7].program, "Id[0]");
	glUniform3fARB(location, 1.0, 1.0, 1.0);
	location = glGetUniformLocationARB(s[7].program, "Is[0]");
	glUniform3fARB(location, .7, .7, .7);
	location = glGetUniformLocationARB(s[7].program, "ma[0]");
	glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
	location = glGetUniformLocationARB(s[7].program, "md[0]");
	glUniform3fARB(location, LIGHTLEVEL, LIGHTLEVEL, LIGHTLEVEL);
	location = glGetUniformLocationARB(s[7].program, "ms[0]");
	glUniform4fARB(location, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, LIGHTLEVEL * 0.9, 25.6);

	//������ ��� ��� ��������� �������� �� ����� (�������)
	std::string strtmp1;
	int i, inmans = 0;
	for (0; count + inmans <= WispList.size(); count++) {
		i = count - 1 + inmans;
		if (WispList[i].inman) {
			inmans++;
			count--;
			continue;
		}

		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "light_pos[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].pos[0], WispList[i].pos[1], WispList[i].pos[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "Ia[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "Id[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "Is[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].color[0], WispList[i].color[1], WispList[i].color[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "ma[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].ma[0], WispList[i].ma[1], WispList[i].ma[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "md[") += std::to_string(count) += "]").c_str());
		glUniform3fARB(location, WispList[i].md[0], WispList[i].md[1], WispList[i].md[2]);
		location = glGetUniformLocationARB(s[7].program, ((strtmp1 = "ms[") += std::to_string(count) += "]").c_str());
		glUniform4fARB(location, WispList[i].ms[0], WispList[i].ms[1], WispList[i].ms[2], WispList[i].ms[3]);

	}
	location = glGetUniformLocationARB(s[7].program, "count");
	glUniform1iARB(location, count);
	//
	if (stay)
		man_Stay.DrawObj();//man_Stay.DrawObj();
	else
		if (running)
			if (stepRight)
				man_stepRight.DrawObj();
			else
				man_stepLeft.DrawObj();
		else
			if (stepRight)
				man_shortstepRight.DrawObj();
			else
				man_shortstepLeft.DrawObj();
	//������ ����� � �������
	Shader::DontUseShaders();

	glTranslated(-7.25, 0, 18);

	for (int p = 0; p < Vadim.PickedUpWisps.size(); p++) {
		DrawBox(WispList[Vadim.PickedUpWisps[p]].color[0], WispList[Vadim.PickedUpWisps[p]].color[1], WispList[Vadim.PickedUpWisps[p]].color[2]);
		glTranslated(1.5, 0, 0);
	}

	POP;
}


#pragma region �����


//glActiveTexture(GL_TEXTURE0);
//loadModel("models\\monik2\\monik2.obj", &monik);
//monikTex.loadTextureFromFile("models\\monik2\\t.bmp");

//loadModel("models\\m\\m2.obj", &m);
//mTex.loadTextureFromFile("models\\m\\m.bmp");

//loadModel("models\\monkey.obj_m", &monkey);
//monkeyTex.loadTextureFromFile("textures//tex.bmp");
//monkeyTex.bindTexture();


//frac.VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
//frac.FshaderFileName = "shaders\\frac.frag"; //��� ����� ������������ �������
//frac.LoadShaderFromFile(); //��������� ������� �� �����
//frac.Compile(); //�����������

//cassini.VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
//cassini.FshaderFileName = "shaders\\cassini.frag"; //��� ����� ������������ �������
//cassini.LoadShaderFromFile(); //��������� ������� �� �����
//cassini.Compile(); //�����������

//s[0].VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
//s[0].FshaderFileName = "shaders\\light.frag"; //��� ����� ������������ �������
//s[0].LoadShaderFromFile(); //��������� ������� �� �����
//s[0].Compile(); //�����������

//s[1].VshaderFileName = "shaders\\v.vert"; //��� ����� ���������� ������
//s[1].FshaderFileName = "shaders\\textureShader.frag"; //��� ����� ������������ �������
//s[1].LoadShaderFromFile(); //��������� ������� �� �����
//s[1].Compile(); //�����������

//double* VectToMass(Vector3 vector) {
//	double mass[3];
//	mass[0] = vector.X();
//	mass[1] = vector.Y();
//	mass[2] = vector.Z();
//	return mass;
//}


	//s[1].UseShader();
	/*
	//�������� ���������� � ������.  ��� ���� - ���� ����� uniform ���������� �� �� �����.
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//��� 2 - �������� �� ��������
	glUniform3fARB(location, light.pos.X(), light.pos.Y(),light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .7, .7, .7);


	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.8, 0.8, 0.8);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.8, 0.8, 0.8, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());



	PUSH;
	//glTranslated(5,5,0);
	//glRotated(90, 1, 0, 0);
	monikTex.bindTexture();
	monik.DrawObj();

	POP;

	PUSH;
	glTranslated(0,0,-10);
	glRotated(90, 1, 0, 0);
	mTex.bindTexture();
	m.DrawObj();

	POP;
	*/
	/*
	//������ ��������
	objModel.DrawObj();


	Shader::DontUseShaders();

	//������, ��� ��������
	glPushMatrix();
		glTranslated(-5,15,0);
		//glScaled(-1.0,1.0,1.0);
		objModel.DrawObj();
	glPopMatrix();



	//��������

	s[1].UseShader();
	int l = glGetUniformLocationARB(s[1].program, "tex");
	glUniform1iARB(l, 0);     //��� ��� ����� �� ��������� �������� ������� �� GL_TEXTURE0
	glPushMatrix();
	glRotated(-90, 0, 0, 1);
	monkeyTex.bindTexture();
	monkey.DrawObj();
	glPopMatrix();
	*/



	//
	//void DrawQuad()
	//{
	//	double A[] = { 0,0 };
	//	double B[] = { 1,0 };
	//	double C[] = { 1,1 };
	//	double D[] = { 0,1 };
	//	glBegin(GL_QUADS);
	//	glColor3d(.5, 0, 0);
	//	glNormal3d(0, 0, 1);
	//	glTexCoord2d(0, 0);
	//	glVertex2dv(A);
	//	glTexCoord2d(1, 0);
	//	glVertex2dv(B);
	//	glTexCoord2d(1, 1);
	//	glVertex2dv(C);
	//	glTexCoord2d(0, 1);
	//	glVertex2dv(D);
	//	glEnd();
	//}

	//������������ ����� ����� ��������

	//class a {
	//public:
	//	float f = 0;
	//	bool tuda = true;
	//	float h = 0.275; //-------���� ���� ��������
	//	void operator ++() {
	//
	//		if (tuda)
	//		{
	//			if (f <= 1.0001)
	//			{
	//				f += h;
	//			}
	//			else
	//			{
	//				f -= h;
	//				if (f < 0)
	//					f = 0;
	//				tuda = false;
	//			}
	//		}
	//		else
	//		{
	//			if (f > 0)
	//			{
	//				f -= h;
	//				if (f < 0)
	//					f = 0;
	//			}
	//			else
	//			{
	//				f += h;
	//				tuda = true;
	//			}
	//		}
	//	}
	//};

	//////��������� ��������

	/*
	{

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		frac.UseShader();

		int location = glGetUniformLocationARB(frac.program, "size");
		glUniform2fARB(location, (GLfloat)ogl->getWidth(), (GLfloat)ogl->getHeight());

		location = glGetUniformLocationARB(frac.program, "uOffset");
		glUniform2fARB(location, offsetX, offsetY);

		location = glGetUniformLocationARB(frac.program, "uZoom");
		glUniform1fARB(location, zoom);

		location = glGetUniformLocationARB(frac.program, "Time");
		glUniform1fARB(location, Time);

		DrawQuad();

	}
	*/

	//////���� �������

	/*
	{

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		cassini.UseShader();

		int location = glGetUniformLocationARB(cassini.program, "size");
		glUniform2fARB(location, (GLfloat)ogl->getWidth(), (GLfloat)ogl->getHeight());


		location = glGetUniformLocationARB(cassini.program, "Time");
		glUniform1fARB(location, Time);

		DrawQuad();
	}

	*/


	//�����:
	/*
	class Butterfly {
	public:
		a animationTick;
		double pos[3];
		double move[3];
		double rotXdeg;
		double rotZdeg;

		void RandomizeAnimationTick() {
			animationTick.f = GetRandomDouble(0, 1);
		}

		Butterfly() {
			pos[0] = 0;
			pos[1] = 0;
			pos[2] = 0;
			move[0] = 0;
			move[1] = 0;
			move[2] = 0;
			rotXdeg = 0;
			rotZdeg = 0;
			RandomizeAnimationTick();
		}

		Butterfly(Vector3 position) {
			pos[0] = position.X();
			pos[1] = position.Y();
			pos[2] = position.Z();
			move[0] = 0;
			move[1] = 0;
			move[2] = 0;
			rotXdeg = 0;
			rotZdeg = 0;
			RandomizeAnimationTick();
		}

		Butterfly(const double position[3]) {
			pos[0] = position[0];
			pos[1] = position[1];
			pos[2] = position[2];
			move[0] = 0;
			move[1] = 0;
			move[2] = 0;
			rotXdeg = 0;
			rotZdeg = 0;
			RandomizeAnimationTick();
		}

		void SetPosition(const double position[3]) {
			pos[0] = position[0];
			pos[1] = position[1];
			pos[2] = position[2];
		}

		void draw() {
			PUSH;
			glTranslated(pos[0], pos[1], pos[2]);
			glRotated(rotXdeg, 1, 0, 0);
			glRotated(rotZdeg, 0, 0, 1);


			double r;
			double Tushka_NePopa[] = { 0,0,4.5 };
			double Tushka_NePopa_h[] = { 2.5,0,2.5 };
			double Tushka_Popa_h[] = { 2,0,-4 };
			double Tushka_Popa[] = { 0,0,-5 };

	#pragma region ������ �����
			//������ �����
			glPushMatrix();
			{
				glColor3b(97, 67, 37);
				double tmp[3];
				double fat = 0.77, r, r2, z2, h_r = (Tushka_NePopa[2] - Tushka_Popa[2]) / 2, dz = h_r / 100;
				double Head_Center[] = { 0,0,Tushka_NePopa[2] - h_r };
				glTranslated(Head_Center[0], Head_Center[1], Head_Center[2]);
				for (double z = -h_r; z <= h_r; z += dz)
				{
					glBegin(GL_TRIANGLE_STRIP);
					if (z > h_r)
						z = h_r;

					tmp[2] = z;
					r = sqrt((1 - ((z * z) / (h_r * h_r))) / (fat * fat));


					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						//1
						if (z == -h_r)
						{
							glNormal3d(0, 0, -h_r);
							glVertex3d(0, 0, -h_r);
						}
						else
						{
							tmp[0] = r * cos(t);
							tmp[1] = r * sin(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
						//2
						z2 = z + dz;
						r2 = sqrt((1 - ((z2 * z2) / (h_r * h_r))) / (fat * fat));
						if (z2 >= h_r)
						{
							glNormal3d(0, 0, h_r);
							glVertex3d(0, 0, h_r);
						}
						else
						{
							tmp[2] = z2;
							tmp[0] = r2 * sin(t);
							tmp[1] = r2 * cos(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
					}
					//
					glEnd();

				}
			}
			glPopMatrix();
	#pragma endregion

	#pragma region ������ ������
			//������ ������
			glPushMatrix();
			{
				glColor3ub(77, 130, 37);
				double tmp[3];
				double r, r2, z2, h_r = 1.2, dz = h_r / 100;
				double Head_Center[] = { Tushka_NePopa[0],Tushka_NePopa[1],Tushka_NePopa[2] + h_r * 0.45 };
				glTranslated(Head_Center[0], Head_Center[1], Head_Center[2]);
				for (double z = -h_r; z <= h_r; z += dz)
				{
					glBegin(GL_TRIANGLE_STRIP);
					if (z > h_r)
						z = h_r;

					tmp[2] = z;
					r = sqrt(h_r * h_r - z * z);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						//1
						if (z == -h_r)
						{
							glNormal3d(0, 0, -h_r);
							glVertex3d(0, 0, -h_r);
						}
						else
						{
							tmp[0] = r * cos(t);
							tmp[1] = r * sin(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
						//2
						z2 = z + dz;
						r2 = sqrt(h_r * h_r - z2 * z2);
						if (z2 >= h_r)
						{
							glNormal3d(0, 0, h_r);
							glVertex3d(0, 0, h_r);
						}
						else
						{
							tmp[2] = z2;
							tmp[0] = r2 * sin(t);
							tmp[1] = r2 * cos(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
					}

					glEnd();

				}
			}
			glPopMatrix();
			//.
	#pragma endregion

	#pragma region ����� 1
	//����� 1
			glPushMatrix();
			glRotated(5.1, 0, 0, 1);
			glRotatef(78.6 * animationTick.f, 0, 0, 1);
			{
				double wing_Up_h1[] = { 5,0,19 };
				double wing_Up_h2[] = { 12,0,5 };
				double wing_Down_h3[] = { 14.1,0,-3.6 };
				double wing_Down_h4[] = { 4.5,0,-15 };

				double wCircleUp[] = { 5.028,0,6 };
				double wCircleDown[] = { 5.169,0,-4.399 };

				double wingUp[] = { 0.66,0,3.9 };
				double wingFar[] = { 5.7,0,0.3 };
				double wingCenter[] = { 0.9,0,0 };
				double wingDown[] = { 0.75,0,-3.9 };
				glColor4ub(100, 150, 180, 177);
				glNormal3d(0, -1, 0);

				glBegin(GL_QUADS);
				glVertex3d(0, 0, wingUp[2]);
				glVertex3dv(wingUp);
				glVertex3dv(wingDown);
				glVertex3d(0, 0, wingDown[2]);
				glEnd();

				double tmp[3];
				short int R = 223;
				short int G = 115;
				short int B = 255;
				int i = 0;
				glBegin(GL_TRIANGLE_FAN);
				glVertex3dv(wingCenter);
				for (double t = 0; t <= 1.0001; t += 0.01, i++)
				{
					tmp[0] = f(wingUp[0], wing_Up_h1[0], wing_Up_h2[0], wingFar[0], t);
					tmp[1] = f(wingUp[1], wing_Up_h1[1], wing_Up_h2[1], wingFar[1], t);
					tmp[2] = f(wingUp[2], wing_Up_h1[2], wing_Up_h2[2], wingFar[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);
					R--;
					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 4;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 9;
						R -= 6;
						G -= 3;
					}
				}

				for (double t = 0; t <= 1.0001; t += 0.01, i--)
				{
					tmp[0] = f(wingFar[0], wing_Down_h3[0], wing_Down_h4[0], wingDown[0], t);
					tmp[1] = f(wingFar[1], wing_Down_h3[1], wing_Down_h4[1], wingDown[1], t);
					tmp[2] = f(wingFar[2], wing_Down_h3[2], wing_Down_h4[2], wingDown[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);

					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 1;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 3;
						R -= 2;
						G -= 1;
					}
				}
				glEnd();

				glPushMatrix();
				glTranslated(wCircleUp[0], wCircleUp[1], wCircleUp[2]);
				r = 1.2;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(130, 10, 10, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(210, 200, 1, 210);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
				glPushMatrix();
				glTranslated(wCircleDown[0], wCircleDown[1], wCircleDown[2]);
				r = 1.7;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(230, 100, 1, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(130, 10, 10, 210);
					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 314)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
			}
			glPopMatrix();
			//.
	#pragma endregion

	#pragma region ����� 2
	//����� 2
			glPushMatrix();
			glRotated(5.1, 0, 0, -1);
			glRotatef(78.6 * animationTick.f, 0, 0, -1);
			{
				double wing_Up_h1[] = { -5,0,19 };
				double wing_Up_h2[] = { -12,0,5 };
				double wing_Down_h3[] = { -14.1,0,-3.6 };
				double wing_Down_h4[] = { -4.5,0,-15 };

				double wCircleUp[] = { -5.028,0,6 };
				double wCircleDown[] = { -5.169,0,-4.399 };

				double wingUp[] = { -0.66,0,3.9 };
				double wingFar[] = { -5.7,0,0.3 };
				double wingCenter[] = { -0.9,0,0 };
				double wingDown[] = { -0.75,0,-3.9 };
				glColor4ub(100, 150, 180, 177);
				glNormal3d(0, -1, 0);

				glBegin(GL_QUADS);
				glVertex3d(0, 0, wingUp[2]);
				glVertex3dv(wingUp);
				glVertex3dv(wingDown);
				glVertex3d(0, 0, wingDown[2]);
				glEnd();

				double tmp[3];
				short int R = 223;
				short int G = 115;
				short int B = 255;
				int i = 0;
				glBegin(GL_TRIANGLE_FAN);
				glVertex3dv(wingCenter);
				for (double t = 0; t <= 1.0001; t += 0.01, i++)
				{
					tmp[0] = f(wingUp[0], wing_Up_h1[0], wing_Up_h2[0], wingFar[0], t);
					tmp[1] = f(wingUp[1], wing_Up_h1[1], wing_Up_h2[1], wingFar[1], t);
					tmp[2] = f(wingUp[2], wing_Up_h1[2], wing_Up_h2[2], wingFar[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);
					R--;
					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 4;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 9;
						R -= 6;
						G -= 3;
					}
				}

				for (double t = 0; t <= 1.0001; t += 0.01, i--)
				{
					tmp[0] = f(wingFar[0], wing_Down_h3[0], wing_Down_h4[0], wingDown[0], t);
					tmp[1] = f(wingFar[1], wing_Down_h3[1], wing_Down_h4[1], wingDown[1], t);
					tmp[2] = f(wingFar[2], wing_Down_h3[2], wing_Down_h4[2], wingDown[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);

					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 1;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 3;
						R -= 2;
						G -= 1;
					}
				}
				glEnd();

				glPushMatrix();
				glTranslated(wCircleUp[0], wCircleUp[1], wCircleUp[2]);
				r = 1.2;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(130, 10, 10, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(210, 200, 1, 210);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
				glPushMatrix();
				glTranslated(wCircleDown[0], wCircleDown[1], wCircleDown[2]);
				r = 1.7;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(230, 100, 1, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(130, 10, 10, 210);
					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 314)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();

			}
			glPopMatrix();
			//.
	#pragma endregion
			POP;
		}

		void DrawLow(Vector3 scale = Vector3(0.1, 0.1, 0.1)) {
			PUSH;
			glTranslated(pos[0], pos[1], pos[2]);
			glScaled(scale.X(), scale.Y(), scale.Z());
			glRotated(rotXdeg, 1, 0, 0);
			glRotated(rotZdeg, 0, 0, 1);




			double r;
			double Tushka_NePopa[] = { 0,0,4.5 };
			double Tushka_NePopa_h[] = { 2.5,0,2.5 };
			double Tushka_Popa_h[] = { 2,0,-4 };
			double Tushka_Popa[] = { 0,0,-5 };

	#pragma region ������ �����
			//������ �����
			glPushMatrix();
			{
				glColor3b(97, 67, 37);
				double tmp[3];
				double fat = 0.77, r, r2, z2, h_r = (Tushka_NePopa[2] - Tushka_Popa[2]) / 2, dz = h_r / 100;
				double Head_Center[] = { 0,0,Tushka_NePopa[2] - h_r };
				glTranslated(Head_Center[0], Head_Center[1], Head_Center[2]);
				for (double z = -h_r; z <= h_r; z += dz)
				{
					glBegin(GL_TRIANGLE_STRIP);
					if (z > h_r)
						z = h_r;

					tmp[2] = z;
					r = sqrt((1 - ((z * z) / (h_r * h_r))) / (fat * fat));


					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						//1
						if (z == -h_r)
						{
							glNormal3d(0, 0, -h_r);
							glVertex3d(0, 0, -h_r);
						}
						else
						{
							tmp[0] = r * cos(t);
							tmp[1] = r * sin(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
						//2
						z2 = z + dz;
						r2 = sqrt((1 - ((z2 * z2) / (h_r * h_r))) / (fat * fat));
						if (z2 >= h_r)
						{
							glNormal3d(0, 0, h_r);
							glVertex3d(0, 0, h_r);
						}
						else
						{
							tmp[2] = z2;
							tmp[0] = r2 * sin(t);
							tmp[1] = r2 * cos(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
					}
					//
					glEnd();

				}
			}
			glPopMatrix();
	#pragma endregion

	#pragma region ������ ������
			//������ ������
			glPushMatrix();
			{
				glColor3ub(77, 130, 37);
				double tmp[3];
				double r, r2, z2, h_r = 1.2, dz = h_r / 10;
				double Head_Center[] = { Tushka_NePopa[0],Tushka_NePopa[1],Tushka_NePopa[2] + h_r * 0.45 };
				glTranslated(Head_Center[0], Head_Center[1], Head_Center[2]);
				for (double z = -h_r; z <= h_r; z += dz)
				{
					glBegin(GL_TRIANGLE_STRIP);
					if (z > h_r)
						z = h_r;

					tmp[2] = z;
					r = sqrt(h_r * h_r - z * z);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 10)
					{
						//1
						if (z == -h_r)
						{
							glNormal3d(0, 0, -h_r);
							glVertex3d(0, 0, -h_r);
						}
						else
						{
							tmp[0] = r * cos(t);
							tmp[1] = r * sin(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
						//2
						z2 = z + dz;
						r2 = sqrt(h_r * h_r - z2 * z2);
						if (z2 >= h_r)
						{
							glNormal3d(0, 0, h_r);
							glVertex3d(0, 0, h_r);
						}
						else
						{
							tmp[2] = z2;
							tmp[0] = r2 * sin(t);
							tmp[1] = r2 * cos(t);
							glNormal3dv(tmp);
							glVertex3dv(tmp);
						}
					}

					glEnd();

				}
			}
			glPopMatrix();
			//.
	#pragma endregion

	#pragma region ����� 1
	//����� 1
			glPushMatrix();
			glRotated(5.1, 0, 0, 1);
			glRotatef(78.6 * animationTick.f, 0, 0, 1);
			{
				double wing_Up_h1[] = { 5,0,19 };
				double wing_Up_h2[] = { 12,0,5 };
				double wing_Down_h3[] = { 14.1,0,-3.6 };
				double wing_Down_h4[] = { 4.5,0,-15 };

				double wCircleUp[] = { 5.028,0,6 };
				double wCircleDown[] = { 5.169,0,-4.399 };

				double wingUp[] = { 0.66,0,3.9 };
				double wingFar[] = { 5.7,0,0.3 };
				double wingCenter[] = { 0.9,0,0 };
				double wingDown[] = { 0.75,0,-3.9 };
				glColor4ub(100, 150, 180, 177);
				glNormal3d(0, -1, 0);

				glBegin(GL_QUADS);
				glVertex3d(0, 0, wingUp[2]);
				glVertex3dv(wingUp);
				glVertex3dv(wingDown);
				glVertex3d(0, 0, wingDown[2]);
				glEnd();

				double tmp[3];
				short int R = 223;
				short int G = 115;
				short int B = 255;
				int i = 0;
				glBegin(GL_TRIANGLE_FAN);
				glVertex3dv(wingCenter);
				for (double t = 0; t <= 1.0001; t += 0.01, i++)
				{
					tmp[0] = f(wingUp[0], wing_Up_h1[0], wing_Up_h2[0], wingFar[0], t);
					tmp[1] = f(wingUp[1], wing_Up_h1[1], wing_Up_h2[1], wingFar[1], t);
					tmp[2] = f(wingUp[2], wing_Up_h1[2], wing_Up_h2[2], wingFar[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);
					R--;
					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 4;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 9;
						R -= 6;
						G -= 3;
					}
				}

				for (double t = 0; t <= 1.0001; t += 0.01, i--)
				{
					tmp[0] = f(wingFar[0], wing_Down_h3[0], wing_Down_h4[0], wingDown[0], t);
					tmp[1] = f(wingFar[1], wing_Down_h3[1], wing_Down_h4[1], wingDown[1], t);
					tmp[2] = f(wingFar[2], wing_Down_h3[2], wing_Down_h4[2], wingDown[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);

					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 1;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 3;
						R -= 2;
						G -= 1;
					}
				}
				glEnd();

				glPushMatrix();
				glTranslated(wCircleUp[0], wCircleUp[1], wCircleUp[2]);
				r = 1.2;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(130, 10, 10, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(210, 200, 1, 210);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
				glPushMatrix();
				glTranslated(wCircleDown[0], wCircleDown[1], wCircleDown[2]);
				r = 1.7;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(230, 100, 1, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(130, 10, 10, 210);
					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 314)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
			}
			glPopMatrix();
			//.
	#pragma endregion

	#pragma region ����� 2
	//����� 2
			glPushMatrix();
			glRotated(5.1, 0, 0, -1);
			glRotatef(78.6 * animationTick.f, 0, 0, -1);
			{
				double wing_Up_h1[] = { -5,0,19 };
				double wing_Up_h2[] = { -12,0,5 };
				double wing_Down_h3[] = { -14.1,0,-3.6 };
				double wing_Down_h4[] = { -4.5,0,-15 };

				double wCircleUp[] = { -5.028,0,6 };
				double wCircleDown[] = { -5.169,0,-4.399 };

				double wingUp[] = { -0.66,0,3.9 };
				double wingFar[] = { -5.7,0,0.3 };
				double wingCenter[] = { -0.9,0,0 };
				double wingDown[] = { -0.75,0,-3.9 };
				glColor4ub(100, 150, 180, 177);
				glNormal3d(0, -1, 0);

				glBegin(GL_QUADS);
				glVertex3d(0, 0, wingUp[2]);
				glVertex3dv(wingUp);
				glVertex3dv(wingDown);
				glVertex3d(0, 0, wingDown[2]);
				glEnd();

				double tmp[3];
				short int R = 223;
				short int G = 115;
				short int B = 255;
				int i = 0;
				glBegin(GL_TRIANGLE_FAN);
				glVertex3dv(wingCenter);
				for (double t = 0; t <= 1.0001; t += 0.01, i++)
				{
					tmp[0] = f(wingUp[0], wing_Up_h1[0], wing_Up_h2[0], wingFar[0], t);
					tmp[1] = f(wingUp[1], wing_Up_h1[1], wing_Up_h2[1], wingFar[1], t);
					tmp[2] = f(wingUp[2], wing_Up_h1[2], wing_Up_h2[2], wingFar[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);
					R--;
					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 4;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 9;
						R -= 6;
						G -= 3;
					}
				}

				for (double t = 0; t <= 1.0001; t += 0.01, i--)
				{
					tmp[0] = f(wingFar[0], wing_Down_h3[0], wing_Down_h4[0], wingDown[0], t);
					tmp[1] = f(wingFar[1], wing_Down_h3[1], wing_Down_h4[1], wingDown[1], t);
					tmp[2] = f(wingFar[2], wing_Down_h3[2], wing_Down_h4[2], wingDown[2], t);
					glColor4ub(R, G, B, 177);
					glVertex3dv(tmp);

					if (i % 2 == 0)
						G++;
					if (i % 4 == 0)
						B--;
					if (i > 26 && i < 39)
					{
						R--;
						G += 3;
					}
					if (i > 39 && i < 70)
					{
						G -= 2;
						B -= 1;
					}
					if (i > 70 && i < 90)
					{
						B += 2;
						R += 3;
					}
					if (i > 90)
					{
						B -= 3;
						R -= 2;
						G -= 1;
					}
				}
				glEnd();

				glPushMatrix();
				glTranslated(wCircleUp[0], wCircleUp[1], wCircleUp[2]);
				r = 1.2;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(130, 10, 10, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(210, 200, 1, 210);

					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 100)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();
				glPushMatrix();
				glTranslated(wCircleDown[0], wCircleDown[1], wCircleDown[2]);
				r = 1.7;
				i = 0;
				tmp[1] = 0.005;
				while (i <= 1) {
					glColor4ub(230, 100, 1, 210);
					glBegin(GL_TRIANGLE_FAN);
					glVertex3d(0, tmp[1], 0);
					glColor4ub(130, 10, 10, 210);
					for (double t = 0; t <= 2 * PI + 0.0001; t += PI / 314)
					{
						tmp[0] = r * cos(t);
						tmp[2] = r * sin(t);
						glVertex3dv(tmp);
					}

					glEnd();

					tmp[1] = -0.005;
					i++;
				}
				glPopMatrix();

			}
			glPopMatrix();
			//.
	#pragma endregion
			POP;
		}

	};

	class ButterflyList {
	public:
		Butterfly all[128];
		short NowSpawned;

		ButterflyList() {
			NowSpawned = 0;
		}

		void Draw() {
			for (int i = 0; i < NowSpawned; i++)
			{
				all[i].DrawLow();
			}
		}

		void Spawn(const double position[3]) {
			NowSpawned++;
			all[NowSpawned - 1].SetPosition(position);
		}

		void Spawn(Vector3 positionv) {
			double position[3];
			position[0] = positionv.X();
			position[1] = positionv.Y();
			position[2] = positionv.Z();

			NowSpawned++;
			all[NowSpawned - 1].SetPosition(position);
		}

		void Spawn() {
			NowSpawned++;
		}

		void Tick() {
			for (int i = 0; i < NowSpawned; i++)
				++all[i].animationTick;
		}

	};
	ButterflyList Butters;
	Butterfly Alice;
	*/
#pragma endregion

