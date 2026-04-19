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
	float4	diff : COLOR0;
};

struct VS_OUTPUT
{
	float4	pos : POSITION;
	float4	diff : COLOR0;
	float4	vposition : TEXCOORD6;
};



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT	VS_Draw(VS_INPUT IN)
{
    VS_OUTPUT OUT;

	float4	pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	float4	tpos=mul(pos, MatrixWVP);

	OUT.diff=IN.diff;
	OUT.pos=tpos;
	OUT.vposition=tpos;

    return OUT;
}



float4	PS_Draw(VS_OUTPUT IN, float2 vPos : VPOS) : COLOR
{
	float4	res;
/*
	//helper code to get screen texture coordinates
	float2	uv=vPos.xy*ScreenSize.y;
	uv.y*=ScreenSize.z;
	//get position relative to camera
	//depth for sky is 1.0 always. you can use real mesh depth, but it's just a part of cube rotating around the camera and close to it
	const float	depth=1.0;
	float4	tempvec;
	tempvec.xy=uv.xy*2.0-1.0;
	tempvec.y=-tempvec.y;
	tempvec.z=depth;
	tempvec.w=1.0;
	tempvec=mul(tempvec, MatrixWVPInverse);
	float3	worldposition=tempvec.xyz/tempvec.w;
	float3	worlddirection=normalize(tempvec.xyz-CameraPosition.xyz);
*/
	//Sky have fog applied as color of vertices IN.diff, but this fog is vertical, while game
	//world use distant fog. You should apply own fog here to mix to game world distant fog,
	//probably bottom of the sky as 0.0 depth and middle as 1.0 will be okay for computing fog

	res=IN.diff; //only diffuse color from vertices used by game

	//dither, because night sky is very buggy
	float2	jitteruv;
	jitteruv=frac(vPos.xy*0.0625)+0.03125;//16 pixels
	float4	jitter=tex2D(SamplerNoise, jitteruv);
	const float	ditheramount=0.008;
	res.xyz+=(-jitter.x*ditheramount) + (ditheramount*0.5);
	res.xyz=max(res.xyz, 0.0);

	//output alpha channel for masking sun which drawed later. If alpha is 0 sun not drawed,
	//so you can make clouds or something like that to cover sun
	res.w=1.0;

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

	AlphaBlendEnable=FALSE; //write alpha to mask sun
//	AlphaBlendEnable=TRUE;//used by game
//	SrcBlend=SRCALPHA;
//	DestBlend=INVSRCALPHA;
	AlphaTestEnable=FALSE;
	}
}


