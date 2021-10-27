#include "svpng.inc"
#include <iostream>
#include "material.h"
#include "camera.h"
#include "hitable_list.h"
#include "sphere.h"
#include <float.h>
#include <omp.h>//添加并行库
#include <random>
#include <time.h>//添加计时库

#define W 800
#define H 400

// normal shader
// 这是默认图像，我们编写ray_color来代替
Vec3 color(const Ray &r, hitable *world) {
  hit_record rec;
  if (world->hit(r, 0.0, FLT_MAX, rec)) {
    //有撞点：即为球体，将球体颜色设置为球在该点的单位法向量的色彩表映射值
    return 0.5 * //衰减
           Vec3(rec.normal.x() + 1, rec.normal.y() + 1, rec.normal.z() + 1);//rec.normal就是在撞点处的法向
  } else {
    // 无撞点：将颜色设置为背景色
    Vec3 unit_direction = unit_vector(r.Direction());
    float t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * Vec3(1.0, 1.0, 1.0) +
           t * Vec3(0.5, 0.7, 1.0); // white, light blue
  }
}

/******************** material shader ************************/
/**
 * @brief 光线追踪函数
 * @param r: 光线
 * @param world： 场景中的物体集合
 * @param depth: 光线弹射次数，到达最大次数之后，返回0
 */

Vec3 ray_color(const Ray &r, hitable *world, int depth) {
  hit_record rec;
  //如果光线到达反射次数,停止
  //递归的过程，每次调用ray_colordepth都自减
  if(depth <= 0)
    return Vec3(0,0,0);

  //如果光线与物体相交,设置反射后的颜色

  if (world->hit(r, 0.001, INFINITY, rec)) {
    Ray scattered;
    Vec3 attenu;//衰减
    if(rec.mat->scatter(r,rec,scattered,attenu))//如果光线能够继续反射（不能继续反射会返回0）
      return attenu * ray_color(scattered,world,depth-1);//递归寻找下一个光线
      //虽然scatter的返回值仅仅是一个布尔值，但是由于该函数引用传参，故颜色参数也被修改
    return Vec3(0,0,0);//如果不能继续反射，则返回000白色
  }
  

  //如果光线与任何物体不相交,设置背景颜色
  Vec3 unit_direction = unit_vector(r.Direction());
  auto t = 0.5*(unit_direction.y() + 1.0);
  return (1.0-t)*Vec3(1.0, 1.0, 1.0) + t*Vec3(0.5, 0.7, 1.0);
}

unsigned char img[W * H * 3];

int main() {
  unsigned char *p = img;

  // 声明
  Camera cam;

  int SamplingRate = 100; //采样次数
  int Depth =
      16; //光线弹射次数, 如果是一个空心玻璃球的话,弹射次数至少>4次，思考一下？

  //新建材质，漫反射示例
  // Material *diffuse_center = new Material(Vec3(0.5, 0.5, 0.5), Diffuse);//这里的Vec3是颜色
  // Material *diffuse_ground = new Material(Vec3(0.5, 0.5, 0.5), Diffuse);
  //新建材质，金属示例
  // Material *diffuse_center = new Material(Vec3(0.7, 0.3, 0.3), Diffuse);//这里的Vec3是颜色
  // Material *diffuse_ground = new Material(Vec3(0.8, 0.8, 0.0), Diffuse);
  // Material *metal_left = new Material(Vec3(0.8,0.8,0.8), 1.0,Metal);
  // Material *metal_right = new Material(Vec3(0.8,0.6,0.2), 0.3,Metal);
  //新建材质，透明示例
  Material *diffuse_center = new Material(Vec3(0.1, 0.2, 0.5), Diffuse);//这里的Vec3是颜色
  Material *diffuse_ground = new Material(Vec3(0.8, 0.8, 0.0), Diffuse);
  Material *dielectric_left = new Material(1.5,Dielectric);
  Material *metal_right = new Material(Vec3(0.8,0.6,0.2), 0.0,Metal);
 
  
  /*将所有能够被撞击的物体信息保存在列表中*/
  //漫反射示例
  // std::vector<hitable *> list;
  // list.push_back(new sphere(Vec3(0.0, -100.5, -1.0), 100.0, diffuse_ground));//这里的Vec3是坐标
  // list.push_back(new sphere(Vec3(0.0, 0.0, -1.0), 0.5, diffuse_center));
  //金属示例
  // std::vector<hitable *> list;
  // list.push_back(new sphere(Vec3(0.0, -100.5, -1.0), 100.0, diffuse_ground));//这里的Vec3是坐标
  // list.push_back(new sphere(Vec3(0.0, 0.0, -1.0), 0.5, diffuse_center));
  // list.push_back(new sphere(Vec3(-1.0,0.0,-1.0),0.5,metal_left));
  // list.push_back(new sphere(Vec3(1.0,0.0,-1.0),0.5,metal_right));
  //透明示例
  std::vector<hitable *> list;
  list.push_back(new sphere(Vec3(0.0, -100.5, -1.0), 100.0, diffuse_ground));//这里的Vec3是坐标
  list.push_back(new sphere(Vec3(0.0, 0.0, -1.0), 0.5, diffuse_center));
  list.push_back(new sphere(Vec3(1.0,0.0,-1.0),0.5,metal_right));
  list.push_back(new sphere(Vec3(-1.0,0.0,-1.0),0.5,dielectric_left));
  list.push_back(new sphere(Vec3(-1.0,0.0,-1.0),-0.4,dielectric_left));

  hitable *world = new hitable_list(list, list.size());
  // 添加计时
  clock_t start, end;
  start = time(NULL);

  #pragma omp parallel for 
      for (int j = H - 1; j >= 0; j--) {
        for (int i = 0; i < W; i++) {
          Vec3 col(0, 0, 0);
          for (int s = 0; s < SamplingRate; s++) {
            float u = float(i + drand48()) / float(W);
            float v = float(j + drand48()) / float(H);

            Ray r = cam.getRay(u, v);
            //col += color(r, world);
            col += ray_color(r, world, Depth);//加入光追颜色
          }
          col /= float(SamplingRate);
          
          int temp = (W * (H - 1 - j) + i) * 3;
          col[0] = sqrt(col[0]);
          col[1] = sqrt(col[1]);
          col[2] = sqrt(col[2]);
          p[temp] = int((255.99) * (col[0])); 
          p[temp + 1] = int((255.99) * (col[1]));
          p[temp + 2] = int((255.99) * (col[2]));
        }
    }
  
  end = time(NULL);
  double time_used = (end-start);
  svpng(fopen("bg.png", "wb"), W, H, img, 0);
  std::cout << "finished, used "<< time_used <<"s" << std::endl;

  //释放资源
  //  delete diffuse_center, diffuse_ground;
    // delete diffuse_center, diffuse_ground,metal_left,metal_right;
  delete diffuse_center, diffuse_ground,metal_right,dielectric_left;

  return 0;
}
