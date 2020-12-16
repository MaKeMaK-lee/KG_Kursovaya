uniform sampler2D tex;

varying vec2 texCoord;

void main(void)
{
    gl_FragColor = vec4(texture2D(tex,texCoord).rgb,0.3); //натягиваем текстуру
    gl_FragColor-=vec4(0.2,0.2,0.2,0.0);  //чучуть ее затемняем
}
