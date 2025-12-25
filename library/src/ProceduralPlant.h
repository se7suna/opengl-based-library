#ifndef PROCEDURAL_PLANT_H
#define PROCEDURAL_PLANT_H

#include <memory>
#include "Mesh.h"
#include "Texture.h"

struct PottedPlant {
    std::shared_ptr<Mesh> pot;
    std::shared_ptr<Mesh> soil;
    std::shared_ptr<Mesh> leaves;

    PBRTextureMaterial potMat;
    PBRTextureMaterial soilMat;
    PBRTextureMaterial leavesMat;
};

PottedPlant CreatePottedPlant(unsigned int seed);

#endif
