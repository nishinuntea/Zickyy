cbuffer WorldBuffer : register(b0)
{
    matrix World;
}

cbuffer ViewBuffer : register(b1)
{
    matrix View;
}

cbuffer ProjectionBuffer : register(b2)
{
    matrix Projection;
}

struct VS_IN
{
    float4 Position : POSITION0;
    float4 Normal : NORMAL0;
    float4 Diffuse : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

struct PS_IN
{
    float4 Position : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float4 Diffuse : COLOR0;
};

void main(in VS_IN input, out PS_IN output)
{
    matrix worldViewProjection = mul(World, View);
    worldViewProjection = mul(worldViewProjection, Projection);
    output.Position = mul(input.Position, worldViewProjection);
    output.Normal = normalize(mul(float4(input.Normal.xyz, 0.0f), World).xyz);
    output.Diffuse = input.Diffuse;
}
