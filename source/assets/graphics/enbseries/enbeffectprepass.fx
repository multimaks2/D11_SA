
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
float4 ScreenSize; float4 Timer;float ENightDayFactor;
float4 SunDirection; float EInteriorFactor; float FadeFactor; float FieldOfView;
float4 MatrixVP[4]; float4 MatrixInverseVP[4]; float4 MatrixVPRotation[4]; float4 MatrixInverseVPRotation[4];
float4 MatrixView[4];float4 MatrixInverseView[4];float4 CameraPosition;float GameTime;float4 CustomShaderConstants1[8];
float4 WeatherAndTime; float4 FogFarColor; float4 FogParam;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Textures
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
texture2D texOriginal;
texture2D texColor;
texture2D texDepth;
texture2D texBlueN < string ResourceName="BlueNoise.png"; >;
texture2D texNoise1 < string ResourceName="clouds1.png"; >;
texture2D texNoise2 < string ResourceName="clouds2.png"; >;
texture2D texNoise3 < string ResourceName="clouds3.png"; >;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Sampler Inputs
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

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

sampler2D SamplerBlueNoise = sampler_state { Texture = <texBlueN>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;AddressW=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise1 = sampler_state { Texture = <texNoise1>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise2 = sampler_state { Texture = <texNoise2>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise3 = sampler_state { Texture = <texNoise3>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};

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

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

bool Enable_Debug <
string UIName = "Enable_Debug";
> = {false};

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
           pos.xy*=0.4;
		   
	dx.vpos = pos;
	dx.txcoord.xy = v.txcoord.xy;
	
	return dx;
}

VS_OUTPUT_POST VS_PostProcess3(VS_INPUT_POST v)
{
	VS_OUTPUT_POST dx;
	float4 pos = float4(v.pos.x,v.pos.y,v.pos.z,1.0);
           pos.xy*=0.85;
		   
	dx.vpos = pos;
	dx.txcoord.xy = v.txcoord.xy;
	
	return dx;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 WorldPos(in float2 coord)
{
   float4 tv; 
          tv.xy = coord.xy*2.0-1.0;   
          tv.y = -tv.y;   
          tv.z = tex2D(SamplerDepth, coord.xy).x;
          tv.w = 1.0;
   float4 wp;
          wp.x = dot(tv, MatrixInverseVPRotation[0]);
          wp.y = dot(tv, MatrixInverseVPRotation[1]);
          wp.z = dot(tv, MatrixInverseVPRotation[2]);
          wp.w = dot(tv, MatrixInverseVPRotation[3]);
          wp.xyz/= wp.w;
   return wp;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float noise(in float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);
	float2 uv = (p.xy+float2(37.0,17.0)*p.z) + f.xy;
	float2 rg = tex2Dlod(SamplerNoise1, float4((uv.xy+0.5)/256.0,0,0)).yx;
	return lerp(rg.x, rg.y, saturate(f.z));
}

float ns( float3 p )
{
 float f;
       f = 0.5000*noise(p);
       f+= 0.2000*noise(p)*1.6;
       f+= -0.1500*noise(p);
       f+= 0.1000*noise(p);
return f;
}

#define cl_start 275.0
#define cl_end   730.0
#define cl_time (Timer.x * 10000.0*0.4)

float wsc2(float a, float a2)
{ 
    float4 wx = WeatherAndTime;
    float sc = a,
          sn = a;
    if (wx.x==0 || wx.x==2 || wx.x==6 || wx.x==11 || wx.x==13 || wx.x==17 || wx.x==18) sc = a2;
    if (wx.y==0 || wx.y==2 || wx.y==6 || wx.y==11 || wx.y==13 || wx.y==17 || wx.y==18) sn = a2;
	
    float mix = lerp(sc, sn, wx.z);
   return mix;
}

float cl2(float3 p, out float cloudHeight, float bnoise)
{
    float cov = wsc2(1.7, 3.0);
  	cloudHeight = saturate((p.z-cl_start)/(cl_end-cl_start));
    float3 v0 = float3(0.05, 0.2, -0.65);

    p += v0 * p.z;
    p.y += cl_time*20.0;

    float3 p1 = p;
	       p1.xy-= cl_time*146.0;
		   
    float3 p2 = p;
	       p2.xy+= cl_time*15.0;		   
	
    float clouds1 = clamp((tex2Dlod(SamplerNoise2, float4(-0.00005*9.99*p2.yx * 0.93,0,0)).x*1.2*0.56-0.18*0.0)*0.45, 0.0, 0.35);	
          clouds1*= smoothstep(0.0, 0.05, cloudHeight) * smoothstep(1.0, 0.5, cloudHeight);	
          clouds1 = lerp(1.0, clouds1, 0.85);
		  
		  p.xy += cl_time*35.0;		
    float clouds2 = clouds1*clamp((tex2Dlod(SamplerNoise3, float4(0.0002*0.5*p1.yx * 0.93 * float2(0.9,1.0), 0,0)).x*2.15*0.56-0.28*cov)*0.8, 0.0, 0.35);
          clouds2 *= smoothstep(0.0, 0.5, cloudHeight) * smoothstep(1.0, 0.5, cloudHeight);				  
	
  	float sh = pow(clouds2, 0.3+0.3*smoothstep(0.1, 0.5, cloudHeight));
    	if(sh <= 0.0) return 0.0;

	float d = max(0.0, sh-0.2);
	float res = 0.2*saturate(5.0 * d-(ns(bnoise+p*0.05*1.3) + 0.05) + 0.15)*(ns(bnoise+p*0.05*1.3) + 0.05);
   return res;	
}

float bn(float2 vpos)
{
	float n = tex2Dlod(SamplerBlueNoise, float4(vpos.xy*0.01, 0.0, 0.0));
   return n;
}

float m1(float a1, float a2, float a3, float a4, float a5)
{
    float res = clamp((a1-a3)/(a2-a3),0.0,1.0);
	      res = res*(a4-a5)+a5;
   return res;
}

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

float GetDitherring(float2 vpos)
{
	half res = 0.0;
	for(half i = 1-2; i<= 0; i++)
	{
      half2 c = floor(vpos*exp2(i)) % 2.0;
      half d2 = 2.0*c.x-4.0*c.x*c.y+3.0*c.y;
		res += exp2(2.0*(i+2.0))*d2;
	}
	half res1 = 4.0*exp2(4.0)-4.0;
	return res/res1+1.0/exp2(4.0);

}

float4 VolumetricClouds(float3 campos, float3 wpos, float2 vpos, float depth)
{
   float3 dir = normalize(wpos.xyz); 
   float d0 = distance(float3(0,0,0), wpos.xyz),
         m0 = clamp(campos.z, 300.0, 600.0),
         BlueNoise = bn(vpos.xy*0.3)*0.7,
         r1 = (m0 - campos.z)/dir.z,
         r2 = 1.0,
         alpha = 0.0;
		  
    int samples = 256;
    float4 color = 1.0;
 
    float3 rayPlane = float3(campos.xy + dir.xy * r1, m0);
    alpha = r1;

    if (r1 >= 0.0 && r1 < 6000.0)
    {		   
    float T = 1.0;    
    float d3 = dithering(vpos.xy, 4.0) * -0.2; 	
    if (depth == 1) d0 = 6000.0;
	
	[fastopt]
    for(int i=0; T >= 0.13 && rayPlane.z <= 600.0 && rayPlane.z >= 300.0 && (i < samples) && r1 - r2 < d0; i++)			
	{        
        float cloudHeight = saturate((rayPlane.z-600.0)/(300.0-600.0));
		float density = cl2(rayPlane, cloudHeight, BlueNoise);				
		float rayStep = m1(density+d3, 1.0, -15.5, 6.0, 155.0)*m1(r1, 0.0, 10000.0, 1.0, 5.09)/1.17;			
			
		if(density>0.0)
		{	
		    float cov = exp(-density*rayStep/1.5);
		    float a = T*(cov/1.0);
                  alpha = alpha*(1.0-a)+a*r1;			
		          T*= cov;		
        }		
		rayPlane += dir*rayStep;
        r1 += rayStep;
        r2 = rayStep;
	}
		   color.xyz = 1.0;		   
           color.w = saturate((T - 0.95)/(-0.82));
    return color;
    }
    return float4(0.0, 0.0, 0.0, 0.0);	
}

float4 PS_CloudsAlpha(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
   float2 coord = IN.txcoord.xy;
   float4 r0 = tex2D(SamplerColor, coord.xy);
   float  d0 = tex2D(SamplerDepth, coord.xy).x;
   float4 wp = WorldPos(coord);
//---------------------------------------------------------------------------------	
   float4 clouds = float4(0.0,0.0,0.0,1.0); 
          clouds = VolumetricClouds(CameraPosition.xyz, wp.xyz, vPos.xy, d0);
          clouds.a = -clouds.a;

   float SkyMask = tex2D(SamplerDepth, coord.xy).x > 0.999999;

		r0.xyz = lerp(SkyMask, 0.0, min(1.0, clouds.a*clouds.a*1.0));
 return r0;
	 
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float gdb
<
        string UIName="GodRays : Brightness";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=2.0;
> = {1.25};

float mdb
<
        string UIName="MoonRays : Brightness";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=2.0;
> = {1.05};

float RayAlpha
<
        string UIName="GodRays : Alpha";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=1.5;
> = {1.02};

#define SAMPLES	6
#define BSIZE 4

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float3 Config2(float3 c1, float3 c2, float3 c3, float3 c4, float3 c5e, float3 c5, float3 c6, float3 c7,  float3 c7b, float3 c8, float3 c9)
{
   float t0 = GameTime;		
   float x1 = smoothstep(0.0, 2.0, t0);
   float x1z = smoothstep(2.0, 3.0, t0);   
   float x2 = smoothstep(3.0, 4.0, t0);
   float x3 = smoothstep(4.0, 6.0, t0);
   float x4 = smoothstep(6.0, 7.0, t0);
   float x5e = smoothstep(8.0, 11.0, t0);
   float x5 = smoothstep(16.0, 17.0, t0);
   float x6 = smoothstep(18.0, 19.0, t0);
   float x7 = smoothstep(19.0, 20.0, t0);
   float x7a = smoothstep(20.0, 21.0, t0);  
   float x7b = smoothstep(21.0, 22.0, t0);   
   float x8 = smoothstep(22.0, 23.0, t0);
   float x9 = smoothstep(23.0, 24.0, t0);

   
   float3 t1 = lerp(c1, c1, x1); // Ночь
          t1 = lerp(t1, c1*0.4, x1z); // Ночь 1   
          t1 = lerp(t1, c2*0.4, x2); // Ночь 2
          t1 = lerp(t1, c3, x3); // Утро 3
          t1 = lerp(t1, c4, x4); // Утро 4
          t1 = lerp(t1, c5e, x5e); // День 5
          t1 = lerp(t1, c5, x5); // День 5
          t1 = lerp(t1, c6, x6); // Вечер 6	 
          t1 = lerp(t1, c7, x7); // Вечер 7
          t1 = lerp(t1, c7, x7a); // Вечер 7
          t1 = lerp(t1, c7b, x7b); // Вечер 7		  
          t1 = lerp(t1, c8, x8); // Вечер 8
          t1 = lerp(t1, c9, x9); // Вечер 9		  
   return t1;
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


float3 fixSunDirection()
{
   float3 v0 = SunDirection.xyz;
   float3 v2 = normalize(float3(-0.09, -0.94, 0.31));	
   float x1 = smoothstep(0.0, 2.0, GameTime);
   float x1z = smoothstep(2.0, 4.0, GameTime);    
   float x2 = smoothstep(4.0, 22.0, GameTime);
   float x3 = smoothstep(22.0, 23.0, GameTime);	
   float x4 = smoothstep(23.0, 24.0, GameTime);	   
   float3 sv = lerp(v2, v2, x1);
          sv = lerp(sv, v0, x1z);   
          sv = lerp(sv, v0, x2);
          sv = lerp(sv, v2, x3);
          sv = lerp(sv, v2, x4);   
   return sv;
}

float4 PS_GodRays(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{   
    float2 coord = IN.txcoord.xy;
    float3 SunDir = fixSunDirection();
    float4 wpos = WorldPos(coord);
    float3 dir = normalize(wpos.xyz);   	
    float f = 0.0;
    float il = 1.0;        
	float4 r = float4(SunDir.xyz, 1.0);	 
	float4 w = 0.0;
	       w.x = mul(r, MatrixVPRotation[0]);
		   w.y = mul(r, MatrixVPRotation[1]);
	       w.z = mul(r, MatrixVPRotation[2]);
	       w.w = mul(r, MatrixVPRotation[3]);
	       w.xyz/= w.w;
	       w.y = -w.y;	
	       w.xy = w.xy*0.5+0.5;

    float d3 = dithering(vPos.xy, 3.0)*1.0;

    float mask = saturate(0.004 - dot(-SunDir.xyz, dir.xyz));
          mask = pow(mask, 1.728);	 	 
          mask = ((mask*mask)/5.0)*3.82;
		  mask*= 0.3425;
		
    float2 sm = (coord - w)*(1.0/float(SAMPLES));
	
      for(int i=0; 1.0 >= 0.1 && ( i<SAMPLES) && mask > 0; i++)
      {
        coord -= sm;
        float2 uv = coord+sm*d3;		
		float2 center;
               center.x = uv.x-0.5;
               center.y = uv.y-0.5;
		float z = 1.0/1.25;
		float2 uv2 = z*(center.xy*0.5)+0.5;		
	
		float gr = tex2D(SamplerColor, uv2).x;
		float4 s = 0.0;
		       s.xyz = gr;
	           s.w = uv.y<0.0||uv.y>1.0 ? 0.0:1.0;
	           s.w*= uv.x<0.0||uv.x>1.0 ? 0.0:1.0;		
	           s*= il * 4.21;	
        f+= mask*s*s.w;
        il*= 0.94;
      }

    float ra = Config2(0.75, 1.07,  1.07,  1.07,  1.07,  1.07,  1.07,  0.0,  0.0,  0.0,  0.75);		  
    float a = WeatherSetts(ra*RayAlpha, 0.0);  
          f = saturate(mask*pow(f, 0.72)*0.2665)*a;
   return f;
}



float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma))/sigma;
}

float normpdf3(in float3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma))/sigma;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 PS_Blur(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;
    float2 center;
           center.x = coord.x-0.5;
           center.y = coord.y-0.5;
    float s = 1.0/0.58963;
    float2 uv = s*(center.xy*0.5)+0.5;
		
	float3 c;	
	float3 c2 = tex2D(SamplerColor, uv.xy).rgb;
	
	const int kSize = (BSIZE-1)/2;
	float kernel[BSIZE];		
	float final = 0.0;
	float z = 0.0;
	
		for (int j = 0; j <= kSize; ++j)
		{
			kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), 20.0);
		}
		
	float f;
	float z1 = 1.0/normpdf(0.0, 1.07);
			
		for (int i=-kSize; i <= kSize; ++i)
		{
			for (int j=-kSize; j <= kSize; ++j)
			{
				c = tex2D(SamplerColor, uv.xy+float2(float(i)*0.0019*0.5,float(j)*0.0019*0.5)).rgb;
				f = normpdf3(c-c2, 1.07)*z1*kernel[kSize+j]*kernel[kSize+i];
				z+= f;
				final+= f*c;
			}
		}			
	float res = saturate(final/z);		
   return res;
}


////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float Radius
<
        string UIName="GodRays : Radius";
        string UIWidget="Spinner";
        float UIMin=1.0;
        float UIMax=2.0;
> = {1.05};

float4 PS_Blur2(VS_OUTPUT_POST IN, float2 vPos : VPOS) : COLOR
{
	float2 coord = IN.txcoord.xy;
	float4 color = tex2D(SamplerOriginal, coord.xy);
    if(Enable_Debug) color = 0.0;
	
	float3 c = 0.0;	
	float3 c2 = tex2D(SamplerColor, coord.xy);
	float z = 0.0;		
	float z1 = 1.0/normpdf(0.0, 200.0);	
	float3 f = 0.0;
	
    for (int i = 0; i <= 8; ++i)	
	    {				
		 float angle = 0.7854 * i;
		 float2 s = float2(cos(angle), sin(angle));
		        s/= 100.0;		   
				   
	      c = tex2D(SamplerColor, coord.xy+s*0.2);
		  z+= normpdf3(c-c2, 0.2)*z1;
		  f+=(normpdf3(c-c2, 0.2)*z1)*c;
	    }
	float3 blur = f/z;
	
    float3 co1 = float3(0.627, 0.804, 0.98),
           co2 = float3(1, 0.667, 0.784),
           co3 = float3(1, 0.824, 0.588),
           co4 = float3(0.882, 0.878, 0.706),
           co5e = float3(0.804, 0.804, 0.784),
           co5 = float3(0.882, 0.875, 0.765),		   
           co6 = float3(1, 0.745, 0.588),
           co7 = float3(1, 0.784, 0.667),
           co8 = float3(0.863, 0.922, 1),		   
           co9 = float3(0.627, 0.804, 0.98);	

    float3 colorRays = Config2(co1*mdb, co2,  co3,  co4,  co5e,  co5,  co6,  co7,  co7,  co8,  co9*mdb);		
    float rad = Config2(1.6, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 1.4, 1.6);		
	float alpha = saturate(pow(blur, rad*Radius));

	       color.rgb = lerp(color.rgb, colorRays*gdb,  alpha);		   		   
	return color;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

technique PostProcess
{
	pass P0
	{
      VertexShader = compile vs_3_0 VS_PostProcess2();
      PixelShader  = compile ps_3_0 PS_CloudsAlpha();
	}
}

technique PostProcess2
{
	pass P0
	{
      VertexShader = compile vs_3_0 VS_PostProcess3();
      PixelShader  = compile ps_3_0 PS_GodRays();
	}
}

technique PostProcess3
{
	pass P0
	{
      VertexShader = compile vs_3_0 VS_PostProcess();
      PixelShader  = compile ps_3_0 PS_Blur();
	}
}

technique PostProcess4
{
	pass P0
	{
      VertexShader = compile vs_3_0 VS_PostProcess();
      PixelShader  = compile ps_3_0 PS_Blur2();
	}
}
/**/