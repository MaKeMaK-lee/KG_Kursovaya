#define WISPMAXCOUNT 129 //При изменении необходимо также изменить это в Render.cpp и др. шейдерах
#define MAXDISTANCEA 78
#define MAXDISTANCED 30


varying vec3 Normal;
varying vec3 Position;

uniform vec3 Ia[WISPMAXCOUNT];
uniform vec3 Id[WISPMAXCOUNT];
uniform vec3 Is[WISPMAXCOUNT];
uniform vec3 light_pos[WISPMAXCOUNT];
uniform vec3 ma[WISPMAXCOUNT];
uniform vec3 md[WISPMAXCOUNT];
uniform vec4 ms[WISPMAXCOUNT];
		
uniform int count;

uniform vec3 camera;

//Текстура (Тупа цвет)
uniform vec4 TexVec = vec4(0.6588,0.8471,1,1);

void main(void)
{
	//Свет

	gl_FragColor = vec4(0.0,0.0,0.0,1.0);
	vec3 view_vector = normalize(camera - Position);
	vec3 color_amb;
	vec3 light_vector;
	vec3 color_dif;
	vec3 r;
	vec3 color_spec;
	
	for (int i = 0;i<count;i++){
	vec3 tmplp = light_pos[i]-Position;	
	float lt = length(tmplp);

	vec3 color_amb = (lt<=MAXDISTANCEA?Ia[i]*ma[i]*-(lt/MAXDISTANCEA-1):0);
	
	vec3 light_vector = normalize(tmplp);
	vec3 color_dif = (lt<=MAXDISTANCED?Id[i]*md[i]*dot(light_vector, light_pos[i].x > 200?Normal*-1:Normal)*-(lt/MAXDISTANCED-1):0);
	
	vec3 r = reflect(light_vector,light_pos[i].x > 200?Normal*-1:Normal);
	vec3 color_spec = Is[i]*ms[i].xyz*pow(max(0.0,dot(-r,view_vector)),ms[i].w);
	
	gl_FragColor += vec4(color_amb + color_dif + color_spec, 1);
	}
	//gl_FragColor += vec4(0.06125,0.06125,0.06125,1);
	//gl_FragColor = gl_FragColor / (count+1);
	gl_FragColor *= TexVec;
	gl_FragColor.w = 0.3;
	
}