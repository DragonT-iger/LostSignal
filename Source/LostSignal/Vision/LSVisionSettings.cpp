#include "Vision/LSVisionSettings.h"

// Places the vision settings under the standard Project section in UE settings UI.
FName ULSVisionSettings::GetCategoryName() const
{
	return TEXT("Project");
}
