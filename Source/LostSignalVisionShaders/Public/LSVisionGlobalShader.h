#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderGraphFwd.h"
#include "ShaderParameterStruct.h"

class FRDGBuilder;

class LOSTSIGNALVISIONSHADERS_API FLSVisionMaskCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FLSVisionMaskCS);
	SHADER_USE_PARAMETER_STRUCT(FLSVisionMaskCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector2f, VisionOrigin)
		SHADER_PARAMETER(float, VisionRadius)
		SHADER_PARAMETER(float, FeatherWidth)
		SHADER_PARAMETER(FVector2f, WorldMin)
		SHADER_PARAMETER(FVector2f, WorldMax)
		SHADER_PARAMETER(FVector2f, OutputTextureSize)
		SHADER_PARAMETER(FLinearColor, VisibleColor)
		SHADER_PARAMETER(FLinearColor, HiddenColor)
		SHADER_PARAMETER(uint32, PolygonPointCount)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float2>, PolygonPoints)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RWOutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
};

namespace LSVision
{
	struct LOSTSIGNALVISIONSHADERS_API FMaskDispatchInputs
	{
		FRDGTextureRef OutputTexture = nullptr;
		FVector2f VisionOrigin = FVector2f::ZeroVector;
		float VisionRadius = 1000.0f;
		float FeatherWidth = 30.0f;
		FVector2f WorldMin = FVector2f(-1000.0f, -1000.0f);
		FVector2f WorldMax = FVector2f(1000.0f, 1000.0f);
		FLinearColor VisibleColor = FLinearColor::White;
		FLinearColor HiddenColor = FLinearColor::Black;
		uint32 PolygonPointCount = 0;
		FRDGBufferSRVRef PolygonPointsSRV = nullptr;
	};

	LOSTSIGNALVISIONSHADERS_API void AddVisionMaskPass(FRDGBuilder& GraphBuilder, const FMaskDispatchInputs& Inputs);
}
