
float FuncMasking(float2 uv)
{
	float mask=tex2Dlod(SamplerNormal, float4(uv, 0.0, 0.0)).w;
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

#define MSIZE 8

float normpdf(in float x, in float3 sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma))/sigma;
}

float normpdf3(in float3 v, in float3 sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma))/sigma;
}

float ssao_p
<
	string UIName="SSAO: Brightness";
	string UIWidget="Spinner";
	float UIMin=0.0;
	float UIMax=2.5;
> = {1.5};

float4 BlurFunction(float2 uv, float r, float2 coord, float center_d, float3 center_n, inout float w_total)
{
	float4 c = tex2D(SamplerColor, uv);
	float d = tex2Dlod(SamplerDepth, float4(uv, 0.0, 1.0)).x;
	float3 n = tex2D(SamplerNormal, uv).xyz*2.0-1.0;	  
	float LinDepth = -100.0 * 1.0 / (d * (100.0 - 1.0) - 100.0);
  
	const float BlurSigma = 6 * 0.5;
	const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);
  
	float ddiff = (LinDepth - center_d) * 11.0;
          ddiff = pow(ddiff, 0.4);
  
	float3 normal_term = (dot(n.xyz, center_n.xyz) * 20.0 - 20.5);
	       normal_term = pow(normal_term, -0.55);
  
	float w = exp2(-r*r*BlurFalloff - ddiff*ddiff)*2.0*pow(normal_term*normal_term, 1.5);
  
	w_total += w;

	return c*w;
}

float4 AO_Blur(float2 coord)
{		
	float3 c = tex2D(SamplerColor, coord.xy).rgb;
    float m2 = maskPed(coord); 
	float2 of = float2(ScreenSize.y, ScreenSize.y * ScreenSize.z);	
	
	const int kSize = (MSIZE-1)/6;
	float kernel[MSIZE];		
	float3 final_color = float3(0.0, 0.0, 0.0);
	
	float Z = 0.0;		
		for (int j = 0; j <= kSize; ++j)
		{
			kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), 10.0);
		}
				
    float3 cc;
    float3 nn; 	
    float factor;
    float factor2 = lerp(20.5, 25.0, m2);	
	float3 normal;
	
	float3 center_n = tex2D(SamplerNormal, coord).xyz*2.0-1.0;
	float bZ = 1.0/normpdf(0.0, 0.10);

		for (int i=-kSize; i <= kSize; ++i)
		{
			for (int j=-kSize; j <= kSize; ++j)
			{
				cc = tex2D(SamplerColor, coord.xy+float2(float(i)*5.4, float(j)*5.4)*of).rgb;
	            nn = tex2D(SamplerNormal, coord.xy+float2(float(i)*5.4, float(j)*5.4)*of).xyz*2.0-1.0;	
	            			
	            normal = (dot(nn.xyz, center_n.xyz) * 20.0 - factor2);
	            normal = pow(normal, -0.55);				
	            normal = pow(normal, -2.4);

				factor = normpdf3(cc-c, normal*0.8)*bZ*kernel[kSize+j]*kernel[kSize+i];
				Z += factor;
				final_color += factor*cc;			
			}
		}		
	float4 blur = 1.0;
	       blur.rgb = final_color/Z;
	return blur;
}

float4 AO_Blur1(float2 coord)
{		
	float3 c = tex2D(SamplerColor, coord.xy).rgb;
    float m2 = maskPed(coord); 	
	float2 of = float2(ScreenSize.y, ScreenSize.y * ScreenSize.z);		
	const int kSize = (MSIZE-1)/6;
	float kernel[MSIZE];		
	float3 final_color = float3(0.0, 0.0, 0.0);
	
	float Z = 0.0;		
		for (int j = 0; j <= kSize; ++j)
		{
			kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), 10.0);
		}
				
    float3 cc;
    float3 nn;	
	
    float factor;
    float factor2 = lerp(20.5, 25.0, m2);		
	float3 normal;	
	
	float3 center_n = tex2D(SamplerNormal, coord).xyz*2.0-1.0;
	float bZ = 1.0/normpdf(0.0, 0.10);

		for (int i=-kSize; i <= kSize; ++i)
		{
			for (int j=-kSize; j <= kSize; ++j)
			{
				cc = tex2D(SamplerColor, coord.xy+float2(float(i)*1.75, float(j)*1.75)*of).rgb;
	            nn = tex2D(SamplerNormal, coord.xy+float2(float(i)*1.75, float(j)*1.75)*of).xyz*2.0-1.0;	
			
	            normal = (dot(nn.xyz, center_n.xyz) * 20.0 - factor2);
	            normal = pow(normal, -0.55);
	            normal = pow(normal, -2.4);
				
				factor = normpdf3(cc-c, normal*0.8)*bZ*kernel[kSize+j]*kernel[kSize+i];
				Z += factor;
				final_color += factor*cc;			
			}
		}		
	float4 blur = 1.0;
	       blur.rgb = final_color/Z;
	return blur;
}

float4 AO_Blur2(float2 coord)
{
    float4 center_c = tex2D(SamplerColor, coord);
    float center_d = tex2D(SamplerDepth, coord).x;
    float3 center_n = tex2D(SamplerNormal, coord).xyz*2.0-1.0;	
    float LinDepth = -100.0 * 1.0 / (center_d * (100.0 - 1.0) - 100.0);
	float2 of = float2(ScreenSize.y, ScreenSize.y * ScreenSize.z);	
    float4 c_total = center_c;
    float w_total = 1.0;
  
    float a1 = 2.0*3.1416/6;	
    float a2 = 2.0*3.1416 /3;	
	
    for (float i = 1; i <= 6; ++i)
     {			
        float angle = a1 * i;
        float2 dir = float2(cos(angle), sin(angle));
		       dir/= 0.01;
        float2 uv = coord + dir * 0.08*of;		
        c_total += BlurFunction(uv, i, coord, LinDepth, center_n, w_total);
     }
	
    for (float k = 1; k <= 3; ++k)
     {			
        float angle = a2 * k;
        float2 dir = float2(cos(angle), sin(angle));
		       dir/= 0.01;
        float2 uv = coord + dir * 0.03*of;		
        c_total += BlurFunction(uv, k, coord, LinDepth, center_n, w_total);	
     }	

	float4 blur = c_total/w_total;
		 
	float LinDepth2 = (5.3*0.003) / (5.3 + center_d * (0.003 - 5.3));  
	float pows = lerp(2.0, 3.2, saturate(LinDepth2));
	
	      blur = pow(blur, pows*ssao_p);		  		  
          blur = saturate(blur);

    float4 result;   
    if( 1000.0 < 0.01 || center_d == 1)
     {
	      result = 1.0;
     }	
	      result = lerp(blur, 1.0, result);	  
   return result;
}