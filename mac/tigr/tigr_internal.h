// can't use pragma once here because this file probably will endup in .c
#ifndef __TIGR_INTERNAL_H__
#define __TIGR_INTERNAL_H__

#include "tigr.h"

// Graphics configuration.
#define TIGR_GAPI_GL

// Creates a new bitmap, with extra payload bytes.
Tigr* tigrBitmap2(int w, int h, int extra);

// Resizes an existing bitmap.
void tigrResize(Tigr* bmp, int w, int h);

// Calculates the biggest scale that a bitmap can fit into an area at.
int tigrCalcScale(int bmpW, int bmpH, int areaW, int areaH);

// Calculates a new scale, taking minimum-scale flags into account.
int tigrEnforceScale(int scale, int flags);

// Calculates the correct position for a bitmap to fit into a window.
void tigrPosition(Tigr* bmp, int scale, int windowW, int windowH, int out[4]);

// ----------------------------------------------------------

#include "TargetConditionals.h"
#define __MACOS__ 1

#ifdef TIGR_GAPI_GL
#include <OpenGL/gl3.h>

typedef struct {
    void* glContext;
    GLuint tex[2];
    GLuint vao;
    GLuint program;
    GLuint uniform_projection;
    GLuint uniform_model;
    GLuint uniform_parameters;
    int gl_legacy;
    int gl_user_opengl_rendering;
} GLStuff;
#endif

#define MAX_TOUCH_POINTS 10

typedef struct {
    int shown, closed;
#ifdef TIGR_GAPI_GL
    GLStuff gl;
#endif

    Tigr* widgets;
    int widgetsWanted;
    unsigned char widgetAlpha;
    float widgetsScale;

    float p1, p2, p3, p4;

    int flags;
    int scale;
    int pos[4];
    int lastChar;
    char keys[256], prev[256];
    int mouseInView;
    int mouseButtons;
} TigrInternal;
// ----------------------------------------------------------

TigrInternal* tigrInternal(Tigr* bmp);

void tigrGAPICreate(Tigr* bmp);
void tigrGAPIDestroy(Tigr* bmp);
int tigrGAPIBegin(Tigr* bmp);
int tigrGAPIEnd(Tigr* bmp);
void tigrGAPIPresent(Tigr* bmp, int w, int h);

#endif
