
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

float la
<
	string UIName="Lighting: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.26};

float tg
<
	string UIName="Lighting: Gamma";
	string UIWidget="Spinner";
	float UIMin=1.0;
	float UIMax=3.0;
> = {1.2};

float ts
<
	string UIName="Lighting: Saturation";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=3.0;
> = {1.1};

float shb
<
	string UIName="Shadow: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=10.0;
> = {1.0};

float spworld
<
	string UIName="Specular: World";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.12};

float spcar
<
	string UIName="Specular: Cars";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=6.0;
> = {4.0};

float cg
<
	string UIName="Cars: Gamma";
	string UIWidget="Spinner";
	float UIMin=0.40;
	float UIMax=4.50;
> = {0.78};

float cb
<
	string UIName="Cars: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.40;
	float UIMax=1.50;
> = {0.78};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

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
float4x4	MatrixWVP;
float4x4	MatrixWVPInverse;
float4x4	MatrixWorld;
float4x4	MatrixProj;
float4	FogParam; //x - nearclip, y - farclip, z - fog start, w - fog end
float4	FogFarColor;

//textures
texture2D texOriginal;
texture2D texColor;
texture2D texDepth;
texture2D texDepthMipMap;
texture2D texNormal;
texture2D texEnv;
texture2D texNoise;
texture2D texShadow;

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
	MipFilter = NONE;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerShadow = sampler_state
{
	Texture   = <texShadow>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;//NONE;
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

#include "SSAO.fx"
////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
VS_OUTPUT_POST VS_PostProcess(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;

	float4 pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	OUT.vpos=pos;
	OUT.txcoord.xy=IN.txcoord.xy;

	return OUT;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 NormalPos(float2 txcoord, float depth)
{
	float4 tv;
	       tv.xy = txcoord.xy*2.0-1.0;
	       tv.y = -tv.y;
	       tv.z = depth;
	       tv.w = 1.0;
		   
	float4 n = tex2D(SamplerNormal, txcoord.xy);
	       n.xyz = n.xyz*2.0-1.0;
  		   
	       tv.xyz = n.xyz;
	       tv.x = -tv.x;
	       tv.w = 1.0;
	
	float4 np0;	
	       np0.x = dot(tv.xyz, MatrixInverseView[0]);
	       np0.y = dot(tv.xyz, MatrixInverseView[1]);
	       np0.z = dot(tv.xyz, MatrixInverseView[2]);
	       np0.xyz = normalize(np0.xyz);
	return np0;
}

float4 wp(float2 c, float d)
{
   float4 t; 
          t.xy = c.xy*2.0-1.0;
          t.y = -t.y;
          t.z = d;
          t.w = 1.0;
   float4 w;		  
          w.x = dot(t, MatrixInverseVPRotation[0]);
          w.y = dot(t, MatrixInverseVPRotation[1]);
          w.z = dot(t, MatrixInverseVPRotation[2]);
          w.w = dot(t, MatrixInverseVPRotation[3]);
          w.xyz/= w.w;
   return w;
}

float4 wp2(float2 c, float d)
{
   float4 t; 
          t.xy = c.xy*2.0-1.0;
          t.y = -t.y;  
          t.z = d;
          t.w = 1.0;
   float4 w;
          w.x = dot(t, MatrixInverseVP[0]);
          w.y = dot(t, MatrixInverseVP[1]);
          w.z = dot(t, MatrixInverseVP[2]);
          w.w = dot(t, MatrixInverseVP[3]);
          w.xyz/= w.w;
          w.w = 1.0;
          w.z = dot(w, MatrixVP[2]);
   return w;
}

// Вспомогательный элемент, для настройки цвета или яркости. Вариант 2
float3 Config2(float3 c1, float3 c2, float3 c3, float3 c4, float3 c5e, float3 c5, float3 c6, float3 c7, float3 c8)
{
   float t0 = GameTime;		
   float x1 = smoothstep(0.0, 4.0, t0);
   float x2 = smoothstep(4.0, 5.0, t0);
   float x3 = smoothstep(5.0, 6.0, t0);
   float x4 = smoothstep(6.0, 7.0, t0);
   float x5e = smoothstep(8.0, 11.0, t0);
   float x5 = smoothstep(16.0, 17.0, t0);
   float x6 = smoothstep(18.0, 19.0, t0);
   float x7 = smoothstep(19.0, 20.0, t0);
   float x8 = smoothstep(22.0, 23.0, t0);	
   float3 t1 = lerp(c1, c1, x1); // Ночь
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

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

// Коррекция Освещения
float3 tonemapping(float3 color)
{
//=Uncharted2Tonemap=
    float A = 1.0;
	float B = 0.25;
	float C = 0.52;
	float D = 0.34;
	float E = 0.0;
	float F = 1.0;

    float3 tm1 = ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
//===================
	float3 n0 = normalize(color.xyz);
	float3 ct0=color.xyz/n0.xyz;
	       ct0=pow(ct0, 1.1);
           n0.xyz = pow(n0.xyz, ts);   
           color.xyz = ct0*n0.xyz;  
		 
    float maxcol = max(color.x, max(color.y, color.z));		 		 
    float3 r0 = 1.0-exp(-maxcol),
           r2 = 1.0-exp(-color);	

         float3 tm0 = lerp(float3(r2.x, r2.y, r2.z), color*float3(r0.x, r0.y, r0.z)/maxcol, 0.2);		 
		 
	     color.rgb = lerp(tm0, tm1, 0.25);	 
  return color;	
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

float3 getVec(float2 coord)
{
    float fov = tan(50.0 * 0.0087266462597222);
    float a = -5.0/5.0;
    float2 uv = (coord * 2.0 - 1.0);
    return float3(uv.x*fov, uv.y*fov/a, 1.0);
}

float3 getPos(float2 coord, float depth)
{
    float LinDepth = -100.0 * 0.48 / (depth * (100.0 - 0.48) - 100.0);
    return getVec(coord) * LinDepth;
}

float3 getPos2(float2 coord)
{
    float  d0 = tex2Dlod(SamplerDepth, float4(coord.xy, 0.0, 0.0)).x;  
	float LinDepth = -100.0 * 0.48 / (d0 * (100.0 - 0.48) - 100.0);	
    return getVec(coord) * LinDepth;
}

float3 getNormal(float2 coord)
{
	float3 of = float3(ScreenSize.y,ScreenSize.y * ScreenSize.z,0);	
	
	float3 p = getPos2(coord.xy);
	float3 dx1 = -p+getPos2(coord.xy + of.xz),
	       dy1 = -p+getPos2(coord.xy + of.zy);
	
	float3 dx2 =  p-getPos2(coord.xy - of.xz),
	       dy2 =  p-getPos2(coord.xy - of.zy);

	       dx1 = lerp(dx1, dx2, abs(dx1.z) > abs(dx2.z));
	       dy1 = lerp(dy1, dy2, abs(dy1.z) > abs(dy2.z));

	return normalize(cross(dy1, dx1));
}

float AO_Radius
<
	string UIName="SSAO: Radius";
	string UIWidget="Spinner";
	float UIMin=0.2;
	float UIMax=4.0;
> = {1.05};

float AO_Alpha
<
	string UIName="SSAO_Alpha";
	string UIWidget="Spinner";

	float UIMin=0.0;
	float UIMax=1.0;
> = {0.88};

bool Enable_Debug <
string UIName = "Enable_Debug";
> = {false};

#define SSAO 14

float4 PS_AO(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
//==================================================================
	float2 coord = IN.txcoord.xy;	
	float4 color = tex2D(SamplerColor, coord.xy);
	float d3 = dithering(vPos.xy, 4.0) * 0.93;
//==================================================================
    float  d0 = tex2Dlod(SamplerDepth, float4(coord.xy, 0.0, 0.0)).x;  
    float  d1 = pow(tex2Dlod(SamplerDepth, float4(coord.xy, 0.0, 0.0)).x, 0.01*0.50);  
	       d1 = lerp(1.0, 0.0, saturate(d1));	
	
    float LinDepth = 1.0*0.05 / (1.0 + d0 * (0.05 - 1.0));	
    float4 worldpos = wp(coord, d0);
    float4 npos = NormalPos(coord, d0);

    float m1 = maskCar(coord);   // Авто + перс
    float m2 = maskPed(coord);   // Перс
    float m3 = maskWater(coord); // Вода		
		
    float3 pos = getPos(coord.xy, d0);	
    float3 n = getNormal(coord);
    float3 n2 = tex2D(SamplerNormal, coord).xyz*2.0-1.0;
           n2 = n2*float3(-1.85, 1.0, 1.0);
           n2 = lerp(n2, n, LinDepth);		   
	
	float2 uv;
	float ao = 0;
	float sr = -2.5;
    float d = 0.25*1.5/(SSAO*(n2.z + sr));
		
	float2 dir;
	sincos(d3*42.528, dir.x, dir.y);
	       dir*= d;

    float radius = LinDepth;	
          radius = lerp(0.4*AO_Radius, 0.16*AO_Radius, radius);

	  [fastopt]		
    for (int dx = 0; 1.0 >= 0.1 && ( dx<SSAO) && d1 > 0; dx++)	
      { 	
		   uv = dir.xy * (dx+1.0*d3);
		   dir.xy = mul(dir.xy, float2x2(0.76465, -0.64444, 0.64444, 0.76465));				   		   
		   uv/= 0.5;
			
		float3 v1 = normalize(float3(0, 0, -1));
		float L = saturate(0.7-dot(npos, v1));
              L = pow(L, 1.0);
		      L = lerp( 1.2*L+0.5, 1.0, saturate(m1+m2+m3));	
		  
		float depth = tex2Dlod(SamplerDepth, float4(coord.xy + uv*radius, 0.0, 1.0)).x;
		float3 sample_pos = getPos(coord.xy + uv*radius, depth);
		float3 occlusion_vector = (sample_pos-pos)/1.4;
		float dist = length(occlusion_vector);

		float AO = clamp( dot(-n2.xyz, normalize(occlusion_vector)*L*2.35) - 0.18, 0.0, 1.0);
        float rangeA = 1.0 - clamp(dist/3.0, 0.0, 1.0);
	
		   ao += 1.45 * (AO*rangeA);
      }
	float ssao = 1.0 - ao / float(SSAO);
          ssao = pow(ssao, 2.5);
          ssao = ssao*1.0;

		  color.xyz = 0.0;			  
		  color.xyz = ssao; 
   return color;
}

float4 PS_Blur(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
   float2 txcoord = IN.txcoord.xy;
   float4 blur = AO_Blur(txcoord);
   return blur;
}

float4 PS_Blur1(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
   float2 txcoord = IN.txcoord.xy;
   float4 blur = AO_Blur1(txcoord);
   return blur;
} 


float3 WeatherSetts(float3 a, float3 a2)
{ 
    float4 wx = WeatherAndTime;	
    float3 sc = a;
    float3 sn = a;   
    if (wx.x==4 || wx.x==7 || wx.x==8 || wx.x==9 || wx.x==12 || wx.x==15 || wx.x==16) sc = a2;
    if (wx.y==4 || wx.y==7 || wx.y==8 || wx.y==9 || wx.y==12 || wx.y==15 || wx.y==16) sn = a2;	
	
    float3 mix = lerp(sc, sn, wx.z);	
    return mix;
}
 
float3 AmbRef(float3 dir)
{
	dir = normalize(dir);
	float2 cd;
	       cd.x = atan2(dir.y,-dir.x)/6.2831853071 + 0.5;
	       cd.y = acos(dir.z)/3.1415926536;
	       cd.x = 1-cd.x;
	return tex2D(SamplerEnv, cd.xy).xyz;
}

float LightningSets()
{
    float4 wx = WeatherAndTime;	
    float c = 0.0,
          n = 0.0;

    if (wx.x==8 || wx.x==16) c = 1.0;
    if (wx.y==8 || wx.y==16) n = 1.0;

    float mix = lerp(c, n, wx.z);
    float3 res = FogFarColor;
	float3 st = normalize(res.xyz);
	float3 cs = res.xyz/st.xyz;
	       cs = pow(cs, 6.0);
	       res.xyz = cs;	
	float f = saturate(res);
   return f*mix;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
float4 GetLighting(float2 txcoord)
{
//=============================================================	
// Цвет, глубина, тени и прочее...
//=============================================================		
	float4 color = tex2D(SamplerOriginal, txcoord.xy);
if(Enable_Debug) color.xyz = 1.0;
	float4 color1 = color;		
	float4 shadow = tex2D(SamplerShadow, txcoord.xy);
    float depth = tex2D(SamplerDepth, txcoord.xy).x;	
    float4 wpos = wp2(txcoord, depth);	
	float fadefact = GetFog(wpos);	// Игровой Туман
    float4 wpos0 = wp(txcoord, depth);	
    float3 wpos1 = normalize(wpos0.xyz);		
    float4 npos = NormalPos(txcoord, depth);		
		   
// Маски ======================================================		
    float m1 = maskCar(txcoord);   // Авто + перс
    float m1a = maskCar1(txcoord);   // Авто + перс	(Glass off)
    float m2 = maskPed(txcoord);   // Перс
    float m3 = maskWater(txcoord); // Вода	
	float m0 = FuncMasking(txcoord); // Авто Фулл	
	      m0 = lerp(m1, 0.0, m0);		  
	color = lerp(pow(color,cg)*cb, color, m1+m2);	 		   
	color.xyz = lerp(color.xyz, pow(color.xyz, 0.84)*1.12, m0);
// Маска Specular		   
	float3 st0 = normalize(color.xyz);
	float3 ct0=color.xyz/st0.xyz;
	       ct0=pow(ct0, 1.3);
           st0.xyz = pow(st0.xyz, 1.0);   
	float  c0 = ct0*st0;
           c0 = saturate(c0*7.6); 
		   	  
//=============================================================
// Исправляем неправильную работу SunDirection
//=============================================================
   float t0 = GameTime;
   float3 vec0 = SunDirection.xyz;
   float3 s0 = normalize(float3(0.0, 0.0, 0.0));
   
   float s1 = smoothstep(0.0, 1.0, t0);	
   float s2 = smoothstep(1.0, 23.0, t0);
   float s3 = smoothstep(23.0, 24.0, t0);
   
   float3 vec1 = lerp(s0, vec0, s1);
          vec1 = lerp(vec1, vec0, s2); 
          vec1 = lerp(vec1, vec0, s3);

//=============================================================		
// Дополнительное
//=============================================================	
 float wpos2 = length(wpos0.xyz);
 float3 camdir = wpos0.xyz / wpos2;	
 float3 X2 = normalize(-vec1+camdir); 	
 
//=============================================================		
// Направление
//=============================================================	
	float3 MoonVector = normalize(float3(-0.04732, -0.39, 0.317));
	float3 vec2 = normalize(float3(0.0, 0.0, 1.0)); 	   
	float3 vec3 = normalize(float3(0.0, 0.0, -1.0));
	float3 vec4 = normalize(wpos1.xyz-vec1.xyz*float3(1.05, 1.05, 2.0));  	
	float3 vec5 = normalize(wpos1.xyz-MoonVector.xyz*float3(1.05, 1.05, 2.0));     
//=============================================================
// Создание Освещения
//=============================================================
	float4 sh2 = shadow; //Тень	 МАСКА

			
// Освещение Карты
    float3 co1 = float3(0, 0, 0),
           co2 = float3(0.176, 0.0196, 0.00784),
           co3 = float3(1, 0.275, 0.0784),
           co4 = float3(0.745, 0.529, 0.275),
           //co5e = float3(0.608, 0.608, 0.549),
           co5e = float3(0.745, 0.629, 0.475),		   
           co5 = float3(0.863, 0.647, 0.392),		   
           co6 = float3(1, 0.176, 0.0588),
           co7 = float3(0, 0, 0),
           co8 = float3(0, 0, 0);
		   
    float3 shNight = float3(0.157, 0.471, 1),		   
           shDay = float3(0.667, 0.863, 1);
		   
    float3 grDay = float3(1, 0.902, 0.667),		   
           grMr = float3(1, 0.882, 0.784);		   
	
    float3 colorlightting = Config2(co1, co2,  co3,  co4*1.2,  co5e,  co5,  co6,  co7,  co8);
    float3 colorShadow = Config2(shNight, shDay,  shDay,  shDay*1.2,  shDay,  shDay,  shDay,  shDay,  shNight);
    float3 colorAmb = Config2(shNight, grMr,  grDay,  grDay,  grDay,  grDay,  grMr,  grMr,  shNight);
    float shL = Config2(0.02, 0.04,  0.045,  0.06,  0.08,  0.07,  0.045,  0.04,  0.04);	

	float lt = saturate(dot(-npos.xyz*float3(1.0, 1.0, 1.5), vec1.xyz));	
    float lt2 = saturate(0.5 - dot(npos.xyz*float3(1.5, 1.5, 1.5), vec1.xyz));
		  lt2 = pow(lt2, 2.4); 	
	      lt2 = lerp(lt2, lt, m1);
  	  	  
// Маска для блика
	float ml = dot(npos.xyz*float3(1.0, 1.0, 2.0), vec1.xyz);
	      sh2*= saturate(0.0-ml*6.0);	
	float mshadow = saturate(sh2);
	
	float3 specColor = Config2(co1, co2,  co3,  co4,  co5e*0.5,  co5*0.5,  co6,  co7*0.0,  co8*0.0);		    
//=============================================================   
// Блик (Specular Peds)	
//============================================================= 
	float bl1 = lerp(0.12, 0.01, m2),
          bl2 = lerp(2.5, 0.50, m2),
          bl3 = lerp(10.0, 0.40, m2);	
  
    float spec = pow(bl1-dot( npos.xyz*float3(1.14, 1.14, 1.2), vec4.xyz), 0.15);
          spec = pow(spec, 100.0*bl2);
          spec = (spec*spec)*1.5;
          spec/= 2.5*bl3;
    float3 spec2 = pow(color, 10.0*0.2)*spec*4.0;
   
    float specm = saturate(-0.24-dot(-npos.xyz*float3(1.14, 1.14, 1.2), vec5.xyz));  
          specm = pow(specm, 100.0*0.24);
          specm = (specm*specm)/0.05;		   
   
//=============================================================
// Блик (Specular Cars)	
//=============================================================    
    float spec3 = pow(0.01-dot(npos.xyz, X2.xyz), 0.15);
          spec3 = pow(spec3, 40.0);
          spec3 = (spec3*spec3)*1.5;
          spec3/= 3.75;	 	  
//=============================================================	
//=============================================================	   
	       color.xyz*= lerp(cb, 1.0, m1+m2);			
		  
	float3 specmix = spec3*color*color*2.0;	
	float3 diff = color/3.0;	
	
	       color.xyz = pow(color.xyz, 2.2);
//=============================================================
//=============================================================	 
	float car1 = lerp(1.0, 0.0, m1+m2);
	float car2 = lerp(1.0, 0.0, m1a+m2);
	float car3 = lerp(1.0, 0.35, m1a);
	float refA1 = lerp(car3, 1.0, saturate(m2+m1+m3));
 
	float dotSky = saturate(0.9 - dot(npos.xyz, normalize(float3(0,0,1)) ));	
          dotSky = pow(dotSky, 0.8);
		  
	float fresnel = 0.001-dot(npos, -camdir);		
		  fresnel = saturate(pow(fresnel, 0.333));		  

	float colO = saturate(pow(tex2D(SamplerOriginal, txcoord.xy), 1.0)*2.0);	
	      colO = lerp(1.0, colO, 0.6);  
		  
	float3 rf = reflect(wpos0.xyz, npos.xyz);
		  	
	float3 cubemap = AmbRef(rf);
	
	float3 n0 = normalize(cubemap.xyz);
	float3 ct0x=cubemap.xyz/n0.xyz;
	       ct0x=pow(ct0x, 0.8);
           n0.xyz = pow(n0.xyz, 0.8);   
           cubemap.xyz = ct0x*n0.xyz;  		

	float3 SkyRefl = lerp(cubemap*0.1*shb, 0.0, fresnel*0.85); 
		
	float3 AmbLight = lerp(3.0*colorAmb, colorShadow*1.5, saturate(dotSky));

    float ls = LightningSets();
          ls = lerp(1.0, 1.7, ls);	
	float3 AmbCloudy = lerp(float3(0.431, 0.471, 0.51), float3(0.745, 0.824, 0.863)*ls, saturate(dotSky));

	float ssao = AO_Blur2(txcoord);
	      ssao = lerp(1.0, ssao, AO_Alpha);	
		  
// Еб*ня которые лучше бы удалить... Но влом
	float dotSSAO = saturate(dot(-npos.xyz, normalize(float3(0.0, 0.0, 1.0))));	
	float ssao2 = lerp(ssao, 1.0, 0.6*dotSSAO);	
//============================================================	
   float sm1 = smoothstep(0.0, 2.0, GameTime),	
         sm2a = smoothstep(2.0, 4.0, GameTime),   
         sm2 = smoothstep(4.0, 5.0, GameTime),	 
         sm3 = smoothstep(5.0, 6.0, GameTime),			 
         sm4 = smoothstep(6.0, 23.0, GameTime),
         sm5 = smoothstep(23.0, 24.0, GameTime),
         sm6 = smoothstep(21.0, 24.0, GameTime);		 
   float ti = lerp(1.0, 1.0, sm1);
         ti = lerp(ti, 0.0, sm2a);     
         ti = lerp(ti, 0.0, sm2);   
         ti = lerp(ti, 0.0, sm3); 		  
         ti = lerp(ti, 0.0, sm4);
         ti = lerp(ti, 1.0, sm5);
         ti = lerp(ti, 1.0, sm6);	
		  	   
	float Light1 = WeatherSetts(la, 0.0);
	float Light2 = WeatherSetts(spworld, 0.0);	
	float3 AmbientLighting = WeatherSetts(AmbLight*1.5, AmbCloudy)*shL*shb;
	float3 MoonC = pow(color.xyz*0.4, 0.5);	
	float3 MoonLight = specm*float3(0.0784, 0.137, 0.263)*MoonC*Light2*1.8;
       	   MoonLight = WeatherSetts(MoonLight, 0.0);
	
	float3 DFL1 = diff*lt2*50.0*Light1*colorlightting*refA1; // Освещение
	
	float3 CC1 = ssao*(SkyRefl+(AmbientLighting)) + shadow*pow(DFL1, 0.95);		
	float3 CC2 = ssao2*(SkyRefl+(AmbientLighting) + shadow*pow(DFL1, 0.95));		
	
	       color.xyz*= lerp(CC1, CC2, car2); // Освещение + Тени + SSAO
	       color.xyz+= ti*ssao*MoonLight;
		  
    float g2 = -0.02 + dot(X2, npos.xyz);	
		  g2 = pow(g2, 130.0);
	      g2 = (g2/0.01)*0.01;
          g2 = saturate(g2)*4.0;

	float3 DFL2 = WeatherSetts(mshadow*g2*0.01*colorlightting, 0.0);
	
	       color.xyz+= lerp(0.0, pow(DFL2, 1.0/1.4), m0); // Освещение для шин и салона

	float pedmask = lerp(1.0, 0.65, m2);
	       color.xyz+= lerp(0.0, spec2*specColor*c0*Light2, mshadow*(m1+m2)*pedmask*fadefact); // Спекуляр для всего остального		   

	float3 DFL3 = WeatherSetts(specmix*spcar*colorlightting, 0.0);
		  color.xyz+= lerp(0.0, pow(DFL3, 0.67), car2*mshadow);	 // Спекуляр для тачек   			   
		  color.xyz = pow(color.xyz,0.454545);		  
		 
	             float4 res = color;
	if (depth>=0.999999)res = color1;
	
	float fr = 0.001-dot(npos, -camdir);		
		  fr = saturate(pow(fr, 0.0111));
		  	
		   res.xyz+= lerp(lerp(0.0, ssao*cubemap*1.85, saturate(m0+m2*0.25)), 0.0, 0.97*fr);
	return res;
}


////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 PS_SADX(VS_OUTPUT_POST dx, float2 vPos : VPOS) : COLOR
{
// Цвет, глубина, тени и прочее... ============================
	float2 txcoord = dx.txcoord.xy;
	float4 colorOriginal = tex2D(SamplerOriginal, txcoord.xy); // Цвет	
	float orig1 = pow(tex2D(SamplerOriginal, txcoord.xy).rgb*1.2, 1.65);	
	float3 orig2 = tex2D(SamplerOriginal, txcoord.xy).rgb;	
	
	float4 color = GetLighting(txcoord); // Освещение + тени		
// Маски ======================================================
	float m0 = FuncMasking(txcoord), // Авто Фулл		
          m1 = maskCar(txcoord),   // Авто + перс
          m2 = maskPed(txcoord),   // Перс
          m3 = maskWater(txcoord); // Вода	
// Tone Mapping ===============================================
	float3 colorOr = color.xyz;	
	       color.rgb = tonemapping(color.xyz*3.0);
	       color.rgb = saturate(color.xyz);
	       color.rgb = pow(color.xyz,tg);	
	       color.rgb = lerp(colorOr, color.rgb, saturate(m2+m1+m3));
// ============================================================
    float3 st = normalize(orig2.xyz);
	float3 ct=orig2.xyz/st.xyz;
	       ct=pow(ct, 0.0);
           st.xyz = pow(st.xyz, 1.05);   
           orig2.xyz = ct*st.xyz;  
           orig2.xyz*= 2.0;

   float t1 = smoothstep(0.0, 1.0, GameTime),
         t1a = smoothstep(1.0, 2.0, GameTime),    
         t2 = smoothstep(2.0, 4.0, GameTime),   	 			 
         t4 = smoothstep(6.0, 22.0, GameTime);		 
	
   float s1 = smoothstep(0.0, 4.0, GameTime),	  
         s2 = smoothstep(4.0, 5.0, GameTime),	 
         s3 = smoothstep(5.0, 6.0, GameTime),			 
         s4 = smoothstep(6.0, 7.0, GameTime),
         s5e = smoothstep(8.0, 11.0, GameTime),		
         s5 = smoothstep(16.0, 17.0, GameTime),
         s6 = smoothstep(18.0, 19.0, GameTime),
         s7 = smoothstep(19.0, 20.0, GameTime),
         s8 = smoothstep(22.0, 24.0, GameTime);
		 
   float ti = lerp(1.0, 1.0, t1);
         ti = lerp(ti, 0.5, t1a);
         ti = lerp(ti, 0.1, t2);     
         ti = lerp(ti, 0.0, s2);   
         ti = lerp(ti, 0.0, s3); 		  
         ti = lerp(ti, 0.0, t4);
         ti = lerp(ti, 1.0, s8);		 
	
   float3 ti2 = lerp(float3(0.667, 0.863, 0.98), float3(0.667, 0.863, 0.98), s1);     
          ti2 = lerp(ti2, float3(0.667, 0.863, 0.98), s2);   
          ti2 = lerp(ti2, float3(0.784, 0.914, 0.98), s3); 		  
          ti2 = lerp(ti2, float3(0.824, 0.863, 0.882), s4);
          ti2 = lerp(ti2, float3(0.824, 0.863, 0.882), s5e);
          ti2 = lerp(ti2, float3(0.824, 0.863, 0.882), s5);
          ti2 = lerp(ti2, float3(0.784, 0.914, 0.98), s6);
          ti2 = lerp(ti2, float3(0.667, 0.863, 0.98), s7);
          ti2 = lerp(ti2, float3(0.667, 0.863, 0.98), s8);		  

		   color.rgb*= WeatherSetts(1.0, ti2);
		   color.rgb = lerp(color.rgb, orig2, ti*0.7*saturate(orig1)*saturate((m0+0.09)+m2*0.7)); //Ночной прелайт
		   
		   color.rgb = lerp(color.rgb, colorOriginal.rgb, saturate(m3));	   
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
		PixelShader  = compile ps_3_0 PS_AO();

		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}

technique PostProcess2
{
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_Blur();

		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}

technique PostProcess3
{
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_Blur1();

		ALPHATESTENABLE=FALSE;
		SEPARATEALPHABLENDENABLE=FALSE;
		AlphaBlendEnable=FALSE;
		SRGBWRITEENABLE=FALSE;
	}
}


technique PostProcess4
{
	pass P0
	{
		VertexShader = compile vs_3_0 VS_PostProcess();
		PixelShader  = compile ps_3_0 PS_SADX();
	}
}

