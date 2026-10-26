// Dynamic sources keep these uniforms editable in ReShade performance mode.
uniform float2 SketchDepthPixel < source = "mousepoint"; >;
uniform bool SketchDepthAvailable < source = "bufready_depth"; >;

texture2D SketchSceneDepth : DEPTH;
sampler2D SketchDepthSampler
{
    Texture = SketchSceneDepth;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};
texture2D SketchDepthSample
{
    Width = 1;
    Height = 1;
    Format = RGBA32F;
};

float4 SketchDepthVS(uint id : SV_VertexID) : SV_Position
{
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}
float4 SketchDepthPS(float4 position : SV_Position) : SV_Target
{
    float2 dimensions = float2(tex2Dsize(SketchDepthSampler));
    float2 requested = SketchDepthPixel / float2(BUFFER_WIDTH, BUFFER_HEIGHT);
    int2 pixel = int2(clamp(floor(requested * dimensions), float2(0, 0), dimensions - 1));
    float2 uv = (float2(pixel) + .5) / dimensions;
    return float4(tex2Dfetch(SketchDepthSampler, pixel).r, uv, SketchDepthAvailable ? 1 : 0);
}

technique SketchDepthCapture < enabled = false; hidden = true; >
{
    pass
    {
        VertexShader = SketchDepthVS;
        PixelShader = SketchDepthPS;
        RenderTarget = SketchDepthSample;
    }
}
