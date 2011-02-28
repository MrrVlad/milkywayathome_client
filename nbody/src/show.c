/* Copyright 2010 Matthew Arsenault, Travis Desell, Dave Przybylo,
Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
Magdon-Ismail and Rensselaer Polytechnic Institute.

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

#include <string.h>
#include "nbody_util.h"
#include "nbody_types.h"
#include "milkyway_util.h"
#include "show.h"

/* A bunch of boilerplate for debug printing */

const char* showBool(const mwbool x)
{
    switch (x)
    {
        case FALSE:
            return "false";
        case TRUE:
            return "true";
        default:
            return "invalid boolean (but true)";
    }
}

const char* showCriterionT(const criterion_t x)
{
    switch (x)
    {
        case Exact:
            return "Exact";
        case NewCriterion:
            return "NewCriterion";
        case BH86:
            return "BH86";
        case SW93:
            return "SW93";
        case InvalidCriterion:
            return "InvalidCriterion";
        default:
            return "Bad criterion_t";
    }
}

const char* showSphericalT(const spherical_t x)
{
    switch (x)
    {
        case SphericalPotential:
            return "SphericalPotential";
        case InvalidSpherical:
            return "InvalidSpherical";
        default:
            return "Bad spherical_t";
    }
}

const char* showDiskT(const disk_t x)
{
    switch (x)
    {
        case MiyamotoNagaiDisk:
            return "MiyamotoNagaiDisk";
        case ExponentialDisk:
            return "ExponentialDisk";
        case InvalidDisk:
            return "InvalidDisk";
        default:
            return "Bad disk_t";
    }
}

const char* showHaloT(const halo_t x)
{
    switch (x)
    {
        case LogarithmicHalo:
            return "LogarithmicHalo";
        case NFWHalo:
            return "NFWHalo";
        case TriaxialHalo:
            return "TriaxialHalo";
        case InvalidHalo:
            return "InvalidHalo";
        default:
            return "Bad halo_t";
    }
}

const char* showDwarfModelT(const dwarf_model_t x)
{
    switch (x)
    {
        case DwarfModelOther:
            return "DwarfModelOther";
        case DwarfModelPlummer:
            return "DwarfModelPlummer";
        case DwarfModelKing:
            return "DwarfModelKing";
        case DwarfModelDehnen:
            return "DwarfModelDehnen";
        case InvalidDwarfModel:
            return "InvalidDwarfModel";
        default:
            return "Bad dwarf_model_t";
    }
}

char* showSpherical(const Spherical* s)
{
    char* buf;

    if (!s)
        return NULL;

    if (0 > asprintf(&buf,
                     "{\n"
                     "      type  = %s\n"
                     "      mass  = %g\n"
                     "      scale = %g\n"
                     "    };\n",
                     showSphericalT(s->type),
                     s->mass,
                     s->scale))
    {
        fail("asprintf() failed\n");
    }

    return buf;
}

char* showHalo(const Halo* h)
{
    char* buf;

    if (!h)
        return NULL;

    if (0 > asprintf(&buf,
                     "{ \n"
                     "      type         = %s\n"
                     "      vhalo        = %g\n"
                     "      scale_length = %g\n"
                     "      flattenX     = %g\n"
                     "      flattenY     = %g\n"
                     "      flattenZ     = %g\n"
                     "      c1           = %g\n"
                     "      c2           = %g\n"
                     "      c3           = %g\n"
                     "      triaxAngle   = %g\n"
                     "    };\n",
                     showHaloT(h->type),
                     h->vhalo,
                     h->scale_length,
                     h->flattenX,
                     h->flattenY,
                     h->flattenZ,
                     h->c1,
                     h->c2,
                     h->c3,
                     h->triaxAngle))
    {
        fail("asprintf() failed\n");
    }

    return buf;
}

char* showDisk(const Disk* d)
{
    char* buf;

    if (!d)
        return NULL;

    if (0 > asprintf(&buf,
                     "{ \n"
                     "      type         = %s\n"
                     "      mass         = %g\n"
                     "      scale_length = %g\n"
                     "      scale_height = %g\n"
                     "    };\n",
                     showDiskT(d->type),
                     d->mass,
                     d->scale_length,
                     d->scale_height))
    {
        fail("asprintf() failed\n");
    }

    return buf;
}

char* showBody(const bodyptr p)
{
    char* buf;
    char* vel;
    char* pos;

    if (!p)
        return NULL;

    vel = showVector(Vel(p));
    pos = showVector(Pos(p));

    if (0 > asprintf(&buf,
                     "body { \n"
                     "      mass   = %g\n"
                     "      pos    = %s\n"
                     "      vel    = %s\n"
                     "      ignore = %s\n"
                     "    };\n",
                     Mass(p),
                     pos,
                     vel,
                     showBool(ignoreBody(p))))

    {
        fail("asprintf() failed\n");
    }

    free(vel);
    free(pos);

    return buf;
}

/* For debugging. Need to make this go away for release since it uses
 * GNU extensions */
char* showPotential(const Potential* p)
{
    int rc;
    char* buf;
    char* sphBuf;
    char* diskBuf;
    char* haloBuf;

    if (!p)
        return NULL;

    sphBuf  = showSpherical(&p->sphere[0]);
    diskBuf = showDisk(&p->disk);
    haloBuf = showHalo(&p->halo);

    rc = asprintf(&buf,
                  "{\n"
                  "    sphere = %s\n"
                  "    disk = %s\n"
                  "    halo = %s\n"
                  "    rings  = { unused pointer %p }\n"
                  "  };\n",
                  sphBuf,
                  diskBuf,
                  haloBuf,
                  p->rings);

    if (rc < 0)
        fail("asprintf() failed\n");

    free(sphBuf);
    free(diskBuf);
    free(haloBuf);

    return buf;
}

char* showDwarfModel(const DwarfModel* d)
{
    char* buf;
    char* icBuf;

    if (!d)
        return NULL;

    icBuf = showInitialConditions(&d->initialConditions);

    if (0 > asprintf(&buf,
                     "  { \n"
                     "      type              = %s\n"
                     "      nbody             = %d\n"
                     "      mass              = %g\n"
                     "      ignoreFinal       = %s\n"
                     "      initialConditions = %s\n"
                     "    };\n",
                     showDwarfModelT(d->type),
                     d->nbody,
                     d->mass,
                     showBool(d->ignoreFinal),
                     icBuf))
    {
        fail("asprintf() failed\n");
    }

    free(icBuf);

    return buf;
}

char* showInitialConditions(const InitialConditions* ic)
{
    char* buf;

    if (!ic)
        return NULL;

    if (0 > asprintf(&buf,
                     "{ \n"
                     "          position     = { %g, %g, %g }\n"
                     "          velocity     = { %g, %g, %g }\n"
                     "        };\n",
                     X(ic->position),
                     Y(ic->position),
                     Z(ic->position),
                     X(ic->velocity),
                     Y(ic->velocity),
                     Z(ic->velocity)))
    {
        fail("asprintf() failed\n");
    }

    return buf;
}

/* Most efficient function ever */
char* showNBodyCtx(const NBodyCtx* ctx)
{
    char* buf;
    char* potBuf;
    char* modelBuf;

    size_t totalLen = 0;
    char** allModels;
    unsigned int i;


    if (!ctx)
        return NULL;

    potBuf = showPotential(&ctx->pot);

    allModels = mwMalloc(sizeof(char*) * ctx->modelNum);

    for (i = 0; i < ctx->modelNum; ++i)
    {
        allModels[i] = showDwarfModel(&ctx->models[i]);
        totalLen += strlen(allModels[i]);
    }

    modelBuf = (char*) mwCalloc(totalLen + 1, sizeof(char));
    for (i = 0; i < ctx->modelNum; ++i)
    {
        strcat(modelBuf, allModels[i]);
        free(allModels[i]);
    }
    free(allModels);

    if (0 > asprintf(&buf,
                     "ctx = { \n"
                     "  pot = %s\n"
                     "  time_evolve     = %g\n"
                     "  time_orbit      = %g\n"
                     "  timestep        = %g\n"
                     "  orbit_timestep  = %g\n"
                     "  headline        = %s\n"
                     "  outfilename     = %s\n"
                     "  histogram       = %s\n"
                     "  histout         = %s\n"
                     "  outfile         = %p\n"
                     "  sunGCDist       = %g\n"
                     "  criterion       = %s\n"
                     "  usequad         = %s\n"
                     "  allowIncest     = %s\n"
                     "  outputCartesian = %s\n"
                     "  seed            = %ld\n"
                     "  tree_rsize      = %g\n"
                     "  theta           = %g\n"
                     "  eps2            = %g\n"
                     "  freqOut         = %u\n"
                     "  nbody           = %d\n"
                     "  modelNum        = %u\n"
                     "  models          = %s\n"
                     "};\n",
                     potBuf,
                     ctx->time_evolve,
                     ctx->time_orbit,
                     ctx->timestep,
                     ctx->orbit_timestep,
                     ctx->headline,
                     ctx->outfilename,
                     ctx->histogram,
                     ctx->histout,
                     ctx->outfile,
                     ctx->sunGCDist,
                     showCriterionT(ctx->criterion),
                     showBool(ctx->usequad),
                     showBool(ctx->allowIncest),
                     showBool(ctx->outputCartesian),
                     ctx->seed,
                     ctx->tree_rsize,
                     ctx->theta,
                     ctx->eps2,
                     ctx->freqOut,
                     ctx->nbody,
                     ctx->modelNum,
                     modelBuf))
    {
        fail("asprintf() failed\n");
    }

    free(potBuf);
    free(modelBuf);

    return buf;
}

void printNBodyCtx(const NBodyCtx* ctx)
{
    char* buf = showNBodyCtx(ctx);
    puts(buf);
    free(buf);
}

void printInitialConditions(const InitialConditions* ic)
{
    char* buf = showInitialConditions(ic);
    puts(buf);
    free(buf);
}

char* showVector(const mwvector v)
{
    char* buf;

    if (asprintf(&buf, "{ %g, %g, %g }", X(v), Y(v), Z(v)) < 0)
        fail("asprintf() failed\n");

    return buf;
}

void printVector(const mwvector v)
{
    char* buf = showVector(v);
    puts(buf);
    free(buf);
}

char* showFitParams(const FitParams* fp)
{
    char* buf;
    if (0 > asprintf(&buf,
                     "fit-params  = { \n"
                     "  useFitParams     = %s\n"
                     "  modelMass        = %g\n"
                     "  modelRadius      = %g\n"
                     "  reverseOrbitTime = %g\n"
                     "  simulationTime   = %g\n"
                     "};\n",
                     showBool(fp->useFitParams),
                     fp->modelMass,
                     fp->modelRadius,
                     fp->reverseOrbitTime,
                     fp->simulationTime))

    {
        fail("asprintf() failed\n");
    }

    return buf;
}

char* showHistogramParams(const HistogramParams* hp)
{
    char* buf;
    if (0 > asprintf(&buf,
                     "histogram-params = { \n"
                     "  phi      = %g\n"
                     "  theta    = %g\n"
                     "  psi      = %g\n"
                     "  startRaw = %g\n"
                     "  endRaw   = %g\n"
                     "  binSize  = %g\n"
                     "  center   = %g\n"
                     "};\n",
                     hp->phi,
                     hp->theta,
                     hp->psi,
                     hp->startRaw,
                     hp->endRaw,
                     hp->binSize,
                     hp->center))

    {
        fail("asprintf() failed\n");
    }

    return buf;
}

void printFitParams(const FitParams* fp)
{
    char* buf = showFitParams(fp);
    puts(buf);
    free(buf);
}

void printHistogramParams(const HistogramParams* hp)
{
    char* buf = showHistogramParams(hp);
    puts(buf);
    free(buf);
}

void printBody(const bodyptr p)
{
    char* buf = showBody(p);
    puts(buf);
    free(buf);
}

void printDwarfModel(const DwarfModel* p)
{
    char* buf = showDwarfModel(p);
    puts(buf);
    free(buf);
}

void printHalo(const Halo* h)
{
    char* buf = showHalo(h);
    puts(buf);
    free(buf);
}

void printDisk(const Disk* d)
{
    char* buf = showDisk(d);
    puts(buf);
    free(buf);
}

void printPotential(const Potential* p)
{
    char* buf = showPotential(p);
    puts(buf);
    free(buf);
}

