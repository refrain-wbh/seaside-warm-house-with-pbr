#version 330
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoords;
layout (location = 3) in mat4 aInstanceModel;	//实例化数组

uniform mat4 projection;
uniform mat4 view;
//uniform mat4 model;
uniform vec3 light_position;

out vec3 light_vector;
out vec3 normal_vector;
out vec3 halfway_vector;
out float fog_factor;
out vec2 tex_coord;

void main()
{
	mat4 model = aInstanceModel;
	gl_Position = view * model * vec4(aPos, 1.0);
	vec4 v = gl_Position;

	fog_factor = min(-gl_Position.z/200.0, 1.0);
	gl_Position = projection * gl_Position;

	vec3 normal1 = normalize(aNormal);

	light_vector = normalize((view * vec4(light_position, 1.0)).xyz - v.xyz);
	normal_vector = (inverse(transpose(view * model)) * vec4(normal1, 0.0)).xyz;
    halfway_vector = light_vector + normalize(-v.xyz);

	tex_coord = aTexCoords.xy;
}