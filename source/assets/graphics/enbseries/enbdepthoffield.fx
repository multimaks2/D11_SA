
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//      /XXXXXXX\          __                  |HHHHHHHHH\                                          \HH\     /HH/    // 
//     |XX|   |XX|         XX                  |HH|    \HHH\   /\                                    \HH\   /HH/     //
//     |XX|    |X|        /XX\                 |HH|      \HH\  \/                                     \HH\ /HH/      //
//      \XX\             /XXXX\                |HH|       |HH| __ __  __     ___     ___      __       \HHHHH/       //
//       \XX\           /XX/\XX\               |HH|       |HH| || || /__\   /___\   /___\   __||__      \HHH/        //
//     _   \XX\        /XX/  \XX\              |HH|       |HH| || ||//  \\ //   \\ //   \\ |==  ==|     /HHH\        //
//    |X|    \XX\     /XXXXXXXXXX\             |HH|       |HH| || ||/      ||===|| ||    -    ||       /HHHHH\       //
//    \XX\    |XX|   /XX/      \XX\            |HH|      /HH/  || ||       ||      ||         ||      /HH/ \HH\      // 
//     \XX\   /XX/  /XX/        \XX\  ________ |HH|    /HHH/   || ||       \\    _ \\    _    ||  _  /HH/   \HH\     //
//      \XXXXXXX/  /XX/          \XX\|________||HHHHHHHHH/     || ||        \===//  \===//    \\=// /HH/     \HH\    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++            ENBSeries effect file              ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++      SA_DirectX by Maxim Dubinov(Makarus)     ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++       Visit http://www.vk.com/sadirectx       ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++    Visit http://www.facebook.com/sadirectx    ++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++    https://www.youtube.com/channel/UCrASy-x5DgwHpYiDv41RL2Q    ++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++          Visit http://enbdev.com              ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++    Copyright (c) 2007-2021 Boris Vorontsov    ++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


//+++++++++++++++++++++++++++++
//external parameters, do not modify
//+++++++++++++++++++++++++++++
//keyboard controlled temporary variables (in some versions exists in the config file). Press and hold key 1,2,3...8 together with PageUp or PageDown to modify. By default all set to 1.0
float4	tempF1; //0,1,2,3
float4	tempF2; //5,6,7,8
float4	tempF3; //9,0
//x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
float4	ScreenSize;
//x=generic timer in range 0..1, period of 16777216 ms (4.6 hours), w=frame time elapsed (in seconds)
float4	Timer;
//changes in range 0..1, 0 means that night time, 1 - day time
float	ENightDayFactor;
//changes 0 or 1. 0 means that exterior, 1 - interior
float	EInteriorFactor;
//.x - current weather index, .y - incoming weather index, .z - weather transition, .w - time of the day in 24 standart hours. Weather index is value from _weatherlist.ini, for example WEATHER002 means index==2, but index==0 means that weather not captured.
float4	WeatherAndTime;
//adaptation delta time for focusing
float	FadeFactor;
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

float4	PrevMatrixVPRotation[4];
float4	PrevMatrixInverseVPRotation[4];
float4	PrevCameraPosition;

float	EnableDepthOfField; //1.0 if enabled
float	EnableMotionBlur; //1.0 if enabled


//textures
texture2D texOriginal; //always first pass input texture
texture2D texColor; //result of each previous pass
texture2D texDepth;
texture2D texNormal;
texture2D texEnv;
texture2D texNoise;
texture2D texPalette;
texture2D texFocus; //computed focusing depth
texture2D texCurr; //4*4 texture for focusing
texture2D texPrev; //4*4 texture for focusing

sampler2D SamplerOriginal = sampler_state
{
	Texture   = <texOriginal>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;//NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerColor = sampler_state
{
	Texture   = <texColor>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;//NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerDepth = sampler_state
{
	Texture   = <texDepth>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerNormal = sampler_state
{
	Texture   = <texNormal>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;//NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerEnv = sampler_state
{
	Texture   = <texEnv>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

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

sampler2D SamplerPalette = sampler_state
{
	Texture   = <texPalette>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;//NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

//for focus computation
sampler2D SamplerCurr = sampler_state
{
	Texture   = <texCurr>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;//NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

//for focus computation
sampler2D SamplerPrev = sampler_state
{
	Texture   = <texPrev>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};
//for dof only in PostProcess techniques
sampler2D SamplerFocus = sampler_state
{
	Texture   = <texFocus>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

struct VS_OUTPUT_POST
{
	float4 vpos  : POSITION;
	float2 txcoord : TEXCOORD0;
};

struct VS_INPUT_POST
{
	float3 pos  : POSITION;
	float2 txcoord : TEXCOORD0;
};



////////////////////////////////////////////////////////////////////
//begin focusing code
////////////////////////////////////////////////////////////////////
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT_POST VS_Focus(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;

	float4 pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	OUT.vpos=pos;
	OUT.txcoord.xy=IN.txcoord.xy;

	return OUT;
}



//SRCpass1X=ScreenWidth;
//SRCpass1Y=ScreenHeight;
//DESTpass2X=4;
//DESTpass2Y=4;
float4 PS_ReadFocus(VS_OUTPUT_POST IN) : COLOR
{
	float2 uvsrc=IN.txcoord.xy;

	const float2 offset[4]=
	{
		float2(0.0, 0.3),
		float2(0.0, -0.3),
		float2(0.3, 0.0),
		float2(-0.3, 0.0)
	};

	float res=tex2D(SamplerDepth, uvsrc).x;
	for (int i=0; i<4; i++)
	{
		uvsrc.xy=offset[i];
		uvsrc.xy+=IN.txcoord;
		res+=tex2D(SamplerDepth, uvsrc).x;
	}
	res*=0.2;

	return res;
}



//SRCpass1X=4;
//SRCpass1Y=4;
//DESTpass2X=4;
//DESTpass2Y=4;
float4 PS_WriteFocus(VS_OUTPUT_POST IN) : COLOR
{
	float2 uvsrc=0.0;//IN.txcoord0.xy;

	//const float pixsizex=4.0;
//	uvsrc.xy-=(0.5+0.0625)/4.0;
//	const float step=0.125/4.0;
	float	pos=0.0001;
	float	res=0.0;
	float	curr=0.0;
	float	prev=0.0;
	float	currmax=0.0;
	float	prevmax=0.0;
	for (int ix=0; ix<16; ix++)
	{
		float	tcurr=tex2D(SamplerCurr, (uvsrc.xy*0.25+0.125)).x;//the same for all 4 pixels
		float	tprev=tex2D(SamplerPrev, (uvsrc.xy*0.25+0.125)).x;//the same for all 4 pixels
		currmax=max(currmax, tcurr);
		prevmax=max(prevmax, tprev);
		curr+=tcurr;
		prev+=tprev;
		uvsrc.x+=1.0;
		if (uvsrc.x>4.0)
		{
			uvsrc.x=0.0;
			uvsrc.y+=1.0;
		}
	}
	curr*=0.0625;
	prev*=0.0625;

	res=lerp(prev, curr, saturate(FadeFactor));//time elapsed factor
	res=max(res, 0.0);

	return res;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
technique ReadFocus
{
	pass P0
	{
		VertexShader = compile vs_3_0 VS_Focus();
		PixelShader  = compile ps_3_0 PS_ReadFocus();

		ZEnable=FALSE;
		CullMode=NONE;
		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		FogEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}



technique WriteFocus
{
	pass P0
	{
		VertexShader = compile vs_3_0 VS_Focus();
		PixelShader  = compile ps_3_0 PS_WriteFocus();

		ZEnable=FALSE;
		CullMode=NONE;
		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		FogEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}
////////////////////////////////////////////////////////////////////
//end focusing code
////////////////////////////////////////////////////////////////////



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT_POST VS_BypassMB(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;

	float4	pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	if (EnableMotionBlur!=0.0) pos.xy=10000.0;

	OUT.vpos=pos;
	OUT.txcoord.xy=IN.txcoord.xy;

	return OUT;
}



float4 PS_Bypass(VS_OUTPUT_POST IN) : COLOR
{
	float4	res=tex2D(SamplerColor, IN.txcoord.xy);
	return res;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT_POST VS_PostProcess(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;

	float4	pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	if (EnableMotionBlur==0.0) pos.xy=10000.0;

	OUT.vpos=pos;
	OUT.txcoord.xy=IN.txcoord.xy;

	return OUT;
}



float	FuncMasking(float4 uv)
{
	float	mask=tex2Dlod(SamplerNormal, uv).w;
	if ((mask>=(50.0/255.0)) && (mask!=1.0))
		mask=0.0;
	else mask=1.0;
	return mask;
}



float4 PS_ProcessPass2(VS_OUTPUT_POST IN, float2 vPos : VPOS, uniform float seed) : COLOR
{
	float4	res;
	float4	coord;
	coord.xy=IN.txcoord.xy;
	coord.zw=0.0;

	float4	color=tex2D(SamplerColor, IN.txcoord.xy);
	color.xyz*=color.xyz; //gamma 2

	float	scenedepth=tex2D(SamplerDepth, IN.txcoord.xy).x;

	float4	tempvec;
	float4	temppos;
	float4	worldpos;
	float4	prevworldpos;
	float4	prevscreen;
	float4	currscreen;
	currscreen.xy=IN.txcoord.xy;

	//position of pixel
	tempvec.xy=IN.txcoord.xy*2.0-1.0;
	tempvec.y=-tempvec.y;
	tempvec.z=scenedepth;
	tempvec.w=1.0;

	//screen to world position
	worldpos.x=dot(tempvec, MatrixInverseVPRotation[0]);
	worldpos.y=dot(tempvec, MatrixInverseVPRotation[1]);
	worldpos.z=dot(tempvec, MatrixInverseVPRotation[2]);
	worldpos.w=dot(tempvec, MatrixInverseVPRotation[3]);
	worldpos.xyz/=worldpos.w;

	//reduce masking if distance is too big
	float	distfact=dot(worldpos.xyz, worldpos.xyz);
	distfact=saturate(distfact*0.00001);
	distfact*=distfact;

	worldpos.xyz+=CameraPosition;

	//screen to prev world position
	prevworldpos.x=dot(tempvec, PrevMatrixInverseVPRotation[0]);
	prevworldpos.y=dot(tempvec, PrevMatrixInverseVPRotation[1]);
	prevworldpos.z=dot(tempvec, PrevMatrixInverseVPRotation[2]);
	prevworldpos.w=dot(tempvec, PrevMatrixInverseVPRotation[3]);
	prevworldpos.xyz/=prevworldpos.w;

	prevworldpos.xyz+=PrevCameraPosition;

	//modify blurring distance
	float3	dir;
	dir=(worldpos.xyz-prevworldpos.xyz);
	dir=dir/Timer.w * 0.008;
	//limit maximal blurring range
	
	float	dirlen=length(dir.xyz);
	float	dirlimit=5.0;//pow(tempF1.x,4);
	if (dirlen>dirlimit) dir.xyz*=dirlimit / max(dirlen, 0.0000000000001);
	
	prevworldpos.xyz=prevworldpos.xyz+5.0*dir.xyz;


	//prev world to screen
	tempvec.xyz=prevworldpos.xyz;
	tempvec.w=1.0;
	prevscreen.x=dot(tempvec, MatrixVP[0]);
	prevscreen.y=dot(tempvec, MatrixVP[1]);
	prevscreen.z=dot(tempvec, MatrixVP[2]);
	prevscreen.w=dot(tempvec, MatrixVP[3]);
	prevscreen.xyz/=prevscreen.w;
	prevscreen.xy=prevscreen.xy*float2(0.5,-0.5) + 0.5;

	prevscreen.z=1.0/max(1.0-prevscreen.z, 0.0000000000001);
	currscreen.z=1.0/max(1.0-scenedepth, 0.0000000000001);


	float	mask;
	mask=FuncMasking(coord);

	float2	jitteruv;
	jitteruv=frac((vPos.xy+seed)*0.0625)+0.03125;//16 pixels
	float4	jitter=tex2D(SamplerNoise, jitteruv*10.0);

	float	step=0.02;
	float	shift=step*jitter.y;

	float	weight=1.0;
	int		i=0;//to fix compiler bug, add new condition
	
	[fastopt]
	while ((shift<1.0) && (i<32))
	{
		i++;
		float	tempmask;
		float4	tempcolor;
		float	tempweight;
		coord.xyz=lerp(currscreen.xyz, prevscreen.xyz, shift);

		tempmask=FuncMasking(coord);

		if (mask==0.0)
		{
			float	tempdepth=tex2Dlod(SamplerDepth, coord).x;
			tempdepth=1.0/max(1.0-tempdepth, 0.0000000000001);
			//if (tempdepth>coord.z) tempmask=0.0;
			tempmask*=saturate((coord.z-tempdepth)*1000.0);
		}

		tempmask=saturate(tempmask+distfact); //blur everything at high distance

		shift+=step;

		//reduce some artifacts when outside of screen
		float2	uvfact=saturate(-coord.xy)+saturate(coord.xy-1.0);
		uvfact.x+=uvfact.y;
		tempmask=saturate(tempmask - uvfact.x*100000.0*0.1);

		tempcolor=tex2Dlod(SamplerColor, coord);
		tempcolor.xyz*=tempcolor.xyz; //gamma 2
		//v1 constant blurring
		color+=tempcolor * tempmask;
		weight+=tempmask;
		//v2 fading out blurring
	//	tempweight=(1.1-shift) * tempmask;
	//	color+=tempcolor * tempweight;
	//	weight+=tempweight;
	}
	color/=weight;
	color.xyz=pow(color.xyz, 0.5); //gamma 2 restore
	res.xyz=color;

	res.w=1.0;
	return res;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//just low quality motion blur without depth of field
technique PostProcess
{
	pass p0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_ProcessPass2(0.0);

		ZEnable=FALSE;
		CullMode=NONE;
		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		StencilEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
	//this pass is active if motion blur is disabled
	pass p1
	{
		VertexShader = compile vs_3_0 VS_BypassMB();
		PixelShader  = compile ps_3_0 PS_Bypass();

		ZEnable=FALSE;
		CullMode=NONE;
		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		StencilEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}




