[[FX]]

sampler2D normal_map = sampler_state
{
  Filter = Bilinear;
  Address = Wrap;
};

sampler2D color_map = sampler_state
{
  Filter = Bilinear;
  Address = Wrap;
};

context RENDER
{
  VertexShader = compile GLSL VS_RENDER;
  PixelShader = compile GLSL FS_RENDER;
  CullMode = Back;
  ZWriteEnable = false;
  ZEnable = false;
}

context LIGHT
{
  VertexShader = compile GLSL VS_LIGHT;
  PixelShader = compile GLSL FS_LIGHT;
  CullMode = Back;
  BlendMode = AddBlended;
  ZWriteEnable = false;
  ZEnable = false;
}

[[VS_RENDER]]

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 tangent;
attribute vec2 texCoords0;

uniform mat4 worldMat;
uniform mat3 worldNormalMat;

uniform mat4 viewMat;
uniform mat4 projMat;

varying vec2 texCoords;
varying vec3 worldNormal;
varying vec3 worldTangent;
varying vec3 worldBinormal;
varying vec3 eyeVec;

void main(void)
{
  vec4 worldPos = worldMat * vec4(vertPos, 1.0);
  texCoords = texCoords0;
  worldNormal = worldNormalMat * normal;
  worldTangent = worldNormalMat * tangent;
  worldBinormal = cross(worldNormal, worldTangent);
  
  worldPos = viewMat * worldPos;
  
  vec3 eye = normalize(worldPos).xyz;
  
  eyeVec.x = dot(eye, worldTangent);
  eyeVec.y = dot(eye, worldBinormal);
  eyeVec.z = dot(eye, worldNormal);
  
  gl_Position = projMat * worldPos;
}

[[FS_RENDER]]

uniform sampler2D normal_map;
uniform sampler2D color_map;

varying vec2 texCoords;
varying vec3 worldNormal;
varying vec3 worldTangent;
varying vec3 worldBinormal;
varying vec3 eyeVec;

void main(void)
{
  /*vec2 newTexCoords = texCoords;
  if(eyeVec.z >= 0.0) discard;
  float sampl = texture2D(normal_map, newTexCoords).a;
  float cutof = 0.99;
  vec2 shiftVec = eyeVec.xy / eyeVec.z;
  while(sampl < cutof)
  {
    cutof -= 0.03;
    newTexCoords -= shiftVec * 0.001;
    sampl = texture2D(normal_map, newTexCoords).a;
  }

  vec3 color = texture2D(color_map, newTexCoords).rgb;
  
  gl_FragColor = vec4(color, 1.0);*/
  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}


[[VS_LIGHT]]

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 tangent;
attribute vec2 texCoords0;

uniform mat4 worldMat;
uniform mat3 worldNormalMat;
uniform vec4 lightPos;

uniform mat4 viewMat;
uniform mat4 projMat;

varying vec2 texCoords;
varying vec3 worldNormal;
varying vec3 worldTangent;
varying vec3 worldBinormal;
varying vec3 lightVec;
varying vec3 eyeVec;

void main(void)
{
  vec4 worldPos = worldMat * vec4(vertPos, 1.0);
  texCoords = texCoords0;
  worldNormal = worldNormalMat * normal;
  worldTangent = worldNormalMat * tangent;
  worldBinormal = cross(worldNormal, worldTangent);
  
  vec3 light = normalize(worldPos - lightPos).xyz;
  
  lightVec.x = dot(light, worldTangent);
  lightVec.y = dot(light, worldBinormal);
  lightVec.z = dot(light, worldNormal);
  
  lightVec = normalize(lightVec);
  
  worldPos = viewMat * worldPos;
  
  vec3 eye = normalize(worldPos).xyz;
  
  eyeVec.x = dot(eye, worldTangent);
  eyeVec.y = dot(eye, worldBinormal);
  eyeVec.z = dot(eye, worldNormal);
  
  eyeVec = normalize(eyeVec);
  
  gl_Position = projMat * worldPos;
}

[[FS_LIGHT]]

uniform sampler2D normal_map;
uniform sampler2D color_map;

varying vec2 texCoords;
varying vec3 worldNormal;
varying vec3 worldTangent;
varying vec3 worldBinormal;
varying vec3 lightVec;
varying vec3 eyeVec;

void main(void)
{
  vec2 newTexCoords = texCoords;
  if(eyeVec.z >= 0.0) discard;
  float sampl = texture2D(normal_map, newTexCoords).a;
  float cutof = 0.99;
  vec2 shiftVec = eyeVec.xy / eyeVec.z;
  while(sampl < cutof)
  {
    cutof -= 0.03;
    newTexCoords -= shiftVec * 0.001;
    sampl = texture2D(normal_map, newTexCoords).a;
  }

  vec3 mapNormal = (texture2D(normal_map, newTexCoords).xyz - vec3(0.5, 0.5, 0.5)) * 2.0;
  vec3 color = texture2D(color_map, newTexCoords).rgb;
  
  color *= dot(-lightVec, mapNormal) * 0.5;
  
  /*float specular = dot(reflect(lightVec, mapNormal), -eyeVec);
  if(specular > 0.0)
  {
    specular = pow(specular, 10.0) * 0.5;
    color += vec3(specular, specular, specular);
  }*/
  
  gl_FragColor = vec4(color, 1.0);
}
