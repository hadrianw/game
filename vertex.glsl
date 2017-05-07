#version 330 core
  
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 offset;

uniform mat3x2 transform;

void main()
{
	gl_Position = vec4(
		(transform * vec3(position + offset, 1)).xy, 0.0, 1.0
	);
}
