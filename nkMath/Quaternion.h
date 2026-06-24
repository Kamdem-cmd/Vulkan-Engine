#pragma once
#include <cmath>
#include "NKVec3.h"

class Quaternion
{
private:
    union
    {
        float mX, mY, mZ, mW; 
        float mData[4];
    };

public:
    // ============================================================
    // Constructeurs: par defaut, avec initialisation des variables
    //                , avec tableau
    // ============================================================

    Quaternion() : mX(0), mY(0), mZ(0), mW(0) {}
    Quaternion(float x, float y, float z, float w) : mX(x), mY(y), mZ(z), mW(w) {}
    Quaternion(float data[4]) { mData[0] = data[0]; mData[1] = data[1]; mData[2] = data[2]; mData[3] = data[3]; }

};