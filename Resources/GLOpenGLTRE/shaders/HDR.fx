// -------------------------------------------------------------
// Bright-pass filter, separable blur, and scene composite
// -------------------------------------------------------------

float2 horzTapOffsets[7];
float2 vertTapOffsets[7];
float4 texelWeights[7];
float4 pixelSize;
float  exposureLevel;
float  brightPassCutoff;

sampler2D fullResMap;
sampler2D renderMap;

// -------------------------------------------------------------
// Bright pass: isolate everything above the cutoff, discard the rest
// -------------------------------------------------------------
struct VS_OUTPUT_BrightPass
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUTPUT_BrightPass VSBrightPass(float4 Pos : POSITION)
{
    VS_OUTPUT_BrightPass Out = (VS_OUTPUT_BrightPass)0;
    Out.Pos.xy = Pos.xy;
    Out.Pos.z  = 0.5f;
    Out.Pos.w  = 1.0f;

    Out.Tex = Pos.xy * 0.5f + 0.5f;

    return Out;
}

float4 PSBrightPass(float2 Tex : TEXCOORD0) : COLOR
{
    float4 col = tex2D(renderMap, Tex);

    col -= brightPassCutoff.xxxx;
    col  = max(col, 0.0f);

    return col;
}

// -------------------------------------------------------------
// Separable 13-tap symmetric Gaussian blur: horizontal pass
// -------------------------------------------------------------
struct VS_OUTPUT_Blur
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUTPUT_Blur VSBlurHorizontal(float4 Pos : POSITION)
{
    VS_OUTPUT_Blur Out = (VS_OUTPUT_Blur)0;
    Out.Pos.xy = Pos.xy;
    Out.Pos.z  = 0.5f;
    Out.Pos.w  = 1.0f;

    Out.Tex = Pos.xy * 0.5f + 0.5f;

    return Out;
}

float4 PSBlurHorizontal(float2 Tex : TEXCOORD0) : COLOR0
{
    float4 sum = tex2D(renderMap, Tex) * texelWeights[0];

    for (int i = 1; i < 7; ++i)
    {
        sum += tex2D(renderMap, Tex + horzTapOffsets[i]) * texelWeights[i];
        sum += tex2D(renderMap, Tex - horzTapOffsets[i]) * texelWeights[i];
    }

    return sum;
}

// -------------------------------------------------------------
// Separable 13-tap symmetric Gaussian blur: vertical pass
// -------------------------------------------------------------
VS_OUTPUT_Blur VSBlurVertical(float4 Pos : POSITION)
{
    VS_OUTPUT_Blur Out = (VS_OUTPUT_Blur)0;
    Out.Pos.xy = Pos.xy;
    Out.Pos.z  = 0.5f;
    Out.Pos.w  = 1.0f;

    Out.Tex = Pos.xy * 0.5f + 0.5f;

    return Out;
}

float4 PSBlurVertical(float2 Tex : TEXCOORD0) : COLOR0
{
    float4 sum = tex2D(renderMap, Tex) * texelWeights[0];

    for (int i = 1; i < 7; ++i)
    {
        sum += tex2D(renderMap, Tex + vertTapOffsets[i]) * texelWeights[i];
        sum += tex2D(renderMap, Tex - vertTapOffsets[i]) * texelWeights[i];
    }

    return sum;
}

// -------------------------------------------------------------
// Composite the blurred bloom buffer back over the full-res scene
// -------------------------------------------------------------
struct VS_OUTPUT_Composite
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUTPUT_Composite VSComposite(float4 Pos : POSITION)
{
    VS_OUTPUT_Composite Out;
    Out.Pos.xy = Pos.xy;
    Out.Pos.z  = 0.5f;
    Out.Pos.w  = 1.0f;

    Out.Tex = Pos.xy * 0.5f + 0.5f;

    return Out;
}

float4 PSComposite(float2 Tex : TEXCOORD0) : COLOR0
{
    float4 scene = tex2D(fullResMap, Tex);
    float4 bloom = tex2D(renderMap, Tex);

    scene.rgb += (exposureLevel / 12.0f) * bloom.rgb;

    return scene;
}

// -------------------------------------------------------------
// Techniques
// -------------------------------------------------------------
technique BrightPass
{
    pass P0
    {
        VertexShader = compile arbvp1 VSBrightPass();
        PixelShader  = compile arbfp1 PSBrightPass();
    }
}

technique BlurHorizontal
{
    pass P0
    {
        VertexShader = compile arbvp1 VSBlurHorizontal();
        PixelShader  = compile arbfp1 PSBlurHorizontal();
    }
}

technique BlurVertical
{
    pass P0
    {
        VertexShader = compile arbvp1 VSBlurVertical();
        PixelShader  = compile arbfp1 PSBlurVertical();
    }
}

technique SceneComposite
{
    pass P0
    {
        VertexShader = compile arbvp1 VSComposite();
        PixelShader  = compile arbfp1 PSComposite();
    }
}
