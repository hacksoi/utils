#ifndef NS_TEXTURE_RENDERER_H
#define NS_TEXTURE_RENDERER_H

/* TODO: 
    -filenames are kinda slow due to the compares. 
    */

#include "ns_common_renderer.h"
#include "ns_opengl_functions.h"
#include "ns_common.h"

#define MAX_TEXTURES 128
#define INVALID_TEXTURE_ID 0

#define FLOATS_PER_VERTEX 5

/* tr = TextureRenderer */
struct tr_texture
{
    bool IsSubTexture;

    const char *Name;
    v2 Dimensions;
    GLuint Id;
};

struct texture_renderer
{
    common_renderer Common;

    tr_texture Textures[MAX_TEXTURES];
    int NumTextures;

    GLuint LastTextureId;
};

const char *GlobalTextureRendererVertexShaderSource = R"STR(
#version 330 core
layout (location = 0) in vec3 Pos;
layout (location = 1) in vec2 vsTexCoord;

#define MAX_Z 128.0f

uniform float Zoom;
uniform vec2 WindowDimensions;
uniform vec2 CameraPos;

out vec2 fsTexCoord;

void main()
{
    vec2 HalfWindowDimensions = 0.5f*WindowDimensions;
    vec2 CameraPosAdjusted = CameraPos - HalfWindowDimensions;

    /* In the game, Z = 0 means Z = 1.0. */
    float ZAdjusted;
    if (Pos.z == 0.0f)
    {
        ZAdjusted = 1.0f;
    }
    else if (Pos.z > 0.0f)
    {
        ZAdjusted = 1.0f + Pos.z;
    }
    else
    {
        ZAdjusted = 1.0f/(1.0f + abs(Pos.z));
    }

    vec2 CameraPosZAdjusted = CameraPosAdjusted/ZAdjusted;
    vec2 ViewSpacePos = Pos.xy - CameraPosZAdjusted;
    /* Apply zoom. */
    vec2 RelToCenter = ViewSpacePos - HalfWindowDimensions;
    ViewSpacePos = Zoom*RelToCenter + HalfWindowDimensions;

    vec2 SnappedViewSpacePos = floor(ViewSpacePos);
    vec2 ClipPos = ((2.0f * SnappedViewSpacePos) / WindowDimensions) - 1.0f;
    gl_Position = vec4(ClipPos, 0.0f, 1.0f);

    fsTexCoord = vsTexCoord;
}
)STR";

const char *GlobalTextureRendererFragmentShaderSource = R"STR(
#version 330 core
in vec2 fsTexCoord;

uniform sampler2D Texture;

uniform int fsDebugValue;

out vec4 OutputColor;

void main()
{
    vec4 TexColor = texture(Texture, fsTexCoord);
    OutputColor = vec4(TexColor.r, TexColor.g, TexColor.b, TexColor.a);
}
)STR";

/* The min and max pixel coords are the values you'd get if you put your cursor directly on top of the min and max pixels
   of the image in paint.net. */
rect2 GetPixelTexCoordsFromPdn(rect2 RawPixelTexCoords, v2 TextureSize)
{
    /* X is obvious. For Y, just know that when you flip it (by subtracting it from the height), you actually get the pixel
       right above it. */
    RawPixelTexCoords.Min.Y++;
    RawPixelTexCoords.Max.X++;

    RawPixelTexCoords.Min.Y = TextureSize.Y - RawPixelTexCoords.Min.Y;
    RawPixelTexCoords.Max.Y = TextureSize.Y - RawPixelTexCoords.Max.Y;
    rect2 Result = RECT2(RawPixelTexCoords.Min, RawPixelTexCoords.Max);
    return Result;
}

rect2 GetNormalizedTexCoordsFromPaintDotNet(rect2 RawPixelTexCoords, v2 TextureSize)
{
    rect2 PixelTexCoords = GetPixelTexCoordsFromPdn(RawPixelTexCoords, TextureSize);
    rect2 NormalizedTexCoords = PixelTexCoords/TextureSize;
    return NormalizedTexCoords;
}

texture_renderer CreateTextureRenderer(int WindowWidth, int WindowHeight, const char *VertexShaderSource, const char *FragmentShaderSource)
{
    texture_renderer Result = {};
    Result.Common = CreateCommonRenderer(WindowWidth, WindowHeight, Megabytes(1),
                                         VertexShaderSource, FragmentShaderSource);

    Result.Common.Vbo = CreateAndBindVertexBuffer();
    Result.Common.Vao = CreateAndBindVertexArray();
    SetVertexAttributeFloat(0, 3, FLOATS_PER_VERTEX, 0);
    SetVertexAttributeFloat(1, 2, FLOATS_PER_VERTEX, 3);
    glBindVertexArray(0);

    return Result;
}

texture_renderer CreateTextureRenderer(int WindowWidth, int WindowHeight)
{
    texture_renderer Result = CreateTextureRenderer(WindowWidth, WindowHeight, 
                                                    GlobalTextureRendererVertexShaderSource, GlobalTextureRendererFragmentShaderSource);
    return Result;
}

tr_texture *GetTexture(texture_renderer *TextureRenderer, const char *Name)
{
    tr_texture *Result = NULL;
    for (int I = 0; I < TextureRenderer->NumTextures; I++)
    {
        tr_texture *Tex = &TextureRenderer->Textures[I];
        if (!strcmp(Tex->Name, Name))
        {
            Result = Tex;
            break;
        }
    }
    Assert(Result != NULL);
    return Result;
}

tr_texture *AddTexture(texture_renderer *TextureRenderer, const char *Name, unsigned int TextureId, int TextureWidth, int TextureHeight)
{
    Assert(TextureId != 0);

    tr_texture *NewTexture;
    ArrayGetLastAndAddOne(NewTexture, TextureRenderer->Textures, TextureRenderer->NumTextures);
    NewTexture->Name = Name;
    NewTexture->Dimensions = V2(TextureWidth, TextureHeight);
    NewTexture->Id = TextureId;

    return NewTexture;
}

tr_texture *AddTexture(texture_renderer *TextureRenderer, const char *FilePath, uint32_t *PixelValuesToMask = NULL, int NumPixelValuesToMask = 0)
{
    int ChannelCount, TextureWidth, TextureHeight;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *TextureData = stbi_load(FilePath, &TextureWidth, &TextureHeight, &ChannelCount, 0);

    if (PixelValuesToMask != NULL)
    {
        if (IsLittleEndian())
        {
            /* Reverse the pixel values the user passed. */
            for (int I = 0; I < NumPixelValuesToMask; I++)
            {
                ReverseBytes(&PixelValuesToMask[I]);
            }
        }

        int TotalBytes = 4*TextureWidth*TextureHeight;
        for (int I = 0; I < TotalBytes; I += 4)
        {
            uint32_t *PixelValuePtr = (uint32_t *)&TextureData[I];
            uint32_t PixelValue = *PixelValuePtr;
            bool IsMaskValue = false;
            for (int J = 0; J < NumPixelValuesToMask; J++)
            {
                if (PixelValue == PixelValuesToMask[J])
                {
                    IsMaskValue = true;
                    break;
                }
            }
            if (IsMaskValue)
            {
                /* Remove the alpha component. */
                if (IsLittleEndian())
                {
                    PixelValue &= 0x00ffffff;
                }
                else
                {
                    PixelValue &= 0xffffff00;
                }

                /* Write it back. */
                *PixelValuePtr = PixelValue;
            }
        }
    }

    unsigned int TextureId;
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    /* Note: if this crashes, it's probably because the width of the image in bytes is not a multiple of 4 bytes (https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads). */
    glTexImage2D(GL_TEXTURE_2D, 0, ChannelCount == 3 ? GL_RGB : GL_RGBA, TextureWidth, TextureHeight, 0, ChannelCount == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, TextureData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(TextureData);

    const char *Basename = GetBasename((char *)FilePath);
    tr_texture *NewTexture = AddTexture(TextureRenderer, (const char *)Basename, TextureId, TextureWidth, TextureHeight);
    return NewTexture;
}

#if 0
tr_texture *AddSubTexture(texture_renderer *TextureRenderer, tr_texture *ParentTexture, const char *Name, rect2 PixelTexCoords)
{
    GetLastAndAddOne(tr_texture *NewSubTexture, TextureRenderer->Textures, TextureRenderer->NumTextures);
    NewSubTexture->IsSubTexture = true;
    NewSubTexture->Name = Name;
    NewSubTexture->Dimensions = GetDimensions(PixelTexCoords);
    NewSubTexture->ParentName = ParentTexture->Name;
    NewSubTexture->NormalizedTexCoords = PixelTexCoords/ParentTexture->Dimensions;
    return NewSubTexture;
}

tr_texture *AddSubTexture(texture_renderer *TextureRenderer, const char *ParentName, const char *Name, rect2 PixelTexCoords)
{
    tr_texture *ParentTexture = GetTexture(TextureRenderer, ParentName);
    AddSubTexture(TextureRenderer, ParentTexture, Name, PixelTexCoords);
}

tr_texture *AddSubTexturePdn(texture_renderer *TextureRenderer, const char *ParentName, const char *Name, rect2 TexCoordsPdn)
{
    tr_texture *ParentTexture = GetTexture(TextureRenderer, ParentName);
    rect2 PixelTexCoords = GetPixelTexCoordsFromPdn(TexCoordsPdn, ParentTexture->Dimensions);
    AddSubTexture(TextureRenderer, ParentTexture, Name, PixelTexCoords);
}
#endif

void Render(texture_renderer *TextureRenderer)
{
    BeginRender(&TextureRenderer->Common, TextureRenderer->Common.NumVertexData);
    glBindTexture(GL_TEXTURE_2D, TextureRenderer->LastTextureId);
    Assert(TextureRenderer->Common.NumVertexData % FLOATS_PER_VERTEX == 0);
    glDrawArrays(GL_TRIANGLES, 0, TextureRenderer->Common.NumVertexData/FLOATS_PER_VERTEX);
    EndRender(&TextureRenderer->Common);

    /* Reset. */
    TextureRenderer->Common.NumVertexData = 0;
    TextureRenderer->LastTextureId = INVALID_TEXTURE_ID;
}

void DrawTextureNormalizedTexCoords(texture_renderer *TextureRenderer, tr_texture *Texture, rect2 PosCoords, float Z, rect2 NormalizedTexCoords, bool DrawReversed)
{
    Assert(!Texture->IsSubTexture);

    if (TextureRenderer->LastTextureId != INVALID_TEXTURE_ID)
    {
        if (TextureRenderer->LastTextureId != Texture->Id)
        {
            Render(TextureRenderer);
        }
    }

    if (DrawReversed)
    {
        Swap(&NormalizedTexCoords.Min.X, &NormalizedTexCoords.Max.X);
    }

    InsertTexture((float *)TextureRenderer->Common.VertexData, TextureRenderer->Common.MaxVertexDataBytes, &TextureRenderer->Common.NumVertexData,
                  QUAD2(PosCoords), Z, QUAD2(NormalizedTexCoords));

    TextureRenderer->LastTextureId = Texture->Id;
}

#if 0
void DrawTextureNormalizedTexCoords(texture_renderer *TextureRenderer, const char *Name, rect2 PosCoords = RECT2_ZERO, float Z = 1.0f, 
                                    rect2 TexCoords = RECT2_ZERO, bool DrawReversed = false)
{
    tr_texture *Texture = GetTexture(TextureRenderer, Name);
    DrawTextureNormalizedTexCoords(TextureRenderer, Texture, PosCoords, Z, TexCoords, DrawReversed);
}

void DrawTexture(texture_renderer *TextureRenderer, const char *Name, rect2 PosCoords, float Z = 1.0f,
                 rect2 TexCoords = RECT2_ZERO, bool DrawReversed = false)
{
    tr_texture *Tex = GetTexture(TextureRenderer, Name);

    /* Get the normalized tex coords. */
    rect2 NormalizedTexCoords;
    if (Tex->IsSubTexture)
    {
        Assert(TexCoords == RECT2_ZERO);
        NormalizedTexCoords = Tex->NormalizedTexCoords;
        Name = Tex->ParentName;
    }
    else
    {
        if (TexCoords == RECT2_ZERO)
        {
            /* Do the entire image. */
            TexCoords = RECT2(V2(0.0f, 0.0f), V2(Tex->Dimensions.X, Tex->Dimensions.Y));
        }

        NormalizedTexCoords = TexCoords/Tex->Dimensions;
        Assert(NormalizedTexCoords.Min >= V2_ZERO && NormalizedTexCoords.Max >= V2_ZERO);
    }
    DrawTextureNormalizedTexCoords(TextureRenderer, Name, PosCoords, Z, NormalizedTexCoords, DrawReversed);
}

void DrawTexture(texture_renderer *TextureRenderer, const char *Name, v2 Pos, float Z, rect2 TexCoords, bool DrawReversed = false)
{
    v2 TexCoordsDimensions = GetSize(TexCoords);
    rect2 PosCoords = RECT2(Pos, Pos + TexCoordsDimensions);
    DrawTexture(TextureRenderer, Name, PosCoords, Z, TexCoords, DrawReversed);
}
#endif

void DrawTexturePdn(texture_renderer *TextureRenderer, const char *Name, v2 Pos, float Z, rect2 PixelTexCoordsPdn, bool DrawReversed = false)
{
    tr_texture *Tex = GetTexture(TextureRenderer, Name);
    rect2 PixelTexCoords = GetPixelTexCoordsFromPdn(PixelTexCoordsPdn, Tex->Dimensions);
    rect2 NormalizedTexCoords = PixelTexCoords/Tex->Dimensions;
    v2 Size = GetSize(PixelTexCoords);
    rect2 PosCoords = RectFromPosSize(Pos, Size);
    DrawTextureNormalizedTexCoords(TextureRenderer, Tex, PosCoords, Z, NormalizedTexCoords, DrawReversed);
}

void DrawTexture(texture_renderer *TextureRenderer, const char *Name, v2 Pos, v2 Size)
{
    tr_texture *Tex = GetTexture(TextureRenderer, Name);
    rect2 NormalizedTexCoords = RECT2(0.0f, 0.0f, 1.0f, 1.0f);
    rect2 PosCoords = RectFromPosSize(Pos, Size);
    DrawTextureNormalizedTexCoords(TextureRenderer, Tex, PosCoords, 1.0f, NormalizedTexCoords, false);
}

u32 GetId(texture_renderer *TextureRenderer, const char *TextureName)
{
    tr_texture *Texture = GetTexture(TextureRenderer, TextureName);
    u32 Result = Texture->Id;
    return Result;
}

rect2 GetNormalizedTexCoordsPdn(texture_renderer *TextureRenderer, const char *TextureName, rect2 PixelTexCoordsPdn)
{
    tr_texture *Texture = GetTexture(TextureRenderer, TextureName);
    rect2 PixelTexCoords = GetPixelTexCoordsFromPdn(PixelTexCoordsPdn, Texture->Dimensions);
    rect2 NormalizedTexCoords = PixelTexCoords/Texture->Dimensions;
    return NormalizedTexCoords;
}

void SetCameraPos(texture_renderer *TextureRenderer, v2 NewCameraPos)
{
    TextureRenderer->Common.CameraPos = NewCameraPos;
}

void SetZoom(texture_renderer *TextureRenderer, float NewZoom)
{
    TextureRenderer->Common.Zoom = NewZoom;
}

void SetWindowSize(texture_renderer *TextureRenderer, int NewWindowWidth, int NewWindowHeight)
{
    SetWindowSize(&TextureRenderer->Common, NewWindowWidth, NewWindowHeight);
}

void SetFragShaderDebugValue(texture_renderer *TextureRenderer, int NewDebugValue)
{
    TextureRenderer->Common.FragShaderDebugValue = NewDebugValue;
}

void ResetFragShaderDebugValue(texture_renderer *TextureRenderer)
{
    TextureRenderer->Common.FragShaderDebugValue = 0;
}

#endif