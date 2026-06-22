#include "LSVisionGlobalShader.h"

#include "GlobalShader.h"
#include "GlobalRenderResources.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "ShaderCompilerCore.h"

IMPLEMENT_GLOBAL_SHADER(FLSVisionMaskCS, "/LostSignalVisionShaders/Private/TopDownVisionMask.usf", "MainCS", SF_Compute);

// Queues the compute shader pass that rasterizes the current vision polygon into the mask RT.
void LSVision::AddVisionMaskPass(FRDGBuilder& GraphBuilder, const FMaskDispatchInputs& Inputs)
{
	if (Inputs.OutputTexture == nullptr)
	{
		return;
	}

	FLSVisionMaskCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLSVisionMaskCS::FParameters>();
	PassParameters->VisionOrigin = Inputs.VisionOrigin;
	PassParameters->VisionRadius = Inputs.VisionRadius;
	PassParameters->FeatherWidth = Inputs.FeatherWidth;
	PassParameters->WorldMin = Inputs.WorldMin;
	PassParameters->WorldMax = Inputs.WorldMax;
	PassParameters->OutputTextureSize = FVector2f(
		static_cast<float>(Inputs.OutputTexture->Desc.Extent.X),
		static_cast<float>(Inputs.OutputTexture->Desc.Extent.Y));
	PassParameters->HiddenColor = Inputs.HiddenColor;
	PassParameters->PolygonPointCount = Inputs.PolygonPointCount;
	PassParameters->PolygonPoints = Inputs.PolygonPointsSRV;
	PassParameters->PolygonPointFlags = Inputs.PolygonPointFlagsSRV;
	// 노이즈 텍스쳐 미지정 시에도 바인딩은 유효해야 하므로 흰색 텍스쳐로 폴백(이때 Amplitude=0이라 흔들림 없음).
	PassParameters->NoiseTexture = Inputs.NoiseTexture != nullptr ? Inputs.NoiseTexture : GWhiteTexture->TextureRHI.GetReference();
	PassParameters->NoiseSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
	PassParameters->NoiseScale = Inputs.NoiseScale;
	PassParameters->NoiseWidth = Inputs.NoiseWidth;
	PassParameters->OccluderFeatherScale = Inputs.OccluderFeatherScale;
	PassParameters->RWOutputTexture = GraphBuilder.CreateUAV(Inputs.OutputTexture);

	TShaderMapRef<FLSVisionMaskCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	const FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(Inputs.OutputTexture->Desc.Extent, FIntPoint(8, 8));

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("LostSignal.Vision.Mask"),
		ComputeShader,
		PassParameters,
		GroupCount);
}
