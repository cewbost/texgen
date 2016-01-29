[[FX]]

float4 translation;

context OVERLAY
{
	VertexShader = compile GLSL VS_OVERLAY;
	PixelShader = compile GLSL FS_OVERLAY;
	CullMode = None;
	BlendMode = Blend;
}

[[VS_OVERLAY]]

uniform mat4 projMat;
uniform vec4 translation;
attribute vec2 vertPos;

void main(void)
{
  gl_Position = projMat * vec4(vertPos.x + translation.x, vertPos.y, 1, 1);
}

[[FS_OVERLAY]]

uniform vec4 olayColor;

void main(void)
{
	gl_FragColor = olayColor;
}
