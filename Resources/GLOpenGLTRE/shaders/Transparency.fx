uniform extern float4x4 matViewProjection : ViewProjection;	// Positions arrive pre-transformed into view space; this is really just the projection matrix.


sampler2D	DiffuseMap;
float		fMaterialBrightness;

// Fog (matches fixed-function GL_EXP)
bool		bFogEnabled;	
float		fFogDensity;	
float3	fvFogColor;		

float3 ApplyFog(float3 fvColour, float fFogDistance)
{
	if(bFogEnabled)
	{
		float fFogFactor = saturate( exp(-fFogDensity * fFogDistance) );
		fvColour = lerp(fvFogColor, fvColour, fFogFactor);
	}
	return fvColour;
}

//--------------------------------------------------------------//
// Solid, for materials without textures
//--------------------------------------------------------------//

struct SOLID_VS_INPUT 
{
   float4 Position : POSITION;
   float4 Colour :	 COLOR0;
};

struct SOLID_VS_OUTPUT 
{
   float4 Position :			POSITION0;
   float4 Colour :				COLOR0;
   float  FogDistance :			TEXCOORD0;
};


struct SOLID_PS_INPUT 
{
   float4 Colour :	 			COLOR0;
   float  FogDistance :			TEXCOORD0;
};

SOLID_VS_OUTPUT Solid_vs_main( SOLID_VS_INPUT Input )
{
	SOLID_VS_OUTPUT Output;
	Output.Position      	= mul ( Input.Position, matViewProjection );
	Output.Colour			= Input.Colour;

	// Input.Position is already view-space, so it IS the fog distance basis - no transform needed.
	Output.FogDistance    	= -Input.Position.z;
	
	return (Output);
};


float4 Solid_ps_main( SOLID_PS_INPUT Input) : COLOR0
{
	float4 fvColour = Input.Colour * fMaterialBrightness;
	fvColour.rgb = ApplyFog(fvColour.rgb, Input.FogDistance);
	return fvColour;
};



//--------------------------------------------------------------//
// Unlit materials
//--------------------------------------------------------------//

struct UNLIT_VS_INPUT 
{
   float4 Position :	POSITION;
   float2 Texcoord :	TEXCOORD0;
   float4 Colour :		COLOR0;
};

struct UNLIT_VS_OUTPUT 
{
   float4 Position :	POSITION0;
   float2 Texcoord :	TEXCOORD0;
   float4 Colour :		COLOR0;
   float  FogDistance :	TEXCOORD1;
};


struct UNLIT_PS_INPUT 
{
   float4 Colour :	 	COLOR0;
   float2 Texcoord :	TEXCOORD0;
   float  FogDistance :	TEXCOORD1;
};

UNLIT_VS_OUTPUT Unlit_vs_main( UNLIT_VS_INPUT Input )
{
	UNLIT_VS_OUTPUT Output;
	Output.Position      	= mul ( Input.Position, matViewProjection );
	Output.Colour			= Input.Colour;
	Output.Texcoord			= Input.Texcoord;

	// Input.Position is already view-space, so it IS the fog distance basis - no transform needed.
	Output.FogDistance    	= -Input.Position.z;
	
	return (Output);
};


float4 Unlit_ps_main(UNLIT_PS_INPUT Input) : COLOR0
{
	float4 fvColour = Input.Colour * tex2D( DiffuseMap, Input.Texcoord ) * fMaterialBrightness;
	fvColour.rgb = ApplyFog(fvColour.rgb, Input.FogDistance);
	return fvColour;
};


//--------------------------------------------------------------//
// Technique Section for unlit untextured
//--------------------------------------------------------------//
technique UnlitNoTexture
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Solid_vs_main();
      FragmentProgram = compile arbfp1 Solid_ps_main();
   }
}


//--------------------------------------------------------------//
// Technique Section for unlit textured
//--------------------------------------------------------------//
technique UnlitTexture
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Unlit_vs_main();
      FragmentProgram = compile arbfp1 Unlit_ps_main();
   }
}

