// beta version

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

texture2D texBlueN < string ResourceName="BlueNoise.png"; >;
texture2D texNoise1 < string ResourceName="clouds1.png"; >;
texture2D texNoise2 < string ResourceName="clouds2.png"; >;
texture2D texNoise3 < string ResourceName="clouds3.png"; >;

sampler2D SamplerBlueNoise = sampler_state { Texture = <texBlueN>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;AddressW=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise1 = sampler_state { Texture = <texNoise1>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise2 = sampler_state { Texture = <texNoise2>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};
sampler2D SamplerNoise3 = sampler_state { Texture = <texNoise3>;MinFilter=LINEAR;MagFilter=LINEAR;MipFilter=LINEAR;AddressU=Wrap;AddressV=Wrap;SRGBTexture=FALSE;MaxMipLevel=0;MipMapLodBias=0;};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 wpc(in float2 c)
{
   float4 tv; 
          tv.xy = c.xy*2.0-1.0;
          tv.y = -tv.y;   
          tv.z = tex2D(SamplerDepth, c.xy).x;
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

float ldr < string UIName="Clouds3D : Direct Lighting"; string UIWidget="Spinner";	float UIStep=0.01;	float UIMin=0.0;	float UIMax=10.0; > = {1.77};
float amb < string UIName="Clouds3D : Ambient Lighting"; string UIWidget="Spinner";	float UIMin=0.0;	float UIMax=100.0; > = {32.0};

#define cl_start 275.0
#define cl_end   730.0
#define cl_time (Timer.x * 10000.0*0.4)

float noise(in float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);
	float2 uv = (p.xy+float2(37.0,17.0)*p.z) + f.xy;
	float2 rg = tex2Dlod(SamplerNoise1, float4((uv.xy+0.5)/256.0,0,0)).yx;
	return lerp( rg.x, rg.y, saturate(f.z));
}

float ns(float3 p)
{
 float v;
       v = 0.5000*noise( p );
       v+= 0.2000*noise( p )*1.6;
       v+= -0.1500*noise( p );
       v+= 0.1000*noise( p );
return v;
}

float3 fixSunDir()
{
   float3 v0 = SunDirection.xyz,
          v2 = normalize(float3(-0.09, -0.94, 0.31));
   float x1 = smoothstep(0.0, 2.0, GameTime),
         x1z = smoothstep(2.0, 4.0, GameTime),  
         x2 = smoothstep(4.0, 22.0, GameTime),
         x3 = smoothstep(22.0, 23.0, GameTime),
         x4 = smoothstep(23.0, 24.0, GameTime);
   float3 sv = lerp(v2, v2, x1);
          sv = lerp(sv, v0, x1z);
          sv = lerp(sv, v0, x2);
          sv = lerp(sv, v2, x3);
          sv = lerp(sv, v2, x4);
   return sv;
}

float sunray(in float2 coord)
{	  
   float3 sv = fixSunDir();	  
   float4 wp = wpc(coord);
   float3 p = normalize(wp.xyz);

   float f = saturate(-0.044 - dot(-sv, p));
         f = pow(f, 2.0);
         f = (f*f)/20.0;
  return f;
}

float3 wsc(float3 a, float3 a2)
{ 
    float4 wx = WeatherAndTime;
    float3 sc = a,
           sn = a;
    if (wx.x==4 || wx.x==7 || wx.x==8 || wx.x==9 || wx.x==12 || wx.x==15 || wx.x==16) sc = a2;
    if (wx.y==4 || wx.y==7 || wx.y==8 || wx.y==9 || wx.y==12 || wx.y==15 || wx.y==16) sn = a2;	
	
    float3 mix = lerp(sc, sn, wx.z);	
    return mix;
}

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

float cl1(float3 p, out float cloudHeight, float rc, float bnoise)
{
    float cov = wsc2(1.7, 3.0);
  	cloudHeight = saturate((p.z-cl_start)/(cl_end-cl_start));
	
    p += 0.0 * p.z;
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
	
	float t = -0.15;
	float d = max(0.0, sh-0.0);
    return 0.05*saturate(5.0 * d-(ns(bnoise+p*0.05*0.1) + 0.0) + t - rc);
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

float3 tonemapping(float3 color)
{
	float3 cn0 = normalize(color.xyz),
	       ct0=color.xyz/cn0.xyz;
	       ct0=pow(ct0, 0.65);
           cn0.xyz = pow(cn0.xyz, 0.9);   
           color.xyz = ct0*cn0.xyz;  
           color.xyz*= 0.04;

    float maxcol = max(color.x, max(color.y, color.z));		 		 
    float3 r0 = 1.0-exp(-maxcol),
           r2 = 1.0-exp(-color);	

         color.xyz = lerp(float3(r2.x, r2.y, r2.z), color*float3(r0.x, r0.y, r0.z)/maxcol, 0.05);
  return color;	
}

float bn(float2 vpos)
{
	float n = tex2Dlod(SamplerBlueNoise, float4(vpos.xy*0.01, 0.0, 0.0));
   return n;
}

float lightRay(float3 p, float3 sundir, float ch, float r, float RayAll, float rc, float bnoise)
{
    int samples = 3;
    float step = (r*150.0)/float(samples);   
    float3 v0 = float3(0.05, 0.2, -0.65);
	
    float lighRay = 0.0;    
    p+= sundir*step*11.5;
    p+= v0 * p.z * float3(1,1,0);	
	
    for(int j=0; j<samples; j++)
    {
        float ch;		
        lighRay += cl1( p + sundir * 0.70 / RayAll * float(j) * step, ch, rc, bnoise);
    }  

	float res = rcp(lighRay*lighRay+0.001*5.37); 
   return res;
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
// Знаю - данный код бред идиота, но пока только так. В будущем избавлюсь от подобного
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

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 VolumetricClouds(float3 campos, float3 wpos, float2 vpos, float depth, float2 coord)
{
   float3 dir = normalize(wpos.xyz); 
   float dist = distance(float3(0,0,0), wpos.xyz),
         m0 = clamp(campos.z, 300.0, 600.0),
         BlueNoise = bn(vpos.xy*0.3)*0.7,
         r1 = (m0 - campos.z)/dir.z,
         r2 = 1.0,
         alpha = 0.0;
		  
    int samples = 256;
    float4 color = 1.0;
	
    float3 co1 = float3(0.196, 0.235, 0.267),
           co2 = float3(0.706, 0.451, 0.353),
           co3 = float3(1, 0.549, 0.412),
           co4 = float3(0.969, 0.784, 0.627),
           co5e = float3(0.925, 0.902, 0.863),
           co5 = float3(0.98, 0.882, 0.784),		   
           co6 = float3(0.588, 0.314, 0.275),
           co7 = float3(0, 0, 0),
           co8 = float3(0, 0, 0),		   
           co9 = float3(0.196, 0.235, 0.267);

    float3 c = Config2(co1, co2*0.5,  co3,  co4,  0.95*co5e,  0.95*co5,  co6,  co7, co7,  co8, co9);

    float3 cd = float3(0.824, 0.784, 0.588),
           cd1 = float3(0.353, 0.275, 0.118),
           cn = float3(0.231, 0.235, 0.251),	   
           AmbM = float3(1.0, 0.961, 1.0),
           AmbD0 = float3(0.737, 0.961, 1.0),
           AmbD1 = float3(0.588, 0.784, 1.0),
           AmbD2 = float3(0.667, 0.824, 0.902),
           AmbE1 = float3(0.725, 0.647, 0.725),	   
           AmbE2 = float3(0.176, 0.216, 0.314),		   
           AmbN = float3(0.373, 0.647, 1.0);
		   
    float3 c2 = Config2(cn*2.0, cd*0.4,  cd,  cd1,  0.0,  0.0,  cd*0.5,  cd*0.4, cd*0.2,  cd*0.1, cn*2.0);
    float3 c3 = Config2(0.04*AmbN, 0.5*AmbM,  0.8*AmbM,  AmbD0,  2.4*AmbD1,  1.4*AmbD2,  0.8*AmbM,  0.6*AmbE1, 0.4*AmbE2, 0.25*AmbE2, 0.04*AmbN);
    float3 c4 = Config2(0.01, 0.2,  0.6,  1.2,  1.2,  0.8,  0.65,  0.35, 0.18, 0.014, 0.01);	
   
    float3 rayPlane = float3(campos.xy + dir.xy * r1, m0);
    alpha = r1;

    if (r1 >= 0.0 && r1 < 6000.0)
    {		   
    float T = 1.0;    

    float d3 = dithering(vpos.xy, 4.0) * -0.2; 	
	float rc1 = saturate(sunray(coord)*7.5);	    
	float3 rc2 = lerp(1.0, 1.0+6.2*c2, rc1);	  
	float rr = lerp(0.2, 0.14, smoothstep(0.25,0.65,SunDirection.z));	

   float sm1 = smoothstep(0.0, 4.0, GameTime),	  
         sm2 = smoothstep(4.0, 5.0, GameTime),	 
         sm3 = smoothstep(5.0, 6.0, GameTime),			 
         sm4 = smoothstep(6.0, 7.0, GameTime),
         sm5e = smoothstep(8.0, 11.0, GameTime),		
         sm5 = smoothstep(16.0, 17.0, GameTime),
         sm6 = smoothstep(18.0, 19.0, GameTime),
         sm7 = smoothstep(19.0, 20.0, GameTime),
         sm8 = smoothstep(22.0, 24.0, GameTime);
	
   float3 ti2 = lerp(float3(0.451, 0.675, 0.843), float3(0.451, 0.675, 0.843), sm1);     
          ti2 = lerp(ti2, float3(0.451, 0.675, 0.843), sm2);   
          ti2 = lerp(ti2, float3(0.635, 0.847, 0.98), sm3); 		  
          ti2 = lerp(ti2, float3(0.725, 0.792, 0.824), sm4);
          ti2 = lerp(ti2, float3(0.725, 0.792, 0.824), sm5e);
          ti2 = lerp(ti2, float3(0.725, 0.792, 0.824), sm5);
          ti2 = lerp(ti2, float3(0.635, 0.847, 0.98), sm6);
          ti2 = lerp(ti2, float3(0.451, 0.675, 0.843), sm7);
          ti2 = lerp(ti2, float3(0.451, 0.675, 0.843), sm8);		  

	float3 CloudyColor = wsc(1.0, ti2);

    if (depth == 1) dist = 6000.0;
	
	[fastopt]
    for(int i=0; T >= 0.16 && rayPlane.z <= 600.0 && rayPlane.z >= 300.0 && (i < samples) && r1 - r2 < dist; i++)			
	{        
        float cloudHeight = saturate((rayPlane.z-600.0)/(300.0-600.0));
        float cloudHeight2 = saturate((rayPlane.z-230.0)/(660.0-230.0));
		float density = cl2(rayPlane, cloudHeight, BlueNoise);
				
		float rayStep = m1(density+d3, 1.0, -15.5, 6.0, 155.0)*m1(r1, 0.0, 10000.0, 1.0, 5.09)/1.17;			
			
		if(density>0.0)
		{
			float lighting = lightRay(rayPlane, fixSunDir(), cloudHeight, rr, 0.04, rc1, BlueNoise);				
            float3 ambientLight = cloudHeight2*c3*amb*100.0;
            float3 ambientCl = cloudHeight2*c4*amb*100.0*CloudyColor;		
		    float3 res = ambientLight + pow(c*lighting * ldr * 0.05 * rc2, 3.2);					
		           res = wsc(res, ambientCl);
                   color.xyz += 0.5 * T * (1.0 - exp(-density * rayStep)) * res;
	
		    float cov = exp(-density*rayStep/1.5);
		    float a = T*(cov/1.0);
                  alpha = alpha*(1.0-a)+a*r1;			
		          T*= cov;		
        }		
		rayPlane += dir*rayStep;
        r1 += rayStep;
        r2 = rayStep;
	}
		   color.xyz = tonemapping(color.xyz*0.14);
		   color.xyz = pow(color.xyz, 0.75);		   
           color.w = saturate((T - 0.95)/-0.82);
		   color.w*= wsc(smoothstep(11000.0, 500.0, alpha), smoothstep(5000.0, 1.0, alpha));
    return color;
    }
    return float4(0.0, 0.0, 0.0, 0.0);	
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////