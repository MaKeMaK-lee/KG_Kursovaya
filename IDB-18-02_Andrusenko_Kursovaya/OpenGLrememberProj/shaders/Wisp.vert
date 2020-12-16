void main(void) 
{      
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix*gl_Vertex;   //умножаем матрицу проекции на видовую матрицу и на координаты точки
}