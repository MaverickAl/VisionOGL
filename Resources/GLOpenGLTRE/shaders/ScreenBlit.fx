
sampler ScreenImage;

struct VS_OUTPUTScreen
{
    float4 Pos			: POSITION;
	float2 TexCoords	: TEXCOORD0;	
};


VS_OUTPUTScreen VSScreen(float4 Pos      : POSITION)
{
    VS_OUTPUTScreen Out;        
    Out.Pos.xy = Pos.xy;
	Out.Pos.z = 0.5f;
	Out.Pos.w = 1.0f;
	
	Out.TexCoords = float2(0.5f, 0.5f) * Pos.xy + 0.5f.xx;	
	
    return Out;
}


float4 PSScreen(float2 Tex : TEXCOORD0) : COLOR0
{
    
	float4 FullScreenImage = tex2D(ScreenImage, Tex);
	
	return FullScreenImage;
}



technique ScreenBlit
{
    pass P0
    {
        VertexShader = compile arbvp1 VSScreen();
        PixelShader  = compile arbfp1 PSScreen();
    }
}
