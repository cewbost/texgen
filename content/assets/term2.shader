[[FX]]

sampler2D fontMap = sampler_state
{
	Address = Wrap;
	Filter = None;
};

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
attribute vec2 texCoords0;
varying vec2 texCoords;

void main(void)
{
  texCoords = texCoords0;
  gl_Position = projMat * vec4(vertPos.x + translation.x, vertPos.y, 1, 1);
}

[[FS_OVERLAY]]

uniform sampler2D fontMap;

uniform vec4 olayColor;

varying vec2 texCoords;

void main(void)
{
  float opacity = texture2D(fontMap, texCoords).r;
  if(texCoords.x > 1.0)
    opacity = 1.0 - opacity;
  
	gl_FragColor = vec4(olayColor.rgb, opacity);
  //gl_FragColor = vec4(texCoords.x, texCoords.y, 0.0, 1.0);
  //gl_FragColor = olayColor;
}
