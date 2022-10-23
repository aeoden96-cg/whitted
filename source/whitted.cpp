//[header]
// A simple program to demonstrate how to implement Whitted-style ray-tracing
//[/header]
//[compile]
// Download the whitted.cpp file to a folder.
// Open a shell/terminal, and run the following command where the files is saved:
//
// c++ -o whitted whitted.cpp -std=c++11 -O3
//
// Run with: ./whitted. Open the file ./out.png in Photoshop or any program
// reading PPM files.
//[/compile]
//[ignore]
// Copyright (C) 2012  www.scratchapixel.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//[/ignore]


#include "utils.h"
#include "Hittable.h"
#include "Sphere.h"
#include "MeshTriangle.h"
#include "Light.h"
#include "teapotdata.h"

#include "yaml-cpp/yaml.h"
#include "PatchMesh.h"

HittableList loadSceneFromFile(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename + ".yaml");

    HittableList objects;

    std::map<std::string, MaterialType> materials;

    materials["DIFFUSE_AND_GLOSSY"] = DIFFUSE_AND_GLOSSY;
    materials["REFLECTION_AND_REFRACTION"] = REFLECTION_AND_REFRACTION;
    materials["REFLECTION"] = REFLECTION;
    materials["METAL"] = METAL;




    for (auto object : config["objects"]) {
        auto type = object["type"].as<std::string>();
        auto material_name = object["material"].as<std::string>();
        auto material = materials[material_name];

        if(type == "sphere") {
            auto center_v = object["center"].as<std::vector<float>>();
            auto radius = object["radius"].as<float>();
            auto color = object["color"].as<std::vector<float>>();
            Sphere s(glm::vec3(center_v[0], center_v[1], center_v[2]), radius);
            s.materialType = material;
            s.diffuseColor = glm::vec3(color[0], color[1], color[2]);
            objects.push_back(std::make_unique<Sphere>(s));
        } else if(type == "plane") {
            auto up_left_v = object["up_left"].as<std::vector<float>>();
            auto up_right_v = object["up_right"].as<std::vector<float>>();
            auto down_left_v = object["down_left"].as<std::vector<float>>();
            auto down_right_v = object["down_right"].as<std::vector<float>>();
            auto material_name = object["material"].as<std::string>();
            auto material = materials[material_name];

            std::vector<glm::vec3> vertices;
            vertices.push_back(glm::vec3(up_left_v[0], up_left_v[1], up_left_v[2]));
            vertices.push_back(glm::vec3(up_right_v[0], up_right_v[1], up_right_v[2]));
            vertices.push_back(glm::vec3(down_right_v[0], down_right_v[1], down_right_v[2]));
            vertices.push_back(glm::vec3(down_left_v[0], down_left_v[1], down_left_v[2]));

            std::vector<glm::uvec3> vertIndices;
            vertIndices.push_back(glm::uvec3(0, 1, 3));
            vertIndices.push_back(glm::uvec3(1, 2, 3));

            std::vector<glm::uvec2> uvIndices;
            uvIndices.push_back(glm::uvec2(0, 0));
            uvIndices.push_back(glm::uvec2(1, 0));
            uvIndices.push_back(glm::uvec2(1, 1));
            uvIndices.push_back(glm::uvec2(0, 1));



            MeshTriangle *mesh = new MeshTriangle(vertices, vertIndices, uvIndices);
    
            objects.push_back(std::unique_ptr<MeshTriangle>(mesh));
        } else {
            throw std::runtime_error("Unknown object type");
        }
    }

    return objects;
}



// Returns true if the ray intersects an Hittable, false otherwise.
// \param orig is the ray origin
// \param dir is the ray direction
// \param objects is the list of objects the scene contains
// \param[out] tNear contains the distance to the cloesest intersected Hittable.
// \param[out] index stores the index of the intersect triangle if the interesected Hittable is a mesh.
// \param[out] uv stores the u and v barycentric coordinates of the intersected point
// \param[out] *hitObject stores the pointer to the intersected Hittable (used to retrieve material information, etc.)
// \param isShadowRay is it a shadow ray. We can return from the function sooner as soon as we have found a hit.
bool trace(
    const glm::vec3 &orig, const glm::vec3 &dir,
    const std::vector<std::unique_ptr<Hittable>> &objects,
    hit_record &rec)
{
    for (const auto &object : objects) {
        float tNearK = kInfinity;
        uint32_t indexK;
        glm::vec2 uvK;
        if (object->intersect(
                orig,
                dir,
                tNearK,
                indexK,
                uvK,
                rec) && tNearK < rec.t) {
            rec.t = tNearK;
            rec.triIndex = indexK;
            rec.uv = uvK;
            rec.p = orig + dir * rec.t;
            rec.t = tNearK;
            rec.uv = uvK;
            rec.triIndex = indexK;
            rec.object = object.get();
            rec.object->getSurfaceProperties(dir, rec);
        }
    }

    return (rec.object != nullptr);
}


glm::vec3 castRay(
    const glm::vec3 &orig, const glm::vec3 &dir,
    const std::vector<std::unique_ptr<Hittable>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights,
    const Options &options,
    uint32_t depth);


glm::vec3 getReflectionColor (
    hit_record& rec, 
    const glm::vec3& dir, 
    const  Options& options,
    int depth,
    const std::vector<std::unique_ptr<Hittable>>& objects,
    const std::vector<std::unique_ptr<Light>>& lights)
{
    glm::vec3 reflectionDirection = normalize(reflect(dir, rec.normal));
    glm::vec3 reflectionRayOrig = (glm::dot(reflectionDirection, rec.normal) < 0) ?
                    rec.p - rec.normal * options.bias :
                    rec.p + rec.normal * options.bias;

    return castRay(
        reflectionRayOrig, 
        reflectionDirection, 
        objects, 
        lights, 
        options, 
        depth + 1);
}

glm::vec3 randomInUnitSphere()
{
    glm::vec3 p;
    do {
        p = 2.0f * glm::vec3(drand48(), drand48(), drand48()) - glm::vec3(1, 1, 1);
    } while (glm::dot(p, p) >= 1.0);
    return p;
}

glm::vec3 getRefractionColor (
    hit_record& rec, 
    const glm::vec3& dir, 
    const Options& options,
    int depth,
    const std::vector<std::unique_ptr<Hittable>>& objects,
    const std::vector<std::unique_ptr<Light>>& lights)
{
    glm::vec3 refractionDirection = normalize(refract(dir, rec.normal, rec.object->ior));
                
    glm::vec3 refractionRayOrig = (glm::dot(refractionDirection, rec.normal) < 0) ?
        rec.p - rec.normal * options.bias :
        rec.p + rec.normal * options.bias;

    return castRay(
        refractionRayOrig, 
        refractionDirection, 
        objects, 
        lights, 
        options, 
        depth + 1);
    
}

// Implementation of the Whitted-syle light transport algorithm (E [S*] (D|G) L)
//
// This function is the function that compute the color at the intersection point
// of a ray defined by a position and a direction. Note that thus function is recursive (it calls itself).
//
// If the material of the intersected Hittable is either reflective or reflective and refractive,
// then we compute the reflection/refracton direction and cast two new rays into the scene
// by calling the castRay() function recursively. When the surface is transparent, we mix
// the reflection and refraction color using the result of the fresnel equations (it computes
// the amount of reflection and refractin depending on the surface normal, incident view direction
// and surface refractive index).
//
// If the surface is duffuse/glossy we use the Phong illumation model to compute the color
// at the intersection point.
glm::vec3 castRay(
    const glm::vec3 &orig, const glm::vec3 &dir,
    const std::vector<std::unique_ptr<Hittable>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights,
    const Options &options,
    uint32_t depth)
{
    if (depth > options.maxDepth) {
        return options.backgroundColor;
    }

    glm::vec3 hitColor = options.backgroundColor;
    hit_record rec;
    
    if (trace(orig, dir, objects, rec)) {
        
        glm::vec2 uv = rec.uv;
        uint32_t index = rec.triIndex;
        glm::vec3 N = rec.normal;
        glm::vec2 st = rec.st;
        glm::vec3 hitPoint = rec.p;
        float tnear = rec.t;


//        float s = std::max(0.0f, - glm::dot(rec.normal,dir));
//        hitColor = {s,s,s}; //* glm::vec3(hitTexCoordinates.x, hitTexCoordinates.y, 1);
//

        switch (rec.object->materialType) {
            case REFLECTION_AND_REFRACTION:
            {
                float kr = fresnel(dir, N, rec.object->ior);

                glm::vec3 reflectionColor = getReflectionColor(
                    rec, dir, options, depth, objects, lights);

                glm::vec3 refractionColor = getRefractionColor(
                    rec, dir, options, depth, objects, lights);


                hitColor = reflectionColor * kr + refractionColor * (1 - kr);
                break;
            }
            case REFLECTION:
            {
                float kr = fresnel(dir, N, rec.object->ior);

                glm::vec3 reflectionColor = getReflectionColor(
                    rec, dir, options, depth, objects, lights);

                hitColor = reflectionColor * kr;
                break;
            }
            case METAL:
            {
                float roughness = 0.1f;
                float kr = fresnel(dir, N, rec.object->ior);

                glm::vec3 reflectionColor = getReflectionColor(
                    rec,
                    dir + roughness * randomInUnitSphere(),
                    options,
                    depth,
                    objects,
                    lights);

                hitColor = reflectionColor * kr;
                break;
            }
            default:
            {
                // We use the Phong illumation model int the default case. The phong model
                // is composed of a diffuse and a specular reflection component.

                glm::vec3 lightAmt = glm::vec3(0,0,0), specularColor =  glm::vec3(0,0,0);
                glm::vec3 shadowPointOrig = (glm::dot(dir, N) < 0) ?
                    hitPoint + N * options.bias :
                    hitPoint - N * options.bias;

                // Loop over all lights in the scene and sum their contribution up
                // We also apply the lambert cosine law here though we haven't explained yet what this means.
                for (const auto &light : lights) {
                    glm::vec3 lightDir = light->position - hitPoint;
                    lightDir = normalize(lightDir);
                    float LdotN = std::max(0.f, glm::dot(lightDir, N));
                    //Hittable *shadowHitObject = nullptr;
                    hit_record shadowRec;
                    // is the point in shadow, and is the nearest occluding Hittable closer to the Hittable than the light itself?
                    bool inShadow = trace(shadowPointOrig, lightDir, objects, shadowRec) &&
                        shadowRec.t * shadowRec.t < glm::dot(lightDir, lightDir);

                    if(!inShadow) {
                       lightAmt += light->intensity * LdotN;
                    }

                    glm::vec3 reflectionDirection = reflect(-lightDir, N);
                    specularColor += powf(std::max(0.f, -glm::dot(reflectionDirection, dir)), rec.object->specularExponent) * light->intensity;
                }
                hitColor = lightAmt * rec.object->evalDiffuseColor(st) * rec.object->Kd + specularColor * rec.object->Ks;
                break;
            }
        }
    }

    return hitColor;
}

// [comment]
// The main render function. This where we iterate over all pixels in the image, generate
// primary rays and cast these rays into the scene. The content of the framebuffer is
// saved to a file.
// [/comment]
void render(
        Options &options,
    const std::vector<std::unique_ptr<Hittable>> &objects,
    const std::vector<std::unique_ptr<Light>> &lights)
{
    glm::vec3 orig(0);
    multVecMatrix(glm::vec3(0), orig,options.cameraToWorld);
    glm::vec3 *framebuffer = new glm::vec3[options.width * options.height];
    glm::vec3 *pix = framebuffer;
    float scale = tan(glm::radians(options.fov * 0.5));
    float imageAspectRatio = options.width / (float)options.height;
    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)options.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)options.height) * scale;
//            glm::vec3 dir = normalize(glm::vec3(x, y, -1));
            glm::vec3 dir;
            multDirMatrix(glm::vec3(x, y, -1), dir,options.cameraToWorld);
            dir = glm::normalize(dir);
            *(pix++) = castRay(orig, dir, objects, lights, options, 0);
        }
        std::cout << "\rRendering (" << 1 << " spp) " << 100.f * j / (options.height - 1) << "%" << std::flush;
    }

    // save framebuffer to file
    std::ofstream ofs;
    ofs.open("./out.ppm");
    ofs << "P6\n" << options.width << " " << options.height << "\n255\n";
    for (uint32_t i = 0; i < options.height * options.width; ++i) {
        char r = (char)(255 * clamp_(0, 1, framebuffer[i].x));
        char g = (char)(255 * clamp_(0, 1, framebuffer[i].y));
        char b = (char)(255 * clamp_(0, 1, framebuffer[i].z));
        ofs << r << g << b;
    }

    ofs.close();

    delete [] framebuffer;
}

// In the main function of the program, we create the scene (create objects and lights)
// as well as set the options for the render (image widht and height, maximum recursion
// depth, field-of-view, etc.). We then call the render function().

glm::vec3 evalBezierCurve(const std::vector<glm::vec3> & P, const float &t) 
{ 
    float b0 = (1 - t) * (1 - t) * (1 - t); 
    float b1 = 3 * t * (1 - t) * (1 - t); 
    float b2 = 3 * t * t * (1 - t); 
    float b3 = t * t * t; 
 
    return P[0] * b0 + P[1] * b1 + P[2] * b2 + P[3] * b3; 
} 
 
glm::vec3 evalBezierPatch(const std::vector<glm::vec3> controlPoints, const float &u, const float &v) 
{ 
    std::vector<glm::vec3> uCurve; 
    for (int i = 0; i < 4; ++i){
        uCurve[i] = evalBezierCurve({controlPoints[i * 4 + 0], controlPoints[i * 4 + 1], controlPoints[i * 4 + 2], controlPoints[i * 4 + 3]}, u);
    }
        
    return evalBezierCurve(uCurve, v); 
} 
 
glm::vec3 derivBezier(const std::vector<glm::vec3>& P, const float &t) 
{ 
    return -3 * (1 - t) * (1 - t) * P[0] + 
        (3 * (1 - t) * (1 - t) - 6 * t * (1 - t)) * P[1] + 
        (6 * t * (1 - t) - 3 * t * t) * P[2] + 
        3 * t * t * P[3]; 
} 

glm::vec3 dUBezier(const std::vector<glm::vec3>& controlPoints, const float &u, const float &v) 
{ 
    std::vector<glm::vec3> P; 
    std::vector<glm::vec3> vCurve; 
    for (int i = 0; i < 4; ++i) { 
        P[0] = controlPoints[i]; 
        P[1] = controlPoints[4 + i]; 
        P[2] = controlPoints[8 + i]; 
        P[3] = controlPoints[12 + i]; 
        vCurve[i] = evalBezierCurve(P, v); 
    } 
 
    return derivBezier(vCurve, u); 
} 

glm::vec3 dVBezier(const  std::vector<glm::vec3>& controlPoints, const float &u, const float &v) 
{ 
    std::vector<glm::vec3> uCurve; 
    for (int i = 0; i < 4; ++i) { 
        uCurve[i] = evalBezierCurve({controlPoints[i * 4 + 0], controlPoints[i * 4 + 1], controlPoints[i * 4 + 2], controlPoints[i * 4 + 3]}, u);
    } 
 
    return derivBezier(uCurve, v); 
}

void createPolyTeapot(const glm::mat4& o2w, std::vector<std::unique_ptr<Hittable>> &objects)
{
    uint32_t divs = 8;
    std::unique_ptr<glm::vec3 []> P(new glm::vec3[(divs + 1) * (divs + 1)]);
    std::unique_ptr<uint32_t []> nvertices(new uint32_t[divs * divs]);
    std::unique_ptr<uint32_t []> vertices(new uint32_t[divs * divs * 4]);
    std::unique_ptr<glm::vec3 []> N(new glm::vec3[(divs + 1) * (divs + 1)]);
    std::unique_ptr<glm::vec2 []> st(new glm::vec2[(divs + 1) * (divs + 1)]);

    // face connectivity - all patches are subdivided the same way so there
    // share the same topology and uvs
    for (uint16_t j = 0, k = 0; j < divs; ++j) {
        for (uint16_t i = 0; i < divs; ++i, ++k) {
            nvertices[k] = 4;
            vertices[k * 4    ] = (divs + 1) * j + i;
            vertices[k * 4 + 1] = (divs + 1) * j + i + 1;
            vertices[k * 4 + 2] = (divs + 1) * (j + 1) + i + 1;
            vertices[k * 4 + 3] = (divs + 1) * (j + 1) + i;
        }
    }

    glm::vec3 controlPoints[16];
    for (int np = 0; np < kTeapotNumPatches; ++np) { // kTeapotNumPatches
        // set the control points for the current patch
        for (uint32_t i = 0; i < 16; ++i)
            controlPoints[i][0] = teapotVertices[teapotPatches[np][i] - 1][0],
            controlPoints[i][1] = teapotVertices[teapotPatches[np][i] - 1][1],
            controlPoints[i][2] = teapotVertices[teapotPatches[np][i] - 1][2];

        // generate grid
        for (uint16_t j = 0, k = 0; j <= divs; ++j) {
            float v = j / (float)divs;
            for (uint16_t i = 0; i <= divs; ++i, ++k) {
                float u = i / (float)divs;
                P[k] = evalBezierPatch(controlPoints, u, v);
                glm::vec3 dU = dUBezier(controlPoints, u, v);
                glm::vec3 dV = dVBezier(controlPoints, u, v);
                N[k] = glm::normalize(glm::cross(dU,dV));
                st[k].x = u;
                st[k].y = v;
            }
        }

        objects.push_back(std::unique_ptr<PatchMesh>(new PatchMesh(o2w, divs * divs, nvertices, vertices, P, N, st)));
    }
}

int main(int argc, char **argv)
{
    YAML::Node config = YAML::LoadFile("config.yaml");
    
    // creating the scene (adding objects and lights)
    std::vector<std::unique_ptr<Hittable>> objects;
    std::vector<std::unique_ptr<Light>> lights;

    auto fileName = config["scene"].as<std::string>();

    YAML::Node input = YAML::LoadFile(fileName + ".yaml");

    objects = loadSceneFromFile(fileName);


    //o2w is the object to world transformation
    //it is the transformation that transforms the object from its local space to the world space
    //in this case, the object is the teapot, and the world is the scene
    //the teapot is centered at the origin, so we need to translate it to the position we want
    //to translate it to the position (0, 0, 0), we need to translate it by (-0.5, -0.5, -0.5)
    glm::mat4 o2w = glm::translate(glm::mat4(1.0f), glm::vec3(-1 ,-3, -5));
    //to rotate it, we need to rotate it by 90 degrees around the x axis
    o2w = glm::rotate(o2w, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    //to scale it, we need to scale it by 0.5
    o2w = glm::scale(o2w, glm::vec3(0.7, 0.7, 0.7));


    createPolyTeapot(o2w, objects);

    std::cout << "Created poly teapot" << std::endl;


    lights.push_back(std::unique_ptr<Light>(new Light(glm::vec3(0,4,-11), glm::vec3(1,1,1))));
    lights.push_back(std::unique_ptr<Light>(new Light(glm::vec3(0,0,5), glm::vec3(1,1,1))));

    // setting up options
    Options options;
    options.width = 80;
    options.height = 80;
    options.fov = 80;
    options.backgroundColor = glm::vec3(0.235294, 0.67451, 0.843137);
    options.maxDepth = 4;
    options.bias = 0.00001;
    //cameraToWorld is the inverse of the camera matrix
    //it is the matrix that transforms from world space to camera space
    //we create it by rotating around the y axis by 180 degrees and then translating by (0,0,5)
    options.cameraToWorld = glm::mat4(1.0f);
    //options.cameraToWorld = glm::translate(options.cameraToWorld, glm::vec3(0,1,5));
    //options.cameraToWorld = glm::rotate(options.cameraToWorld, glm::radians(50.0f), glm::vec3(0,1,0));

    // finally, render
    render(options, objects, lights);

    

    return 0;
}