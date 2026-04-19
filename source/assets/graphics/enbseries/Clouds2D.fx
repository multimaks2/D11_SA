texture2D sNs < string ResourceName="Noise.png"; >;
texture2D sNs2 < string ResourceName="Noise2.png"; >;
texture2D sNs4 < string ResourceName="Noise4.png"; >;

sampler2D SamplerNs = sampler_state { Texture = (sNs); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = Wrap; AddressV = Wrap; AddressW = Wrap; MIPMAPLODBIAS = 0.0; };
sampler2D SamplerNs2 = sampler_state { Texture = (sNs2); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = Wrap; AddressV = Wrap; AddressW = Wrap; MIPMAPLODBIAS = 0.0; };
sampler2D SamplerNs4 = sampler_state { Texture = (sNs4); MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR; AddressU = Wrap; AddressV = Wrap; AddressW = Wrap; MIPMAPLODBIAS = 0.0; };

float Coverage(in float v, in float d, in float c)
{
	c = clamp(c - (1.0 - v), 0.0, 1.0 - d)/(1.0 - d);
	c = max(0.0, c * 1.1 - 0.1);
	c = c = c * c * (3.0 - 2.0 * c);
	return c;
}

float3 Wsc(float3 a, float3 a2)
{ 
    float4 wx = WeatherAndTime;
    float3 sc = a,
           sn = a;
    if (wx.x==4 || wx.x==7 || wx.x==8 || wx.x==9 || wx.x==12 || wx.x==15 || wx.x==16) sc = a2;
    if (wx.y==4 || wx.y==7 || wx.y==8 || wx.y==9 || wx.y==12 || wx.y==15 || wx.y==16) sn = a2;	
	
    float3 mix = lerp(sc, sn, wx.z);	
    return mix;
}

float Ws(float a, float a2)
{ 
    float4 wx = WeatherAndTime;
    float sc = a,
          sn = a;
    if (wx.x==3 || wx.x==4 || wx.x==7 || wx.x==8 || wx.x==9 || wx.x==12 || wx.x==15 || wx.x==16) sc = a2;
    if (wx.y==3 || wx.y==4 || wx.y==7 || wx.y==8 || wx.y==9 || wx.y==12 || wx.y==15 || wx.y==16) sn = a2;	
	
    float mix = lerp(sc, sn, wx.z);
   return mix;
}

float Noise1x(in float2 p)
{	
    float t = (Timer.x * 5.0)* 30.0;
          p.xy += 1.0 * t;
    float2 uv = (p * float2(17.2, 17.2));	
	       uv = (uv  + 0.5) / 20.0;
    float r0 = tex2D(SamplerNs, uv*0.04);
	      r0 = 1.0 - r0;
    float r1 = tex2D(SamplerNs2, uv*0.02);
		
    float4 wx = WeatherAndTime;
    float CovC = r0,
          CovN = r0;
	  
    if (wx.x==2 || wx.x==6 || wx.x==11 || wx.x==14 || wx.x==17) CovC = r1;
    if (wx.y==2 || wx.y==6 || wx.y==11 || wx.y==14 || wx.y==17) CovN = r1;

    float mix = lerp(CovC, CovN, wx.z);
   return mix;
}

float Noise2x(in float2 p)
{	
    float t = (Timer.x *30.0)* 30.0; // 
	      p.xy += 1.0 * t;
    float2 uv = (p * float2(17.2, 17.2));
	       uv = (uv + 0.5) / 20.0;
    float r0 = tex2D(SamplerNs2, uv*0.04);
	      r0 = 1.0 - r0;
    float r1 = tex2D(SamplerNs4, uv*0.04);
	
    float4 wx = WeatherAndTime;
    float CovC = r0,
          CovN = r0;
	  
    if (wx.x==11 || wx.x==14 || wx.x==17) CovC = r1;
    if (wx.y==11 ||wx.y==14 || wx.y==17) CovN = r1;

    float mix = lerp(CovC, CovN, wx.z);
   return mix;	
}

float SunLight(float3 worldP)
{		
   float3 f = SunDirection.xyz;	

   float3 v0= f.xyz;
   float3 v2= normalize(float3(-0.09, -0.94, 0.31));	
   float x1 = smoothstep(0.0, 4.0, GameTime),	
         x2 = smoothstep(4.0, 22.0, GameTime),
         x3 = smoothstep(22.0, 23.0, GameTime),	
         x4 = smoothstep(23.0, 24.0, GameTime);	   
   float3 sv = lerp(v2, v0, x1);
          sv = lerp(sv, v0, x2);
          sv = lerp(sv, v2, x3);
          sv = lerp(sv, v2, x4);
		  
   float3 np = normalize(worldP.xyz);
   float f0 = 0.95 - dot(-sv, np);
         f0 = pow(f0, 6.07);
   float r0 = (f0*f0);
   
   return r0/40000.0;
}

float4 GenerateClouds(float3 worldpos, in float3 sunlight)
{		
  float3 f = SunDirection.xyz;	

	float t = (Timer.x * 1000.0)*30.0;	//
	float3 vecm = normalize(float3(-0.09, -0.94, 0.31));
    float3 sv0=-f.xyz;
    float3 sv2=-vecm;
	
   float t0 = GameTime;
   float x1 = smoothstep(0.0, 3.0, t0);
   float x2 = smoothstep(3.0, 4.0, t0);
   float x3 = smoothstep(4.0, 6.0, t0);
   float x4 = smoothstep(6.0, 7.0, t0);
   float xE = smoothstep(8.0, 11.0, t0);
   float x5 = smoothstep(16.0, 17.0, t0);
   float x6 = smoothstep(18.0, 19.0, t0);
   float x7 = smoothstep(19.0, 20.0, t0);
   float xG = smoothstep(20.0, 21.0, t0);  
   float xZ = smoothstep(21.0, 22.0, t0);
   float x8 = smoothstep(22.0, 23.0, t0);
   float x9 = smoothstep(23.0, 24.0, t0);
   float x10 = smoothstep(4.0, 22.0, t0);
   float x11 = smoothstep(22.0, 23.0, t0);	
   float x12 = smoothstep(23.0, 24.0, t0);	   
   float3 sv = lerp(sv2, sv0, x1);
          sv = lerp(sv, sv0, x10);
          sv = lerp(sv, sv2, x11);	  	
          sv = lerp(sv, sv2, x12);	
		  
	float3 wp = worldpos/1.5;
		   wp.x *= 0.1;
		   wp.y *= 0.15;	
		   wp.y -= t * 0.01;	
	float3 wp1 = wp * float3(1.0, 0.5, 1.0) + float3(0.0, t * 0.01, 0.0);
	
	float noise  = 	Noise2x(wp * float3(1.0, 0.5, 1.0) + float3(0.0, t * 0.01, 0.0));
		   wp *= 3.0;
		   wp.xy -= t * 0.05;
		   wp.x += 2.0;
	float3 wp2 = wp;
	
		   wp.x *= 2.0;
		   wp.y *= 2.0;	
		
		  noise += (2.0 - abs(Noise1x(wp) * 0.8)) * 0.25;
		   wp.xy*= 10.0;
		   wp.xy -= t * 0.035;
	float3 wp3 = wp;
		  
          noise += (2.0 - abs(Noise1x(wp) * 1.0)) * 0.03;
		  noise /= 0.80;
		  
float4 wx = WeatherAndTime;
float CovC = 0.25;
float CovN = 0.25;
if (wx.x==0) CovC = 0.0;
if (wx.x==3) CovC = 0.50;
if (wx.x==6) CovC = 0.45;
if (wx.x==10) CovC = 0.0;
if (wx.x==11) CovC = 0.30;
if (wx.x==13) CovC = 0.30;

if (wx.y==0) CovN = 0.0;
if (wx.y==3) CovN = 0.50;
if (wx.y==6) CovN = 0.45;
if (wx.y==10) CovN = 0.0;
if (wx.y==11) CovN = 0.30;
if (wx.y==13) CovN = 0.30;     
		  
	float ti = max(sin(Timer.x*100.0*0.01),0);  
		  	  
	float coverage = lerp(CovC, CovN, wx.z);	  	
	float dn = 0.1 - 0.3 * 0.3;  
	noise = Coverage(coverage, dn, noise);
	
	float d0 = Noise2x(wp1 + sv.xyz * 0.70 / 2.3);
		  d0 += (2.0 - abs(Noise2x(wp2 + sv.xyz * 0.70 / 2.3) * 0.8)) * 0.10;	
		  d0 += (2.0 - abs(Noise1x(wp3 + sv.xyz * 0.70 / 2.3) * 1.0)) * 0.02; 
	      d0 = Coverage(0.64, dn, d0);
	
	float bf = lerp(clamp(pow(noise, 0.5) * 1.0, 0.0, 1.0), 0.5, pow(sunlight, 1.0));
		  d0 *= bf;	
			  
   float3 lt = lerp(float3(0.0275, 0.0353, 0.0471)*5.8, float3(0.0275, 0.0353, 0.0471)*5.8, x1);
          lt = lerp(lt, float3(0.64, 0.6, 0.6)*0.7, x2);
          lt = lerp(lt, float3(1.0, 0.727, 0.635)*1.3, x3);
          lt = lerp(lt, float3(1.0, 0.95, 0.9)*1.10, x4);
          lt = lerp(lt, float3(1.0, 1.0, 1.0)*1.35, xE);
          lt = lerp(lt, float3(1.0, 0.95, 0.9)*1.35, x5);
          lt = lerp(lt, float3(1.0, 0.75, 0.7), x6);		 
          lt = lerp(lt, float3(1.0, 0.75, 0.7), x7);
		  lt = lerp(lt, 0.5, xG);
		  lt = lerp(lt, float3(0.0275, 0.0353, 0.0471)*5.8, xZ);
          lt = lerp(lt, float3(0.0275, 0.0353, 0.0471)*5.8, x8);
          lt = lerp(lt, float3(0.0275, 0.0353, 0.0471)*5.8, x9);

   float3 ShColor = float3(0.114, 0.126, 0.138);		  
   float3 ShColor2 = float3(0.051, 0.0784, 0.118)*0.65;
   
   float3 sh = lerp(ShColor2, ShColor2, x1);
          sh = lerp(sh, ShColor*1.5, x2);
          sh = lerp(sh, ShColor*3.0, x3);
          sh = lerp(sh, float3(0.380, 0.397, 0.425)*1.15, x4);
          sh = lerp(sh, float3(0.380, 0.397, 0.425)*1.35, xE);
          sh = lerp(sh, float3(0.380, 0.390, 0.425)*1.20, x5);
          sh = lerp(sh, ShColor*3.2, x6);	 
          sh = lerp(sh, ShColor*2.8, x7);
		  sh = lerp(sh, ShColor*2.2, xG);
		  sh = lerp(sh, ShColor2*1.3, xZ);
          sh = lerp(sh, ShColor2*1.3, x8);
          sh = lerp(sh, ShColor2*1.3, x9);

   float3 fmix = lerp(0.03, 0.11, x1);
          fmix = lerp(fmix, 0.11, x2);
          fmix = lerp(fmix, 0.3, x3);			  
          fmix = lerp(fmix, 0.5, x4);
          fmix = lerp(fmix, 0.5, xE);
          fmix = lerp(fmix, 0.5, x5);
          fmix = lerp(fmix, 0.3, x6);
          fmix = lerp(fmix, 0.2, x7); 
          fmix = lerp(fmix, 0.12, xG);	
          fmix = lerp(fmix, 0.1, xZ);
          fmix = lerp(fmix, 0.08, x8);	  
          fmix = lerp(fmix, 0.03, x9);		  
			  
   float3 fmix2 = lerp(0.02, 0.1, x1);
          fmix2 = lerp(fmix2, 0.1, x2);
          fmix2 = lerp(fmix2, 0.2, x3);			  
          fmix2 = lerp(fmix2, 0.2, x4);
          fmix2 = lerp(fmix2, 0.2, xE);
          fmix2 = lerp(fmix2, 0.2, x5);
          fmix2 = lerp(fmix2, 0.1, x6);
          fmix2 = lerp(fmix2, 0.1, x7); 
          fmix2 = lerp(fmix2, 0.09, xG);	
          fmix2 = lerp(fmix2, 0.08, xZ);
          fmix2 = lerp(fmix2, 0.05, x8);	  
          fmix2 = lerp(fmix2, 0.02, x9);
          fmix2*= 2.0;		  
   float3 ColorSun = lerp(float3(0.0392, 0.0588, 0.0784)*4.0, float3(0.05, 0.04, 0.02)*0.0, x1);
          ColorSun = lerp(ColorSun, float3(0.05, 0.025, 0.02)*2.5, x2);
          ColorSun = lerp(ColorSun, float3(0.45, 0.07, 0.0), x3);
          ColorSun = lerp(ColorSun, float3(0.392, 0.275, 0.118)*0.3, x4);
          ColorSun = lerp(ColorSun, float3(0.392, 0.275, 0.118)*0.2, xE);
          ColorSun = lerp(ColorSun, float3(0.392, 0.275, 0.118)*0.2, x5);
          ColorSun = lerp(ColorSun, float3(0.392, 0.08, 0.0), x6); 
          ColorSun = lerp(ColorSun, float3(0.392, 0.08, 0.0), x7);
		  ColorSun = lerp(ColorSun, float3(0.392, 0.118, 0.0588), xG);
		  ColorSun = lerp(ColorSun, float3(0.392, 0.118, 0.0588)*0.4, xZ);
          ColorSun = lerp(ColorSun, float3(0.0392, 0.0588, 0.0784)*4.0, x8);		  
          ColorSun = lerp(ColorSun, float3(0.0392, 0.0588, 0.0784)*4.0, x9);

   float3 m0 = float3(min(1.0, d0), min(1.0, d0), min(1.0, d0));
   float3 color = lerp((pow(sunlight, 0.2)*ColorSun*2.0)+lt*0.60, sh*1.1, m0); 
   float3 colorPasm = lerp(fmix, fmix2, m0); 
   float3 SunCurrent = Wsc(color, colorPasm);
   float CloudsAlpha = Ws((noise*noise*noise), 0.0);
	
	float4 r0 = float4(SunCurrent.rgb, CloudsAlpha);
	return r0;

}

float IntersectPlane(float3 pos, float3 dir)
{
	return -pos.z/dir.z;
}