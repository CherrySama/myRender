#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <vector>
#include <iostream>
#include <algorithm>

extern Model* model;
extern Matrix4f ModelView;
extern Matrix4f Viewport;
extern Matrix4f Projection;

extern Vec3f light_dir;
extern Vec3f       eye;
extern Vec3f    center;
extern Vec3f        up;



void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(Vec3f eye, Vec3f center, Vec3f up);

struct IShader {
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer);




#endif //__OUR_GL_H__

