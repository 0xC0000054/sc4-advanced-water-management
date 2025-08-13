////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-advanced-water-management, a DLL Plugin for
// SimCity 4 that improves interoperability with advanced water management.
//
// Copyright (c) 2024, 2025 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "WaterBuildingPropertyUtil.h"
#include "cISC4Occupant.h"
#include "cISCPropertyHolder.h"
#include "SCPropertyUtil.h"

static const uint32_t kWaterProducedProperty = 0x88ed3303;
static const uint32_t kWaterSourceProperty = 0x48f23a7e;

bool WaterBuildingPropertyUtil::IsWaterProducer(cISC4Occupant* pOccupant, WaterSource& waterSource)
{
	cISCPropertyHolder* pPropertyHolder = pOccupant->AsPropertyHolder();

	if (pPropertyHolder)
	{
		uint32_t waterProduced = 0;

		if (SCPropertyUtil::GetPropertyValue(pPropertyHolder, kWaterProducedProperty, waterProduced))
		{
			if (waterProduced > 0)
			{
				uint8_t source = 0;

				if (SCPropertyUtil::GetPropertyValue(pPropertyHolder, kWaterSourceProperty, source))
				{
					waterSource = static_cast<WaterSource>(source);
					return true;
				}
			}
		}
	}

	return false;
}

bool WaterBuildingPropertyUtil::IsNonAquiferWaterSource(cISC4Occupant* pOccupant)
{
	cISCPropertyHolder* pPropertyHolder = pOccupant->AsPropertyHolder();

	if (pPropertyHolder)
	{
		uint8_t source = 0;

		if (SCPropertyUtil::GetPropertyValue(pPropertyHolder, kWaterSourceProperty, source))
		{
			WaterSource waterSource = static_cast<WaterSource>(source);

			return waterSource != WaterSource::Aquifer;
		}
	}

	return false;
}
