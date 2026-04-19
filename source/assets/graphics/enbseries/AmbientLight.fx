
float3 CalculateSunRay(float3 wpos, float3 SunDirectionR, float GameTime)
{	  
   float3 sv = SunDirectionR;
   float3 sv2 = normalize(float3(-0.0833, -0.946, 0.317));	
   float t = GameTime;
	
   float x1 = smoothstep(0.0, 4.0, t);
   float x2 = smoothstep(4.0, 5.0, t);
   float x3 = smoothstep(5.0, 6.0, t);
   float x4 = smoothstep(6.0, 7.0, t);
   float xE = smoothstep(8.0, 11.0, t);
   float x5 = smoothstep(16.0, 17.0, t);
   float x6 = smoothstep(18.0, 19.0, t);
   float x7 = smoothstep(19.0, 20.0, t);
   float xG = smoothstep(20.0, 21.0, t);  
   float xZ = smoothstep(21.0, 22.0, t);
   float x8 = smoothstep(22.0, 23.0, t);
   float x9 = smoothstep(23.0, 24.0, t);
   
   float3 t0 = lerp(0.0, 0.0, x1);
          t0 = lerp(t0, float3(1.0, 0.9, 0.5)*3.0, x2);
          t0 = lerp(t0, float3(1, 0.949, 0.51)*4.7, x3);
          t0 = lerp(t0, float3(1, 0.949, 0.51)*2.2, x4);
          t0 = lerp(t0, float3(1.0, 1.0, 1.0)*0.6, xE);
          t0 = lerp(t0, float3(1.0, 1.0, 1.0)*0.6, x5);
          t0 = lerp(t0, float3(1, 0.949, 0.51)*3.0, x6);	 
          t0 = lerp(t0, float3(1.0, 0.9, 0.5)*0.8, x7);
		  t0 = lerp(t0, float3(1.0, 0.9, 0.5)*0.6, xG);
		  t0 = lerp(t0, float3(1.0, 0.9, 0.5)*0.4, xZ);
          t0 = lerp(t0, float3(1.0, 0.9, 0.5)*0.2, x8);
          t0 = lerp(t0, 0.0, x9);	

   float3 t3 = lerp(0.0, 0.0, x1);
          t3 = lerp(t3, float3(1.0, 0.8, 0.35)*3.0, x2);
          t3 = lerp(t3, float3(1, 0.949, 0.627)*4.7, x3);
          t3 = lerp(t3, float3(1, 0.949, 0.627)*4.0, x4);
          t3 = lerp(t3, float3(1.0, 1.0, 1.0)*2.0, xE);
          t3 = lerp(t3, float3(1.0, 1.0, 1.0)*2.0, x5);
          t3 = lerp(t3, float3(1, 0.949, 0.627)*2.0, x6);		 
          t3 = lerp(t3, float3(1.0, 0.447, 0.0)*0.8, x7);
		  t3 = lerp(t3, float3(1.0, 0.447, 0.0)*0.6, xG);
		  t3 = lerp(t3, float3(1.0, 0.447, 0.0)*0.4, xZ);
          t3 = lerp(t3, float3(1.0, 0.447, 0.0)*0.2, x8);
          t3 = lerp(t3, 0.0, x9); 	   

   float c1 = 18.0;
   float c2 = 1.35;

   float factor1 = (0.04 - dot(-sv, wpos));
         factor1 = pow(factor1, c1);	
   float factor2 = 0.6 - dot(-sv, wpos);
         factor2 = pow(factor2, c2);
		 
   float factor3 = (0.04 - dot(-sv2, wpos));
         factor3 = pow(factor3, c1);	
   float factor4 = 0.6 - dot(-sv2, wpos);
         factor4 = pow(factor4, c2);		 

   float3 f1 = (factor1*factor1)/5.0;    
   float3 f2 = (factor2*factor2)/10.0;
   
   float3 f1x = (factor3*factor3)/5.0;    
   float3 f2x = (factor4*factor4)/10.0;   
   float3 fnight = (f1x*float3(0.0863, 0.137, 0.176))+(f2x*float3(0.0, 0.0, 0.0));     
		  fnight*= smoothstep(0.0, 0.3, -wpos.y);   
   
   float y1 = smoothstep(0.0, 2.0, t);
   float yZ = smoothstep(2.0, 3.0, t);   
   float y2 = smoothstep(4.0, 23.0, t);
   float y3 = smoothstep(23.0, 24.0, t);   
		 
   float3 t5 = lerp(1.0, 0.5, y1);
          t5 = lerp(t5, 0.0, yZ);  
          t5 = lerp(t5, 0.0, y2);
          t5 = lerp(t5, 1.0, y3);	 

   float3 f3 = (f1*t0*0.50)+(f2*t3*0.20)+(1.2*fnight*t5);//	   
   return f3;
}


float3 CalculateMoon(float3 wpos, float3 SunDirectionR, float GameTime)
{
  
   float3 sv = SunDirectionR;
   float3 sv2 = normalize(float3(-0.0833, -0.946, 0.317));	
   float t = GameTime;	
   float c1 = 18.0;
   float c2 = 1.35;
		 
   float factor3 = 0.04 - dot(-sv2, wpos);
         factor3 = pow(factor3, c1);	
   float factor4 = 0.6 - dot(-sv2, wpos);
         factor4 = pow(factor4, c2);		 
   
   float3 f1x = (factor3*factor3)/5.0;    
   float3 f2x = (factor4*factor4)/10.0;   
   float3 fnight = (f1x*float3(0.0863, 0.137, 0.176))+(f2x*float3(0.0, 0.0, 0.0));     
		  fnight*= smoothstep(0.0, 0.3, -wpos.y);   
   
   float y1 = smoothstep(0.0, 2.0, t);
   float yZ = smoothstep(2.0, 3.0, t);   
   float y2 = smoothstep(4.0, 23.0, t);
   float y3 = smoothstep(23.0, 24.0, t);   
		 
   float3 t5 = lerp(1.0, 0.5, y1);
          t5 = lerp(t5, 0.0, yZ);  
          t5 = lerp(t5, 0.0, y2);
          t5 = lerp(t5, 1.0, y3);	 

   float3 f3 = (fnight*t5*1.8);   
   return f3;
}


#define zenithDensity(x) 0.7 / pow(max(x - 0.1, 0.35e-2), 0.75)

float3 getSkyAbsorption(float3 x, float y)
{	
	float3 absorption = x * -y;
	       absorption = lerp(exp2(absorption), float3(0.569, 0.392, 0.275),  0.0);
	return absorption;
}

float3 jodieReinhardTonemap(float3 c)
{
    float l = dot(c, float3(0.2126, 0.7152, 0.0722));
    float3 tc = c / (c + 1.0);

    return lerp(c / (l + 1.0), tc, tc);
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

float WeatherSetts2(float a, float a2, float a3)
{ 
    float4 wx = WeatherAndTime;	
    float sc = a;
    float sn = a;   
    if (wx.x==8 || wx.x==16) sc = a2;
    if (wx.y==8 || wx.y==16) sn = a2;	
	
    float mix = lerp(sc, sn, pow(wx.z, a3));	
    return mix;
}

float3 GetAtmosphericScattering2(float3 wpos, float3 SunDirectionR, float GameTime, float sky1n, float sky2n, float SkyNight, float Clight, float sky1a, float sky2a)
{   
   float t = GameTime;		
   float x1 = smoothstep(0.0, 4.0, t);
   float x2 = smoothstep(4.0, 5.0, t);
   float x3 = smoothstep(5.0, 6.0, t);
   float x4 = smoothstep(6.0, 7.0, t);
   float xE = smoothstep(8.0, 11.0, t);
   float x5 = smoothstep(16.0, 17.0, t);
   float x6 = smoothstep(18.0, 19.0, t);
   float x7 = smoothstep(19.0, 20.0, t);
   float xG = smoothstep(20.0, 21.0, t);  
   float xZ = smoothstep(21.0, 22.0, t);
   float x8 = smoothstep(22.0, 23.0, t);
   float x9 = smoothstep(23.0, 24.0, t);  

   float t6 = lerp(0.0, 0.0, x1);
         t6 = lerp(t6, 0.0, x2);
         t6 = lerp(t6, 0.0, x3);
         t6 = lerp(t6, 0.0, x4);
         t6 = lerp(t6, 1.0, xE);
         t6 = lerp(t6, 0.6, x5);
         t6 = lerp(t6, 0.0, x6);
         t6 = lerp(t6, 0.0, x7);		  
         t6 = lerp(t6, 0.0, x8);    
         t6 = lerp(t6, 0.0, x9);     
		  
// Создание основы неба	  
   float3 vec1 = normalize(float3(0.0, 0.0, 1.0));	  
   float sky = saturate(0.95*sky1a - dot(vec1, wpos));
	     sky = pow(sky, 2.32*sky2a);
		 
   float sky2 = (1.05*sky1n - dot(vec1, wpos));
	     sky2 = pow(sky2, 40.0*sky2n);		 

   float3 SkyColorTop = float3(0.584, 0.804, 1.0);
   float3 SkyColorBot = float3(0.878, 0.878, 0.804);   
   float3 absorption = getSkyAbsorption(SkyColorTop, sky2);

//=============================================================
// Исправляем неправильную работу SunDirection
//=============================================================
   float t0 = GameTime;
   float3 vec0 = SunDirectionR;
   float3 mn0 = normalize(float3(0.0, 0.0, -1.0));
   
   float s1 = smoothstep(0.0, 1.0, t0);	
   float s2 = smoothstep(1.0, 23.0, t0);
   float s3 = smoothstep(23.0, 24.0, t0);
   
   float3 vec2 = lerp(mn0, vec0, s1);
          vec2 = lerp(vec2, vec0, s2); 
          vec2 = lerp(vec2, vec0, s3);  

   float3 sv = SunDirectionR;	
		  
   float vec3 = max(sv.z + 0.1 - 0.0, 0.0);   
   float vec5 = max(sv.z + 0.40 - 0.0, 0.0);

   float vec6 = lerp(0.4, 0.4, x1);
         vec6 = lerp(vec6, 0.4, x2);
         vec6 = lerp(vec6, 0.4, x3);
         vec6 = lerp(vec6, 0.4, x4);
         vec6 = lerp(vec6, 0.4, xE);
         vec6 = lerp(vec6, 0.5, x5);
         vec6 = lerp(vec6, 0.8, x6);
         vec6 = lerp(vec6, 0.9, x7);
         vec6 = lerp(vec6, 0.9, xG);	
         vec6 = lerp(vec6, 1.0, xZ);			  
         vec6 = lerp(vec6, 1.0, x8);    
         vec6 = lerp(vec6, 1.0, x9);  

   float hor1 = lerp(0.1, 0.1, x1);
         hor1 = lerp(hor1, 0.07, x2);
         hor1 = lerp(hor1, 0.07, x3);
         hor1 = lerp(hor1, 0.0, x4);
         hor1 = lerp(hor1, 0.0, xE);
         hor1 = lerp(hor1, 0.0, x5);
         hor1 = lerp(hor1, 0.07, x6);
         hor1 = lerp(hor1, 0.07, x7);
         hor1 = lerp(hor1, 0.07, xG);	
         hor1 = lerp(hor1, 0.12, xZ);			  
         hor1 = lerp(hor1, 0.12, x8);
		 
   float hor2 = lerp(0.1, 0.1, x1);
         hor2 = lerp(hor2, 0.7, x2);
         hor2 = lerp(hor2, 0.8, x3);
         hor2 = lerp(hor2, 1.0, x4);
         hor2 = lerp(hor2, 1.0, xE);
         hor2 = lerp(hor2, 1.0, x5);
         hor2 = lerp(hor2, 0.8, x6);
         hor2 = lerp(hor2, 0.7, x7);
         hor2 = lerp(hor2, 0.6, xG);	
         hor2 = lerp(hor2, 0.4, xZ);			  
         hor2 = lerp(hor2, 0.1, x8);		 
	    
   float3 sunAbsorption = getSkyAbsorption(SkyColorTop, zenithDensity(sv.z + vec6));
   float3 sAbs = sunAbsorption * 0.5 + 0.5 * length(sunAbsorption);  

   float3 Ray = CalculateSunRay(wpos, SunDirectionR, GameTime)* sunAbsorption;   
   float3 Moon = CalculateMoon(wpos, SunDirectionR, GameTime);  
	
    float3 SkyColor = lerp(SkyColorTop, SkyColorBot, sky);  	
	       SkyColor*= lerp(sky, 1.0, 0.4); 		   
           SkyColor = pow(SkyColor, 3.2);
           SkyColor = pow(SkyColor, (1.0 / lerp(1.2, 1.07, t6)  ));		   
	       SkyColor = lerp(SkyColor*pow(absorption, 1.0), SkyColor, vec3); 
		   SkyColor+= float3(0.95, 0.85, 1.0)*saturate(sky2)*hor1;	
		   SkyColor*= 12.0;			   
	       SkyColor*= sAbs;		   
		   
   float sm1 = smoothstep(0.0, 2.0, GameTime),	
         sm2a = smoothstep(2.0, 4.0, GameTime),   
         sm2 = smoothstep(4.0, 5.0, GameTime),	 
         sm3 = smoothstep(5.0, 6.0, GameTime),			 
         sm4 = smoothstep(6.0, 23.0, GameTime),
         sm5 = smoothstep(23.0, 24.0, GameTime),
         sm6 = smoothstep(21.0, 24.0, GameTime);		 
   float ti = lerp(1.0, 1.0, sm1);
         ti = lerp(ti, 0.7, sm2a);     
         ti = lerp(ti, 0.0, sm2);   
         ti = lerp(ti, 0.0, sm3); 		  
         ti = lerp(ti, 0.0, sm4);
         ti = lerp(ti, 1.0, sm5);
         ti = lerp(ti, 1.0, sm6);
		 
	float3 fnight = lerp(float3(0.50, 0.70, 0.95)*0.04, float3(0.50, 0.70, 0.95)*0.30, pow(sky, 0.85));
	float3 Cloudy = lerp(float3(1, 1, 1), float3(0.498, 0.498, 0.498), pow(sky, 0.75));	
	
           SkyColor = lerp(SkyColor, fnight*SkyNight, ti);		   
           SkyColor+= Moon+(Ray*10.0*sAbs);		   		   
           SkyColor = jodieReinhardTonemap(SkyColor);	
           SkyColor = pow(SkyColor, 1.1)*1.1;

           SkyColor = WeatherSetts(SkyColor, pow(Cloudy*hor2*Clight, 1.7));  
    return SkyColor;	
}
