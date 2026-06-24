#pragma once
#include <cmath>

template<class T> class NkVec4
{
private:
    union
    {
        struct { T mX, mY, mZ, mW; };
        struct { T mA, mB, mC, mD; };
        T mData[4];
    };

public:
    // ============================================================
    // Constructeurs: par defaut, avec initialisation des variables
    //                , avec tableau
    // ============================================================

    NkVec4() : mX(0), mY(0), mZ(0), mW(0) {}
    NkVec4(T x, T y, T z, T w) : mX(x), mY(y), mZ(z), mW(w) {}
    NkVec4(T data[4]) { mData[0] = data[0]; mData[1] = data[1]; mData[2] = data[2]; mData[3] = data[3]; }

    // ============================================================
    // Operateurs: addition, soustraction, multiplication
    //             par un scalaire
    // ============================================================

    NkVec4 operator +(const NkVec4& v) const { return NkVec4(this->mX + v.mX, this->mY + v.mY, this->mZ + v.mZ, this->mW + v.mW);}
    NkVec4 operator -(const NkVec4& v) const { return NkVec4(this->mX - v.mX, this->mY - v.mY, this->mZ - v.mZ, this->mW - v.mW); }
    NkVec4 operator *(float k)          const { return NkVec4(this->mX * k,    this->mY * k,    this->mZ * k,    this->mW * k);    }

    // Egalite
    bool operator ==(const NkVec4& v) const { return mX == v.mX && mY == v.mY && mZ == v.mZ && mW == v.mW; }

    // Acces par indice
    T& operator [](int i)             { return mData[i]; }
    const T& operator [](int i) const { return mData[i]; }

    // ============================================================
    // Methodes: produit scalaire, module, normalise le vecteur
    //           normal, vecteur normal, magnitude
    // ============================================================

    // Carre de la magnitude
    float LengthSquared() const { return float(mX * mX + mY * mY + mZ * mZ + mW * mW); }

    // Magnitude (norme du vecteur)
    float Length() const { return std::sqrt(LengthSquared()); }

    // Produit scalaire
    float Dot(const NkVec4& v) const { return float(this->mX * v.mX + this->mY * v.mY + this->mZ * v.mZ + this->mW * v.mW); }

    // Produit Vectoriel
    NkVec4 CrossProduct(const NkVec4& v) const
    {
        return NkVec4(
            this->mY * v.mZ - this->mZ * v.mY,
            this->mZ * v.mX - this->mX * v.mZ,
            this->mX * v.mY - this->mY * v.mX
        );
    }

    // Normalise le vecteur (longueur = 1)
    NkVec4 Normalize() const
    {
        float len = Length();
        if (len < 0.0001f) return NkVec4(0, 0, 0);
        return NkVec4(mX / len, mY / len, mZ / len);
    }

    // Projette ce vecteur sur un autre vecteur 'v'
    NkVec4 ProjectOn(const NkVec4& v) const
    {
        float dotProduct = this->Dot(v);
        float magSq = v.LengthSquared();

        if (magSq < 0.0001f) return NkVec4(0, 0, 0);

        float scalar = dotProduct / magSq;
        return NkVec4(v.mX * scalar, v.mY * scalar, v.mZ * scalar);
    }

    // ============================================================
    // Interpolations
    // ============================================================

    // Interpolation Lineaire (Lerp)
    // Trace une ligne droite de 'a' vers 'b'
    // t=0 -> a  |  t=0.5 -> milieu  |  t=1 -> b
    static NkVec4 Lerp(const NkVec4& a, const NkVec4& b, float t)
    {
        return NkVec4(
            a.mX + (b.mX - a.mX) * t,
            a.mY + (b.mY - a.mY) * t,
            a.mZ + (b.mZ - a.mZ) * t
        );
    }

    // Interpolation Cubique de Hermite
    // Memes points de depart et d'arrivee que Lerp, mais avec des tangentes
    // qui controlent la forme de la courbe aux extremites.
    // ta = tangente au point 'a' (direction et vitesse de depart)
    // tb = tangente au point 'b' (direction et vitesse d'arrivee)
    // Polynomes de base :
    //   h00 =  2t3 - 3t2 + 1
    //   h10 =   t3 - 2t2 + t
    //   h01 = -2t3 + 3t2
    //   h11 =   t3 -  t2
    static NkVec4 CubicHermite(const NkVec4& a, const NkVec4& ta,
                                const NkVec4& b, const NkVec4& tb,
                                float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        float h00 =  2.0f*t3 - 3.0f*t2 + 1.0f;
        float h10 =       t3 - 2.0f*t2 + t;
        float h01 = -2.0f*t3 + 3.0f*t2;
        float h11 =       t3 -      t2;

        return NkVec4(
            h00*a.mX + h10*ta.mX + h01*b.mX + h11*tb.mX,
            h00*a.mY + h10*ta.mY + h01*b.mY + h11*tb.mY,
            h00*a.mZ + h10*ta.mZ + h01*b.mZ + h11*tb.mZ
        );
    }

    // Interpolation Cubique de Bezier
    // Courbe definie par 4 points de controle :
    //   p0 = point de depart
    //   p1 = point de controle pres du depart
    //   p2 = point de controle pres de l'arrivee
    //   p3 = point d'arrivee
    // Polynomes de Bernstein :
    //   B0(t) = (1-t)3         B1(t) = 3(1-t)2*t
    //   B2(t) = 3(1-t)*t2      B3(t) = t3
    static NkVec4 CubicBezier(const NkVec4& p0, const NkVec4& p1,
                               const NkVec4& p2, const NkVec4& p3,
                               float t)
    {
        float u  = 1.0f - t;
        float u2 = u * u;
        float u3 = u2 * u;
        float t2 = t * t;
        float t3 = t2 * t;

        float b0 =      u3;
        float b1 = 3.0f*u2*t;
        float b2 = 3.0f*u *t2;
        float b3 =      t3;

        return NkVec4(
            b0*p0.mX + b1*p1.mX + b2*p2.mX + b3*p3.mX,
            b0*p0.mY + b1*p1.mY + b2*p2.mY + b3*p3.mY,
            b0*p0.mZ + b1*p1.mZ + b2*p2.mZ + b3*p3.mZ
        );
    }
};
