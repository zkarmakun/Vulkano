
void main(
    float3 InPosition : ATTRIBUTE0,
    float2 InUV : ATTRIBUTE1,
    out float2 OutUV : TEXCOORD0,
    out float4 OutPosition : SV_POSITION)
{
    OutPosition = float4(InPosition, 1);
    OutUV = InUV;
}