struct VSInput {
    float4 inPosition;
    float4 inColor;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = input.inPosition;
    output.color = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return float4(vertIn.color);
}