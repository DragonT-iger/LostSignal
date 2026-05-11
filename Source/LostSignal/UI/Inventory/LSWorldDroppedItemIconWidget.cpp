#include "UI/Inventory/LSWorldDroppedItemIconWidget.h"

#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"

ULSWorldDroppedItemIconWidget::ULSWorldDroppedItemIconWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = FVector2D(64.0f, 64.0f);
}

void ULSWorldDroppedItemIconWidget::SetIconTexture(UTexture2D* InIconTexture)
{
	IconBrush.SetResourceObject(InIconTexture);
	if (InIconTexture)
	{
		IconBrush.ImageSize = FVector2D(InIconTexture->GetSizeX(), InIconTexture->GetSizeY());
	}

	if (IconImage.IsValid())
	{
		IconImage->SetImage(&IconBrush);
	}
}

TSharedRef<SWidget> ULSWorldDroppedItemIconWidget::RebuildWidget()
{
	IconImage = SNew(SImage)
		.Image(&IconBrush);

	return IconImage.ToSharedRef();
}

void ULSWorldDroppedItemIconWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (IconImage.IsValid())
	{
		IconImage->SetImage(&IconBrush);
	}
}
