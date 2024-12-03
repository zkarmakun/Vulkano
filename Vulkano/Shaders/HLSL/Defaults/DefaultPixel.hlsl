void main(
    in float2 UV : TEXCOORD0,
    out float4 OutColor : SV_Target0)
{
    OutColor = float4(UV.xy, 0.0, 1.0);
}