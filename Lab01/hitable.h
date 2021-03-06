#ifndef HITABLE_H
#define HITABLE_H

#include "ray.h"

class Material;
struct hit_record {
  float t;
  Vec3 p;
  Vec3 normal;
  Material *mat;
  bool front_face;

  inline void set_face_normal(const Ray& r, const Vec3& outward_normal)
  {
    //根据光线方向设置法向，使得法线方向总是向外的
    front_face = dot(r.Direction(), outward_normal) < 0;
    normal = front_face ? outward_normal :-outward_normal;
  }
}; /*该结构体记录“撞点”处的信息：离光线起点的距离t、撞点的坐标向量p、撞点处的法向量normal。*/

class hitable {
public:
  virtual bool hit(const Ray &r, float t_min, float t_max,
                   hit_record &rec) const = 0;
  /*hit()在此被声明为虚函数，则hitable为抽象类。抽象类的子类中必须实现其虚函数*/
};
#endif // HITABLE_H