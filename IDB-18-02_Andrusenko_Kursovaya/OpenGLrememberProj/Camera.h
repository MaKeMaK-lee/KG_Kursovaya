#ifndef CAMERA_H
#define CAMERA_H

#include "Object.h"
#include "MyVector3d.h"
#include "Ray.h"


//��������� ��� ������� � ������
class Camera : public Object
{
public:

	Vector3 lookPoint;
	Vector3 normal;

	Camera()
	{

	}

	virtual void LookAt()
	{
		
	}

	virtual void SetUpCamera()
	{

	}

	Ray getLookRay(double camDist, double fi1, double fi2, int wndX, int wndY, double FOV, double aspect)
	{
		GLint    viewport[4];    // ��������� viewport-a.
		GLdouble projection[16]; // ������� ��������.
		GLdouble modelview[16];  // ������� �������.
//		GLdouble vx, vy, vz;       // ���������� ������� ���� � ������� ��������� viewport-a.
		GLdouble wx, wy, wz;       // ������������ ������� ����������.

		//����������� ������ �� ����������� ��� ���������� ������ ���������
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOV,aspect, .1, 100.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
			camDist * cos(fi2) * sin(fi1),
			camDist * sin(fi2));
		LookAt();

		glGetIntegerv(GL_VIEWPORT, viewport);           // ����� ��������� viewport-a.
		glGetDoublev(GL_PROJECTION_MATRIX, projection); // ����� ������� ��������.
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);   // ����� ������� �������.
		// ��������� ������� ���������� ������� � ������� ��������� viewport-a.
				 
		gluUnProject(wndX, wndY, 0, modelview, projection, viewport, &wx, &wy, &wz);
		Vector3 o(wx, wy, wz);
		gluUnProject(wndX, wndY, 1, modelview, projection, viewport, &wx, &wy, &wz);
		Vector3 p(wx, wy, wz);
		Ray r;
		r.origin = o;
		r.direction = (p - o).normolize();
		return r;

	}
};

#endif