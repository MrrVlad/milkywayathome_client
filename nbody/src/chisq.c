// A BOINC compatible fitness calculation routine for the Orphan FILE* density profile project
// B. Willett Feb. 25, 2010
// Adapted for use with B&H treecode May 11, 2010


// Takes a treecode position, converts it to (l,b), then to (lambda, beta), and then constructs a histogram of the density in lambda.

// Then calculates the cross correlation between the model histogram and the data histogram
// A maximum correlation means the best fit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nbody_priv.h"
#include "chisq.h"

/* FIXME: Magic numbers */
#define phi d2r(128.79)
#define theta d2r(54.39)
#define psi d2r(90.70)
#define beginning ((real) -50.0)
#define end ((real) 50.0)
#define binsize ((real) 3.0)

real chisq(const NBodyCtx* ctx, NBodyState* st)
{
    real chisqval = 0.0;
    real totalnum = 0.0;
    int i, j;

    const int nbody = ctx->model.nbody;
    const bodyptr endp = st->bodytab + nbody;
    bodyptr p;
    FILE* f;

    // Histogram prep
    int index1, index2;
    int maxindex1 = (int) rfloor(end / binsize);
    int maxindex2 = (int) rabs(rfloor(beginning / binsize));

    real* histodata1 = calloc(sizeof(real), maxindex1 + 1);
    real* histodata2 = calloc(sizeof(real), maxindex2 + 1);

    real bcos, bsin, lsin, lcos;
    vector lbr;
    real lambda;
    const real cosphi = rcos(phi);
    const real sinphi = rsin(phi);
    const real sinpsi = rsin(psi);
    const real cospsi = rcos(psi);
    const real costh  = rcos(theta);
    const real sinth  = rsin(theta);

    printf("Transforming simulation results...");
    for (p = st->bodytab; p < endp; ++p)
    {
        // Convert to (l,b) (involves convert x to Sun-centered)
        // Leave in radians to make rotation easier
        cartesianToLbr_rad(ctx, lbr, Pos(p));

        // Convert to (lambda, beta) (involves a rotation using the Newberg et al (2009) rotation matrices)

        bcos = rcos(B(lbr));
        bsin = rsin(B(lbr));
        lsin = rsin(L(lbr));
        lcos = rcos(L(lbr));

        /* CHECKME: Do we need fma here? */
        lambda = r2d(atan2(
                         - (sinpsi * cosphi + costh * sinphi * cospsi) * bcos * lcos
                         + (-sinpsi * sinphi + costh * cosphi * cospsi) * bcos * lsin
                         + cospsi * sinth * bsin,

                           (cospsi * cosphi - costh * sinphi * sinpsi) * bcos * lcos
                         + (cospsi * sinphi + costh * cosphi * sinpsi) * bcos * lsin
                         + sinpsi * sinth * bsin ));

        // Create the histogram
        if (lambda > 0.0 && lambda < end)
        {
            index1 = (int)rfloor(lambda / binsize);  // floor?
            histodata1[index1]++;
            totalnum += 1.0;
        }
        else if (lambda > beginning && lambda < 0.0)
        {
            index2 = abs(((int) rfloor(lambda / binsize) + 1));
            histodata2[index2]++;
            totalnum += 1.0;
        }
        else
        {
            printf("I happen.\n");
        }
    }
    printf("done\n");
    printf("Total num in range: %f\n", totalnum);

    // Find the largest entry
    // Get the single largest bin so we can normalize over it
    //CHECKME: Why the +1's in the sizes of histodatas?

    f = fopen("histout", "w");
    if (f == NULL)
    {
        perror("Writing histout");
        return NAN;
    }

    printf("...file open...");

    // Print out the histogram
    real foo;

    for (i = 0, foo = -binsize; foo > beginning; foo -= binsize, ++i) // foo = -binsize or foo = 0?
    {
        fprintf(f, "%f %f\n", rfma(0.5, binsize, foo), histodata2[i] / totalnum);
    }

    for (i = 0, foo = 0; foo < end; foo += binsize, ++i)
    {
        fprintf(f, "%f %f\n", rfma(0.5, binsize, foo) , histodata1[i] / totalnum);
    }
    fclose(f);

    printf("done\n");

    // Calculate the chisq value by reading a file called "histogram" (the real data histogram should already be normalized)

    f = fopen(ctx->histogram, "r");
    if (f == NULL)
    {
        perror("Opening histogram");
        return NAN;
    }

    int fsize;
    real* fileLambda = NULL, *fileCount = NULL, *fileCountErr = NULL;
    int filecount = 0;

    fseek(f, 0L, SEEK_END);
    fsize = ceil((double) (ftell(f) + 1) / 3);  /* Make sure it's big enough, avoid fun with integer division */
    fseek(f, 0L, SEEK_SET);

    fileLambda   = (real*) callocSafe(sizeof(real), fsize);
    fileCount    = (real*) callocSafe(sizeof(real), fsize);
    fileCountErr = (real*) callocSafe(sizeof(real), fsize);

    /* FIXME: Proper reading. Avoid CPP annoying */
    while (fscanf(f,
                  #ifdef DOUBLEPREC  /* Temporary work around */
                    "%lf %lf %lf\n",
                  #else
                  "%f %f %f\n",
                  #endif
                  &fileLambda[filecount],
                  &fileCount[filecount],
                  &fileCountErr[filecount]) != EOF)
    {
        printf("%g %g %g\n",
               fileLambda[filecount],
               fileCount[filecount],
               fileCountErr[filecount]);
        ++filecount;
    }

    fclose(f);

    // Calculate the chisq
    for (i = 0, foo = -binsize; foo >= beginning; foo -= binsize, ++i)
    {
        real limit = rfma( 0.5, binsize, foo );
        for (j = 0; j < filecount; ++j)
        {
            if ( REQ(fileLambda[j], limit) )
            {
                chisqval += sqr((fileCount[j] - (histodata2[i] / totalnum)) / fileCountErr[j]);
            }
        }
    }

    for (i = 0, foo = 0.0; foo <= end; foo += binsize, ++i)
    {
        real limit = rfma(0.5, binsize,  foo);
        for (j = 0; j < filecount; ++j)
        {
            if ( REQ(fileLambda[j], limit) )
            {
                chisqval += sqr((fileCount[j] - (histodata1[i] / totalnum)) / fileCountErr[j]);
            }
        }
    }

    free(fileLambda);
    free(fileCount);
    free(fileCountErr);

    free(histodata1);
    free(histodata2);

    printf("CHISQ = %f\n", chisqval);

    // MAXIMUM likelihood, multiply by -1
    real likelihood = -chisqval;
    printf("likelihood = %f\n", likelihood);
    return likelihood;
}

