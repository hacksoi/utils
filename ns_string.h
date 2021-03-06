/* NOT JUST FOR STRINGS, ALSO CHARS. */

#ifndef NS_STRING_H
#define NS_STRING_H

#include "ns_common.h"

#include <string.h>

/* Chars */

char ToLowerCase(char C)
{
    char Result = (C >= 'A' && C <= 'Z') ? (C + ('a' - 'A')) : C;
    return Result;
}

bool IsNumber(char C)
{
    bool Result = C >= '0' && C <= '9';
    return Result;
}

inline internal 
int ConvertHexCharToInt(char HexChar)
{
    int Result = -1;
    if(HexChar >= '0' && HexChar <= '9')
    {
        Result = HexChar - '0';
    }
    else if(HexChar >= 'A' && HexChar <= 'F')
    {
        Result = HexChar - 'A' + 10;
    }
    else if(HexChar >= 'a' && HexChar <= 'f')
    {
        Result = HexChar - 'a' + 10;
    }
    return Result;
}

inline internal 
char ConvertIntToHexChar(uint8_t Int)
{
    Assert(Int < 16);
    char Result;
    if (Int >= 0 && Int <= 9)
    {
        Result = '0' + Int;
    }
    else
    {
        Result = 'a' + (Int - 10);
    }
    return Result;
}

/* Strings */

int GetLength(char *String)
{
    int Result = (int)strlen(String);
    return Result;
}

int GetLength(const char *String)
{
    int Result = (int)strlen((char *)String);
    return Result;
}

int
StrCmp(char *String1, char *String2)
{
    int Result = strcmp(String1, String2);
    return Result;
}

int
StringGetToken(char *dst, char *src, int dst_size, char delimiter = ' ')
{
    int len = 0;
    while(src[len] &&
          src[len] != delimiter &&
          dst_size)
    {
        dst[len] = src[len];
        len++;
    }
    dst[len] = 0;
    return len;
}

/* Checks for equality excluding null-terminators. */
inline bool
CheckStringEqualsWeak(char *String1, char *String2)
{
    for(; (*String1 && *String2) && (*String1 == *String2); String1++, String2++);
    return !*String2;
}

/* Checks if String1 contains String2. */
inline bool
CheckStringContains(char *String1, char *String2)
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
        if(CheckStringEqualsWeak(String1, String2))
        {
            return true;
        }
    }

    return false;
}

inline void
ns_string_reverse(char *String, int Length)
{
    for(int i = 0; i < Length/2; i++)
    {
        char Tmp = String[i];
        String[i] = String[Length - i - 1];
        String[Length - i - 1] = Tmp;
    }
}

inline void
ReverseString(char *String, int Length)
{
    ns_string_reverse(String, Length);
}

inline int
ns_string_from_int(char *Dest, int Value)
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

    ns_string_reverse(Dest, NumDigits);

    return NumDigits;
}

int IntToString(char *Dest, int Value)
{
    int Result = ns_string_from_int(Dest, Value);
    return Result;
}

int StringToInt(char *String, int GetLength)
{
    int Result = 0;
    for (int I = 0; I < GetLength; I++)
    {
        char Digit = String[I] - '0';
        Result *= 10;
        Result += Digit;
    }
    return Result;
}

int StringToInt(char *String)
{
    int GetLength = (int)strlen(String);
    int Result = StringToInt(String, GetLength);
    return Result;
}

uint32_t ConvertHexStringToInt(char *HexString, int HexStringLength)
{
    if (HexString[0] == '0' && !IsNumber(HexString[1]))
    {
        Assert(ToLowerCase(HexString[1]) == 'x');
        HexString += 2;
        HexStringLength -= 2;
    }

    uint32_t Result = 0;
    while (HexStringLength--)
    {
        Assert((*HexString >= '0' && *HexString <= '9') || (ToLowerCase(*HexString) >= 'a' && ToLowerCase(*HexString) <= 'f'));
        Result *= 16;
        Result += ConvertHexCharToInt(*HexString);
        HexString++;
    }
    return Result;
}

void ConvertMultipleSpacesToSingleSpace(char *String, int *StringLength_InOut)
{
    int StringLength = *StringLength_InOut;

    for (int I = 0; I < StringLength - 1; I++)
    {
        char CurChar = String[I];
        char NextChar = String[I + 1];

        /* Are we at a stretch of spaces? */
        if (CurChar == ' ' && NextChar == ' ')
        {
            /* How many spaces? */
            int NumSpaces = 0;
            for (int J = I;; J++)
            {
                char Char = String[J];
                if (Char == ' ')
                {
                    NumSpaces++;
                }
                else
                {
                    break;
                }
            }

            /* Remove the extra spaces. */
            int NumSpacesToRemove = NumSpaces - 1;
            RemoveConsecutiveElements(String, StringLength, I + 1, NumSpacesToRemove);

            StringLength -= NumSpacesToRemove;
        }
    }

    *StringLength_InOut = StringLength;
}

void TrimWhitespace(char *String, int *StringLength_InOut)
{
    int StringLength = *StringLength_InOut;

    /* Count whitespace in front. */
    int NumWhitespace = 0;
    for (int I = 0; I < StringLength; I++)
    {
        if (isspace(String[I]))
        {
            NumWhitespace++;
        }
        else
        {
            break;
        }
    }

    /* Remove them. */
    RemoveConsecutiveElements(String, StringLength, 0, NumWhitespace);

    /* Count whitespace in back. */
    NumWhitespace = 0;
    int FirstNonWhitespaceBackIdx;
    for (FirstNonWhitespaceBackIdx = StringLength - 1; FirstNonWhitespaceBackIdx > 0; FirstNonWhitespaceBackIdx--)
    {
        if (isspace(String[FirstNonWhitespaceBackIdx]))
        {
            NumWhitespace++;
        }
        else
        {
            break;
        }
    }

    /* Remove them. */
    RemoveConsecutiveElements(String, StringLength, FirstNonWhitespaceBackIdx + 1, NumWhitespace);

    *StringLength_InOut = StringLength;
}

char *GetBasename(char *FullFilePath)
{
    int Length = GetLength(FullFilePath);
    char *Result = FullFilePath;
    for (int I = Length - 1; I > 0; I--)
    {
        if (FullFilePath[I] == '/' || FullFilePath[I] == '\\')
        {
            Result = &FullFilePath[I  + 1];
            break;
        }
    }
    return Result;
}

#endif
