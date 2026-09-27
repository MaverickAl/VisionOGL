//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
static const int   kMaxTaps            = 16;   // Largest sample-offset array we support
static const float kBrightPassKnee     = 5.0f;  // Values below this are considered non-bloom
static const float kBrightPassRolloff  = 10.0f; // Controls how sharply bright areas isolate
static const float kBrightPassContrast = 1.5f;  // 1.0 = linear falloff; higher suppresses moderately-bright pixels more than near-blown-out ones


//-----------------------------------------------------------------------------
// Engine-supplied parameters
//-----------------------------------------------------------------------------
float2 g_sampleOffsets[kMaxTaps];
float4 g_sampleWeights[kMaxTaps];

float  g_middleGray;
float  g_bloomScale;

sampler s0;
sampler s1;


//-----------------------------------------------------------------------------
// Fullscreen-quad vertex shader shared by every pass below
//-----------------------------------------------------------------------------
struct ScreenQuadOut
{
    float4 Pos       : POSITION;
    float2 TexCoords : TEXCOORD0;
};

ScreenQuadOut VSScreen(float4 Pos : POSITION)
{
    ScreenQuadOut Out;
    Out.Pos.xy = Pos.xy;
    Out.Pos.z  = 0.5f;
    Out.Pos.w  = 1.0f;

    // Clip-space [-1,1] -> texture-space [0,1]
    Out.TexCoords = Pos.xy * 0.5f + 0.5f;

    return Out;
}

//-----------------------------------------------------------------------------
// Isolate the pixels bright enough to justify blooming, and remap what's
// left into 0..1 so the blur stage has well-behaved input.
//-----------------------------------------------------------------------------
float4 BrightPassPS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 c = tex2D(s0, uv);

    c.rgb *= g_middleGray * 1000.0f;
    c.rgb  = max(c.rgb - kBrightPassKnee, 0.0f);
    c.rgb  = c.rgb / (kBrightPassRolloff + c.rgb);
    c.rgb  = pow(c.rgb, kBrightPassContrast);

    return c;
}

//-----------------------------------------------------------------------------
// One axis of a separable Gaussian blur (call once horizontally, once
// vertically to build a full 2D blur cheaply).
//-----------------------------------------------------------------------------
float4 BloomPS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 accum = 0.0f;

    for (int i = 0; i < 15; ++i)
    {
        float4 tap = tex2D(s0, uv + g_sampleOffsets[i]);
        accum += g_sampleWeights[i] * tap;
    }

    return accum;
}

//-----------------------------------------------------------------------------
// Box-filtered downsample to 1/16 area (4x4 taps).
//-----------------------------------------------------------------------------
float4 Downsample4x4PS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 accum = 0.0f;
    for (int i = 0; i < 16; ++i)
        accum += tex2D(s0, uv + g_sampleOffsets[i]);
    return accum / 16.0f;
}

//-----------------------------------------------------------------------------
// Box-filtered downsample to 1/4 area (2x2 taps).
//-----------------------------------------------------------------------------
float4 Downsample2x2PS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 accum = 0.0f;
    for (int i = 0; i < 4; ++i)
        accum += tex2D(s0, uv + g_sampleOffsets[i]);
    return accum / 4.0f;
}

//-----------------------------------------------------------------------------
// Cheap approximate 5x5 Gaussian using a 12-tap weighted pattern instead of
// the full 25 taps.
//-----------------------------------------------------------------------------
float4 GaussianBlur5x5PS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 accum = 0.0f;
    for (int i = 0; i < 12; ++i)
        accum += g_sampleWeights[i] * tex2D(s0, uv + g_sampleOffsets[i]);
    return accum;
}

//-----------------------------------------------------------------------------
// Composite the bloom buffer back over the main scene.
//-----------------------------------------------------------------------------
float4 SceneCompositePS(in float2 uv : TEXCOORD0) : COLOR
{
    float4 scene = tex2D(s0, uv);
    float4 bloom = tex2D(s1, uv);

    return scene + g_bloomScale * bloom;
}

//-----------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------
technique Bloom
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 BloomPS();
    }
}

technique Downsample4x4
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 Downsample4x4PS();
    }
}

technique Downsample2x2
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 Downsample2x2PS();
    }
}

technique GaussianBlur5x5
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 GaussianBlur5x5PS();
    }
}

technique BrightPass
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 BrightPassPS();
    }
}

technique SceneComposite
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 SceneCompositePS();
    }
}
