struct PS_IN
{
    float4 Position : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float4 Diffuse : COLOR0;
};

void main(in PS_IN input, out float4 outputColor : SV_Target)
{
    outputColor = input.Diffuse;
}
