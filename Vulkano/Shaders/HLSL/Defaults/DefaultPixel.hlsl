struct PSInput
{
    float3 inColor : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.inColor, 1.0);
}