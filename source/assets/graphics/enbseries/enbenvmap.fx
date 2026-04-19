//++++++++++++++++++++++++++++++++++++++++++++
// ENBSeries effect file
// visit http://enbdev.com for updates
// Copyright (c) 2007-2017 Boris Vorontsov
//++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++
//internal parameters, can be modified
//+++++++++++++++++++++++++++++
//none



//+++++++++++++++++++++++++++++
//external parameters, do not modify
//+++++++++++++++++++++++++++++
//keyboard controlled temporary variables (in some versions exists in the config file). Press and hold key 1,2,3...8 together with PageUp or PageDown to modify. By default all set to 1.0
float4	tempF1; //0,1,2,3
float4	tempF2; //5,6,7,8
float4	tempF3; //9,0
//x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
float4	ScreenSize;
//changes in range 0..1, 0 means that night time, 1 - day time
float	ENightDayFactor;
//changes 0 or 1. 0 means that exterior, 1 - interior
float	EInteriorFactor;
//.x - current weather index, .y - incoming weather index, .z - weather transition, .w - time of the day in 24 standart hours. Weather index is value from _weatherlist.ini, for example WEATHER002 means index==2, but index==0 means that weather not captured.
float4	WeatherAndTime;
//x=generic timer in range 0..1, period of 16777216 ms (4.6 hours), w=frame time elapsed (in seconds)
float4	Timer;
//additional info for computations
//float4	TempParameters; 
//fov in degrees
float	FieldOfView;
//time in 0..24 format
float	GameTime;
//sun direction in world space
float4	SunDirection; //.w - 1 when sun is set
//constants set in enbhelper.dll, can be anything captured from game
float4	CustomShaderConstants1[8];


//transposed transform matrix view*projection and inverse of it
float4	MatrixVP[4];
float4	MatrixInverseVP[4];
float4	MatrixVPRotation[4];
float4	MatrixInverseVPRotation[4];
float4	MatrixView[4];
float4	MatrixInverseView[4];
float4	CameraPosition;


float4x4	MatrixWVP;
float4x4	MatrixWVPInverse;
float4x4	MatrixWorld;
float4x4	MatrixProj;
float4	FogParam; //x - nearclip, y - farclip, z - fog start, w - fog end
float4	FogFarColor;


texture2D texNoise; //16*16 texture

sampler2D SamplerNoise = sampler_state
{
	Texture   = <texNoise>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	AddressU  = Wrap;
	AddressV  = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};


struct VS_INPUT
{
	float3	pos : POSITION;
	float2	txcoord0 : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4	pos : POSITION;
	float2	txcoord0 : TEXCOORD0;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT	VS_Draw(VS_INPUT IN)
{
    VS_OUTPUT OUT;

	float4	pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	OUT.pos=pos;
	OUT.txcoord0=IN.txcoord0;

    return OUT;
}

#include "AmbientLight.fx"
//Render target is 256*256 hdr texture with sky from front drawed to it.
//If this shader is valid and loaded, render target will be overwritten.
float4	PS_Draw(VS_OUTPUT IN, float2 vPos : VPOS) : COLOR
{
	float4 res = 1.0;
	float2 texcoord = IN.txcoord0.xy;
	
	float theta = texcoord.y * 3.1415926536;
	float phi = texcoord.x * 6.2831853071;

	float3 camdirection = float3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
	       camdirection = normalize(camdirection);

	float3 camdirfixed = camdirection;
	       camdirfixed.z = max(camdirection.z,0.0);
	       camdirfixed = normalize(camdirfixed.xyz);

	res.xyz = GetAtmosphericScattering2(camdirfixed.xyz, SunDirection.xyz, GameTime, 1.0, 0.25, 4.0, 1.3, 0.9, 0.5);
	
	float sa = WeatherSetts(0.25, 0.95);
	res.xyz *= lerp(1.0, smoothstep(-0.53, 0.05,camdirection.z), sa);

	res.w = 1.0;//1

	return res;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

technique Draw
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Draw();
	PixelShader  = compile ps_3_0 PS_Draw();
	}
}


