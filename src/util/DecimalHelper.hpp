#pragma once
#include "Decimal.h"

/****
 * Compare in by converting to string and checking for the various NaN indicators
 * @param in
 * @returns true if in is a valid decimal
 */
bool decimalIsValid(Decimal in);

/***
 * @param in the value to check
 * @returns true if value is NaN
*/
bool isNaN(Decimal in);

/***
 * @param in the value to check
 * @returns true if value is Inf (infinite)
*/
bool isInf(Decimal in);

/****
 * @param in the value to check
 * @returns the value or 0.0 if NaN
*/
Decimal checkNaN(Decimal in);

/****
 * @param in the value to check
 * @returns the value or 0.0 if Inf
*/
Decimal checkInf(Decimal in);
