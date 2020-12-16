#include "Render.h"

#include <windows.h>

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

Боксы - отрисовка (мб классы выкинуть в файлы)

Боксы - ловить и выпускать.

Шейдеры




*/


#define POP glPopMatrix()
#define PUSH glPushMatrix()
#define TICKMS 50
#define TICKS 0.05
#define MANSCALEONSCENE 1
#define AMASSBOXRAD 0.5

void DrawBox(float, float, float);

double GetRandomDouble(long min = 0, long max = 100)
{
	min *= 1000;
	max *= 1000;
	double r = (double)(rand() % max + min) / 1000;
	return r;
}

double* VectToMass(Vector3 vector) {
	double mass[3];
	mass[0] = vector.X();
	mass[1] = vector.Y();
	mass[2] = vector.Z();
	return mass;
}

inline double f(double p1, double p2, double p3, double p4, double t)
{
	return p1 * (1 - t) * (1 - t) * (1 - t) + 3 * p2 * t * (1 - t) * (1 - t) + 3 * p3 * t * t * (1 - t) + p4 * t * t * t;
}


bool ispaused = false;
short CameraaMode = 0, CameraMode_b = 0;

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;

ObjFile* model;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;

ObjFile objModel, monkey, monik, m, man_stepLeft, man_shortstepLeft, man_stepRight, man_shortstepRight, man_Stay;
Texture monkeyTex, monikTex, mTex, manTex, grassTex;

//старые координаты мыши
int mouseX = 0, mouseY = 0;

float offsetX = 0, offsetY = 0;
float zoom = 10000;
long Time = 0;
int tick_o = 0;
int tick_n = 0;

//Настроечки
double ACCELMUL = 1;
double SPEEDMUL = 10;
double GRAVITY = -9.8;
double RUNNIGMUL = 1.5;
short MAXVADIMAMASS = 10;



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
		amass = 0;
		rotZdeg = 0;
		moveMode = 0;
		stay = true;
		stepRight = true;
		startJumping = false;
		changesteps = 0;
		running = false;
		mass = 55;
		MovementSpeed = 5;
		JumpSpeed = 5.2;
		speedv[0] = 0;
		speedv[1] = 0;
		speedv[2] = 0;
		accelv[0] = 0;
		accelv[1] = 0;
		accelv[2] = 0;
	}

	void AmassUp(const bool isplus = false) {
		if ((!isplus) && amass == 0)
			throw "Некаво адать.";
		if ((isplus) && amass == MAXVADIMAMASS)
			throw "Каво? Слишком тяжело, пеп, не грузите Вадимку";
		isplus ? amass++ : amass--;
		JumpSpeed += isplus ? 0.3 : -0.3;
		switch (amass)
		{
		case 0:
			MovementSpeed = 5;
			break;
		case 1:
			MovementSpeed = 4.8;
			break;
		case 2:
			MovementSpeed = 4.5;
			break;
		case 3:
			MovementSpeed = 4.25;
			break;
		case 4:
			MovementSpeed = 4;
			break;
		case 5:
			MovementSpeed = 3.67;
			break;
		case 6:
			MovementSpeed = 3.33;
			break;
		case 7:
			MovementSpeed = 3;
			break;
		case 8:
			MovementSpeed = 2.5;
			break;
		case 9:
			MovementSpeed = 2;
			break;
		case 10:
			MovementSpeed = 1;
			break;
		}
	}

	void CalculateAccel() {
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

	void CalculateSpeed() {
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

	void CalculatePosition() {
		pos[0] = pos[0] + speedv[0] * TICKS;
		pos[1] = pos[1] + speedv[1] * TICKS;
		(pos[2] = pos[2] + speedv[2] * TICKS) < 0 ? pos[2] = 0 : 0;

	}

	void AnimationTick() {

		if (moveMode == 0)
			stay = true;
		else {
			stay = false;
			if (moveMode == 2)
				running = true;
			else
				running = false;
			//if (changesteps++ >= (running ? 1 : 0)) {
				stepRight = !stepRight;
			//	changesteps = 0;
			//}
		}
	}

	void Draw() {

		PUSH;
		glTranslated(pos[0], pos[1], pos[2]);
		glRotated(rotZdeg, 0, 0, 1);
		manTex.bindTexture();
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
		glTranslated(-5, 0, 18);
		DrawBox(0.2, 1, 0.1);
		POP;
	}


};
Man Vadim;



//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	double camDist0;
	double camDist12;
	//углы поворота камеры
	double fi1, fi2;
	Vector3 prepos;

	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 35;
		camDist0 = camDist;
		camDist12 = 17;
		fi1 = 1;
		fi2 = 1;
	}


	//считает позицию камеры, исходя из углов поворота, вызывается движком
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

		//

		//

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		if (CameraaMode == 1 || CameraaMode == 2) {
			gluLookAt((Vadim.pos[0]) * MANSCALEONSCENE + pos.X(), (Vadim.pos[1]) * MANSCALEONSCENE + pos.Y(), (Vadim.pos[2] + 17) * MANSCALEONSCENE + pos.Z(),
				Vadim.pos[0] * MANSCALEONSCENE, Vadim.pos[1] * MANSCALEONSCENE, (Vadim.pos[2] + 17) * MANSCALEONSCENE,
				normal.X(), normal.Y(), normal.Z());
		}
		if (CameraaMode == 0)
			gluLookAt(pos.X(), pos.Y(), pos.Z() + 17, lookPoint.X(), lookPoint.Y(), lookPoint.Z() + 17, normal.X(), normal.Y(), normal.Z());
	}



}  camera;    //создаем объект камеры

//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}


	//рисует сферу и линии под источником света, вызывается движком
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
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
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

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света


//обработчик движения мыши
void mouseEvent(OpenGL* ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
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



	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y, 92, ogl->aspect);

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

//обработчик вращения колеса  мыши
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

//обработчик нажатия кнопок клавиатуры

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
	//Начнём тик с обработки нажатых клавиш
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

	//Клавишы фсё, таво


	//Настраиваем параметры анимации
	//Vadim's
	Vadim.AnimationTick();

	//Проводим физические изменения сцены
	Vadim.CalculateAccel();
	Vadim.CalculateSpeed();
	Vadim.CalculatePosition();

	//Настраиваем камеру
	/*camera.prepos.setCoords(camera.pos.X(), camera.pos.Y(), camera.pos.Z());
	camera.pos.setCoords(camera.camDist * cos(camera.fi2) * cos(camera.fi1),
		camera.camDist * cos(camera.fi2) * sin(camera.fi1),
		camera.camDist * sin(camera.fi2));*/

	//кончаем его (Тик, не Вадима)
	CameraMode_b--;
	Time = 0;
}

void DrawBox(float r, float g, float b) {
	s[0].UseShader();

	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .7, .7, .7);

	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, r, g, b);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.8, 0.8, 0.8);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.8, 0.8, 0.8, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

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

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
	srand(1);

	camera.lookPoint = { camera.lookPoint.X(), camera.lookPoint.Y(), camera.lookPoint.Z() + 17 };
	//фсё






	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);

	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем


	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	s[5].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[5].FshaderFileName = "shaders\\LightTex.frag"; //имя файла фрагментного шейдера
	s[5].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[5].Compile(); //компилируем


	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	//loadModel("models\\lpgun6.obj_m", &objModel);

	glActiveTexture(GL_TEXTURE0);
	grassTex.loadTextureFromFile("textures\\grass.bmp");




	glActiveTexture(GL_TEXTURE0);
	//loadModel("models\\monik2\\monik2.obj", &monik);
	//monikTex.loadTextureFromFile("models\\monik2\\t.bmp");

	//loadModel("models\\m\\m2.obj", &m);
	//mTex.loadTextureFromFile("models\\m\\m.bmp");

	//loadModel("models\\monkey.obj_m", &monkey);
	//monkeyTex.loadTextureFromFile("textures//tex.bmp");
	//monkeyTex.bindTexture();

	loadModel("models\\man\\man_stepLeft.obj", &man_stepLeft);
	manTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_stepRight.obj", &man_stepRight);
	//man_stepRightTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_Stay.obj", &man_Stay);
	//man_StayTex.loadTextureFromFile("models\\man\\ManTexture.bmp");

	loadModel("models\\man\\man_shortstepLeft.obj", &man_shortstepLeft);
	loadModel("models\\man\\man_shortstepRight.obj", &man_shortstepRight);



	tick_n = GetTickCount64();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100 - 10);
	rec.setText("G - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали", 0, 0, 0);


}

void Render(OpenGL* ogl)
{
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

	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma region Пол
	glActiveTexture(GL_TEXTURE0);
	//настройка материала
	GLfloat pamb[] = { 0.3, 0.8, 0.2, 1. };
	GLfloat pdif[] = { 0.8, 0.8, 0.8, 1. };
	GLfloat pspec[] = { 0.1, 0.1, 0.1, 1. };
	GLfloat psh = 0.1f * 128;
	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, pamb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, pdif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, pspec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, psh);
	PUSH;

	/*s[0].UseShader();
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

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
	glUniform4fARB(location, 0.1, 0.1, 0.1, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());*/

	for (int i = -200; i <= 199; i++)
	{
		grassTex.bindTexture();
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
	POP;
	Shader::DontUseShaders();
#pragma endregion

	PUSH;// -------------------------------------------------------------------------------- Рисуем сцену


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.2, 1. };
	GLfloat dif[] = { 0.8, 0.8, 0.8, 1. };
	GLfloat spec[] = { 0.1, 0.1, 0.1, 1. };
	GLfloat sh = 0.1f * 128;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//===================================
	//Прогать тут 
	PUSH;



	//s[1].UseShader();
	//int l = glGetUniformLocationARB(s[1].program, "tex");
	//glUniform1iARB(l, 0);
	glActiveTexture(GL_TEXTURE0);
	Vadim.Draw();

	Shader::DontUseShaders();

	POP;

	POP;// ---------------------------------------------------------------------------- Закончили рисовать сцену


	//s[1].UseShader();
	/*
	//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени.
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//Шаг 2 - передаем ей значение
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
	//первый пистолет
	objModel.DrawObj();


	Shader::DontUseShaders();

	//второй, без шейдеров
	glPushMatrix();
		glTranslated(-5,15,0);
		//glScaled(-1.0,1.0,1.0);
		objModel.DrawObj();
	glPopMatrix();



	//обезьяна

	s[1].UseShader();
	int l = glGetUniformLocationARB(s[1].program, "tex");
	glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
	glPushMatrix();
	glRotated(-90, 0, 0, 1);
	monkeyTex.bindTexture();
	monkey.DrawObj();
	glPopMatrix();
	*/



	Shader::DontUseShaders();


}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
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
	rec.Draw();



	Shader::DontUseShaders();




}

void resizeEvent(OpenGL* ogl, int newW, int newH)
{

	rec.setPosition(10, newH - 100 - 10);
}

#pragma region Мусор

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

//Своеобразный класс тиков анимации

//class a {
//public:
//	float f = 0;
//	bool tuda = true;
//	float h = 0.275; //-------Енто типа скорость
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

//////Рисование фрактала

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

//////Овал Кассини

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


//Мусор:
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

#pragma region Рисуем тушку
		//Рисуем тушку
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

#pragma region Рисуем голову
		//Рисуем голову
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

#pragma region Крыло 1
//Крыло 1
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

#pragma region Крыло 2
//Крыло 2
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

#pragma region Рисуем тушку
		//Рисуем тушку
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

#pragma region Рисуем голову
		//Рисуем голову
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

#pragma region Крыло 1
//Крыло 1
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

#pragma region Крыло 2
//Крыло 2
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

