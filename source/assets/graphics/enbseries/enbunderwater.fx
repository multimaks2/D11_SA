//++++++++++++++++++++++++++++++++++++++++++++
// ENBSeries effect file
// visit http://enbdev.com for updates
// Copyright (c) 2007-2017 Boris Vorontsov
//++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++
//internal parameters, can be modified
//+++++++++++++++++++++++++++++
//caustics are very slow because procedural noise, replace with other code or animated texture
bool	ECausticsEnable
<
	string UIName="Underwater: EnableCaustics";
> = {true};

float	ECausticsIntensity
<
	string UIName="Underwater: CausticsIntensity";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=10.0;
> = {4.0};




//+++++++++++++++++++++++++++++
//external parameters, do not modify
//+++++++++++++++++++++++++++++
//keyboard controlled temporary variables (in some versions exists in the config file). Press and hold key 1,2,3...8 together with PageUp or PageDown to modify. By default all set to 1.0
float4	tempF1; //0,1,2,3
float4	tempF2; //5,6,7,8
float4	tempF3; //9,0
//x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
float4	ScreenSize;
//changes in range 0..1, 0 means that night time, 1 - day time
float	ENightDayFactor;
//changes 0 or 1. 0 means that exterior, 1 - interior
float	EInteriorFactor;
//.x - current weather index, .y - incoming weather index, .z - weather transition, .w - time of the day in 24 standart hours. Weather index is value from _weatherlist.ini, for example WEATHER002 means index==2, but index==0 means that weather not captured.
float4	WeatherAndTime;
//x=generic timer in range 0..1, period of 16777216 ms (4.6 hours), w=frame time elapsed (in seconds)
float4	Timer;
//additional info for computations
//float4	TempParameters; 
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
float4	WaterParameters1; //x - depth scaling, y - mudiness factor, z - deepness factor, w - is camera underwater and how deep


texture2D texColor; //scene color
texture2D texDepth;
texture2D texNormal;
texture2D texEnv; //sky blurred
texture2D texNoise; //16*16 texture

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

sampler2D SamplerNormal = sampler_state
{
    Texture   = <texNormal>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
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

struct VS_INPUT
{
	float3	pos : POSITION;
	float2	txcoord0 : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4	pos : POSITION;
	float2	txcoord0 : TEXCOORD0;
};



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VS_OUTPUT	VS_Draw(VS_INPUT IN)
{
    VS_OUTPUT OUT;

	float4	pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	OUT.pos=pos;
	OUT.txcoord0=IN.txcoord0;

    return OUT;
}



float3	hash1(float3 p3)
{
	p3 = frac(p3 * float3(0.1031, 0.11369, 0.13787));
	p3 += dot(p3, p3.yxz+19.19);
	p3=float3((p3.x+p3.y), (p3.x+p3.z), (p3.y+p3.z)) * p3.zyx;
	return frac(p3);
//	return frac(float3((p3.x+p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}


float	WorleyNoise(float3 n, float scale)
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
				float3	p=n-frcn + xyz; //p=floor(n) + xyz;

				//tiling
				float3	pp = p;
			//	if (pp.x < 0.0) pp.x = scale + pp.x;
			//	if (pp.y < 0.0) pp.y = scale + pp.y;
				if (pp.z < 0.0) pp.z = scale + pp.z;
			//	if (pp.x > 1.0) pp.x = pp.x - scale;
			//	if (pp.y > 1.0) pp.y = pp.y - scale;
				if (pp.z > 1.0) pp.z = pp.z - scale;

				float2	sqrdist;
				frcn=xyz - frcn;
				p = hash1(pp) + frcn;
				sqrdist.x=dot(p, p);
				//dis.x = min(dis.x, sqrdist.x);
				pp.z+=0.012;
				p = hash1(pp) + frcn;
				sqrdist.y=dot(p, p);
				//sqrdist.xy = min(sqrdist.xy, abs(sqrdist.x + sqrdist.y)*0.5);
				dis.xy = min(dis.xy, sqrdist.xy);
			}
		}
	}
	dis.x=dis.x*dis.y;
	//return saturate(1.0 - sqrt(dis.x));
	return saturate(sqrt(dis.x));
}



float4	PS_Draw(VS_OUTPUT IN, float2 vPos : VPOS) : COLOR
{
	float4	res;
	float4	color;
	float2	coord;

	coord.xy=IN.txcoord0;

	//color=tex2D(SamplerColor, coord);
	//res=color;

	//most code is for computing caustics from procedural noise, it's slow

	float	scenedepth=tex2D(SamplerDepth, IN.txcoord0.xy).x;

	float4 tempvec;
	float4 worldpos;
	float4 normalpos;

	//position of pixel
	tempvec.xy=IN.txcoord0.xy*2.0-1.0;
	tempvec.y=-tempvec.y;
	tempvec.z=scenedepth;
	tempvec.w=1.0;
	//get world position
	worldpos.x=dot(tempvec, MatrixInverseVPRotation[0]);
	worldpos.y=dot(tempvec, MatrixInverseVPRotation[1]);
	worldpos.z=dot(tempvec, MatrixInverseVPRotation[2]);
	worldpos.w=dot(tempvec, MatrixInverseVPRotation[3]);
	worldpos.xyz/=worldpos.w;
	worldpos.xyz+=CameraPosition;


	float	deepfade=max(WaterParameters1.w+CameraPosition.z-worldpos.z, 0.0);
//	deepfade*=0.05; //deepness scaling
//	deepfade=saturate(deepfade);
	deepfade*=0.1; //deepness scaling
	deepfade=deepfade/(deepfade+1.0);

	tempvec.xyz=worldpos-CameraPosition;
	float	distfact=dot(tempvec.xyz,tempvec.xyz);
	distfact=saturate(1.0-distfact*0.000025);

	float	scale=8.0;

	float3	causticspos=worldpos;
	causticspos*=0.25;
	causticspos.z*=0.25;
	causticspos.z+=Timer.x*1677.7216 * 2.0; //animation by time
	causticspos.z=frac(causticspos.z);

	float	caustic=0.0;
	//skip caustics if not visible
	float	usecaustic=0.0;
	if (ECausticsEnable==true) usecaustic=1.0;
	usecaustic*=distfact;
	usecaustic*=0.9-deepfade;
	if (usecaustic>=0.1)
	{
		caustic=WorleyNoise(causticspos.xyz, scale);
	}

	caustic=caustic*caustic; //increase contrast

	//increase contrast
	//caustic=pow(caustic, (1.0+deepfade));
	caustic*=lerp(1.0, caustic, deepfade);

	caustic=caustic-0.1;

	//deepness fade of caustics
	caustic*=1.0-deepfade;

	//fade everything above water surface
	caustic*=saturate((WaterParameters1.w+CameraPosition.z - worldpos.z));

	//fade by distance
	caustic*=distfact;

	//premultiply caustics by envmap maybe? there is no sun data anyway

	//intensity of caustics
	caustic*=ECausticsIntensity;

	//by normal top to bottom fade
	float3	normal=tex2D(SamplerNormal, IN.txcoord0.xy)*2.0-1.0;
	tempvec.xy=normal.xy;
	tempvec.x=-tempvec.x;
	tempvec.z=normal.z;
	normalpos.x=dot(tempvec.xyz, MatrixInverseView[0]);
	normalpos.y=dot(tempvec.xyz, MatrixInverseView[1]);
	normalpos.z=dot(tempvec.xyz, MatrixInverseView[2]);
	normalpos.z=-normalpos.z;
	normal.xyz=normalize(normalpos.xyz);
	caustic*=saturate(normal.z*0.5+0.5);

	color=tex2D(SamplerColor, IN.txcoord0.xy);
	res.xyz=(1.0+caustic) * (1.0-deepfade) * color;

	//make screen darker when go deeper
	//bad idea, there is no light to make darker water properly
	float	deepness=max(WaterParameters1.w, 0.0);
	deepness*=0.1;
	deepness=deepness/(deepness+1.0);
	res.xyz*=1.0-deepness*0.7; //0.7 is limiter to not make everything too dark

	
    float dfog = pow(tex2D(SamplerDepth, coord).x, 11.0);	
    res.xyz = lerp(res, dfog*float3(0.4, 0.8, 0.7)*0.4, 0.95);
	res.w=color.w;
	return res;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
technique Draw
{
    pass p0
    {
	VertexShader = compile vs_3_0 VS_Draw();
	PixelShader  = compile ps_3_0 PS_Draw();
	}
}


