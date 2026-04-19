
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

float4 tempF1; float4 tempF2; float4 tempF3;
float4 ScreenSize; float ENightDayFactor; float EInteriorFactor; float4 WeatherAndTime; float4 Timer;
float FieldOfView; float GameTime; float4 SunDirection; float4 CustomShaderConstants1[8];
float4 MatrixVP[4]; float4 MatrixInverseVP[4]; float4 MatrixVPRotation[4]; float4 MatrixInverseVPRotation[4];
float4 MatrixView[4]; float4 MatrixInverseView[4]; float4 CameraPosition;
float4x4 MatrixWVP; float4x4 MatrixWVPInverse; float4x4 MatrixWorld; float4x4 MatrixProj;
float4 FogParam; float4 FogFarColor; float4 WaterParameters1; float4 WaterParameters2;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Textures
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

texture2D texOriginal;
texture2D texRefl;
texture2D texReflPrev;
texture2D texDepth;
texture2D texNormal;
texture2D texWave < string ResourceName="WaterFoam.png"; >;
texture2D texWater < string ResourceName="WaterRelief.png"; >;
texture2D texAlpha < string ResourceName="WaterAlpha.png"; >;
sampler2D SamplerOriginal = sampler_state { Texture   = <texOriginal>; };

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Sampler Inputs
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

sampler2D SamplerAlpha = sampler_state
{
	Texture   = <texAlpha>;
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

sampler2D SamplerWave = sampler_state
{
	Texture   = <texWave>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = Wrap;
	AddressV  = Wrap;
	AddressW  = Wrap;		
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerExternal = sampler_state
{
	Texture   = <texWater>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = Wrap;
	AddressV  = Wrap;
	AddressW  = Wrap;	
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerNormal = sampler_state
{
	Texture   = <texNormal>;
	MinFilter = Anisotropic;
	MagFilter = Anisotropic;
	MipFilter = LINEAR;
	AddressU  = Wrap;
	AddressV  = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};


sampler2D SamplerRefl = sampler_state
{
	Texture   = <texRefl>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerRefl2 = sampler_state
{
	Texture   = <texReflPrev>;
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

struct PS_OUTPUT3
{
	float4 Color[3] : COLOR0;
};

struct VS_INPUT
{
	float3	pos : POSITION;
	float4	diff : COLOR0;
	float4	spec : COLOR1;
	float2	txcoord0 : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4	pos : POSITION;
	float4	diff : COLOR0;
	float2	txcoord0 : TEXCOORD0;
	float4	txcoord1 : TEXCOORD1;
	float3	normal : TEXCOORD2;
	float3	vnormal : TEXCOORD3;
	float4	vposition : TEXCOORD4;
	float3	wposition : TEXCOORD5;
	float3	eyedir : TEXCOORD6;
};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

bool ECausticsEnable
<
	string UIName="Water: EnableCaustics";
> = {true};

float ECausticsIntensity
<
	string UIName="Water: CausticsIntensity";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.6};

float EWaveBumpiness
<
	string UIName="Water: WaveBumpiness";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.35};

float reflbr
<
	string UIName="Reflection: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.3;
	float UIMax=1.0;
> = {0.55};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

VS_OUTPUT VS_Draw(VS_INPUT IN)
{
	VS_OUTPUT	OUT;
	float4 pos = float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);
	float4 tpos = mul(pos, MatrixWVP);
	float3 cnormal = float3(0.0,0.0,1.0);
	OUT.normal = mul(cnormal, MatrixWorld);
	OUT.vnormal = normalize(mul(cnormal.xyz, MatrixWVP));
	OUT.pos = tpos;
	OUT.vposition = tpos;
	OUT.wposition = mul(pos, MatrixWorld);
	float4 uv;
	uv.xy = IN.txcoord0.xy;
	uv.zw = 0.0;
	OUT.txcoord0 = IN.txcoord0;
	uv.xy = IN.txcoord0.xy+WaterParameters2.w;
	uv.zw = IN.txcoord0.xy-WaterParameters2.w;
	uv.xy = 0.07*pos.xy+WaterParameters2.w;
	OUT.txcoord1 = uv;
	OUT.diff = IN.diff;
	float3 campos = CameraPosition;
	OUT.eyedir=(mul(pos, MatrixWorld) - campos);
 	return OUT;
}

float3 hash1(float3 p3)
{
	p3 = frac(p3 * float3(0.1031, 0.11369, 0.13787));
	p3 += dot(p3, p3.yxz+19.19);
	p3=float3((p3.x+p3.y), (p3.x+p3.z), (p3.y+p3.z)) * p3.zyx;
	return frac(p3);
}

float WorleyNoise(float3 n, float scale)
{
	n *= scale;
	float2	dis=1.0;
	for (float x=-1.0; x<1.01; x+=1.0)
	{
		for (float y=-1.0; y<1.01; y+=1.0)
		{
			for (float z=-1.0; z<1.01; z+=1.0)
			{
				float3	xyz=float3(x,y,z);
				float3	frcn=frac(n);
				float3	p=n-frcn + xyz;
				float3	pp = p;
				if (pp.z < 0.0) pp.z = scale + pp.z;
				if (pp.z > 1.0) pp.z = pp.z - scale;
				float2	sqrdist;
				frcn=xyz - frcn;
				p = hash1(pp) + frcn;
				sqrdist.x=dot(p, p);
				pp.z+=0.012;
				p = hash1(pp) + frcn;
				sqrdist.y=dot(p, p);
				dis.xy = min(dis.xy, sqrdist.xy);
			}
		}
	}
	dis.x=dis.x*dis.y;
	return saturate(sqrt(dis.x));
}

#include "AtmosphericScattering.fx"

float4 WorldScr(float3 pos)
{
	float4 s = mul(float4(pos.xyz, 1.0), MatrixWVP);		   
	       s.xyz/= s.w;
	       s.y = -s.y;
	       s.xy = s.xy * 0.5 + 0.5;		   		   
	return s;		
}

float4 WorldPos(float2 uv)
{
    float depth = tex2Dlod(SamplerDepth, float4(uv.xy, 0.0, 0.0)).x; 	
	
    float sky;   
    if( 1000.0 < 0.01 || depth == 1)
     {
	     sky = 1.0;
     }	
	     sky = 1-sky;		
	
	float4 t;
		   t.xy = uv.xy*2.0-1.0;
		   t.y = -t.y;
		   t.z = depth*sky;
		   t.w = 1.0;		   
		   
    float4 w = mul(t, MatrixWVPInverse);
	       w.xyz/= w.w;
	return w;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float dithering(float2 vpos, int l)
{
	float res = 0.0;
	for(float i = 1-l; i<= 0; i++)
	{
		float Size = exp2(i);
	    float2 coord = floor(vpos*Size) % 2.0;
		float cd = 2.0*coord.x - 4.0*coord.x*coord.y + 3.0*coord.y;
		res += exp2(2.0*(i+l))* cd;
	}
	float res1 = 4.0*exp2(2.0 * l)- 4.0;
	return res/res1 + 1.0/exp2(2.0 * l);
}

float3 wPos(in float2 c)
{
   float d = tex2D(SamplerDepth, c.xy).x;	   
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
   return w;
}

float3 GetNormal(float2 coords)
{
	float3 of = float3(ScreenSize.y,ScreenSize.y * ScreenSize.z,0);	
	
	float3 f = wPos(coords.xy);
	float3 dx1 = - f + wPos(coords.xy + of.xz);
	float3 dx2 =   f - wPos(coords.xy - of.xz);
	float3 dy1 = - f + wPos(coords.xy + of.zy);
	float3 dy2 =   f - wPos(coords.xy - of.zy);

	dx1 = lerp(dx1, dx2, abs(dx1.z) > abs(dx2.z));
	dy1 = lerp(dy1, dy2, abs(dy1.z) > abs(dy2.z));

	return normalize(cross(dy1,dx1));
}

float3 Config2(float3 c1, float3 c2, float3 c3, float3 c4, float3 c5e, float3 c5, float3 c6, float3 c7, float3 c8)
{		
   float x1 = smoothstep(0.0, 4.0, GameTime),
         x2 = smoothstep(4.0, 5.0, GameTime),
         x3 = smoothstep(5.0, 6.0, GameTime),
         x4 = smoothstep(6.0, 7.0, GameTime),
         x5e = smoothstep(8.0, 11.0, GameTime),
         x5 = smoothstep(16.0, 17.0, GameTime),
         x6 = smoothstep(18.0, 19.0, GameTime),
         x7 = smoothstep(19.0, 20.0, GameTime),
         x8 = smoothstep(22.0, 23.0, GameTime);	
   float3 t1 = lerp(0.0, c1, x1); // Ночь
          t1 = lerp(t1, c2, x2); // Ночь 2
          t1 = lerp(t1, c3, x3); // Утро 3
          t1 = lerp(t1, c4, x4); // Утро 4
          t1 = lerp(t1, c5e, x5e); // День 5
          t1 = lerp(t1, c5, x5); // День 5
          t1 = lerp(t1, c6, x6); // Вечер 6	 
          t1 = lerp(t1, c7, x7); // Вечер 7
          t1 = lerp(t1, c8, x8); // Вечер 8
   return t1;
}

float3 tonemp(float3 color)
{
	float3 n0 = normalize(color.xyz);
	float3 ct0=color.xyz/n0.xyz;
	       ct0=pow(ct0, 1.1);
           n0.xyz = pow(n0.xyz, 0.75);   
           color.xyz = ct0*n0.xyz;  

    float maxcol = max(color.x, max(color.y, color.z));		 		 
    float3 r0 = 1.0-exp(-maxcol),
           r2 = 1.0-exp(-color);	

         color.xyz = lerp(float3(r2.x, r2.y, r2.z), color*float3(r0.x, r0.y, r0.z)/maxcol, 0.1);		 
  return color;	
}

float3 RayM(float2 vpos, float3 pos, float3 wpos, float3 wnorm, float3 sky)
{
	float d3 = dithering(vpos.xy,2);
	float d1 = dithering(vpos.xy,1);	
          d3 = lerp(d3, d1, 0.85);	
          d3 = d3*0.000755 + 0.0025;

	float3 view = normalize(pos.xyz - CameraPosition.xyz);		  
	float3 dir = normalize(reflect(view, wnorm));
	float step = d3*sqrt(3.0*pos.z);	
	float3 p = wpos+(dir*step);

	int s = 16;
	int n = 9;
	float2 uv = 0.0;
	bool a = 0;	
	int l = 0;
	
	[fastopt]
	for(int i = 0; i < s; i++)		
	{		
	       uv = WorldScr(p);
	float3 aPos = WorldPos(uv.xy);		
    float sError = distance(aPos.xyz, p.xyz);	

	if(sError*2.0 < 1000.0 && -sError*17.5 > -20.0*step)
	  {
		 i = 0;		 
			if(l < n)
			{
			   step/= 2.0;
			   p -= dir*step;			
			   step*= 2.0*rcp(s)/0.21;
			}
			else
			{		
			   i += s;
			}
		 l++;			
	  }
		p += dir*step/0.93;
		step *= 2.0;
	}	
    a = l > n != 0;

	float4 ssr = 1.0;	
	       ssr.rgb = tex2Dlod(SamplerRefl2, float4(uv, 0.0, 0.0)).rgb;
	       ssr.w*= a;
	
    float nf1 = saturate(50.0*(uv.y));
          nf1*= saturate(120.0*(uv.x));		 
    float nf2 = saturate(50.0+uv.y*(-50.0));
          nf2*= saturate(120.0+uv.x*(-120.0));
		   
    float3 reflection = lerp(sky, ssr.rgb, nf1*nf2*ssr.w);	
	return reflection;
}

PS_OUTPUT3 PS_Draw(VS_OUTPUT IN, float2 vPos : VPOS, uniform sampler2D ColorTexture)
{
	float4 r0 = 0.0;
	float2 of = 0.0;	
	float4 color;
	float2 coord;
	float2 uv1, uv2, uv3, uv4, uv5;
	float3 wed;
	float3 un0, un1;
	float4 origcolor;
	float2 txcoord;
	float3 normal;
	float4 tvec;
	
	txcoord.xy = (IN.vposition.xy /IN.vposition.w)*float2(0.5, -0.5) + 0.5;
	txcoord.xy+= 0.5*float2(ScreenSize.y, ScreenSize.y*ScreenSize.z);
	wed = normalize(-IN.eyedir.xyz);
    float tx = Timer.x*100.0*5.0;
	float ts = 0.06;
	uv1.xy = float2(0.38,0.45)*IN.txcoord1+float2(ts* 10.00, -ts*5.70)*tx;
	uv2.xy = float2(1.80,2.00)*IN.txcoord1+float2( ts*11.00, -ts*7.75)*tx;
	uv3.xy = float2(10.0,10.2)*IN.txcoord1+float2(-ts*5.78,  ts*7.72)*tx;
	uv4.xy = float2(0.13,0.20)*IN.txcoord1+float2(ts*8.40, -ts*10.10)*tx;
	uv5.xy = float2(1.10,1.30)*IN.txcoord1+float2(-ts*5.40, ts*15.10)*tx;		
	color = tex2D(ColorTexture, uv1);
	color+= tex2D(ColorTexture, uv2);
	color+= tex2D(ColorTexture, uv3);	
	color+= tex2D(ColorTexture, uv4);
	color+= tex2D(ColorTexture, uv5);	
	origcolor = color*0.25;
	
    float4 fx = tex2D(SamplerAlpha, 0.07*0.0010 + uv4);
	float2 uvshift = -IN.eyedir.xy * (origcolor.x*2.0-1.0) * 0.001*0.30;
	uv1.xy+= uvshift;
	uv2.xy+= uvshift;
	uv3.xy+= uvshift;	
	uv4.xy+= uvshift;
	uv5.xy+= uvshift;	
	color = tex2D(ColorTexture, uv1);
	color+= tex2D(ColorTexture, uv2);
	color+= tex2D(ColorTexture, uv3);	
	color+= tex2D(ColorTexture, uv4);
	color+= tex2D(ColorTexture, uv5);	
	origcolor = color*0.25;	
	float2 kernel[4]=
	{
		float2( 1.0, 0.0),
		float2(-1.0, 0.0),
		float2( 0.0, 1.0),
		float2( 0.0,-1.0)
	};
	for (int i=0; i<4; i++)
	{
		coord = kernel[i];
		float4 tc;
		//float4 tc2;
		//float4 tc3;
		float4 tc4;
		float4 tc5;			
		tc = tex2D(ColorTexture, 0.07*coord*0.0010 + uv1.xy);
		//tc2 = tex2D(ColorTexture, 0.07*coord*0.0010 + uv2.xy);
		//tc3 = tex2D(ColorTexture, 0.07*coord*0.0010 + uv3.xy);
		tc4 = tex2D(ColorTexture, 0.07*coord*0.0010 + uv4.xy);
		tc5 = tex2D(ColorTexture, 0.07*coord*0.0010 + uv5.xy);		
		//tc.x+= tc2.x;
		//tc.x+= tc3.x;
		tc.x+= tc4.x;	
		tc.x+= tc5.x;			
		of+= coord*(color.x-tc.x);
	}
	un0.xy = of.xy*10.0*2.20*EWaveBumpiness;
	un0.z = sqrt(1.0-dot(of.xy, of.xy));
	un0 = normalize(mul(un0, MatrixWorld));	
	un1.xy = of.xy * 10.0*5.50*EWaveBumpiness;
	un1.z = sqrt(1.0-dot(of.xy, of.xy));
	un1 = normalize(mul(un1, MatrixWorld));
	tvec.xyz = un0;
	normal.x = dot(tvec.xyz, MatrixView[0]);
	normal.y = dot(tvec.xyz, MatrixView[1]);
	normal.z = dot(tvec.xyz, MatrixView[2]);
	normal = normalize(normal);
	of.xy = normal.xy;
	of.y = -of.y;
	of.xy*= 0.05*float2(0.5, 0.0);
	float objdepth = IN.vposition.z/IN.vposition.w;
	float nonlinearobjdepth = objdepth;
	      objdepth = 1.0/max(1.0-objdepth, 0.000000001);
	float planardepth = tex2D(SamplerDepth, txcoord.xy).r;
	float depth = tex2D(SamplerDepth, txcoord.xy).r;

	if (nonlinearobjdepth>depth)
	{
		depth = planardepth;
		of.xy = 0.0;
	}

	float scenedepth = depth;
	      planardepth = 1.0/max(1.0-planardepth,0.000000001);
	      depth = 1.0/max(1.0-depth,0.000000001);
	float depthfact = (depth-objdepth)*WaterParameters1.z;
	      depthfact = depthfact*WaterParameters1.x;
	      depthfact = depthfact/(depthfact*0.50+1.0);

	if (scenedepth>0.99999) depthfact = 1.0;

	float backside = saturate(WaterParameters1.w*10.0);
	      depthfact*= 1.0-backside;
	float shorefade = (planardepth-objdepth);
	      shorefade = shorefade*WaterParameters1.x;
	      shorefade = saturate(13.40*shorefade*1000.0 - 0.05);
	      depthfact*= shorefade;
	
    float2 of2 = of;	
	       of2.xy*= shorefade*0.0;
	       of.xy*= shorefade;

	tvec.xyz = un0;
	tvec.xy*= shorefade;
	normal.x = dot(tvec.xyz, MatrixView[0]);
	normal.y = dot(tvec.xyz, MatrixView[1]);
	normal.z = dot(tvec.xyz, MatrixView[2]);
	normal = normalize(normal);
	un0 = normalize(tvec.xyz);
	tvec.xyz = un1;
	un1 = normalize(tvec.xyz);		

    float4 tv;
	       tv.xy = (txcoord.xy+of2.xy*0.2)*2.0-1.0;
	       tv.y = -tv.y;
	       tv.z = scenedepth;
	       tv.w = 1.0;
	float4 wp;		   
	       wp.x = dot(tv, MatrixInverseVPRotation[0]);
	       wp.y = dot(tv, MatrixInverseVPRotation[1]);
	       wp.z = dot(tv, MatrixInverseVPRotation[2]);
	       wp.w = dot(tv, MatrixInverseVPRotation[3]);
	       wp.xyz/= wp.w;
	       wp.xyz+= CameraPosition;

	float deepfade = max(IN.wposition.z-wp.z, 0.0);
		  deepfade*= 0.1;
		  deepfade = deepfade/(deepfade+1.0);

	      tv.xyz = wp-CameraPosition;
	float distfact = dot(tv.xyz, tv.xyz);
	      distfact = saturate(1.0-distfact*0.000025);

	float3 causticspos = wp;
	       causticspos*= 0.25;
	       causticspos.z*= 0.25;
	       causticspos.z+= Timer.x*1677.7216 * 2.0;
	       causticspos.z = frac(causticspos.z);

	float caustic = 0.0;
	float usecaustic = 0.0;
	if (ECausticsEnable==true) usecaustic = 1.0;
	      usecaustic*= distfact;
	      usecaustic*= 0.9-deepfade;
	if (usecaustic>=0.1)
	{
		caustic = WorleyNoise(causticspos.xyz, 8.0);
	}

	caustic = caustic*caustic;
	caustic*= lerp(1.0, caustic, deepfade);
	caustic = caustic-0.1;
	caustic*= shorefade;
	caustic*= 1.0-deepfade;
	caustic*= saturate((WaterParameters1.w+CameraPosition.z - wp.z));
	caustic*= distfact;
	caustic*= ECausticsIntensity;
	
	float t = GameTime;
	float x1 = smoothstep(0.0, 4.0, t),
	      x2 = smoothstep(4.0, 5.0, t),
	      x3 = smoothstep(5.0, 6.0, t),
	      x4 = smoothstep(6.0, 7.0, t),
	      xE = smoothstep(8.0, 11.0, t),
	      x5 = smoothstep(16.0, 17.0, t),  
	      x50 = smoothstep(13.0, 14.0, t),  
	      x51 = smoothstep(14.0, 17.95, t),  
	      x60 = smoothstep(17.98, 18.0, t),     
	      x6 = smoothstep(18.0, 19.0, t),
	      x7 = smoothstep(19.0, 20.0, t),
	      xG = smoothstep(20.0, 21.0, t),  
	      xZ = smoothstep(21.0, 22.0, t),
	      x8 = smoothstep(22.0, 23.0, t),
	      x9 = smoothstep(23.0, 24.0, t);	  
		   
	float stopfact = saturate(-dot(un0, wed));
	float anglefact = 1.0-saturate(dot(IN.normal.xyz, -wed) * 3.0);
	anglefact*= anglefact;
	anglefact*= anglefact;
	stopfact = saturate(stopfact-anglefact);
	stopfact = saturate((0.1-stopfact)*10.0);
	stopfact*= backside;

	float alphafix = saturate((origcolor.a-0.95)*50.0);

//=============================================================
// Исправляем неправильную работу SunDirection
//=============================================================
     float3 vec0 = SunDirection.xyz;
     float3 s0 = normalize(float3(0.0, 0.0, 0.0));
   
     float s1 = smoothstep(0.0, 1.0, GameTime);	
     float s2 = smoothstep(1.0, 23.0, GameTime);
     float s3 = smoothstep(23.0, 24.0, GameTime);
   
     float3 vec1 = lerp(s0, vec0, s1);
            vec1 = lerp(vec1, vec0, s2); 
            vec1 = lerp(vec1, vec0, s3);
	float3 n3 = GetNormal(txcoord);
	
	r0 = tex2D(SamplerRefl, txcoord.xy);
	
	float3 diffuse = r0.rgb/3.0;
	float bottom = saturate(dot(n3.xyz*float3(1.0, 1.0, 1.5), vec1.xyz));

    float3 co1 = float3(0, 0, 0),
           co2 = float3(0.157, 0.0196, 0.00784),
           co3 = float3(1, 0.353, 0.118),
           co4 = float3(0.706, 0.49, 0.235),
           co5e = float3(0.569, 0.541, 0.471),
           co5 = float3(0.863, 0.647, 0.392),		   
           co6 = float3(1, 0.176, 0.0588),
           co7 = float3(0, 0, 0),
           co8 = float3(0, 0, 0);
	
	float3 colorlightting = Config2(co1, co2,  co3,  co4*1.2,  co5e,  co5,  co6,  co7,  co8);		
	float Light1 = WeatherSetts(0.12, 0.0);	
	float3 DFL1 = diffuse*(bottom)*50.0*Light1*colorlightting; 

    float3 t1 = lerp(float3(0.843, 0.961, 1)*0.89, float3(0.843, 0.961, 1)*0.89, x1);
           t1 = lerp(t1, 0.9, x2);
           t1 = lerp(t1, 0.9, x3);
           t1 = lerp(t1, 1.0, x4);
           t1 = lerp(t1, 1.0, x5);
           t1 = lerp(t1, 0.9, x6);		 
           t1 = lerp(t1, 0.9, x7);
           t1 = lerp(t1, 0.9, x8); 
           t1 = lerp(t1, float3(0.843, 0.961, 1)*0.89, x9);
		   
	float3 SkyCC = float3(0.667, 0.863, 0.98),
           SkyCC2 = float3(0.784, 0.914, 0.98),	
           SkyCC3 = float3(0.824, 0.863, 0.882);
		   
	float3 txx = Config2(SkyCC*0.64, SkyCC*0.74,  SkyCC2*0.80,  SkyCC3*0.85,  SkyCC3*0.85,  SkyCC3*0.85,  SkyCC2*0.74,  SkyCC*0.65,  SkyCC*0.65);	

	float3 Shadow1 = WeatherSetts(t1, txx);
		   
    float3 t2 = lerp(0.2, 0.2, x1);
           t2 = lerp(t2, 0.6, x2);
           t2 = lerp(t2, 0.8, x3);
           t2 = lerp(t2, 1.1, x4);
           t2 = lerp(t2, 1.2, xE);
           t2 = lerp(t2, 1.2, x5);
           t2 = lerp(t2, 1.1, x6);		 
           t2 = lerp(t2, 0.8, x7);
		   t2 = lerp(t2, 0.5, xG);
		   t2 = lerp(t2, 0.5, xZ);
           t2 = lerp(t2, 0.5, x8); 
           t2 = lerp(t2, 0.2, x9);	

    float3 t4 = lerp(0.2, 0.2, x1);
           t4 = lerp(t4, 0.6, x2);
           t4 = lerp(t4, 0.8, x3);
           t4 = lerp(t4, 1.0, x4);
           t4 = lerp(t4, 1.0, xE);
           t4 = lerp(t4, 1.0, x5);
           t4 = lerp(t4, 0.8, x6);		 
           t4 = lerp(t4, 0.6, x7);
           t4 = lerp(t4, 0.3, x8); 

    r0.xyz*= float3(0.941, 0.988, 1)*0.35*Shadow1 + pow(DFL1, 1.0/1.5);

	r0.xyz = tonemp(r0.xyz*2.9);
	r0.xyz = saturate(r0.xyz);
	r0.xyz = pow(r0.xyz,1.25);	

    float wave1 = tex2D(SamplerWave, uv2.xy*2.0),
          wave2 = tex2D(SamplerWave, uv5.xy*1.0);		 		
	
	r0.w = lerp(IN.diff.w*origcolor.a, 1.0, alphafix);
////////////////////////////////////////////////////////////////////////////////////	
    float3 sv0 = SunDirection.xyz;
    float3 sv1 = normalize(float3(0.0, 0.0, 0.0));
    float yy1 = smoothstep(0.0, 1.0, t);	
    float yy2 = smoothstep(1.0, 23.0, t);
    float yy3 = smoothstep(23.0, 24.0, t);
   
    float3 sv3 = lerp(sv1, sv0, yy1);
           sv3 = lerp(sv3, sv0, yy2); 
           sv3 = lerp(sv3, sv1, yy3);
////////////////////////////////////////////////////////////////////////////////////	
	float3 reflX = reflect(wed.xyz*float3(-1.0, -1.0, -1.0), un0.xyz);		
	float refl1 = saturate(1.05-dot(un0.xyz, wed.xyz));
	      refl1 = pow(refl1, 1.0);	  
		  
	float3 skyColor = GetAtmosphericScattering2(reflX.xyz, SunDirection.xyz, GameTime, 1.0, 0.0, 1.5, 0.8);	
	float nonlineardepth = (IN.vposition.z/IN.vposition.w);	
		   
    float3 sv4 = normalize(sv3.xyz+wed.xyz);
    float factor = 0.01 - dot(-sv4, normalize(un1.xyz));
          factor = pow(factor, 100.0*1.1);
		  factor*= pow(refl1, 0.05);
		  factor = (factor*factor)/1.0; 

    float3 t3 = lerp(0.0, 0.0, x1);
           t3 = lerp(t3, 0.0, x2);
           t3 = lerp(t3, float3(0.431, 0.275, 0.196)*0.7, x3);
           t3 = lerp(t3, float3(0.431, 0.275, 0.196)*0.6, x4);
           t3 = lerp(t3, float3(0.216, 0.216, 0.216), xE);
           t3 = lerp(t3, float3(0.216, 0.216, 0.216), x50);
           t3 = lerp(t3, float3(0.216, 0.216, 0.216)*0.5,x51);	
           t3 = lerp(t3, float3(0.431, 0.275, 0.196)*0.6, x60);		  
           t3 = lerp(t3, float3(0.431, 0.275, 0.196)*0.0, x6);		 
           t3 = lerp(t3, 0.0, x7);
           t3 = lerp(t3, 0.0, x8);  
	  
	float3 ScolorDay = t3*0.4,
           ScolorPasm = 0.0; 
	float3 mix = WeatherSetts(ScolorDay, ScolorPasm);
	
	float3 position = WorldPos(txcoord);
	       skyColor.xyz+= saturate(factor*mix);	
	       skyColor.xyz = RayM(vPos, position, IN.wposition.xyz, un0.xyz, skyColor.xyz);	

	float3 st1 = normalize(skyColor.xyz);
	float3 ct1 = skyColor.xyz/st1.xyz;
	       ct1 = pow(ct1, 1.9);
	       st1.xyz = pow(st1.xyz, 0.9);
	       skyColor.xyz = ct1*st1.xyz;	
	       skyColor.xyz*= reflbr;
	r0.xyz = lerp(r0.xyz+caustic*t4, skyColor.xyz, saturate(0.35+depthfact*WaterParameters1.y)*0.99); 	
	
	       fx.xyz+= 0.000001;	
	float3 st4 = normalize(fx.xyz);
	float3 ct4 = fx.xyz/st4.xyz;
	       ct4 = pow(ct4, 5.50);
	       st4.xyz = pow(st4.xyz, 3.93);
	       fx.xyz = ct4*st4.xyz;
           fx.xyz*= 0.23;
    float af0 = saturate(fx);
	r0.xyz = lerp(r0, lerp(r0, float3(1.0, 1.0, 1.0)*t2*3.0, wave2*wave1), af0); 

	float3 ssnormal=normal;
	       ssnormal.yz=-ssnormal.yz;
	float4 r1;
	       r1.xyz = ssnormal*0.5+0.5;	
	       r1.w = 85.0/255.0;
	float4 r2;
	       r2 = nonlineardepth*1.0;
           r2.w = 1.0;
   
	PS_OUTPUT3	OUT;
	OUT.Color[0]=r0;
	OUT.Color[1]=r1;
	OUT.Color[2]=r2;
	return OUT;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

technique Draw
{
	pass p0
    {
	VertexShader = compile vs_3_0 VS_Draw();
	PixelShader  = compile ps_3_0 PS_Draw(SamplerExternal);

	}
}

technique DrawUnderwater
{
	pass p0
    {
	VertexShader = compile vs_3_0 VS_Draw();
	PixelShader  = compile ps_3_0 PS_Draw(SamplerExternal);
	}
}

