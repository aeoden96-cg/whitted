#ifndef HITTABLE_H
#define HITTABLE_H

#include "utils.h"

class Hittable;

struct hit_record {
    hit_record() : object(nullptr),t(kInfinity) {}
    float t;
    glm::vec3 p;
    glm::vec3 normal;
    MaterialType material;
    Hittable *object = nullptr;
    glm::vec2 st;
    glm::vec2 uv;
    unsigned triIndex;
};

class Hittable
{
 public:
    Hittable() :
        materialType(DIFFUSE_AND_GLOSSY),
        ior(1.3), Kd(0.8), Ks(0.2), diffuseColor(0.2), specularExponent(25) {}
    virtual ~Hittable() = default;
    virtual bool intersect(
            const glm::vec3 &orig,
            const glm::vec3 &dir,
            float &tNearK,
            unsigned &indexK,
            glm::vec2 &uvK,
            hit_record& rec) const = 0;

    virtual void getSurfaceProperties(
        const glm::vec3 &,
        hit_record& rec) const = 0;

    virtual glm::vec3 evalDiffuseColor(const glm::vec2 &) const { return diffuseColor; }
    // material properties
    MaterialType materialType;
    float ior;
    float Kd, Ks;
    glm::vec3 diffuseColor;
    float specularExponent;
    glm::mat4 objectToWorld{}, worldToObject{};
    std::vector<glm::vec3> BBox = {{kInfinity, kInfinity, kInfinity}, {-kInfinity, -kInfinity, -kInfinity}};

    Hittable(const glm::mat4 o2w): objectToWorld(o2w), worldToObject(glm::inverse(o2w)){} 
    
};

typedef std::vector<std::unique_ptr<Hittable>> HittableList;







#endif // HITTABLE_H