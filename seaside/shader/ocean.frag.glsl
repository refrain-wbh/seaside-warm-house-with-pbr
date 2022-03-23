#version 330
#extension GL_NV_shadow_samplers_cube : enable

in vec3 normal_vector;
in vec3 light_vector;
in vec3 halfway_vector;
in vec2 tex_coord;
in float fog_factor;

out vec4 fragColor;

//uniform sampler2D water;
uniform samplerCube skybox;

void main()
{
	vec3 normal1         = normalize(normal_vector);
	vec3 light_vector1   = normalize(light_vector);
	vec3 halfway_vector1 = normalize(halfway_vector);

	vec4 c = vec4(1,1,1,1);		//texture(water, tex_coord);

	vec4 emissive_color = vec4(255.0, 255.0, 255.0, 255.0)/255.0;
	vec4 ambient_color  = vec4(0.0, 144.0, 224.0, 255.0)/255.0;		//vec4(0.0, 0.65, 0.75, 1.0);	//青 
	vec4 diffuse_color  = vec4(128.0, 144.0, 224.0, 255.0)/255.0;	//vec4(0.5, 0.65, 0.75, 1.0);	//青灰 128,166,192
	vec4 specular_color = vec4(255.0, 255.0, 255.0, 255.0)/255.0;	//vec4(1.0, 0.25, 0.0,  1.0);	//红

	float emissive_contribution = 0.00;
	float ambient_contribution  = 0.30;
	float diffuse_contribution  = 0.30;
	float specular_contribution = 0.60;

	float d = dot(normal1, light_vector1);
	bool facing = d > 0.0;

	fragColor = emissive_color * emissive_contribution +
		    ambient_color  * ambient_contribution  * c +
		    diffuse_color  * diffuse_contribution  * c * max(d, 0) +
                    (facing ?
			specular_color * specular_contribution * c * max(pow(dot(normal1, halfway_vector1), 120.0), 0.0) :
			vec4(0.0, 0.0, 0.0, 0.0));

	//折射
	vec3 eye_vector1 = normalize(gl_FragCoord.xyz - vec3(0.0));
	vec3 refract_vector1 = refract(eye_vector1, normal1, 1.00/1.33);
	vec4 refract_color = textureCube(skybox, refract_vector1);
	//反射
	vec3 reflect_vector1 = reflect(eye_vector1, normal1);
	vec4 reflect_color = textureCube(skybox, reflect_vector1);

	vec4 waterColor = fragColor;
	fragColor = mix(refract_color, reflect_color, 0.0);
	fragColor = vec4(fragColor.r * 0.10, fragColor.g, fragColor.b, 1.0);
	fragColor = mix(fragColor, waterColor, 1.0);

	//雾化
	vec4 fog_color = vec4(215.0, 231.0, 254.0, 255.0)/255.0;		//vec4(0.25, 0.75, 0.65, 1.0);	//64,192,166
	float coe2 = 0.75f;
	float fog_factor2 = coe2*fog_factor*fog_factor + (1-coe2)*fog_factor;
	fragColor = fragColor * (1.0-fog_factor2) + fog_color * fog_factor2;

	fragColor.a = 0.94;
}