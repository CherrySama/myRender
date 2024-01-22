#include "RenderPipeline.h"
#include <cmath>
#include <limits>
#include <cstdlib>
#include <algorithm>

Matrix4f ModelView;
Matrix4f Viewport;
Matrix4f Projection;
IShader::~IShader() {}



Vec3f m2v(Matrix4f m) {
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}


Matrix4f v2m(Vec3f v) {
    Matrix4f m;
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}


//视角矩阵，用于将(-1,1),(-1,1),(-1,1)映射到(1/8w,7/8w),(1/8h,7/8h),(0,255)
void viewport(int x, int y, int w, int h) {
	Viewport = Matrix4f::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255.f / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255.f / 2.f;
}

void projection(float coeff) {
    Projection = Matrix4f::identity();
    Projection[3][2] = coeff;
}

//朝向矩阵，变换矩阵
//更改摄像机视角=更改物体位置和角度，操作为互逆矩阵
//摄像机变换是先旋转再平移，所以物体需要先平移后旋转，且都是逆矩阵
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix4f::identity();
    Matrix4f translation = Matrix4f::identity();
    Matrix4f rotation = Matrix4f::identity();

    //矩阵的第四列是用于平移的。因为观察位置从原点变为了center，所以需要将物体平移-center
    //正交矩阵的逆 = 正交矩阵的转置
    for (int i = 0; i < 3; i++)
    {
        translation[i][3] = -center[i];
        rotation[0][i] = x[i];
        rotation[1][i] = y[i];
        rotation[2][i] = z[i];
    }
    ModelView = rotation * translation;
}

// 计算质心坐标
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    
    for (int i = 2; i--; ) {
        s[i][0] = B[i] - A[i];
        s[i][1] = C[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    //[u,v,1]和[AB,AC,PA]对应的x和y向量都垂直，所以叉乘
    Vec3f u = cross(s[0], s[1]);
    //三点共线时，会导致u[2]为0，此时返回(-1,1,1)
    if (std::abs(u[2]) > 1e-2)
        //若1-u-v，u，v全为大于0的数，表示点在三角形内部
        return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
    return Vec3f(-1, 1, 1);
}

// Bresenham for drawing line
void line(int x0, int y0, int x1, int y1, TGAImage& _image, TGAColor _color) {
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = (dx > dy ? dx : -dy) / 2;

    for (; ; )
    {
        _image.set(x0, y0, _color);
        if (x0 == x1 && y0 == y1)	break;
        int e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

// 光栅化
void triangle(Vec4f* _pts, IShader& _shader, TGAImage& _image, TGAImage& _zbuffer) {
    // 初始化三角形边界框
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            // 这里pts除以了最后一个分量，实现了透视中的缩放，所以作为边界框
            bboxmin[j] = std::min(bboxmin[j], _pts[i][j] / _pts[i][3]);
            bboxmax[j] = std::max(bboxmax[j], _pts[i][j] / _pts[i][3]);
        }
    }
    // 当前像素坐标P，颜色color
    Vec2i P;
    TGAColor color;

    // 遍历边界框中的每一个像素
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {            
            //这里pts除以了最后一个分量，实现了透视中的缩放，所以用于判断P是否在三角形内
            Vec3f centroid = barycentric(proj<2>(_pts[0] / _pts[0][3]), proj<2>(_pts[1] / _pts[1][3]), proj<2>(_pts[2] / _pts[2][3]), proj<2>(P));
            
            //插值计算P的zbuffer
            //pts[i]为三角形的三个顶点
            //pts[i][2]为三角形的z信息(0~255)
            //pts[i][3]为三角形的投影系数(1-z/c)
            float zp = (_pts[0][2] / _pts[0][3]) * centroid.x + (_pts[1][2] / _pts[1][3]) * centroid.y + (_pts[2][2] / _pts[2][3]) * centroid.z;
            int frag_depth = std::max(0, std::min(255, int(zp + .5)));

            // 若1-u-v，u，v全为大于等于0的数，表示点在三角形内部
            if (centroid.x < 0 || centroid.y < 0 || centroid.z<0 || _zbuffer.get(P.x, P.y)[0]>frag_depth) continue;
            
            // 调用片元着色器计算当前像素颜色
            bool discard = _shader.fragment(centroid, color);
            if (!discard)
            {
                _zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                _image.set(P.x, P.y, color);
            }
        }
    }
}