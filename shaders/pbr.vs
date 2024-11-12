#version 410 core

// Input vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoords;

// Output to the fragment shader
out vec2 fragTexCoords;
out vec3 fragPosition;
out mat3 TBN;

// Uniforms for transformation matrices
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // Compute TBN matrix for normal mapping
    vec3 T = normalize(mat3(uModel) * tangent);
    vec3 N = normalize(mat3(uModel) * normal);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    // Pass texture coordinates to fragment shader
    fragTexCoords = texCoords;

    fragPosition = (uModel * vec4(position, 1)).xyz;
    // Transform vertex position to clip space
    gl_Position = uProjection * uView * uModel * vec4(position, 1);
}
