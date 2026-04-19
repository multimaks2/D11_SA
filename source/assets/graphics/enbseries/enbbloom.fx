
/* VERSION 2.0 */

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
//+++++++++++++++++++++++++    https://www.youtube.com/channel/UCrASy-x5DgwHpYiDv41RL2Q    ++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++          Visit http://enbdev.com              ++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++    Copyright (c) 2007-2021 Boris Vorontsov    ++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++    Основой для создания блума, послужил блум из ICEnhancer    +++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
float4	BloomParameters;
float4	TempParameters;
float4	ScreenSize;
float	GameTime;

struct VS_OUTPUT_POST
{
	float4 vpos  : POSITION;
	float2 txcoord0 : TEXCOORD0;
};
struct VS_INPUT_POST
{
	float3 pos  : POSITION;
	float2 txcoord0 : TEXCOORD0;
};

texture2D texBloom1;
texture2D texBloom2;
texture2D texBloom3;
texture2D texBloom4;
texture2D texBloom5;

sampler2D SamplerBloom1 = sampler_state
{
    Texture   = <texBloom1>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerBloom2 = sampler_state
{
    Texture   = <texBloom2>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerBloom3 = sampler_state
{
    Texture   = <texBloom3>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerBloom4 = sampler_state
{
    Texture   = <texBloom4>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

sampler2D SamplerBloom5 = sampler_state
{
    Texture   = <texBloom5>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU  = Clamp;
	AddressV  = Clamp;
	SRGBTexture=FALSE;
	MaxMipLevel=0;
	MipMapLodBias=0;
};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

VS_OUTPUT_POST VS_Bloom(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;

	OUT.vpos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);
	OUT.txcoord0.xy=IN.txcoord0.xy+TempParameters.xy;
	return OUT;
}

float B0
<
        string UIName="Bloom - Size";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=2.0;
> = {0.25};

float BCont
<
        string UIName="Bloom - Contrast";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {1.00};

float BSat
<
        string UIName="Bloom - Saturate";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {1.00};

float BGam
<
        string UIName="Bloom - Alpha";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {1.00};

float BGamN
<
        string UIName="BloomNight - Alpha";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {1.00};

float B1
<
        string UIName="Bloom - Brightness_Day";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {0.90};

float B2
<
        string UIName="Bloom - Brightness_Night";
        string UIWidget="Spinner";
        float UIMin=0.0;
        float UIMax=3.0;
> = {0.90};

float3 B3
<
	string UIName="Bloom - Color";
	string UIWidget="Color";
> = {1.0, 1.0, 1.0};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
float4 wBloom(float2 rd)
{
	float2 rc = rd*1.02+float2( -0.01, -0.01);
    float4 c = tex2D(SamplerBloom1, rc.xy);
	       c.w = rc.y<0.0||rc.y>1.0 ? 0.0:1.0; 
	       c.w*= rc.x<0.0||rc.x>1.0 ? 0.0:1.0;
	 return c;
}

const float2 offset[16]=
{
float2(-0.94201624, -0.39906216),
float2( 0.94558609, -0.76890725),
float2(-0.09418410, -0.92938870),
float2( 0.34495938,  0.29387760),
float2(-0.91588581,  0.45771432),
float2(-0.81544232, -0.87912464),
float2(-0.38277543,  0.27676845),
float2( 0.97484398,  0.75648379),
float2( 0.44323325, -0.97511554),
float2( 0.53742981, -0.47373420),
float2(-0.26496911, -0.41893023),
float2( 0.79197514,  0.19090188),
float2(-0.24188840,  0.99706507),
float2(-0.81409955,  0.91437590),
float2( 0.19984126,  0.78641367),
float2(-0.14383161, -0.14100790)
};

const float2 offsetX[25]=
{
	float2(-0.707, 0.707),
	float2(-0.383, 0.924),
	float2(0.0, 1.0),
	float2(0.383, 0.924),
	float2(0.707, 0.707),
	
	float2(-0.924, 0.383),
	float2(-0.707, 0.707),
	float2(0.0, 1.0),
	float2(0.707, 0.707),
	float2(0.924, 0.383),
	
	float2(-1.0, 0.0),
	float2(-1.0, 0.0),
	float2(0.0, 0.0),
	float2(1.0, 0.0),
	float2(1.0, 0.0),
	
	float2(-0.924, -0.383),
	float2(-0.707, -0.707),
	float2(0.0, -1.0),
	float2(0.707, -0.707),
	float2(0.924, -0.383),
	
	float2(-0.707, -0.707),
	float2(-0.383, -0.924),
	float2(0.0, -1.0),
	float2(0.383, -0.924),
	float2(0.707, -0.707)
};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 PS_Bloom1(VS_OUTPUT_POST In) : COLOR
{
	float4	bloomuv;
	float4	overbright = 0.0;
	float	bthreshold = 30;
	float2 coord = In.txcoord0.xy;	
	float4 bloomZ = wBloom(coord);
	float4 bloom = 0.0;
	       bloom = lerp(bloom, bloomZ, bloomZ.w);		   
	float4 bloom1 = bloom;		   
	float2 screenfact=1;
	screenfact.y*= ScreenSize.z;
	screenfact.xy*= TempParameters.z*6;
	float4 srcbloom = bloom;

	for (int i=0; i<16; i++)
	{
		bloomuv.xy = In.txcoord0.xy;			
		bloomuv.x = bloomuv.x - 0.5;
		bloomuv.y = 0.5 - bloomuv.y;
		bloomuv.xy -= bloomuv.xy*floor(i*0.59)*0.59;
		bloomuv.xy = -bloomuv.xy;
		bloomuv.xy *= 0.25 + i * 1;
		bloomuv.x = bloomuv.x + 0.5;
		bloomuv.y = 0.5 - bloomuv.y;		
		bloomuv.xy = saturate(bloomuv.xy);

 float4 overbrightX = wBloom(bloomuv);
	    overbright = lerp(overbright, overbrightX, overbrightX.w);		
		overbright.w = dot(overbright.xyz, 0.3333);
		overbright.w = saturate(overbright.w - bthreshold);
		if(overbright.w) bloom.xyz += overbright.xyz;
		else
		{
			bloomuv.xy=offset[i]*0.50;
			bloomuv.xy=(bloomuv.xy*screenfact.xy)+In.txcoord0.xy;
			
	     float4 b1 = wBloom(bloomuv);
	            bloom += lerp(0.0, b1, b1.w);			
		}
	}
	bloom*= 0.06;

	float3 violet=float3(0.75, 0.75, 0.68);
	float ttt=dot(bloom.xyz, 0.333)-dot(srcbloom.xyz, 0.333);
	ttt=max(ttt, 0.0);
	float gray=BloomParameters.z*ttt;
	float mixfact=(gray/(1.0+gray));
	mixfact*=1.0-saturate((TempParameters.w-1.0)*0.5);
	violet.xy+=saturate((TempParameters.w-1.0)*0.5);
	violet.xy=saturate(violet.xy);
	bloom.xyz*=lerp(1.0, violet.xyz, mixfact);

	bloom.w=1.0;
	return bloom;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 PS_Bloom2(VS_OUTPUT_POST In) : COLOR
{
	float4 bloomuv;
	float4 bloomuv2;
	float4 bloom = tex2D(SamplerBloom1, In.txcoord0);
	float2 screenfact = 1.0;
	       screenfact.y*= ScreenSize.z;
	       screenfact.xy/= ScreenSize.x;
	float4 srcbloom = bloom;
	float step = (TempParameters.w-0.5)*1.2;
	screenfact.xy*= step;
	float4 rotvec = 0.0;
	sincos(0.19635, rotvec.x, rotvec.y);
	for (int i=0; i<25; i++)
	{
		bloomuv.xy=offsetX[i]*B0;
		bloomuv.xy=reflect(bloomuv.xy, rotvec.xy);
		bloomuv.xy*=BloomParameters.y;
		bloomuv.xy=(bloomuv.xy*screenfact.xy)+In.txcoord0.xy;
		float4 tempbloom1 = tex2D(SamplerBloom1, bloomuv.xy);
		bloomuv2.xy=offsetX[i]*B0;
		bloomuv2.xy=reflect(bloomuv2.xy, rotvec.xy);
		bloomuv2.xy*=48;
		bloomuv2.xy=(bloomuv2.xy*screenfact.xy)+In.txcoord0.xy;
		float4 tempbloom2 = tex2D(SamplerBloom1, bloomuv2.xy);
		float4 tempbloom = lerp(tempbloom1, tempbloom2, 0.8);
		bloom+=tempbloom;

	}
	float3 violet=B3;
	float ttt=dot(bloom.xyz, 0.333)-dot(srcbloom.xyz, 0.333);
	ttt=max(ttt, 0.0);
	float gray=BloomParameters.z*ttt;
	float mixfact=(gray/(1.0+gray));
	mixfact*=1.0-saturate((TempParameters.w-1.0)*0.3);
	violet.xy+=saturate((TempParameters.w-1.0)*0.3);
	violet.xy=saturate(violet.xy);
	bloom.xyz*=lerp(1.0, violet.xyz, mixfact);
	bloom*=0.046;
	
		   
   float t0 = GameTime;
   float x1 = smoothstep(0.0, 4.0, t0);
   float x2 = smoothstep(4.0, 5.0, t0);
   float x3 = smoothstep(5.0, 6.0, t0);
   float x4 = smoothstep(6.0, 7.0, t0);
   float xE = smoothstep(8.0, 11.0, t0);
   float x5 = smoothstep(16.0, 17.0, t0);
   float x6 = smoothstep(18.0, 19.0, t0);
   float x7 = smoothstep(19.0, 20.0, t0);
   float xG = smoothstep(20.0, 21.0, t0);  
   float xZ = smoothstep(21.0, 22.0, t0);
   float x8 = smoothstep(22.0, 23.0, t0);
   float x9 = smoothstep(23.0, 24.0, t0); 	

   float tl0 = lerp(B2, B2, x1);
          tl0 = lerp(tl0, B1, x2);
          tl0 = lerp(tl0, B1, x3);
          tl0 = lerp(tl0, B1, x4);
          tl0 = lerp(tl0, B1, xE);
          tl0 = lerp(tl0, B1, x5);
          tl0 = lerp(tl0, B1, x6);	 
          tl0 = lerp(tl0, B1, x7);
          tl0 = lerp(tl0, B1, x8);		
          tl0 = lerp(tl0, B2, x9);	

   float tl1 = lerp(BGamN, BGamN, x1);
          tl1 = lerp(tl1, BGam, x2);
          tl1 = lerp(tl1, BGam, x3);
          tl1 = lerp(tl1, BGam, x4);
          tl1 = lerp(tl1, BGam, xE);
          tl1 = lerp(tl1, BGam, x5);
          tl1 = lerp(tl1, BGam, x6); 
          tl1 = lerp(tl1, BGam, x7);
          tl1 = lerp(tl1, BGam, x8);		
          tl1 = lerp(tl1, BGamN, x9);		  

	float bloomA = pow(bloom.xyz, tl1);
	
	float3 st3 = normalize(bloom.xyz);
	float3 ct3 = bloom.xyz/st3.xyz;
	       ct3 = pow(ct3, BCont);
	       st3.xyz = pow(st3.xyz, BSat);		   
	       bloom.xyz = ct3*st3.xyz;	
   

          bloom.xyz*= lerp(0.0, 1.0, saturate(bloomA));

	bloom.w=1.0;
	return bloom*tl0;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

float4 PS_Bloom(VS_OUTPUT_POST In) : COLOR
{
	float4 bloom;
	float4 temp;
	bloom =tex2D(SamplerBloom1, In.txcoord0);
	bloom+=tex2D(SamplerBloom2, In.txcoord0);
	bloom+=tex2D(SamplerBloom3, In.txcoord0);
	bloom+=tex2D(SamplerBloom4, In.txcoord0);
	bloom+=tex2D(SamplerBloom5, In.txcoord0);
	bloom*=0.3;
	temp = bloom;
	float4 bloom1=tex2D(SamplerBloom1, In.txcoord0);
	float4 bloom2=tex2D(SamplerBloom2, In.txcoord0);
	float4 bloom3=tex2D(SamplerBloom3, In.txcoord0);
	float4 bloom4=tex2D(SamplerBloom4, In.txcoord0);
	float4 bloom5=tex2D(SamplerBloom5, In.txcoord0);
	bloom=max(bloom1, bloom2);
	bloom=max(bloom, bloom3);
	bloom=max(bloom, bloom4);
	bloom=max(bloom, bloom5);
	bloom = lerp(temp, bloom, 0.3);
	bloom.w=1.0;
	return bloom;
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SA_DirectX/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

technique BloomPrePass
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Bloom();
	PixelShader  = compile ps_3_0 PS_Bloom1();
	}
}

technique BloomTexture2
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Bloom();
	PixelShader  = compile ps_3_0 PS_Bloom2();
	}
}

technique BloomPostPass
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Bloom();
	PixelShader  = compile ps_3_0 PS_Bloom();
	}
}
