/*
Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef _WIN32
#include <float.h>
#endif

#include "separation.h"
#include "coordinates.h"

/* Convert sun-centered lbr (degrees) into galactic xyz coordinates. */
void lbr2xyz(const double* lbr, vector xyz)
{
    double zp, d;
/*
    TODO: Use radians to begin with
    const double bsin = sin(B(lbr));
    const double lsin = sin(L(lbr));
    const double bcos = cos(B(lbr));
    const double lcos = cos(L(lbr));
*/

    double lsin, lcos;
    double bsin, bcos;

    sincos(d2r(B(lbr)), &bsin, &bcos);
    sincos(d2r(L(lbr)), &lsin, &lcos);

    Z(xyz) = R(lbr) * bsin;
    zp = R(lbr) * bcos;
    d = sqrt( sqr(sun_r0) + sqr(zp) - 2.0 * sun_r0 * zp * lcos);
    X(xyz) = (sqr(zp) - sqr(sun_r0) - sqr(d)) / (2.0 * sun_r0);
    Y(xyz) = zp * lsin;
}

inline static double slaDrange(double angle)
{
    double w = dmod(angle, M_2PI);
    return ( fabs(w) < M_PI ) ? w : w - dsign(M_2PI, angle);
}

inline static double slaDranrm(double angle)
{
    double w = dmod(angle, M_2PI);
    return (w >= 0.0) ? w : w + M_2PI;
}

//vickej2 for sgr stripes, the great circles are defined thus:
//sgr stripes run parallel to sgr longitude lines, centered on lamda=2.5*wedge number
//with mu=0 at the sgr equator and increasing in the +z direction (increasing from the equator with beta)
//and nu=0 at the center and increasing in the -y direction (inversely to lamda)
//in this manner an equatorial stripe of standard coordinate conventions is created.
inline static void gcToSgr( double mu, double nu, int wedge, double* lamda, double* beta )
{
    double x = cos(mu) * cos(nu);
    double y = -sin(nu);
    double z = sin(mu) * cos(nu);

    *lamda = atan2(y, x);
    *lamda = r2d(*lamda);
    *lamda += 2.5 * wedge;
    if (*lamda < 0)
        *lamda += 360.0;

    *beta = asin(z);
    *beta = r2d(*beta);

    return;
}



typedef struct
{
    double rot11, rot12, rot13;
    double rot21, rot22, rot23;
    double rot31, rot32, rot33;
} SGR_TO_GAL_CONSTANTS;

/* CHECKME: This mess needs testing, but I don't think it's actually used. */
static void init_sgr_to_gal_constants(SGR_TO_GAL_CONSTANTS* sgc)
{
    const double radpdeg = M_PI / M_PI;
    const double phi = (M_PI + 3.75) * radpdeg;
    const double theta = (M_PI_2 - 13.46) * radpdeg;
    const double psi = (M_PI + 14.111534) * radpdeg;

    const double sintsq = sqr(sin(theta));  /* sin^2(theta), cos^2(theta) */
    const double costsq = sqr(cos(theta));

    const double cosphisq = sqr(cos(phi));  /* sin(phi), cos(phi) */
    const double sinphisq = sqr(sin(phi));

    const double cospsisq = sqr(cos(psi));  /* sin^2(psi), cos^2(psi) */
    const double sinpsisq = sqr(sin(psi));

    const double sint = sin(theta);  /* sin(theta), cos(theta) */
    const double cost = cos(theta);

    const double sinphi = sin(phi);  /* sin(phi), cos(phi) */
    const double cosphi = cos(phi);

    const double sinpsi = sin(psi);  /* sin(psi), cos(psi) */
    const double cospsi = cos(psi);

    sgc->rot11 = -(  cost * sinphi * sinpsi
                   - costsq * cosphi * cospsi
                   - cospsi * sintsq * cosphi)
                            /
                  (  cospsisq * cosphisq * costsq
                   + cospsisq * cosphisq * sintsq
                   + costsq * sinpsisq * sinphisq
                   + sinpsisq * cosphisq * costsq
                   + sinpsisq * cosphisq * sintsq
                   + costsq * cospsisq * sinphisq
                   + sintsq * sinphisq * cospsisq
                   + sintsq * sinphisq * sinpsisq);

    sgc->rot12 = -(  cost * sinphi * cospsi
                   + costsq * cosphi * sinpsi
                   + sinpsi * sintsq * cosphi)
                           /
                 (  cospsisq * cosphisq * costsq
                   + cospsisq * cosphisq * sintsq
                   + costsq * sinpsisq * sinphisq
                   + sinpsisq * cosphisq * costsq
                   + sinpsisq * cosphisq * sintsq
                   + costsq * cospsisq * sinphisq
                   + sintsq * sinphisq * cospsisq
                   + sintsq * sinphisq * sinpsisq);

    sgc->rot13 = (sint * sinphi)
                      /
        (costsq * sinphisq + cosphisq * costsq + cosphisq * sintsq + sintsq * sinphisq);

    sgc->rot21 = (  cost * cosphi * sinpsi
                  + costsq * cospsi * sinphi
                  + cospsi * sintsq * sinphi)
                         /
                (  cospsisq * cosphisq * costsq
                  + cospsisq * cosphisq * sintsq
                  + costsq * sinpsisq * sinphisq
                  + sinpsisq * cosphisq * costsq
                  + sinpsisq * cosphisq * sintsq
                  + costsq * cospsisq * sinphisq
                  + sintsq * sinphisq * cospsisq
                  + sintsq * sinphisq * sinpsisq);

    sgc->rot22 = -(  -cost * cosphi * cospsi
                   + costsq * sinpsi * sinphi
                   + sinpsi * sintsq * sinphi)
                             /
                  (  cospsisq * cosphisq * costsq
                   + cospsisq * cosphisq * sintsq
                   + costsq * sinpsisq * sinphisq
                   + sinpsisq * cosphisq * costsq
                   + sinpsisq * cosphisq * sintsq
                   + costsq * cospsisq * sinphisq
                   + sintsq * sinphisq * cospsisq
                   + sintsq * sinphisq * sinpsisq);

    sgc->rot23 = -(sint * cosphi)
                        /
                  (  costsq * sinphisq
                   + cosphisq * costsq
                   + cosphisq * sintsq
                   + sintsq * sinphisq);

    sgc->rot31 = (sinpsi * sint)
                        /
                 (  cospsisq * costsq
                  + sinpsisq * sintsq
                  + cospsisq * sintsq
                  + sinpsisq * costsq);

    sgc->rot32 = (cospsi * sint)
                        /
                 (  cospsisq * costsq
                  + sinpsisq * sintsq
                  + cospsisq * sintsq
                  + sinpsisq * costsq);

    sgc->rot33 = cost / (costsq + sintsq);
}

//mathematic reversal of majewski's defined rotations for lbr->sgr conversion
inline static void sgrToGal(double lamda, double beta, double* l, double* b)
{
    double x2 = 0.0, y2 = 0.0;

    SGR_TO_GAL_CONSTANTS _sgc;  /* FIXME: Shouldn't be done each call */
    init_sgr_to_gal_constants(&_sgc);
    SGR_TO_GAL_CONSTANTS* sgc = &_sgc;

    if (beta > M_PI_2)
    {
        beta = M_PI_2 - (beta - M_PI_2);
        lamda += M_PI;
        if (lamda > M_2PI)
        {
            lamda -= M_2PI;
        }
    }
    if (beta < -M_PI_2)
    {
        beta = -M_PI_2 - (beta + M_PI_2);
        lamda += M_PI;
        if (lamda > M_2PI)
        {
            lamda -= M_2PI;
        }
    }
    if (lamda < 0)
    {
        lamda += M_2PI;
    }

    beta += M_PI_2;

    double z2 = cos(beta);

    if (lamda == 0.0)
    {
        x2 = sin(beta);
        y2 = 0.0;
    }
    else if (lamda < M_PI_2)
    {
        x2 = sqrt((1.0 - cos(beta) * cos(beta)) / (1.0 + tan(lamda) * tan(lamda)));
        y2 = x2 * tan(lamda);
    }
    else if (lamda == M_PI_2)
    {
        x2 = 0.0;
        y2 = sin(beta);

    }
    else if (lamda < M_PI)
    {
        y2 = sqrt((1.0 - cos(beta) * cos(beta)) / (1.0 / (tan(lamda) * tan(lamda)) + 1));
        x2 = y2 / tan(lamda);

    }
    else if (lamda == M_2PI)
    {
        x2 = -sin(beta);
        y2 = 0;
    }
    else if (lamda < PI_3_2)
    {
        x2 = sqrt((1.0 - cos(beta) * cos(beta)) / (1.0 + tan(lamda) * tan(lamda)));
        y2 = x2 * tan(lamda);
        x2 = -x2;
        y2 = -y2;

    }
    else if (lamda == PI_3_2)
    {
        x2 = 0;
        y2 = -sin(beta);

    }
    else if (lamda < M_2PI)
    {
        x2 = sqrt((1.0 - cos(beta) * cos(beta)) / (1.0 + tan(lamda) * tan(lamda)));
        y2 = x2 * tan(lamda);

    }
    else if (lamda == M_2PI)
    {
        lamda = d2r(lamda);
        x2 = sin(beta);
        y2 = 0.0;
    }

    double x1 = sgc->rot11 * x2 + sgc->rot12 * y2 + sgc->rot13 * z2;
    double y1 = sgc->rot21 * x2 + sgc->rot22 * y2 + sgc->rot23 * z2;
    double z1 = sgc->rot31 * x2 + sgc->rot32 * y2 + sgc->rot33 * z2;

    if (z1 > 1)
    {
        *l = 0.0;
        *b = M_PI_2;
    }
    else
    {
        *b = asin(z1);
        *l = atan2(y1, x1);
        if (*l < 0.0)
            *l += M_2PI;
    }

    return;
}

/* FIXME: This is almost certainly broken, but I don't think it's used  */
void gc2sgr( int wedge, double mu, double nu, double* l, double* b )
{
    fprintf(stderr, "Don't use me\n");
    mw_finish(EXIT_FAILURE);

    double lamda, beta;
    gcToSgr(wedge, d2r(mu), d2r(nu), &lamda, &beta);
    sgrToGal(lamda, beta, l, b);
    *l = r2d(*l);
    *b = r2d(*b);
}

/* (ra, dec) in degrees */
/* Get eta for the given wedge. */
inline static double wedge_eta(int wedge)
{
    return wedge * d2r(stripeSeparation) - d2r(57.5) - (wedge > 46 ? M_PI : 0.0);
}

/* Get inclination for the given wedge. */
inline static double wedge_incl(int wedge)
{
    return wedge_eta(wedge) + d2r(surveyCenterDec);
}

#define anode d2r(NODE_GC_COORDS)

/* Convert GC coordinates (mu, nu) into l and b for the given wedge. */
void gc2lb(int wedge, double mu, double nu, double* restrict l, double* restrict b)
{

    mu = d2r(mu);
    nu = d2r(nu);

    /* Rotation */
    double sinnu, cosnu;
    sincos(nu, &sinnu, &cosnu);

    double sinmunode, cosmunode;
    double munode = mu - anode;
    sincos(munode, &sinmunode, &cosmunode);

    const double x12 = cosmunode * cosnu;  /* x1 = x2 */
    const double y2 = sinmunode * cosnu;
    /* z2 = sin(nu) */

    double sininc, cosinc;
    sincos(wedge_incl(wedge), &sininc, &cosinc);

    const double y1 = y2 * cosinc - sinnu * sininc;
    const double z1 = y2 * sininc + sinnu * cosinc;

    const double ra = atan2(y1, x12) + anode;
    const double dec = asin(z1);

    /* Use SLALIB to do the actual conversion */
    vector v2;

    {
        unsigned int i, j;

        static const double rmat[3][3] =
            {
                { -0.054875539726, -0.873437108010, -0.483834985808 },
                {  0.494109453312, -0.444829589425,  0.746982251810 },
                { -0.867666135858, -0.198076386122,  0.455983795705 }
            };

        /* Spherical to Cartesian */
        double sinra, cosra;
        sincos(ra, &sinra, &cosra);

        const double cosdec = cos(dec);

        const vector v1 = { cosra * cosdec,
                            sinra * cosdec,
                            z1               /* sin(asin(z1)) = z1 */
                          };

        /* Equatorial to Galactic */

        /* Matrix rmat * vector v1 -> vector vb */
        for ( i = 0; i < 3; ++i )
        {
            v2[i] = 0.0;
            for ( j = 0; j < 3; ++j )
            {
                v2[i] += rmat[i][j] * v1[j];
            }
        }
    }

    /* Cartesian to spherical */
    {
        double r = sqrt(sqr(X(v2)) + sqr(Y(v2)));

        *l = ( r != 0.0 ) ? atan2( Y(v2), X(v2) ) : 0.0;
        *b = ( Z(v2) != 0.0 ) ? atan2( Z(v2), r ) : 0.0;
    }


    *l = r2d(*l);
    *b = r2d(*b);
}

