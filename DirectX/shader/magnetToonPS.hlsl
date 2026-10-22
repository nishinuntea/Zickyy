struct MATERIAL
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Emission;
    float Shininess;
    bool TextureEnable;
    float2 Dummy;
};

cbuffer MaterialBuffer : register(b3)
{
    MATERIAL Material;
}

struct LIGHT
{
    bool Enable;
    bool3 Dummy;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};

cbuffer LightBuffer : register(b4)
{
    LIGHT Light;
}

struct PS_IN
{
    float4 Position : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float4 Diffuse : COLOR0;
};

void main(in PS_IN input, out float4 outputColor : SV_Target)
{
    const float3 normal = normalize(input.Normal);
    const float3 lightDirection = normalize(-Light.Direction.xyz);
    const float lighting = saturate(dot(normal, lightDirection));
    const float toonBand = lighting >= 0.68f ? 1.0f : (lighting >= 0.30f ? 0.68f : 0.34f);
    const float3 baseColor = input.Diffuse.rgb * Material.Diffuse.rgb;
    outputColor = float4(baseColor * toonBand + Material.Emission.rgb, Material.Diffuse.a);
}
