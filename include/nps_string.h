#ifndef NPS_STRING_H
#define NPS_STRING_H

#include <string.h>

#include "common.h"

/* Checks for equality excluding null-terminators. */
internal inline bool
StringEqualsWeak(char *String1, char *String2)
{
    for(; (*String1 && *String2) && (*String1 == *String2); String1++, String2++);
    return !*String2;
}

/* Checks if String1 contains String2. */
internal inline bool
StringContains(char *String1, char *String2)
{
    if(!String1 || !String2)
    {
        return false;
    }

    size_t String2Length = strlen(String2);
    if(String2Length == 0)
    {
        return false;
    }

    char *String1End = String1 + strlen(String1);
    for(; (size_t)(String1End - String1) >= String2Length; String1++)
    {
        if(StringEqualsWeak(String1, String2))
        {
            return true;
        }
    }

    return false;
}

internal inline void
Reverse(char *String, int Length)
{
    for(int i = 0; i < Length/2; i++)
    {
        char Tmp = String[i];
        String[i] = String[Length - i - 1];
        String[Length - i - 1] = Tmp;
    }
}

internal inline uint32_t
ConvertToString(char *Dest, int Value)
{
    if(Value < 0)
    {
        return 0;
    }

    int NumDigits = 0;
    do
    {
        Dest[NumDigits++] = '0' + (Value % 10);
        Value /= 10;
    }
    while(Value > 0);

    Reverse(Dest, NumDigits);

    return NumDigits;
}
#endif
