#pragma once
#include <cmath>

template<class T> class NkVec2
{
private:
    union
    {
        struct { T mX, mY; };
        struct { T mA, mB; };
        T mData[2];
    };

public:
    // ============================================================
    // Constructeurs: par defaut, avec initialisation des variables
    //                , initialisation avec tableau
    // ============================================================

    NkVec2() : mX(0), mY(0) {}
    NkVec2(T x, T y) : mX(x), mY(y) {}
    NkVec2(T data[2]) { mData[0] = data[0]; mData[1] = data[1]; }

    // ============================================================
    // Operateurs: addition, soustraction, multiplication
    //             par un scalaire
    // ============================================================

    NkVec2 operator +(const NkVec2& v) const { return NkVec2(this->mX + v.mX, this->mY + v.mY); }
    NkVec2 operator -(const NkVec2& v) const { return NkVec2(this->mX - v.mX, this->mY - v.mY); }
    NkVec2 operator *(float k)         const { return NkVec2(this->mX * k,    this->mY * k);    }

    // Egalite
    bool operator ==(const NkVec2& v) const { return mX == v.mX && mY == v.mY; }

    // Acces par indice
    T& operator [](int i)             { return mData[i]; }
    const T& operator [](int i) const { return mData[i]; }

    // ============================================================
    // Methodes: produit scalaire, module, normalise le vecteur
    //           normal, magnitude
    // ============================================================

    // Carre de la magnitude (evite une racine carree inutile)
    float LengthSquared() const { return float(mX * mX + mY * mY); }

    // Magnitude (norme du vecteur)
    float Length() const { return std::sqrt(LengthSquared()); }

    // Produit scalaire
    float Dot(const NkVec2& v) const { return float(this->mX * v.mX + this->mY * v.mY); }

    // Normalise le vecteur (longueur = 1), retourne zero si norme trop petite
    NkVec2 Normalize() const
    {
        float len = Length();
        if (len < 0.0001f) return NkVec2(0, 0);
        return NkVec2(mX / len, mY / len);
    }

    // Projette ce vecteur sur un autre vecteur 'v'
    NkVec2 ProjectOn(const NkVec2& v) const
    {
        float dotProduct = this->Dot(v);
        float magSq = v.LengthSquared();

        if (magSq < 0.0001f) return NkVec2(0, 0); // Eviter division par zero

        float scalar = dotProduct / magSq;
        return NkVec2(v.mX * scalar, v.mY * scalar);
    }

    // ============================================================
    // Interpolations
    // ============================================================

    // Interpolation Lineaire (Lerp)
    // Trace une ligne droite de 'a' vers 'b'
    // t=0 -> a  |  t=0.5 -> milieu  |  t=1 -> b
    static NkVec2 Lerp(const NkVec2& a, const NkVec2& b, float t)
    {
        return NkVec2(
            a.mX + (b.mX - a.mX) * t,
            a.mY + (b.mY - a.mY) * t
        );
    }

    // Interpolation Cubique de Hermite (SmoothStep / Hermite)
    // Memes points de depart et d'arrivee que Lerp, mais avec des tangentes
    // qui controlent la forme de la courbe aux extremites.
    // ta = tangente au point 'a' (direction/vitesse de depart)
    // tb = tangente au point 'b' (direction/vitesse d'arrivee)
    // Formule : p(t) = h00*a + h10*ta + h01*b + h11*tb
    //   h00 =  2t³ - 3t² + 1   (base pour 'a')
    //   h10 =   t³ - 2t² + t   (base pour la tangente 'ta')
    //   h01 = -2t³ + 3t²       (base pour 'b')
    //   h11 =   t³ -  t²       (base pour la tangente 'tb')
    static NkVec2 CubicHermite(const NkVec2& a, const NkVec2& ta,
                                const NkVec2& b, const NkVec2& tb,
                                float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        float h00 =  2.0f*t3 - 3.0f*t2 + 1.0f;
        float h10 =       t3 - 2.0f*t2 + t;
        float h01 = -2.0f*t3 + 3.0f*t2;
        float h11 =       t3 -      t2;

        return NkVec2(
            h00*a.mX + h10*ta.mX + h01*b.mX + h11*tb.mX,
            h00*a.mY + h10*ta.mY + h01*b.mY + h11*tb.mY
        );
    }

    // Interpolation Cubique de Bezier
    // Courbe de Bezier cubique definie par 4 points de controle :
    //   p0 = point de depart
    //   p1 = point de controle pres du depart (tire la courbe)
    //   p2 = point de controle pres de l'arrivee (tire la courbe)
    //   p3 = point d'arrivee
    // Formule de Bernstein :
    //   p(t) = (1-t)³*p0 + 3(1-t)²t*p1 + 3(1-t)t²*p2 + t³*p3
    static NkVec2 CubicBezier(const NkVec2& p0, const NkVec2& p1,
                               const NkVec2& p2, const NkVec2& p3,
                               float t)
    {
        float u  = 1.0f - t;
        float u2 = u * u;
        float u3 = u2 * u;
        float t2 = t * t;
        float t3 = t2 * t;

        float b0 =      u3;          // (1-t)³
        float b1 = 3.0f*u2*t;        // 3(1-t)²t
        float b2 = 3.0f*u *t2;       // 3(1-t)t²
        float b3 =      t3;          // t³

        return NkVec2(
            b0*p0.mX + b1*p1.mX + b2*p2.mX + b3*p3.mX,
            b0*p0.mY + b1*p1.mY + b2*p2.mY + b3*p3.mY
        );
    }
};
