#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>
#include "RenderPipeline.h"
#include "Shader.h"


//定义参数
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model* model = NULL;
const int width = 800;
const int height = 800;
const int depth = 255;

Vec3f light_dir(0, 1, 1);
Vec3f       eye(1, 0.5, 1.5);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);


int main(int argc, char** argv) {
    //加载模型
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head/african_head.obj");
    }
    //初始化变换矩阵，投影矩阵，视角矩阵
    lookat(eye, center, up);
    projection(-1.f / (eye - center).norm());
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    light_dir.normalize();
    //初始化image和zbuffer
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    //实例化高洛德着色
    //GouraudShader shader;
    //实例化Phong着色
    PhongShader shader;
    //实例化Toon着色
    //ToonShader shader;
    //实例化Flat着色 
    //FlatShader shader;

    //以模型面作为循环控制量
    for (int i = 0; i < model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j = 0; j < 3; j++) {
            //通过顶点着色器读取模型顶点
            //变换顶点坐标到屏幕坐标（视角矩阵*投影矩阵*变换矩阵*v） ***其实并不是真正的屏幕坐标，因为没有除以最后一个分量
            //计算光照强度
            screen_coords[j] = shader.vertex(i, j);
        }
        //遍历完3个顶点，一个三角形光栅化完成
        //绘制三角形，triangle内部通过片元着色器对三角形着色
        triangle(screen_coords, shader, image, zbuffer);
    }

    image.flip_vertically();
    zbuffer.flip_vertically();
    image.write_tga_file("output.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    delete model;
    return 0;
}

