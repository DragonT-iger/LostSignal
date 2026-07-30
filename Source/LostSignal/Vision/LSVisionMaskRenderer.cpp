#include "Vision/LSVisionMaskRenderer.h"

#include "CoreGlobals.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LSVisionGlobalShader.h"
#include "RenderGraphUtils.h"
#include "TextureResource.h"

ALSVisionMaskRenderer::ALSVisionMaskRenderer()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Uploads polygon data to the render thread and refreshes the visibility mask texture.
void ALSVisionMaskRenderer::RequestMaskUpdate(const FLSVisionPolygonData& PolygonData)
{
	LastRenderedPolygon = PolygonData;

	if (VisibilityMaskRenderTarget == nullptr || IsEngineExitRequested())
	{
		return;
	}

	FTextureRenderTargetResource* RenderTargetResource = VisibilityMaskRenderTarget->GameThread_GetRenderTargetResource();
	if (RenderTargetResource == nullptr)
	{
		return;
	}

	TArray<FVector2f> PolygonUploadData;
	PolygonUploadData.Reserve(PolygonData.Points.Num());

	// 점 플래그(1.0=오클루더 hit, 0.0=열림)를 점과 동일 길이로 맞춰 업로드한다.
	// 플래그가 비거나 길이가 어긋나면 0.0(열림)으로 채워 기존 동작(모든 엣지 페더)으로 안전하게 폴백한다.
	TArray<float> PolygonFlagUploadData;
	PolygonFlagUploadData.Reserve(PolygonData.Points.Num());

	// 폴리곤 점을 Origin(플레이어) 기준 상대 좌표로 내린다. 이 때문에 셰이더의 좌표계는 "월드"가 아니라
	// "플레이어 상대"가 된다 — 절대 월드 위치가 필요한 계산(노이즈 샘플)은 VisionOrigin을 더해 복원해야 한다.
	for (int32 PointIndex = 0; PointIndex < PolygonData.Points.Num(); ++PointIndex)
	{
		const FVector2D LocalPoint = PolygonData.Points[PointIndex] - PolygonData.Origin;
		PolygonUploadData.Add(FVector2f(LocalPoint));
		PolygonFlagUploadData.Add(PolygonData.PointFlags.IsValidIndex(PointIndex) ? PolygonData.PointFlags[PointIndex] : 0.0f);
	}

	const float VisionRadius = PolygonData.VisionRadius;
	const float Extent = PolygonData.Extent;
	// 상대 좌표계의 원점이 놓인 절대 월드 위치. 셰이더가 노이즈를 월드 고정으로 샘플하는 데 쓴다.
	const FVector2f MaskOriginWS(PolygonData.Origin);
	const float FeatherWidthValue = FeatherWidth;
	const FLinearColor HiddenColorValue = HiddenColor;

	// 노이즈 리소스를 게임 스레드에서 확보해 렌더 스레드 람다로 캡처한다. 텍스쳐가 없으면 진폭 0으로 흔들림 비활성.
	FTextureResource* NoiseResource = EdgeNoiseTexture != nullptr ? EdgeNoiseTexture->GetResource() : nullptr;
	const float NoiseScaleValue = EdgeNoiseScale;
	const float NoiseWidthValue = NoiseResource != nullptr ? EdgeNoiseWidth : 0.0f;
	const float OccluderFeatherScaleValue = OccluderFeatherScale;

	ENQUEUE_RENDER_COMMAND(RenderLostSignalVisionMask)(
		[RenderTargetResource, VisionRadius, Extent, MaskOriginWS, FeatherWidthValue, HiddenColorValue, NoiseResource, NoiseScaleValue, NoiseWidthValue, OccluderFeatherScaleValue, PolygonUploadData = MoveTemp(PolygonUploadData), PolygonFlagUploadData = MoveTemp(PolygonFlagUploadData)](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRHITexture* OutputTextureRHI = RenderTargetResource->GetRenderTargetTexture();
			if (OutputTextureRHI == nullptr)
			{
				return;
			}

			FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(OutputTextureRHI, TEXT("LostSignalVisionMaskRT")));

			LSVision::FMaskDispatchInputs Inputs;
			Inputs.OutputTexture = OutputTexture;
			Inputs.VisionOrigin = MaskOriginWS;
			Inputs.VisionRadius = VisionRadius;
			Inputs.FeatherWidth = FeatherWidthValue;
			Inputs.WorldMin = FVector2f(-Extent, -Extent);
			Inputs.WorldMax = FVector2f(Extent, Extent);
			Inputs.HiddenColor = HiddenColorValue;
			Inputs.NoiseTexture = NoiseResource != nullptr ? NoiseResource->TextureRHI.GetReference() : nullptr;
			Inputs.NoiseScale = NoiseScaleValue;
			Inputs.NoiseWidth = NoiseWidthValue;
			Inputs.OccluderFeatherScale = OccluderFeatherScaleValue;

			Inputs.PolygonPointCount = PolygonUploadData.Num();
			if (PolygonUploadData.Num() > 0)
			{
				FRDGBufferRef PolygonBuffer = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("LostSignalVision.PolygonPoints"),
					PolygonUploadData);

				Inputs.PolygonPointsSRV = GraphBuilder.CreateSRV(PolygonBuffer);

				FRDGBufferRef PolygonFlagBuffer = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("LostSignalVision.PolygonPointFlags"),
					PolygonFlagUploadData);

				Inputs.PolygonPointFlagsSRV = GraphBuilder.CreateSRV(PolygonFlagBuffer);
			}

			LSVision::AddVisionMaskPass(GraphBuilder, Inputs);
			GraphBuilder.Execute();
		});
}
