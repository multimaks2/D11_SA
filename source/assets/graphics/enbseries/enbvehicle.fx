
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

float4 tempF1; float4 tempF2; float4 tempF3; float4 ScreenSize; float ENightDayFactor; float EInteriorFactor;
float4 WeatherAndTime; float4 Timer; float FieldOfView; float GameTime; float4 SunDirection; 
float4 CustomShaderConstants1[8]; float4 MatrixVP[4]; float4 MatrixInverseVP[4]; float4 MatrixVPRotation[4];
float4 MatrixInverseVPRotation[4]; float4 MatrixView[4]; float4 MatrixInverseView[4]; float4 CameraPosition;
float4x4 MatrixWVP; float4x4 MatrixWVPInverse; float4x4 MatrixWorld; float4x4 MatrixProj; float4 diffColor;
float4 specColor; float4 ambColor; float4 FogParam; float4 FogFarColor; float4 lightDiffuse[8]; float4 lightSpecular[8];
float4 lightDirection[8]; float4 VehicleParameters1;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Textures
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

texture2D texOriginal;
sampler2D SamplerOriginal = sampler_state { Texture   = <texOriginal>; };
texture2D texRain < string ResourceName = "RainCar.png"; >;
texture2D texNormalRain < string ResourceName = "RainNormal.png"; >;
texture2D texNoise < string ResourceName="enbnoise.png"; >;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//Sampler Inputs
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

sampler2D SamplerNoise = sampler_state
{
   Texture = <texNoise>;
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

sampler2D SamplerNR = sampler_state 
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

sampler2D RainSampler = sampler_state 
{
	Texture = <texRain>;
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

struct PS_OUTPUT3
{
	float4 Color[3] : COLOR0;
};

struct VS_INPUT_N
{
	float3	pos : POSITION;
	float3	normal : NORMAL;
	float2	txcoord0 : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4	pos : POSITION;
	float4	pos1 : POSITION1;	
	float2	txcoord0 : TEXCOORD0;
	float3	viewnormal : TEXCOORD3;
	float3	eyedir : TEXCOORD4;
	float3	wnormal : TEXCOORD5;
	float4	vposition : TEXCOORD6;
	float3	normal : TEXCOORD7;
};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
VS_OUTPUT VS_Draw(VS_INPUT_N IN)
{
    VS_OUTPUT OUT;
	float4	pos;
	pos.xyz=IN.pos.xyz;
	pos.w=1.0;
	float4	tpos;
	tpos=mul(pos, MatrixWVP);
	OUT.pos=tpos;
	OUT.pos1=tpos;	
	OUT.vposition=tpos;
	OUT.txcoord0=IN.txcoord0;	
	float3	wnormal=normalize(mul(IN.normal.xyz, MatrixWorld));
	float3	normal;
	normal.x=dot(wnormal.xyz, MatrixView[0]);
	normal.y=dot(wnormal.xyz, MatrixView[1]);
	normal.z=dot(wnormal.xyz, MatrixView[2]);
	OUT.viewnormal=normalize(normal.xyz);
	OUT.normal=normalize(mul(IN.normal.xyz, MatrixWVP));
	OUT.wnormal=wnormal;
	float3	campos=CameraPosition;
	OUT.eyedir=(mul(pos, MatrixWorld) - campos);
    return OUT;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float A2
<
	string UIName="Alpha mask";
	string UIWidget="Spinner";
	float UIStep=0.0001;
	float UIMin=0.9;
	float UIMax=1.0;
> = {0.96};

float Light0
<
	string UIName="Headlights: brightness";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=25.0;
> = {14.0};

float fr
<
	string UIName="Fresnel: Amount";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=1.0;
> = {0.5};

float3 tonemapping(float3 color)
{
	float3 cn0 = normalize(color.xyz),
	       ct0=color.xyz/cn0.xyz;
	       ct0=pow(ct0, 1.0);
           cn0.xyz = pow(cn0.xyz, 0.9);   
           color.xyz = ct0*cn0.xyz;  
           color.xyz*= 1.0;

    float maxcol = max(color.x, max(color.y, color.z));		 		 
    float3 r0 = 1.0-exp(-maxcol),
           r2 = 1.0-exp(-color);	

         color.xyz = lerp(float3(r2.x, r2.y, r2.z), color*float3(r0.x, r0.y, r0.z)/maxcol, 1.15);
  return color;	
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

float Rain_alltime_c
<
        string UIName="Rain_alltime_c";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=1.0;
> = {0.0};

PS_OUTPUT3 PS_Draw(VS_OUTPUT IN, in float2 vpos : VPOS)
{
    float2  cd0 = IN.txcoord0.xy;
	float4  wx = WeatherAndTime;	
	float3	n0 = normalize(IN.normal.xyz);
	float3	wn = normalize(IN.wnormal.xyz);
	float3	ed = normalize(-IN.vposition.xyz);
	float3	wed = normalize(-IN.eyedir.xyz);
	float4	tex = tex2D(SamplerOriginal, cd0);
	float opacity = saturate(tex.a*diffColor.a);
//-------------------------------------
//-------------------------------------	
    float4 tc = tex;	
    float4 tc2 = tex;		
    float4 gl0 = 1.0;
    float4 gl1 = 1.0;	
		
	float3 st5 = normalize(tc.xyz);
	float3 ct5 = tc.xyz/st5.xyz;
	       ct5 = pow(ct5, 9.0);
		   st5.xyz = pow(st5.xyz, -5.9);
	       tc.xyz = ct5*st5.xyz;
		
	float3 st6 = normalize(tc2.xyz);
	float3 ct6 = tc2.xyz/st6.xyz;
	       ct6 = pow(ct6, 8.0);
		   st6.xyz = pow(st6.xyz, 4.0);
	       tc2.xyz = ct6*st6.xyz;
		
	float3 nt = gl0.xyz*0.6;
	       nt.xyz = min(nt, gl0);
	float3 tr = gl0.xyz*(-3.0)/saturate(opacity + 0.02 + VehicleParameters1.y);
	       gl0.xyz = lerp(tr, nt, saturate(opacity+VehicleParameters1.y));
	float3 nt0 = gl1.xyz*0.0;
	       nt0.xyz = min(nt0, gl1);
	float3 tr0 = gl1.xyz*10.0/saturate(opacity + 0.02 + VehicleParameters1.y);
	       gl1.xyz = lerp(tr0, nt0, saturate(opacity+VehicleParameters1.y));

    float4 sc1 = specColor*1000.0;
    float4 sc2 = tex;
	float3 g2 = normalize(sc1.xyz);
	float3 g4 = normalize(sc2.xyz);
	float3 s2 = sc1.xyz/g2.xyz;
	float3 s4 = sc2.xyz/g4.xyz;
	       s2 = pow(s2, -15.0);
	       s4 = pow(s4, -34.0);
		   g4.xyz = pow(g4.xyz, 3.0);
	       sc2.xyz = s4*g4.xyz;
		   
	float mask0 = max(specColor.x, max(specColor.y, specColor.z));
	      mask0 = saturate(mask0*1000.0);				
	float mask1 = max(s2.x, max(s2.y, s2.z));
	      mask1 = saturate(mask1);
	float mask2 = max(sc2.x, max(sc2.y, sc2.z));
	      mask2 = saturate(mask2);					
	float mask3 = max(tc.x, max(tc.y, tc.z));
	      mask3 = saturate(mask3);				
	float mask4 = max(tc2.x, max(tc2.y, tc2.z));
	      mask4 = saturate(mask4);						
	float mask5 = max(gl0.x, max(gl0.y, gl0.z));
	      mask5 = saturate(mask5);	
    float mask6 = max(gl1.x, max(gl1.y, gl1.z));	
		  mask6 = saturate(mask6*2.0)*0.036;				
		 
	float3	specular=0.0;	
	for (int li=0; li<8; li++)
	{	 	 		 
		float3 sv5 = normalize(lightDirection[li].xyz);
		float specfact = saturate(dot(sv5, wn.xyz));
		      specfact = pow(specfact, 10.0);
		      specular+= saturate(lightDiffuse[li]-lightSpecular[li]) * specfact;		 
	}		  
   
    float4 dc1 = diffColor;
	float3 st8 = normalize(dc1.xyz);
	float3 ct8=dc1.xyz/st8.xyz;
	       ct8=pow(ct8, 1.70);
		   st8.xyz = pow(st8.xyz, 2.10);
	       dc1.xyz = ct8*st8.xyz;		   	   

    float gg = saturate(1.1*mask0*mask3*mask2*mask5)*1.0;
    float gg0 = saturate(10.0*mask0*mask4*mask5)*1.0;	
		   	
	float fadefact = (FogParam.w - IN.vposition.z) / (FogParam.w - FogParam.z);
	
	float nonlineardepth = (IN.vposition.z/IN.vposition.w)*1.0;	
	float3 ssnormal = normalize(IN.viewnormal);
	       ssnormal.yz = -ssnormal.yz;
		   
	float4 r5 = 0.0;		   
	float4 tex2 = tex2D(SamplerOriginal, cd0); 	
			
	float3 st9 = normalize(tex2.xyz);
	float3 ct9 = tex2.xyz/st9.xyz;
	       ct9 = pow(ct9, 1.0);
		   st9.xyz = pow(st9.xyz, 1.0);
	       tex2.xyz = ct9*st9.xyz;	

	float  wpos2 = length(wed.xyz);
    float3 dir = wed.xyz/wpos2;	
	
	float fresnel = saturate(0.001-dot(-wn, dir));		
		  fresnel = pow(fresnel, 0.37);
		  fresnel = lerp(1.0, 0.0, 0.9*fresnel);
	
           r5.xyz = 0.75*(diffColor+ambColor)*tex2;		

	float AMB = 0.7*(0.005*pow(ambColor.rgb, 3.0));
			  			  
	float3 TM0 = tonemapping(tex2.rgb*ambColor.rgb*2.0);
	       TM0.xyz = pow(TM0.xyz, 2.1);   
		   
           r5.xyz = lerp(r5.xyz, Light0*TM0, saturate(AMB));		   
           r5.xyz+= dc1*specular*tex*6.0;  // Ночное Освещение 1
           r5.xyz+= specular*tex*0.1;  // Ночное Освещение 2			   
           r5.xyz*= 1.0;
		   
    float3 sv3 = normalize(float3(0.0, 0.0, 0.7)+wed.xyz);   
    float3 sv4 = normalize(wed.xyz); 	
	float3 np = normalize(wn.xyz);	
	float factor2 = 0.05 - dot(-sv3, np);
          factor2 = pow(factor2, 5.0);
	float fr2 = (factor2*factor2); 
          fr2/= 2.5;	

	float factor3 = 0.1 - dot(-sv4, np);
          factor3 = pow(factor3, 0.8);
	float fr3 = (factor3*factor3); 
          fr3/= 2.5;	
          fr3 = saturate(fr3*2.6);	
          fr3 = lerp(1.0, fr3, fr);
		 
           r5.xyz*= fr3;		   
		   r5.xyz = lerp(pow(r5.xyz,1.3), r5.xyz, saturate(gg+gg0+mask6)); 		   		   
           r5.xyz+= lerp(fresnel*0.05, 0.0, saturate(gg+gg0)); 
           r5.xyz = lerp(FogFarColor.xyz, r5.xyz, saturate(fadefact));
		   
////////////////////////////////////////////////////////////////////////////////////
	float4 xpos	= mul(IN.pos1, MatrixWVPInverse); 
	       xpos.xyz/= xpos.w;	
			   
	float3 worldnormal = normalize(cross(ddx(xpos.xyz),ddy(xpos.xyz)));     

    float3 bl = saturate(pow(worldnormal, 8.0));
           bl /= max(dot(bl, float3(1,1,1)), 0.0001);
		   
	float2 uvX = xpos.yz;
	float2 uvY = xpos.xz;
	float2 uvZ = xpos.xy;
	
	float2 uvX2 = xpos.yz;
	float2 uvY2 = xpos.xz;
	float2 uvZ2 = xpos.xy; 
	
	float3 wnX = ssnormal*0.5+0.5;	
	
	float3 axisSign = worldnormal < 0 ? -1 : 1;	
           uvX.x *= axisSign.x;
           uvY.x *= axisSign.y;
           uvZ.x *= -axisSign.z;	
	
	float3 axisSign2 = worldnormal < 0 ? 1 : -1;	
           uvX2.x *= axisSign2.x;
           uvY2.x *= axisSign2.y;
           uvZ2.x *= -axisSign2.z;		
	
	float3 N1x = tex2D(SamplerNR, uvX);
	float3 N1y = tex2D(SamplerNR, uvY2);
					
	float3 R1x = tex2D(RainSampler, uvX).g;
	float3 R1y = tex2D(RainSampler, uvY2).g;
	
	float2 xx = float2(Timer.x*270.0, 12.0*Timer.x*270.0);
	float3 R2x = tex2D(RainSampler, uvX + xx).b;
	float3 R2y = tex2D(RainSampler, uvY2 + xx).b;
	
	float3 mixRain = R1x*bl.x + R1y*bl.y;
	float3 mixAlpha = R2x*bl.x + R2y*bl.y;			   
		   
	float RainAlpha = WeatherSetts2(Rain_alltime_c, 0.85, 0.5);
	float rainA = lerp(0.0, mixAlpha, mixRain);	
	float rainA2 = lerp(0.0, 1.0, mixRain);
	float rainA3 = lerp(0.0, mixAlpha, mixRain);

	float3 mixNormal = N1x*bl.x + N1y*bl.y;
   
	float3 v = 0.0;
	float Tr2 = Timer.x * 11000.0;
	float wmix = WeatherSetts2(0.0, 0.48, 0.5);
 	float2 n = tex2D(SamplerNoise, uvZ *0.5).rg;  	
	
    for (float r = 4.0 ; r > 0.0 ; r--) 
    {
        float2 x = 2900.0 * r * 0.015,
               p = 6.28 * uvZ * x + (n - 0.5) * 2.0,
               s = sin(p);
        
        float4 d = tex2D(SamplerNoise, round(uvZ * x - 0.25) / x);
        float t = (s.x+s.y) * max(0.0, 1.0 - frac(Tr2 * (d.b + 0.1) + d.g) * 2.0);      

        if (d.r < (4.0-r)*0.08 && t > 0.5) 
        {           
            v = normalize(-float3(cos(p), lerp(0.2, 2.0, t-0.5)))*bl.z;
        }
    }	
	
	float3 mixN = lerp((ssnormal)*0.5+0.5, mixNormal, RainAlpha*rainA);		
	       r5.xyz*= lerp(1.0, 1.35, RainAlpha*rainA3);
	float4 r1;
	       //r1.xyz = ssnormal*0.5+0.5;
		   r1.xyz = mixN + float3(v.xy*0.55, 0.0)*RainAlpha;	
		   		   
           r5.w = lerp(1.0, opacity, 1.0);			   
		   r1.w = lerp(0.99999, A2, gg+gg0+mask6);
	float4 r2;	
	       r2.xyz = nonlineardepth;
	       r2.w = 1.0;
	PS_OUTPUT3	OUT;
	OUT.Color[0]=r5;
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
	PixelShader  = compile ps_3_0 PS_Draw();
	}
}

technique DrawTransparent
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Draw();
	PixelShader  = compile ps_3_0 PS_Draw();

	}
}
/**/
