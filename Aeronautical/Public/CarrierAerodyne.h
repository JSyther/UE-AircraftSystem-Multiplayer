// @2023 All rights reversed by Reverse-Alpha Studios
/**
 * ACarrierAerodyne represents a type of aircraft primarily designed for transporting players and cargo.
 * Unlike fighter jets, it's not intended for combat purposes but rather serves as a carrier or transporter.
 * It inherits functionality and characteristics from the AAerodyne base class.
 */

#pragma once

#include "CoreMinimal.h"
#include "Aeronautical/Aerodyne.h"
#include "CarrierAerodyne.generated.h"


UCLASS()
class SYNTHETICA_API ACarrierAerodyne : public AAerodyne
{
	GENERATED_BODY()

protected:
	ACarrierAerodyne();
	
};
