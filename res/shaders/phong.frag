#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

struct MaterialProps {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

// ── Directional light (Sun) ─────────────────────────────────────────────────
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// ── Point light (fill / accent) ─────────────────────────────────────────────
struct PointLightData {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

// ── Spot light (satellite scan beams) ───────────────────────────────────────
struct SpotLightData {
    vec3 position;
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_POINT_LIGHTS 2
#define MAX_SPOT_LIGHTS 3

uniform MaterialProps   material;
uniform DirLight        dirLight;
uniform PointLightData  pointLights[MAX_POINT_LIGHTS];
uniform int             numPointLights;
uniform SpotLightData   spotLights[MAX_SPOT_LIGHTS];
uniform int             numSpotLights;
uniform vec3           u_viewPos;
uniform sampler2D      u_texture;
uniform int            u_useTexture;
uniform float          u_alpha;
uniform vec3           u_emissive;

// ── Directional light (no attenuation) ──────────────────────────────────────
vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir,
                  vec3 baseDiffuse, vec3 baseAmbient)
{
    vec3 lightDir = normalize(-light.direction);

    vec3 ambient = light.ambient * baseAmbient;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseDiffuse;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return ambient + diffuse + specular;
}

// ── Point light (with attenuation) ──────────────────────────────────────────
vec3 calcPointLight(PointLightData light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec3 baseDiffuse, vec3 baseAmbient)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance
                              + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * baseAmbient;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseDiffuse;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return (ambient + diffuse + specular) * attenuation;
}

// ── Spot light (cone with soft edges) ───────────────────────────────────────
vec3 calcSpotLight(SpotLightData light, vec3 normal, vec3 fragPos, vec3 viewDir,
                   vec3 baseDiffuse)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    // Cone intensity (smooth edge between inner and outer cutoff)
    float theta     = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    if (intensity <= 0.0)
        return vec3(0.0);

    float attenuation = 1.0 / (light.constant + light.linear * distance
                              + light.quadratic * distance * distance);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseDiffuse;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return (diffuse + specular) * attenuation * intensity;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(u_viewPos - FragPos);

    // Resolve base colors (texture overrides material diffuse)
    vec3 baseDiffuse = material.diffuse;
    vec3 baseAmbient = material.ambient;

    if (u_useTexture == 1)
    {
        vec3 texColor = texture(u_texture, TexCoord).rgb;
        baseDiffuse = texColor;
        baseAmbient = texColor * 0.15;
    }

    // Accumulate lighting
    vec3 result = vec3(0.0);
    result += calcDirLight(dirLight, norm, viewDir, baseDiffuse, baseAmbient);

    // Point lights (fill + moonlight)
    for (int i = 0; i < numPointLights; i++)
    {
        result += calcPointLight(pointLights[i], norm, FragPos, viewDir, baseDiffuse, baseAmbient);
    }

    // Spot lights (scan beams)
    for (int i = 0; i < numSpotLights; i++)
    {
        result += calcSpotLight(spotLights[i], norm, FragPos, viewDir, baseDiffuse);
    }

    FragColor = vec4(result + u_emissive, u_alpha);
}
