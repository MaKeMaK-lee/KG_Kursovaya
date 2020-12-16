
varying vec2 texCoord; 
varying vec3 Normal;
varying vec3 Position;

uniform vec3 PosOffset;

uniform float RotZOffset;

void main(void) 
{     
	   
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix*gl_Vertex;   //умножаем матрицу проекции на видовую матрицу и на координаты точки
	Position = gl_Vertex.xyz;
	
	Position = vec3(Position.x*cos(RotZOffset)-Position.y*sin(RotZOffset),Position.x*sin(RotZOffset)+Position.y*cos(RotZOffset), Position.z);
	Position += PosOffset;
	

	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;   
	texCoord = gl_TexCoord[0].xy;   //считываем текстурые координаты в варинг
	Normal = normalize(gl_Normal); /*gl_NormalMatrix*/
	Normal = vec3(Normal.x*cos(RotZOffset)-Normal.y*sin(RotZOffset),Normal.x*sin(RotZOffset)+Normal.y*cos(RotZOffset), Normal.z);
	
}