#include "material.h"
#include <cstdlib>
float get_random_float(float min, float max) {//获得随机数
  static std::random_device dev;
  static std::mt19937 rng(dev());
  static std::uniform_real_distribution<float> dist(
      min, max); // distribution in range [min, max]
  return dist(rng);
}
float random_float()
{
  return rand()/(RAND_MAX+1.0);//返回0～1的随机数
}
//函数get_random_vec3 返回一个随机向量
Vec3 get_random_vec3(float min,float max)
{
  return(Vec3(get_random_float(min,max),get_random_float(min,max),get_random_float(min,max)));
}

//函数random_in_unit_sphere()返回一个单位球内的随机方向
Vec3 random_in_unit_sphere() {
  while(true)
  {
    Vec3 p = get_random_vec3(-1,1);
    if(p.length()>1) continue;
    return p;//返回长度小于1的向量 
  }
}

Vec3 random_sample_hemisphere(const Vec3 &normal) {
//限制方向在上半球
    Vec3 in_unit_sphere = random_in_unit_sphere();
    if (dot(in_unit_sphere, normal) > 0.0) 
        return in_unit_sphere;
    else
        return -in_unit_sphere;
}

Vec3 reflect(Vec3 &v, Vec3 &n) { 
  return v-2*dot(v,n)*n;
}

Vec3 refract(Vec3 &v, Vec3 &n, float ior) {
    auto cos = fmin(dot(-v, n), 1.0);
    Vec3 rPerp =  ior * (v + cos*n);
    Vec3 rParallel = -sqrt(fabs(1.0 - rPerp.squared_length())) * n;
    return rPerp + rParallel;
}

float schlick_reflectance(float cos, float ior) {
  // 真正的玻璃会在边缘处反射
  // 使用Schlick的近似方法算反射系数
  auto r0 = (1-ior) / (1+ior);
  r0 = r0*r0;
  return r0 + (1-r0)*pow((1 - cos),5);
}

Material::Material(Vec3 albedo_, MaterialType type_) {
  albedo = albedo_;
  type = type_;
}

Material::Material(Vec3 albedo_, float fuzz_, MaterialType type_) {
  albedo = albedo_;
  type = type_;
  fuzz = std::min<float>(fuzz_, 1);
}

Material::Material(float ior_, MaterialType type_) {
  ior = ior_;
  type = type_;
}

Material::~Material() {}

bool Material::scatter(const Ray &r, hit_record &rec, Ray &scattered,
                       Vec3 &attenu) {
  switch (type) {
  case Diffuse: {
  auto scatterDirection = rec.normal + random_in_unit_sphere();//随机生成反射光线的方向

  // 不反射的方向：如果上面随机生成的向量正好在rec.normal的反面，则scatterDirection为0
  if (scatterDirection.near_zero())
      scatterDirection = rec.normal;

  scattered = Ray(rec.p, scatterDirection);//反射光线的方向
  attenu = albedo;//衰减
  return true;
}

  case Metal: {
    Vec3 rd = unit_vector(r.Direction());
    Vec3 reflected = reflect(rd, rec.normal);//反射光线上的一个点
    scattered = Ray(rec.p, reflected + fuzz*random_in_unit_sphere());//反射光线方向，加随机方向显示模糊
    attenu = albedo;//设定衰减率
    return (dot(scattered.Direction(), rec.normal) > 0);//返回反射方向是否合法
  }

  case Dielectric: {
    attenu = Vec3(1.0, 1.0, 1.0);
    //根据法向设置折射率比值
    rec.set_face_normal(r,rec.normal);//这是框架没有的
    double refractionRatio = rec.front_face ? (1.0/ior) : ior;

    Vec3 unitDirection = unit_vector(r.Direction());

    double cosTheta = fmin(dot(-unitDirection, rec.normal), 1.0);
    double sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    bool cannotRefract = refractionRatio * sinTheta > 1.0;//这里折射方程没有解，sin不能大于1
    Vec3 direction;


    if (cannotRefract || schlick_reflectance(cosTheta,refractionRatio)>random_float())//真正的玻璃会在边缘处反射
    // 如果随机数小于反射系数，则设置为反射光线，反之，设置为折射光线。
    // 考虑到一个像素点被设置为采样100次，这100次中反射光线的条数基本和schlick_reflectance的值正相关，这样操作可以让反射光线和折射光线中和一下
        direction = reflect(unitDirection, rec.normal);
    else
        direction = refract(unitDirection, rec.normal, refractionRatio);

    scattered = Ray(rec.p, direction);
    return true;
    break;
  }

  default: {
    std::cout << "Unsurportted Materail type!" << std::endl;
    return false;
    break;
  }
  }
}