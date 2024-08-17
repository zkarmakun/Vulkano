

/*cbuffer WEBOS : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4x4 viewMatrix;
};*/

struct VSInput
{
    float3 inPos : POSITION;
    float3 inColor : COLOR;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 outColor : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.outColor = input.inColor;
    //output.pos = mul(projectionMatrix, mul(viewMatrix, mul(modelMatrix, float4(input.inPos, 1.0))));
    output.pos = float4(input.inPos, 1.0);
    return output;
}