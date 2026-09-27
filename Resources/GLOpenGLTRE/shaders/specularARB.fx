//--------------------------------------------------------------//
// Specular
//--------------------------------------------------------------//
uniform extern float3   fvEyePosition  : EyePosition;
uniform extern float3   fvEyeDirection  : EyeDirection;
uniform extern float4x4 matWorld : World;
uniform extern float4x4 matView : View;
uniform extern float4x4 matWorldViewProjection : WorldViewProjection;
//uniform float4x4 matWorldViewProjection : state.matrix.mvp;

// Light info
float3 fvAmbient;
float4 fvLightColour0;
float4 fvLightColour1;
float4 fvLightColour2;

float3 fvLightDirection0;
float3 fvLightDirection1;
float3 fvLightDirection2;

int iPositionalLightCount;
float3 fvPosLightColour0;
float3 fvPosLightColour1;
float3 fvPosLightColour2;
float3 fvPosLightColour3;
float3 fvLightPosition0;
float3 fvLightPosition1;
float3 fvLightPosition2;
float3 fvLightPosition3;

float4 fvLightAttenuation0;  // Constant, linear, quadratic, range 
float4 fvLightAttenuation1;  // Constant, linear, quadratic, range 
float4 fvLightAttenuation2;  // Constant, linear, quadratic, range 
float4 fvLightAttenuation3;  // Constant, linear, quadratic, range 

// Material info
bool 		bHasSpecMap;
bool		bAutoSpecMap;
bool 		bHasEmissiveMap;
bool 		bHasNormalMap;
bool		bTextured;
bool 		bIridescent;
float		fGameBrightness;
sampler2D	DiffuseMap;
sampler2D	SpecularMap;
sampler2D	EmissiveMap;
sampler2D	NormalMap;
sampler2D	NoiseMap;
sampler2D	IridesenceMask;
sampler2D	IridesenceMap;
float		fMaterialBrightness;
float4		fvSolidColour;

// Fog (matches fixed-function GL_EXP)
bool		bFogEnabled;
float		fFogDensity;
float3	fvFogColor;	


static float fSpecularPower = 3.97;	// Standoff was 19.97
static float fSpecularMultiplier = 1.2f;	// Because Eder is too lazy to edit the spec maps
static float fAutoSpecularMultiplier = 0.7f;	// Standoff was 1.0
const float fNoiseStrength = 0.5;	// Strength of iridesence noise
const float fIridesenceStrength = 1.0; // Strength of iridesence color

#ifndef PER_PIXEL_POINT_LIGHTS
#define PER_PIXEL_POINT_LIGHTS 1
#endif

//--------------------------------------------------------------//
// Utility
//--------------------------------------------------------------//
float GetAttenuation(in const float4 fvAtten, in const float3 fvLightPos, in const float3 fvObjPos)
{
	float fDistance = distance(fvLightPos, fvObjPos );
	float rangeContrib = saturate(fvAtten.w-fDistance);
	return saturate( (1.0f/( fvAtten.x + (fvAtten.y*fDistance) + (fvAtten.z*(fDistance*fDistance)) ) )*rangeContrib );
}


float3 GetDiffuse(in const float3 fvNormal, in const float3 fvLightDir, in const float3 fvLightColour)
{

	float  fNDotL = saturate( dot(fvNormal, fvLightDir ) );
	return saturate(fvLightColour.xyz * fNDotL);
}


float3 GetSpecular(float3 fvNormal, float3 fvLightDir, float fLightStrength, float3 fvViewDir)
{
	float3 fvHalf = normalize(fvLightDir + fvViewDir);
	float fSpecular = pow(saturate(dot(fvNormal, fvHalf)), fSpecularPower);
	return fSpecular * fLightStrength; 
}



float3 GetIridesence(float3 fvViewDir, float3 fvNormal, float2 fvTexCoord)
{
   if(bIridescent)
   {
	   //float3 viewDir = fvViewDir; // matView[2].xyz;
	   //float3 viewDir = -matView[2].xyz;
	   //float3 viewDir = -float3( matView[0][2], matView[1][2], matView[2][2] );
	   float3 viewDir = -fvEyeDirection;

	   float3 noiseVector = (tex2D(NoiseMap, fvTexCoord * 10.f).xyz - float3(0.5)) * fNoiseStrength;
	   float3 mask = tex2D(IridesenceMask, fvTexCoord).xyz;

	   float invDotView = 1.0 - max(dot(normalize(fvNormal + noiseVector), viewDir), 0.0);
	   float3 lookupTableCol = tex2D(IridesenceMap, float2(invDotView, 0.5)).rgb;

	   return lookupTableCol * fIridesenceStrength * mask;
	}
	return float3(0);
}

float3 GetNormal(float3 fvNormal, float3 fvTangent, float3 fvBinormal, float2 fvTexCoord)
{
	float3 normal = normalize(fvNormal);
	if(bHasNormalMap)
	{
		float3 tangent = normalize(fvTangent);
		float3 binormal = normalize(fvBinormal);

        float3 nrm_l = tex2D(NormalMap, fvTexCoord).xyz;
        nrm_l.xyz = normalize((nrm_l * 2.0) - 1.0);
        float3 tan_v = normalize(tangent);
        float3 binrm_v = normalize(binormal);
        normal = normalize(nrm_l.x * tan_v + nrm_l.y * binrm_v + nrm_l.z * fvNormal);
	}
	return normal;
}


float3 ApplyFog(float3 fvColour, float fFogDistance)
{
	if(bFogEnabled)
	{
		float fFogFactor = saturate( exp(-fFogDensity * fFogDistance) );
		fvColour = lerp(fvFogColor, fvColour, fFogFactor);
	}
	return fvColour;
}


struct VS_INPUT 
{
   float4 Position : POSITION;
   float2 Texcoord : TEXCOORD0;
   float3 Normal :   NORMAL;
   float4 Colour :	 COLOR0;
   float3 Binormal : TEXCOORD1;  
   float3 Tangent :	 TEXCOORD2;  
};

struct VS_OUTPUT 
{
   float4 Position :			POSITION0;
   float2 Texcoord :       	 	TEXCOORD0;
   float3 ViewDirection : 	  	TEXCOORD1;
   float3 Normal :        	   	TEXCOORD2;
   float4 Colour :				COLOR0;
   float3 PointLightContrib : 	COLOR1; 
   float3 Binormal : TEXCOORD3;  
   float3 Tangent :	 TEXCOORD4;  
   float  FogDistance : TEXCOORD5;
#if PER_PIXEL_POINT_LIGHTS
   float3 WorldPosition :		TEXCOORD6;
#endif
};



VS_OUTPUT Specular_vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output = (VS_OUTPUT)0;

	Output.Position      	= mul ( Input.Position, matWorldViewProjection );
	Output.Texcoord      	= Input.Texcoord;
	if(bHasNormalMap)
	{
		float3 unitNormal = normalize(Input.Normal);
		float3 tangent = normalize(Input.Tangent);
		tangent = (tangent) - unitNormal * dot(Input.Normal, tangent);
		tangent = normalize(tangent);

		float3 binormal = normalize(Input.Binormal);
		binormal = (binormal) - unitNormal * dot(Input.Normal, binormal);
		binormal = normalize(binormal);

		Output.Binormal 		= normalize(mul( float4(binormal, 0.0f), matWorld ).xyz);
		Output.Tangent 			= normalize(mul( float4(tangent, 0.0f), matWorld ).xyz);
	}
	else
	{
		Output.Binormal 		= float3(1);
		Output.Tangent 			= float3(1);
	}


	float3 fvObjectPosition = mul( Input.Position, matWorld ).xyz;
	float4 fvViewSpacePosition = mul( float4(fvObjectPosition, 1.0f), matView );
	Output.FogDistance      	= -fvViewSpacePosition.z;
	Output.ViewDirection    = normalize(fvEyePosition - fvObjectPosition);
	float4 vNormal			= float4(Input.Normal.xyz, 0.0f);

	Output.Normal           = normalize(mul( vNormal, matWorld ).xyz);


	Output.Colour			= Input.Colour;

#if PER_PIXEL_POINT_LIGHTS
	Output.WorldPosition    = fvObjectPosition;
	Output.PointLightContrib = float3(0.0f, 0.0f, 0.0f);
#else
	Output.PointLightContrib = float3(0.0f, 0.0f, 0.0f);
	float attenuation = 0.0f;

	// Light 1
	attenuation = GetAttenuation(fvLightAttenuation0, fvLightPosition0.xyz, fvObjectPosition);
	Output.PointLightContrib += fvPosLightColour0*attenuation;

	// Light 2
	attenuation = GetAttenuation(fvLightAttenuation1, fvLightPosition1.xyz, fvObjectPosition);
	Output.PointLightContrib += fvPosLightColour1*attenuation;

	// Light 3
	attenuation = GetAttenuation(fvLightAttenuation2, fvLightPosition2.xyz, fvObjectPosition);
	Output.PointLightContrib += fvPosLightColour2*attenuation;

	// Light 4
	attenuation = GetAttenuation(fvLightAttenuation3, fvLightPosition3.xyz, fvObjectPosition);
	Output.PointLightContrib += fvPosLightColour3*attenuation;


	Output.PointLightContrib = saturate(Output.PointLightContrib);
#endif
	    
	return( Output );
}

struct PS_INPUT 
{
   float4 Position :			POSITION0;
   float2 Texcoord :        	TEXCOORD0;
   float3 ViewDirection :   	TEXCOORD1;
   float3 Normal :          	TEXCOORD2;
   float4 Colour :	 			COLOR0;
   float3 PointLightContrib : 	COLOR1; 
   float3 Binormal : 			TEXCOORD3;  
   float3 Tangent :	 			TEXCOORD4;  
   float  FogDistance : 		TEXCOORD5;
#if PER_PIXEL_POINT_LIGHTS
   float3 WorldPosition :		TEXCOORD6;
#endif
};


float3 GetPointLightContrib(PS_INPUT Input)
{
#if PER_PIXEL_POINT_LIGHTS
	float3 contrib = float3(0.0f, 0.0f, 0.0f);
	float attenuation;

	attenuation = GetAttenuation(fvLightAttenuation0, fvLightPosition0.xyz, Input.WorldPosition);
	contrib += fvPosLightColour0*attenuation;

	attenuation = GetAttenuation(fvLightAttenuation1, fvLightPosition1.xyz, Input.WorldPosition);
	contrib += fvPosLightColour1*attenuation;

	attenuation = GetAttenuation(fvLightAttenuation2, fvLightPosition2.xyz, Input.WorldPosition);
	contrib += fvPosLightColour2*attenuation;

	attenuation = GetAttenuation(fvLightAttenuation3, fvLightPosition3.xyz, Input.WorldPosition);
	contrib += fvPosLightColour3*attenuation;

	return saturate(contrib);
#else
	return Input.PointLightContrib;
#endif
}


float4 GetDiffuse(float2 vTexCoord)
{
   if(bTextured)
   {	
      return tex2D( DiffuseMap, vTexCoord );
   }
   else
   {
      return fvSolidColour;
   }
}


float4 Ambient_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord )*fMaterialBrightness; 
	float3 fvTotalAmbient   = (fvAmbient.xyz * fvBaseColor.xyz)*Input.Colour.xyz; 
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	// Prophecy doesn't expect us to check normals for point lights	
	float3 fvTotalDiffuse = GetPointLightContrib(Input) *fvRawDiffuse;
	
   float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}


float4 Specular1DirNoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);//normalize(Input.Normal);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz; 
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;


	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}

float4 Specular1DirAutoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz; 

	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}

	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
    float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = (fvBaseColor.r + fvBaseColor.g + fvBaseColor.b)/3.0f;
	fvRawSpecular *= fAutoSpecularMultiplier;
	
	fvTotalSpecular = GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 	

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );

	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

 
   float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}




float4 Specular1DirSpecMap_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz;
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}

	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = float3(0.0f, 0.0f, 0.0f);

	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
	float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = tex2D(SpecularMap, Input.Texcoord).rgb * fSpecularMultiplier;	

	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );
	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}



float4 Specular2DirNoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, 
	fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz; 
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;


	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}

float4 Specular2DirAutoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz; 
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
    float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = (fvBaseColor.r + fvBaseColor.g + fvBaseColor.b)/3.0f;
	fvRawSpecular *= fAutoSpecularMultiplier;
	
	fvTotalSpecular = GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 	
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection1.xyz, fvLightColour1.a, fvViewDirection); 	

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );

	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

 
   float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}




float4 Specular2DirSpecMap_ps_main( PS_INPUT Input) : COLOR0
{
   float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
   float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
   float3 fvViewDirection  = normalize(Input.ViewDirection);
   float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz;  
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}   
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = float3(0.0f, 0.0f, 0.0f);

	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
	float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = tex2D(SpecularMap, Input.Texcoord).rgb * fSpecularMultiplier;	

	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection1.xyz, fvLightColour1.a, fvViewDirection); 

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );
	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}



float4 Specular3DirNoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz;  
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}   

	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection2.xyz, fvLightColour2.xyz);   

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;


	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}

float4 Specular3DirAutoSpec_ps_main( PS_INPUT Input) : COLOR0
{
	float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
	float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
	float3 fvViewDirection  = normalize(Input.ViewDirection);
	float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz;  
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}   
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection2.xyz, fvLightColour2.xyz);   


	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
    float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = (fvBaseColor.r + fvBaseColor.g + fvBaseColor.b)/3.0f;
	fvRawSpecular *= fAutoSpecularMultiplier;
	
	fvTotalSpecular = GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 	
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection1.xyz, fvLightColour1.a, fvViewDirection); 
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection2.xyz, fvLightColour2.a, fvViewDirection); 	

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );

	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

 
   float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
}




float4 Specular3DirSpecMap_ps_main( PS_INPUT Input) : COLOR0
{
   float4 fvBaseColor      = GetDiffuse( Input.Texcoord ); 
   float3 fvNormal         = GetNormal(Input.Normal, Input.Tangent, Input.Binormal, Input.Texcoord);
   float3 fvViewDirection  = normalize(Input.ViewDirection);
   float3 fvTotalAmbient   = ( (fvAmbient.xyz + GetIridesence(fvViewDirection, fvNormal, Input.Texcoord)) * fvBaseColor.xyz)*Input.Colour.xyz;  
	if(bHasEmissiveMap)
	{
	  fvTotalAmbient += tex2D(EmissiveMap, Input.Texcoord).rgb;
	}   
   
	// Diffuse
	float3 fvRawDiffuse	 = Input.Colour.xyz*fvBaseColor.xyz;
	float3 fvTotalDiffuse = float3(0.0f, 0.0f, 0.0f);

	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection0.xyz, fvLightColour0.xyz);   
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection1.xyz, fvLightColour1.xyz);
	fvTotalDiffuse += GetDiffuse(fvNormal, fvLightDirection2.xyz, fvLightColour2.xyz);

	// Prophecy doesn't expect us to check normals for point lights	
	fvTotalDiffuse += GetPointLightContrib(Input);
	fvTotalDiffuse *= fvRawDiffuse;

	// Specular
	float3 fvTotalSpecular = float3(0.0f, 0.0f, 0.0f);

	float3 fvRawSpecular = tex2D(SpecularMap, Input.Texcoord).rgb * fSpecularMultiplier;	

	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection0.xyz, fvLightColour0.a, fvViewDirection); 
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection1.xyz, fvLightColour1.a, fvViewDirection); 
	fvTotalSpecular += GetSpecular(fvNormal, fvLightDirection2.xyz, fvLightColour2.a, fvViewDirection); 

	float3 baseMapHue = float3(1.0f, 1.0f, 1.0f);
	
	// Hack for black surfaces, no branching in arb1
	fvRawDiffuse = saturate(fvRawDiffuse.rgb + 0.1f);
	
	float fMax = max(max( fvRawDiffuse.x, fvRawDiffuse.y ), fvRawDiffuse.z);

	// Use base colour until value of 190, then lerp until value of 250
	baseMapHue = lerp(baseMapHue, fvRawDiffuse.xyz * (1.0f/fMax), saturate((fvRawSpecular.x-0.753)*0.227) );
	// Test: Multiply by the colour map for the specular colour
	fvTotalSpecular *= fvRawSpecular * baseMapHue * fGameBrightness;

	float3 fvFinalColour = ApplyFog(fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular, Input.FogDistance);
   return float4(fvFinalColour, Input.Colour.w*fvBaseColor.w);
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
	Output.Position      	= mul ( Input.Position, matWorldViewProjection );
	Output.Colour			= Input.Colour;

	float3 fvObjectPosition = mul( Input.Position, matWorld ).xyz;
	float4 fvViewSpacePosition = mul( float4(fvObjectPosition, 1.0f), matView );
	Output.FogDistance    	= -fvViewSpacePosition.z;
	
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
   float  FogDistance :	TEXCOORD5;
};


struct UNLIT_PS_INPUT 
{
   float4 Colour :	 	COLOR0;
   float2 Texcoord :	TEXCOORD0;
   float  FogDistance :	TEXCOORD5;
};

UNLIT_VS_OUTPUT Unlit_vs_main( UNLIT_VS_INPUT Input )
{
	UNLIT_VS_OUTPUT Output;
	Output.Position      	= mul ( Input.Position, matWorldViewProjection );
	Output.Colour			= Input.Colour;
	Output.Texcoord			= Input.Texcoord;

	float3 fvObjectPosition = mul( Input.Position, matWorld ).xyz;
	float4 fvViewSpacePosition = mul( float4(fvObjectPosition, 1.0f), matView );
	Output.FogDistance    	= -fvViewSpacePosition.z;
	
	return (Output);
};


float4 Unlit_ps_main(UNLIT_PS_INPUT Input) : COLOR0
{
	float4 fvColour = Input.Colour * tex2D( DiffuseMap, Input.Texcoord ) * fMaterialBrightness;
	fvColour.rgb = ApplyFog(fvColour.rgb, Input.FogDistance);
	return fvColour;
};

//--------------------------------------------------------------//
// Technique Section for Specular
//--------------------------------------------------------------//
technique Specular0Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Ambient_ps_main();
   }

}

technique Diffuse1Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular1DirNoSpec_ps_main();
   }
}

technique AutoSpec1Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular1DirAutoSpec_ps_main();
   }
}

technique SpecMap1Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular1DirSpecMap_ps_main();
   }
}

technique Diffuse2Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular2DirNoSpec_ps_main();
   }
}

technique AutoSpec2Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular2DirAutoSpec_ps_main();
   }
}

technique SpecMap2Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular2DirSpecMap_ps_main();
   }
}


technique Diffuse3Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular3DirNoSpec_ps_main();
   }
}

technique AutoSpec3Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular3DirAutoSpec_ps_main();
   }
}

technique SpecMap3Dir
{
   pass Pass_0
   {
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Specular3DirSpecMap_ps_main();
   }
}


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
      VertexProgram   = compile arbvp1 Specular_vs_main();
      FragmentProgram = compile arbfp1 Unlit_ps_main();
   }
}
