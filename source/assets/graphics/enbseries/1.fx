
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
//++++++++++++++++++++++++++++++++++    Visit http://www.facebook.com/sadirectx    ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++       Visit http://www.vk.com/sadirectx       ++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++    https://www.youtube.com/channel/UCrASy-x5DgwHpYiDv41RL2Q    ++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++          Visit http://enbdev.com              ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++    Copyright (c) 2007-2021 Boris Vorontsov    ++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

float4 tempF1; float4 tempF2; float4 tempF3;
float4 ScreenSize; float4 Timer;float ENightDayFactor;
float4 SunDirection; float EInteriorFactor; float FadeFactor; float FieldOfView;
float4 MatrixVP[4]; float4 MatrixInverseVP[4]; float4 MatrixVPRotation[4]; float4 MatrixInverseVPRotation[4];
float4 MatrixView[4];float4 MatrixInverseView[4];float4 CameraPosition;float GameTime;float4 CustomShaderConstants1[8];
float4 WeatherAndTime; float4 FogFarColor; float4 FogParam;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Textures
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

texture2D texEnv;
texture2D texOriginal;
texture2D texColor;
texture2D texDepth;
texture2D texDepthMipMap;
texture2D texNoise;
texture2D texShadow;
texture2D texPalette;
texture2D texNormal;
texture2D texPd < string ResourceName="NoisePd.png"; >;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Sampler Inputs
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
texture2D texNormalRain < string ResourceName = "RainNormal.png"; >;
texture2D texRipples < string ResourceName="Ripples.png"; >;
texture2D texAlpha < string ResourceName="RipplesAlpha.png"; >;
texture2D texAlpha1 < string ResourceName="RipplesAlpha1.png"; >;
texture2D texRain < string ResourceName="Rain.png"; >;
texture2D texRain2 < string ResourceName="Rain2.png"; >;
texture2D texwrl < string ResourceName="PuddlesRelief.png"; >;

texture2D texenbnoise < string ResourceName="enbnoise.png"; >;
//---------------------------------------------------------------------
sampler2D SamplerENBNoise = sampler_state
{
   Texture = <texenbnoise>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=0;
	MipMapLodBias=0;
}; 

sampler2D SamplerRain = sampler_state
{
   Texture = <texRain>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=0;
	MipMapLodBias=0;
}; 

sampler2D SamplerRain2 = sampler_state
{
   Texture = <texRain2>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerWrl = sampler_state
{
	Texture = <texwrl>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=1;
	MipMapLodBias=0;
};

sampler2D SamplerWrp = sampler_state
{
	Texture = <texRipples>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=1;
	MipMapLodBias=0;
};

sampler2D SamplerAlpha = sampler_state
{
	Texture = <texAlpha>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=1;
	MipMapLodBias=0;
};

sampler2D SamplerAlpha1 = sampler_state
{
	Texture = <texAlpha1>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=TRUE;
	MaxMipLevel=1;
	MipMapLodBias=0;
};


sampler2D NormalRainSampler = sampler_state 
{
	Texture = <texNormalRain>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerPuddle = sampler_state
{
   Texture   = <texPd>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D InputSampler = sampler_state
{
    Texture = (texColor);
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU   = Clamp;
	AddressV   = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerEnv = sampler_state
{
	Texture   = <texEnv>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerOriginal = sampler_state
{
	Texture   = <texOriginal>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;//NONE;
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

sampler2D SamplerColor = sampler_state
{
	Texture   = <texColor>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
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


sampler2D SamplerDepthM = sampler_state
{
	Texture   = <texDepthMipMap>;
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
	MaxMipLevel=2;
	MipMapLodBias=0;
};

sampler2D SamplerShadow = sampler_state
{
	Texture   = <texShadow>;
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
	float4 vpos  : SV_POSITION;
	float2 txcoord : TEXCOORD0;
};

struct VS_INPUT_POST
{
	float3 pos  : POSITION;
	float2 txcoord : TEXCOORD0;
};

#include "AtmosphericScattering.fx"
#include "Clouds2D.fx"
#include "VolumetricClouds.fx"

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

VS_OUTPUT_POST VS_PostProcess(VS_INPUT_POST v)
{
	VS_OUTPUT_POST dx;
	float4 pos = float4(v.pos.x,v.pos.y,v.pos.z,1.0);
	dx.vpos = pos;
	dx.txcoord.xy = v.txcoord.xy;
	
	return dx;
}

VS_OUTPUT_POST VS_PostProcess2(VS_INPUT_POST v)
{
	VS_OUTPUT_POST dx;
	float4 pos = float4(v.pos.x,v.pos.y,v.pos.z,1.0);
	dx.vpos = pos;
	dx.txcoord.xy = v.txcoord.xy;
	
	return dx;
}

VS_OUTPUT_POST VS_PostProcess3(VS_INPUT_POST v)
{
	VS_OUTPUT_POST dx;
	float4 pos = float4(v.pos.x,v.pos.y,v.pos.z,1.0);

	dx.vpos = pos;
	dx.txcoord.xy = v.txcoord.xy;
	
	return dx;
}

struct VS_OUTPUT_POST_FXAA 
{
	float4 vpos : SV_POSITION;
	float2 txcoord : TEXCOORD0;
	float4 interpolatorA : TEXCOORD1;
	float4 interpolatorB : TEXCOORD2;
	float4 interpolatorC : TEXCOORD3;
};

float2 texel = {0.0009765625,0.00130208333333333333333333333333};

VS_OUTPUT_POST_FXAA VS_PostProcessFXAA (VS_INPUT_POST v)
{
	VS_OUTPUT_POST_FXAA o;
	float4 pos=float4(v.pos.x,v.pos.y,v.pos.z,1.0);
	o.vpos=pos;
	
	o.txcoord = v.txcoord.xy;
	
	float4 extents;
	float2 offset = ( texel.xy ) * 0.215;
	extents.xy = v.txcoord.xy - offset;
	extents.zw = v.txcoord.xy + offset;

	float4 rcpSize;
	rcpSize.xy = -texel.xy * 0.5;
	rcpSize.zw = texel.xy * 0.7;			
	
	o.interpolatorA = extents;
	o.interpolatorB = rcpSize;
	o.interpolatorC = rcpSize;
	
	o.interpolatorC.xy *= 0.0;
	o.interpolatorC.zw *= 0.0;
	
	return o;
}

float Luminance( float3 c )
{
	return dot( c, float3(0.22, 0.707, 0.071) );
}

#define FxaaTexTop(t, p) tex2Dlod(t, float4(p, 0.0, 0.0))

inline float TexLuminance( float2 uv )
{
	return Luminance(FxaaTexTop(InputSampler, uv).rgb);
}

float3 FxaaPixelShader(float2 pos, float4 extents, float4 rcpSize, float4 rcpSize2)
{
	float lumaNw = TexLuminance(extents.xy);
	float lumaSw = TexLuminance(extents.xw);
	float lumaNe = TexLuminance(extents.zy);
	float lumaSe = TexLuminance(extents.zw);
	
	float3 centre = FxaaTexTop(InputSampler, pos).rgb;
	float lumaCentre = Luminance(centre);
	
	float lumaMaxNwSw = max( lumaNw , lumaSw );
	lumaNe += 1.0/384.0;
	float lumaMinNwSw = min( lumaNw , lumaSw );
	
	float lumaMaxNeSe = max( lumaNe , lumaSe );
	float lumaMinNeSe = min( lumaNe , lumaSe );
	
	float lumaMax = max( lumaMaxNeSe, lumaMaxNwSw );
	float lumaMin = min( lumaMinNeSe, lumaMinNwSw );
	
	float lumaMaxScaled = lumaMax * 0.0;
	
	float lumaMinCentre = min( lumaMin , lumaCentre );
	float lumaMaxScaledClamped = max( 0.0 , lumaMaxScaled );
	float lumaMaxCentre = max( lumaMax , lumaCentre );
	float dirSWMinusNE = lumaSw - lumaNe;
	float lumaMaxCMinusMinC = lumaMaxCentre - lumaMinCentre;
	float dirSEMinusNW = lumaSe - lumaNw;
	
	if(lumaMaxCMinusMinC < lumaMaxScaledClamped)
		return centre;
	
	float2 dir;
	dir.x = dirSWMinusNE + dirSEMinusNW;
	dir.y = dirSWMinusNE - dirSEMinusNW;
	
	dir = normalize(dir);			
	float3 col1 = FxaaTexTop(InputSampler, pos.xy - dir * rcpSize.zw).rgb;
	float3 col2 = FxaaTexTop(InputSampler, pos.xy + dir * rcpSize.zw).rgb;
	
	float dirAbsMinTimesC = min( abs( dir.x ) , abs( dir.y ) ) * 1000.0;
	dir = clamp(dir.xy/dirAbsMinTimesC, -2.0, 2.0);
	
	float3 col3 = FxaaTexTop(InputSampler, pos.xy - dir * rcpSize2.zw).rgb;
	float3 col4 = FxaaTexTop(InputSampler, pos.xy + dir * rcpSize2.zw).rgb;
	
	float3 rgbyA = col1 + col2;

	float3 rgbyB = ((col3 + col4) * 0.25) + (rgbyA * 0.25);

	if((Luminance(rgbyA) < lumaMin) || (Luminance(rgbyB) > lumaMax))
		return rgbyA * 0.5;
	else
		return rgbyB;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 	GWP(in float2 c, in float d)
{
   float4 t; 
   float4 w;
   t.xy=c.xy*2.0-1.0;
   t.y=-t.y;
   t.z=d;
   t.w=1.0;
   w.x=dot(t, MatrixInverseVPRotation[0]);
   w.y=dot(t, MatrixInverseVPRotation[1]);
   w.z=dot(t, MatrixInverseVPRotation[2]);
   w.w=dot(t, MatrixInverseVPRotation[3]);
   w.xyz/=w.w;
   //w.xyz+=CameraPosition;
   return w;
}

float4 	wp2(float2 txcoord)
{
   float  depth = tex2D(SamplerDepth, txcoord.xy).x;
   float4 tvec; 
          tvec.xy = txcoord.xy*2.0-1.0;   
          tvec.y = -tvec.y;   
          tvec.z = depth;
          tvec.w = 1.0;
   float4 wpos;
          wpos.x = dot(tvec, MatrixInverseVP[0]);
          wpos.y = dot(tvec, MatrixInverseVP[1]);
          wpos.z = dot(tvec, MatrixInverseVP[2]);
          wpos.w = dot(tvec, MatrixInverseVP[3]);
          wpos.xyz/=wpos.w;
          wpos.w=1.0;
          wpos.z=dot(wpos, MatrixVP[2]);
   return wpos;
}

float4 NormalPos(float2 txcoord, float depth)
{
	float4 tv0;
	       tv0.xy = txcoord.xy*2.0-1.0;
	       tv0.y = -tv0.y;
	       tv0.z = depth;
	       tv0.w = 1.0;
		   
	float4 n = tex2D(SamplerNormal, txcoord.xy);
	       n.xyz = n.xyz*2.0-1.0;
 		   
	       tv0.xyz = n.xyz;
	       tv0.x = -tv0.x;
	       tv0.w = 1.0;
	
	float4 np0;	
	       np0.x = dot(tv0.xyz, MatrixInverseView[0]);
	       np0.y = dot(tv0.xyz, MatrixInverseView[1]);
	       np0.z = dot(tv0.xyz, MatrixInverseView[2]);
	       np0.xyz = normalize(np0.xyz);
		   		   
	return np0;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float LinDepth(float2 coord)
{
    float d0 = tex2Dlod(SamplerDepth, float4(coord.xy, 0.0, 0.0)).x;  
    float d1 = (99.01*1.0) / (99.01 + d0 * (1.0 - 100.0));
   return d1;
}

float3 getNormal(float2 coord, float2 offset, float p)
{
	float3 of = float3(offset, 0);
    float x = dot(tex2Dlod(SamplerColor, float4(coord+of.xz,0,0)).xyz, 0.333)*p,
          y = dot(tex2Dlod(SamplerColor, float4(coord+of.zy,0,0)).xyz, 0.333)*p;
	
    float x1 = dot(tex2Dlod(SamplerColor, float4(coord-of.xz,0,0)).xyz, 0.333)*p,
          y1 = dot(tex2Dlod(SamplerColor, float4(coord-of.zy,0,0)).xyz, 0.333)*p;

    float dx = LinDepth(coord + of.xz),
          dy = LinDepth(coord + of.zy);
	
    float dx1 = LinDepth(coord - of.xz),
          dy1 = LinDepth(coord - of.zy);
 
    float2 m = saturate(1.0-float2(abs(dx1-dx), abs(dy1-dy)));

    float3 n;
           n.xy = float2(x1-x, y1-y)*m/offset.xy*0.5;
           n.z = 1.0;

    return normalize(n);       
}

float3 NormalBlend(float3 n1, float3 n2)
{   
	return normalize(float3(n1.xy + n2.xy, n1.z));

}

float CarGam
<
	string UIName="Reflection: Gamma";
	string UIWidget="Spinner";
	float UIMin=0.5;
	float UIMax=2.0;
> = {1.1};

float CarREF
<
	string UIName="SSR: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.4;
	float UIMax=2.0;
> = {0.80};

float skyREF
<
	string UIName="SkyReflection: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.4;
	float UIMax=2.0;
> = {1.05};

float FullMaskCar(float2 uv)
{
	float	mask=tex2Dlod(SamplerNormal, float4(uv, 0.0, 0.0)).w;
	if ((mask>=(50.0/255.0)) && (mask!=1.0))
		mask=0.0;
	else mask=1.0;
	return mask;
}

float maskCar(float2 txcoord)
{
	float4 mcar = tex2D(SamplerNormal, txcoord.xy).w;
       if (mcar.w<0.9995)mcar = 0.0;
    float  m1 = saturate(mcar*2.0);
	       m1 = lerp(0.0, 1.0, m1);
		   m1 = saturate(m1*m1);
    return m1;
}

float maskCar1(float2 txcoord)
{
	float4 mcar = tex2D(SamplerNormal, txcoord.xy).w;
       if (mcar.w<0.995)mcar = 0.0;
    float  m1 = saturate(mcar*2.0);
	       m1 = lerp(0.0, 1.0, m1);
		   m1 = saturate(m1*m1);
    return m1;
}

float maskPed(float2 txcoord)
{
	float4 mped = tex2D(SamplerNormal, txcoord.xy).w;
	   if (mped.w==253/255.0)mped = 0.0;
	float  m2 = saturate(mped*6.0);		   
           m2 = lerp(1.0, 0.0, m2);
		   m2 = saturate(m2*m2);	
    return m2;		  
}

float maskWater(float2 txcoord)
{
	float4 mwater = tex2D(SamplerNormal, txcoord.xy).w;
       if ( mwater.w<0.7) mwater = 0.0;
    float  m3 = saturate(mwater*2.0);
	       m3 = lerp(1.0, 0.0, m3);
		   m3 = saturate(m3*m3);		   
    return m3;			  
}

float3 tonemp(float3 color)
{
	float3 n0 = normalize(color.xyz);
	float3 ct0=color.xyz/n0.xyz;
	       ct0=pow(ct0, 1.15);
           n0.xyz = pow(n0.xyz, 0.85);   
           color.xyz = ct0*n0.xyz;  
           color.xyz*= 1.4;

    float maxcol = max(color.x, max(color.y, color.z));		 		 
    float3 r0 = 1.0-exp(-maxcol),
           r2 = 1.0-exp(-color);	

         color.xyz = lerp(float3(r2.x, r2.y, r2.z), color*float3(r0.x, r0.y, r0.z)/maxcol, 0.3);		 
  return color;	
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float3 wPos(float2 uv)
{	
    float d = tex2Dlod(SamplerDepth, float4(uv.xy, 0.0, 0.0)).x; 	
    float s;   
    if( 1000.0 < 0.01 || d == 1)
     {
	     s = 1.0;
     }	
	     s = 1-s;
		 
	float4 t;
	       t.xy = uv.xy*2.0-1.0;
	       t.y = -t.y;
	       t.z = d*s;
	       t.w = 1.0;
	float4 w;
	       w.x = dot(t, MatrixInverseVPRotation[0]);
	       w.y = dot(t, MatrixInverseVPRotation[1]);
	       w.z = dot(t, MatrixInverseVPRotation[2]);
	       w.w = dot(t, MatrixInverseVPRotation[3]);
	       w.xyz/= w.w;
	return w.xyz;
}

float3 sPos(float3 w)
{
	float4 t;	
	       t.xyz = w;
	       t.w=1.0;	
	float4 s;
	       s.x = dot(t, MatrixVPRotation[0]);
	       s.y = dot(t, MatrixVPRotation[1]);
	       s.z = dot(t, MatrixVPRotation[2]);
	       s.w = dot(t, MatrixVPRotation[3]);
	       s.xyz/= s.w;
	       s.y = -s.y;
	       s.xy = s.xy * 0.5 + 0.5;
	return s.xyz;
}

float3 Config2(float3 c1, float3 c2, float3 c3, float3 c4, float3 c5e, float3 c5, float3 c6, float3 c7, float3 c8, float3 c9)
{
   float t0 = GameTime;		
   float x1 = smoothstep(0.0, 3.0, t0);
   float x2 = smoothstep(3.0, 4.0, t0);
   float x3 = smoothstep(4.0, 6.0, t0);
   float x4 = smoothstep(6.0, 7.0, t0);
   float x5e = smoothstep(8.0, 11.0, t0);
   float x5 = smoothstep(16.0, 17.0, t0);
   float x6 = smoothstep(18.0, 19.0, t0);
   float x7 = smoothstep(19.0, 20.0, t0);
   float x8 = smoothstep(22.0, 23.0, t0);
   float x9 = smoothstep(23.0, 24.0, t0);

   
   float3 t1 = lerp(c1, c1, x1); // Ночь
          t1 = lerp(t1, c2, x2); // Ночь 2
          t1 = lerp(t1, c3, x3); // Утро 3
          t1 = lerp(t1, c4, x4); // Утро 4
          t1 = lerp(t1, c5e, x5e); // День 5
          t1 = lerp(t1, c5, x5); // День 5
          t1 = lerp(t1, c6, x6); // Вечер 6	 
          t1 = lerp(t1, c7, x7); // Вечер 7
          t1 = lerp(t1, c8, x8); // Вечер 8
          t1 = lerp(t1, c9, x9); // Вечер 9		  
   return t1;
}

float3 ComputeRipple(float2 uv, float CurrentTime, float Weight)
{
    float4 rp = tex2D(SamplerWrp, uv)*1.4;
           rp.yz = rp.yz * 2.0-1.0;	
    float3 a0 = tex2D(SamplerAlpha, uv).rgb*1.4;	
	float4 rp3 = lerp(0.0, rp, a0.r);		   	  
    float rp0 = WeatherSetts2(0.0, 2.0, 0.5),
	      DropFrac = frac(rp3.w+CurrentTime),
	      TimeFrac = DropFrac - 1.0 + rp3.x,
	      DropFactor = saturate(0.20 + Weight * 0.80 - DropFrac),
	      PI = 3.14159265359,
	      FF = DropFactor*rp3.x * sin(clamp(TimeFrac * 12.0, 0.0, 5.0) * PI);
	   
	float3 cpl = lerp(0.0, float3(rp3.yz*FF*rp0, 1.0), a0.g);
	return cpl;
}

float3 CR(float3 n, float3 uv)
{
    float Tr = Timer.x * 14000.0;
    float4 TimeMul = float4(0.6, 0.7, 0.8, 1.0); 
    float4 TimeAdd = float4(0.22, 0.44, 0.66, 0.9);
    float4 Times = (Tr * TimeMul + TimeAdd) * 2.4;
           Times = frac(Times);
    float4 Weights = 1.0 - float4(0, 0.25, 0.5, 0.75);
           Weights = saturate(Weights * 4.0);
    float3 Ripple1 = ComputeRipple(uv * 0.18 + float2(0.25, 0.0),  Times.x, Weights.x),
           Ripple2 = ComputeRipple(uv * 0.25 + float2(-0.55, 0.3), Times.y, Weights.y),
           Ripple3 = ComputeRipple(uv * 0.28 + float2(0.6, 0.85),  Times.z, Weights.z),
           Ripple4 = ComputeRipple(uv * 0.30 + float2(0.5, -0.75),  Times.w, Weights.w),
           Ripple5 = ComputeRipple(uv * 0.32 + float2(0.3, -0.42),  Times.w, Weights.w),
           Ripple6 = ComputeRipple(uv * 0.20 + float2(0.35, 0.1),  Times.x, Weights.x);
           n=normalize(float3(n.xy+Ripple1.xy,n.z));
           n=normalize(float3(n.xy+Ripple2.xy,n.z));
           n=normalize(float3(n.xy+Ripple3.xy,n.z));
           n=normalize(float3(n.xy+Ripple4.xy,n.z));	
           n=normalize(float3(n.xy+Ripple5.xy,n.z));
           n=normalize(float3(n.xy+Ripple6.xy,n.z));	
	return n;	
}

float3 ComputeRelief(float2 uv, float CurrentTime, float3 Weight)
{
    float tx = (Timer.x * 1000.0) * 1.05;
    float4 rf = tex2Dlod(SamplerWrl,  float4(uv.xy + float2(0.5, 0.5*tx), 0.0, 0.0));	   
    float RelfSt = step(0.2, rf.a);		   
	float DropFrac = frac(rf.w+CurrentTime);
	float3 DropFactor = saturate(0.2 + Weight * 0.8 - DropFrac);	
	float3 FF = DropFactor * rf.x*RelfSt;
	return float3(rf.yz*FF, 1.0);
}

float3 PR(float3 n, float3 uv)
{
    float3 w = float3(1.2, 1.2, 1.2);
    float tx = (Timer.x * 1000.0) * 1.05;	   
    float3 Relief1 = ComputeRelief(uv * 0.05 + float2(0.55, 0.89),  1.99, w);
    float3 Relief2 = ComputeRelief(uv * 0.25 + float2(0.22, 0.33)*tx,  1.99, w);	
	       n = normalize(float3(n.xy+Relief1.xy,n.z)); 
	       n = normalize(float3(n.xy+Relief2.xy,n.z));
    return n;
}

float AlphaRoads
<
	string UIName="AlphaRoads";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=0.5;
> = {0.17};

float AlphaPuddle
<
	string UIName="AlphaPuddle";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.75};

bool Puddles_alltime
<
         string UIName="Puddles_alltime";	
> = {false};

float Puddles_alltime_coverage
<
        string UIName="Puddles_alltime - coverage";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=1.0;
> = {1.0};

float3 RayM(float2 vpos, float3 wpos, float3 wnorm, float3 sky)
{
	float d3 = dithering(vpos.xy,2)- 3.27;
	float d1 = dithering(vpos.xy,1)- 3.27;	
          d3 = lerp(d3, d1, 0.85);	
          d3 = d3*0.000755 + 0.0025;
		  
	float3 dir = normalize(reflect(normalize(wpos), wnorm));
	float step = d3*sqrt(2000.0*wpos.z);	
	float3 p = wpos+(dir*step);

	int s = 32;
	int n = 9;
	float2 uv = 0.0;
	bool a = 0;	
	int l = 0;
	
	[fastopt]
	for(int i = 0; i < s; i++)		
	{		
	   uv = sPos(p);
	   float3 aPos = wPos(uv.xy);		
       float sError = distance(aPos.xyz, p.xyz);	

	   if(sError < 1000.0 && -sError*17.5 > -17.2*step)
	     {
		    i = 0;		 
			   if(l < n)
			   {
			      step/= 1.44;
			      p -= dir*step;			
			      step*= 1.44*rcp(s)/0.15;
			   }
			   else
			   {		
			      i += s;
			   }
		    l++;			
	     }
		   p += dir*step/1.34;
		   step *= 1.44;
	}	
       a = l > n != 0;

	float4 ssr = 1.0;	
	       ssr.rgb = tex2Dlod(SamplerColor, float4(uv, 0.0, 0.0)).rgb;
	       ssr.w*= a;
	
    float nf1 = saturate(100.0*(uv.y));
          nf1*= saturate(100.0*(uv.x));		 
    float nf2 = saturate(100.0+uv.y*(-100.0));
          nf2*= saturate(100.0+uv.x*(-100.0));
		   
    float3 reflection = lerp(sky*skyREF, ssr.rgb*CarREF, nf1*nf2*ssr.w);	
	return reflection*2.0;
}

float4 PS_SSR(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;	
	float4 color = tex2D(SamplerColor, coord.xy);
	float depth = tex2D(SamplerDepth, coord.xy).x;	
    float m0 = FullMaskCar(coord); // Авто фул + перс	
		
    float4 worldpos = GWP(coord, depth);
    float4 worldnormal = NormalPos(coord, depth);	
	float4 wpos = GWP(coord, depth);	

	float Puddle = tex2D(SamplerPuddle, (worldpos.xy+CameraPosition.xy)*0.012*1.2+0.85);
          Puddle = lerp(0.0, 1-Puddle, saturate(m0));
	float coverage = lerp(1.0, Puddles_alltime_coverage, Puddles_alltime);	
          Puddle = Coverage(coverage, (0.1 - 2.0 * 0.3), Puddle);
          Puddle = pow(Puddle, 1.0);
		  Puddle =lerp(0.0, 1.25, saturate(Puddle));
	
	float3 of = float3(ScreenSize.y,ScreenSize.y * ScreenSize.z,0);	
	float3 normal = getNormal(coord.xy, 1.0*of/worldpos.z * 2.0, 0.001 * Puddle);
          worldnormal.xyz = NormalBlend(worldnormal, normal);	 
          worldnormal.xyz = lerp(worldnormal.xyz, CR(worldnormal.xyz, worldpos.xyz+CameraPosition.xyz), saturate(m0)); 	
	
    float tx = (Timer.x * 1000.0) * 1.05;	
	
	float4 rf = tex2D(SamplerWrl,(worldpos.xy+CameraPosition.xy)* 0.25 * float2(0.8, 1.0)+ tx);
	       rf.xyz = lerp(normalize(float3(0.0, 0.0, 1.0)), normalize(rf.xyz), 0.05);
          worldnormal.xyz = lerp(worldnormal.xyz, NormalBlend(worldnormal.xyz, rf.xyz), saturate(m0));			  
		  
	float3 refl = reflect(normalize(worldpos.xyz), worldnormal.xyz);	
	
	float3 skyColor = GetAtmosphericScattering(refl.xyz, SunDirection.xyz, GameTime, 1.0, 0.0, 1.5, 0.7);
	       skyColor.xyz *= lerp(1.0, smoothstep(-0.02, 0.01, refl.z), 0.85);		   

	float3 w1 = float3(1.0, 1.0, 10.0);    
	float ip = IntersectPlane(w1, refl.xyz);	
	
	float3 sc0 = SunLight(refl.xyz);	
	float4 c0 = GenerateClouds(refl.xyz*ip*3.0, sc0);	
		   c0.a = min(1.0, c0.a);   

           skyColor.xyz = lerp(skyColor.xyz, c0.rgb*1.15, 0.9*c0.a*smoothstep(0.01,-0.02, -refl.z));     
           color.xyz = RayM(vPos, worldpos.xyz, worldnormal.xyz, skyColor.xyz);	
           //color.xyz = skyColor.xyz*2.0;			   
	return color;
}

float DistantFog
<
	string UIName="Distant Fog";
	string UIWidget="Spinner";
	float UIMin=0.1;
	float UIMax=1.5;
> = {1.0};

float GetVolumetricFog(float3 wpos, float2 coord)
{
   float3 worldpos = wpos+CameraPosition; 
   float4 w1 = float4(normalize(wpos.xyz), 1.0);
	      w1.xyz = normalize(worldpos.xyz-float3(CameraPosition.xy, 0.0));	
		   
   float3 vec1 = normalize(float3(0.0, 0.0, 1.0));	  
   float fog = saturate(1.0 - dot(vec1, w1));
	     fog = pow(fog, 8.0);	 

   float p2 = length(wpos.xyz);  
   float3 dp = -1.0/exp(length(0.0-p2)/250.0);  
   float depth = pow(tex2D(SamplerDepth, coord).x, 200*DistantFog);
         depth = lerp(1.0, depth, dp); 		 
   float f0 = pow(depth, -10.0);
		 
  return fog*f0;	
}

// Игровой Туман
float GetFog(float3 wpos)
{
	float	fogdist = wpos.z;
	float	fadefact = (FogParam.w - fogdist) / (FogParam.w - FogParam.z);
	        fadefact = saturate(1.0-fadefact);		  
	        fadefact = lerp(1.0, 0.0, fadefact);
     return fadefact;		
}

float Rain0(in float2 cd)
{	
    float2 uv = cd.yx;
	float2 p = uv/40.0;
	float r = tex2Dlod(SamplerRain2, float4(p*float2(10.0, 2.0), 0.0, 0.0)).y;
	return r;
}

float Rain1(in float2 cd)
{	
    float2 uv = cd.yx;
	float2 p = uv/1.2;
	float r = tex2Dlod(SamplerRain, float4(p*float2(3.5, 0.3), 0.0, 0.0)).y;	
	return r;
}

float Rain2(in float2 cd)
{	
    float2 uv = cd.yx;
	float2 p = uv/0.5;
	float r = tex2Dlod(SamplerRain, float4(p*float2(3.0, 0.5), 0.0, 0.0)).y;
	float r2 = tex2Dlod(SamplerRain2, float4(p*float2(2.0, 0.5), 0.0, 0.0)).y;
	float j = (r+r2);
	return j;
}

float4 PS_DX(VS_OUTPUT_POST_FXAA IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;	
	float3 fxaaRefl = FxaaPixelShader(IN.txcoord, IN.interpolatorA, IN.interpolatorB, IN.interpolatorC);		
    float4 color = tex2D(SamplerColor, coord);
           color.rgb = fxaaRefl; // сглаживание для отражений
		   
	float4 colorOr = tex2D(SamplerOriginal, coord.xy);
	float depth = tex2D(SamplerDepth, coord.xy).x;	

    float4 worldpos = GWP(coord, depth);
    float4 worldnormal = NormalPos(coord, depth);	
	float3 dir = normalize(worldpos.xyz);
	float3 refl = reflect(dir.xyz, worldnormal.xyz);
	float VolFog = GetVolumetricFog(worldpos.xyz, coord);	
// Маски ======================================================
    float m0 = FullMaskCar(coord); // Авто фул + перс
    float m1 = maskCar(coord);   // Авто + перс
    float m1a = maskCar1(coord); // Авто + перс (Glass off)	
    float m2 = maskPed(coord);   // Перс
    float m3 = maskWater(coord); // Вода	 

	float fresnel = 0.001-dot(worldnormal, -dir);		
		  fresnel = saturate(pow(fresnel, 0.37*0.2));
// Фикс SunDirection ========================================== 
   float3 vec = SunDirection.xyz;
   float3 vec0 = float3(0.0, 0.0, 1.03);	   
   float3 s0 = normalize(float3(0.0, 0.0, 0.0));  
   float s1 = smoothstep(0.0, 1.0, GameTime);	
   float s2 = smoothstep(1.0, 23.0, GameTime);
   float s3 = smoothstep(23.0, 24.0, GameTime);  
   float3 vec1 = lerp(s0, vec, s1);
          vec1 = lerp(vec1, vec, s2); 
          vec1 = lerp(vec1, vec, s3);
// ============================================================ 		  
	float3 wpos1 = worldpos.xyz;
	float  wpos2 = length(worldpos.xyz);
	float3 dir2 = worldpos.xyz / wpos2;	
	float3 X2 = normalize(-vec1+dir2); 	
           worldpos.xyz+=CameraPosition.xyz;
// Лужи =======================================================	
	float rh = smoothstep(0.999, 1.0, dot(-vec0, worldnormal.xyz)); // маска для луж
	
	float coverage = lerp(1.0, Puddles_alltime_coverage, Puddles_alltime);          	
	float Puddle = tex2D(SamplerPuddle, worldpos.xy*0.012*1.2+0.85);
          Puddle = lerp(0.0, pow(Puddle*0.9, 0.6), saturate(m0));		
          Puddle = Coverage(coverage, (0.1 - 2.0 * 0.3), Puddle);

	float pd = lerp(0.0, saturate(AlphaRoads + Puddle*AlphaPuddle), saturate(m0));
	float pd2 = lerp(0.0, saturate(Puddle*1.5), saturate(m0));	
	
	      rh*= pd;
	float rh2 = lerp(0.0, rh*pd2, Puddles_alltime);			
	      rh = WeatherSetts2(rh2*1.12, rh, 0.5);
	
    float AlphaCar = lerp(1.0+Puddle, 0.0, saturate(m2+m1+m3));		  		  
	float refLght = lerp(1.2*(1-fresnel) + 0.01, 1.0, AlphaCar);	
// ============================================================ 
		   fresnel = lerp(0.0, 0.97*fresnel, AlphaCar);		 

	       color.rgb = pow(color.rgb, 1.1);	
	float3 n0 = normalize(color.xyz);
	float3 ct0=color.xyz/n0.xyz;
	       ct0=pow(ct0, 1.1);
           n0.xyz = pow(n0.xyz, 1.0);   
           color.xyz = ct0*n0.xyz;  
           color.xyz*= 1.3;		   
           color.rgb = lerp(color*refLght, colorOr, fresnel);
	       color.xyz = tonemp(color.xyz*2.12);	
	       color.xyz = pow(color.xyz, CarGam);		

    float gl = -0.005 + dot(X2, worldnormal.xyz);	
		  gl = pow(gl, 375.0);
	      gl = (gl/0.01)* 0.01;
          gl = saturate(gl)*4.0;
		  
    float g2 = 0.01*0.01 + dot(X2, worldnormal.xyz);	
		  g2 = pow(g2, 1500.0*12.0);
		  g2 = (g2/0.01)* 0.01;
          g2 = saturate(g2)*4.0;

	float4 sh2 = tex2D(SamplerShadow, coord.xy);
	float Masklighting = dot(worldnormal.xyz, vec1.xyz);
	      sh2*= saturate(0.0-Masklighting*6.0);	
	float msh = saturate(sh2);

    float3 specular = WeatherSetts(smoothstep(-0.02, 0.01, refl.z)*(gl+g2)*msh, 0.0);	

           color.rgb = lerp(colorOr.xyz, color+specular, rh+AlphaCar);

    float4 wpx = wp2(coord);		 
    float fadefact = GetFog(wpx.xyz);
     
    float3 wpos = normalize(wpos1.xyz);
//-------------------------------------------------------
// Stars
//-------------------------------------------------------
    float3 p0 = (MatrixInverseVP[3].xyz/MatrixInverseVP[3].w);		  
    float3 ns1 = wpos.xyz;
           ns1.z = abs(ns1.z)*1.40;		   

    float3 np2 = normalize(-p0+ns1);
    float zm1 = smoothstep(0.0, 4.0, GameTime),	
          zm2 = smoothstep(4.0, 5.0, GameTime),	 
          zm3 = smoothstep(5.0, 6.0, GameTime),			 
          zm4 = smoothstep(6.0, 21.0, GameTime),
          zm5 = smoothstep(21.0, 24.0, GameTime);
    float ta = lerp(1.0, 1.0, zm1);
          ta = lerp(ta, 0.3, zm2);   
          ta = lerp(ta, 0.0, zm3); 		  
          ta = lerp(ta, 0.0, zm4);
          ta = lerp(ta, 1.0, zm5);
	
    float4 stars = Stars(np2 * 0.5 + 0.5)*4.48;
		   stars*= step(0.3, stars);
		   stars*= smoothstep(0.0, 0.3, wpos.z);
		   stars.rgb*= ta;
           stars.rgb = WeatherSetts(stars.rgb, 0.0);		  
   
	float3 skyColor = GetAtmosphericScattering(wpos.xyz, SunDirection.xyz, GameTime, depth, 1.0, 1.0, 1.0);
 
	float3 w1 = float3(1.0, 1.0, 10.0);    
	float ip = IntersectPlane(w1, dir.xyz);	
	
	float3 sc0 = SunLight(dir.xyz);	
	float4 c0 = GenerateClouds(dir.xyz*ip*3.0, sc0);	
		   c0.a = min(1.0, c0.a); 

    if( 1000.0 < 0.01 || depth == 1)
	      skyColor.xyz = lerp(skyColor.xyz + stars.rgb, c0.rgb*1.15, 0.9*c0.a*smoothstep(0.0, 0.04, pow(saturate(dir.z), 1.85)));

          color.rgb = lerp(color.rgb, colorOr.rgb, saturate(m3));			
          color.rgb = lerp(skyColor, color.rgb, saturate(fadefact));	

    float3 SunFog = CalculateSunRay(wpos, SunDirection.xyz, GameTime);

    float smz1 = smoothstep(0.0, 4.0, GameTime),	  
          smz2 = smoothstep(4.0, 5.0, GameTime),	 
          smz3 = smoothstep(5.0, 6.0, GameTime),			 
          smz4 = smoothstep(6.0, 7.0, GameTime),
          smz5e = smoothstep(8.0, 11.0, GameTime),		
          smz5 = smoothstep(16.0, 17.0, GameTime),
          smz6 = smoothstep(18.0, 19.0, GameTime),
          smz7 = smoothstep(19.0, 20.0, GameTime),
          smz8 = smoothstep(22.0, 24.0, GameTime);
	
    float3 ti2 = lerp(0.1*float3(0.667, 0.863, 0.98), 0.1*float3(0.667, 0.863, 0.98), smz1);     
           ti2 = lerp(ti2, 0.2*float3(0.667, 0.863, 0.98), smz2);   
           ti2 = lerp(ti2, 0.3*float3(0.784, 0.914, 0.98), smz3); 		  
           ti2 = lerp(ti2, 0.5*float3(0.824, 0.863, 0.882), smz4);
           ti2 = lerp(ti2, 0.5*float3(0.824, 0.863, 0.882), smz5e);
           ti2 = lerp(ti2, 0.5*float3(0.824, 0.863, 0.882), smz5);
           ti2 = lerp(ti2, 0.3*float3(0.784, 0.914, 0.98), smz6);
           ti2 = lerp(ti2, 0.2*float3(0.667, 0.863, 0.98), smz7);
           ti2 = lerp(ti2, 0.1*float3(0.667, 0.863, 0.98), smz8);	

    float3 co1 = float3(0.118, 0.161, 0.212),
           co2 = float3(0.51, 0.373, 0.392),
           co3 = float3(0.647, 0.569, 0.49),
           co4 = float3(0.765, 0.725, 0.627),
           co5e = float3(0.863, 0.863, 0.843),
           co5 = float3(0.843, 0.812, 0.706),		   
           co6 = float3(0.608, 0.49, 0.49),
           co7 = float3(0.51, 0.392, 0.392),
           co8 = float3(0.392, 0.275, 0.294),		   
           co9 = float3(0.118, 0.161, 0.212);
		   
    float3 colorFog = Config2(co1, co2,  co3,  co4,  co5e,  co5,  co6,  co7,  co8, co9);	
           colorFog = WeatherSetts(colorFog+SunFog*0.25, ti2);
		   
		   color.rgb = lerp(color.rgb, colorFog, VolFog);
	
	float4 cloudSum = 1.2;
           cloudSum.a = 0.0;

	float4 clouds = float4(0.0,0.0,0.0,1.0); 
           clouds = VolumetricClouds(CameraPosition.xyz, wpos1.xyz, vPos.xy, depth, coord);

          cloudSum.rgb = lerp(cloudSum.rgb*clouds.rgb, clouds.rgb, clouds.a);
          cloudSum.a = -clouds.a;

          color.xyz = lerp(color, cloudSum.rgb, min(1.0, cloudSum.a * cloudSum.a * 1.21));		
   return color;
}

float4 PS_Rain(VS_OUTPUT_POST_FXAA IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;	
    float4 color = tex2D(SamplerColor, coord);
	float depth = tex2D(SamplerDepth, coord.xy).x;	
    float4 worldpos = GWP(coord, depth);
	float3 dir = normalize(worldpos.xyz);
    float mask = maskWater(coord); // Вода	
	float wpos = length(worldpos.xyz);
		  
	float3x3 vec = float3x3(1,0,0,0,cos(0.25),sin(0.25),0,-sin(0.25),cos(0.25));				   
	float3 rp = mul(dir, vec);		

	float2 f0 = float2(acos(rp.z),atan(rp.x/rp.y));		  		  
	float2 time = float2(Timer.x*31000.0, 0.0); 	
    float4 raindx0 = Rain0(-f0.xy+time*3.7);
	float4 raindx1 = Rain1(-f0.xy+time*1.5);		
	float4 raindx2 = Rain2(-f0.xy+time*1.0);
  
	float3 rd0 = 1.0-1.0/exp(length(0.0-wpos)/0.1);
	float3 rd1 = 1.0-1.0/exp(length(0.0-wpos)/8.0);
	float3 rd2 = 1.0-1.0/exp(length(0.0-wpos)/95.0);		   
	float3 dr0 = step(0.15, rd0*raindx0);
	float3 dr1 = step(0.1, rd1*raindx1);
	float3 dr2 = step(0.1, rd2*raindx2);  
	float a0 = smoothstep(1.0, 0.90, abs(rp.z));
	
	   float3 rn0 = tex2D(SamplerColor, IN.txcoord.xy+raindx2*float2(0.0, -0.07)).rgb;		   
	   float3 rn1 = tex2D(SamplerColor, IN.txcoord.xy+raindx1*float2(0.0, -0.09)).rgb;  	   
	   float3 rn2 = tex2D(SamplerColor, IN.txcoord.xy+raindx0*float2(0.0, -0.11)).rgb;	
	   float ws = WeatherSetts2(0.0, 0.17, 0.5);

	float sh = tex2D(SamplerShadow, coord);
		  sh = WeatherSetts(saturate(sh+0.7), 1.0);
		 color.xyz*= lerp(1.0, sh, mask);

		 color.xyz = lerp(color.xyz, 1.03*rn0+0.022, ws*dr2*a0);		 
		 color.xyz = lerp(color.xyz, 1.03*rn1+0.023, ws*dr1*a0);	
		 color.xyz = lerp(color.xyz, 1.03*rn2+0.024, ws*dr0*a0);		
  return color;
}

float rds1
<
	string UIName="RainDropsScr: Amount";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=5.0;
> = {2.5};

float rds2
<
	string UIName="RainDropsScr: Strength";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
	float UIStep=0.001;	
> = {0.025};

float4 PS_Rain2(VS_OUTPUT_POST_FXAA IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;	
    float4 color = tex2D(SamplerColor, coord);
	
	float time = Timer.x * 11000.0;		
	float2 uv = coord.xy;
	       uv.y/=ScreenSize.z;
		   
	float2 n = tex2D(SamplerENBNoise, uv *1.48).rg;   
	float3 f = tex2D(SamplerColor, coord);
	float ws = WeatherSetts2(0.0, rds2, 0.5);
    
    for (float r = 4.0; r > 0.0; r--) 
    {
        float2 x = 4500.0 * r * 0.015,
               p = 6.28 * uv * x + (n - 0.5) * 2.0,
               s = sin(p);
        
        float4 d = tex2D(SamplerENBNoise, round(uv * x - 0.25) / x);
        float t = (s.x+s.y) * max(0., 1. - frac(time * (d.b + .1) + d.g) * 2.);      

        if (d.r < (rds1-r)*0.08 && t > 0.5) 
        {
          float3 v = normalize(-float3(cos(p), lerp(.2, 2., t-.5)));
                 f.xyz = tex2D(SamplerColor, coord- v.xy * ws).rgb;
        }
    }		
	     color.xyz = f.xyz;
  return color;
}
////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

technique PostProcess
{
	pass P0
	{
      VertexShader = compile vs_3_0 VS_PostProcess();
      PixelShader  = compile ps_3_0 PS_SSR();
	}
}

technique PostProcess2
{	
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcessFXAA();
		PixelShader  = compile ps_3_0 PS_DX();
	}
}

technique PostProcess3
{	
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_Rain();
	}
}

technique PostProcess4
{	
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_Rain2();
	}
}
/**/