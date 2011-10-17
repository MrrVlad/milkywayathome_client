/*
 *  Copyright (c) 2010-2011 Matthew Arsenault
 *  Copyright (c) 2010-2011 Rensselaer Polytechnic Institute
 *
 *  This file is part of Milkway@Home.
 *
 *  Milkway@Home is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "milkyway_timing.h"

#ifdef _WIN32
  #include <windows.h>
#endif /* _WIN32 */

#include <time.h>

#if HAVE_MACH_ABSOLUTE_TIME
  #include <mach/mach.h>
  #include <mach/mach_time.h>
#endif


static const uint64_t nsPerSec = 1000000000ULL;

MWHighResTime mwDiffMWHighResTime(const MWHighResTime* end, const MWHighResTime* start)
{
    MWHighResTime t;

    if (start->nSec > end->nSec)
    {
        t.sec = end->sec - start->sec - 1;
		t.nSec = nsPerSec + end->nSec - start->nSec;
	}
    else
    {
        t.sec = end->sec - start->sec;
		t.nSec = end->nSec - start->nSec;
    }

    return t;
}

void mwIncMWHighResTime(MWHighResTime* a, const MWHighResTime* b)
{
    uint64_t dSec;

    a->sec += b->sec;
    a->nSec += b->nSec;

    dSec = a->nSec / nsPerSec;

    a->sec += dSec;
    a->sec -= dSec * nsPerSec;
}

#if HAVE_CLOCK_GETTIME

int mwGetHighResTime_RealTime(MWHighResTime* t)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts))
    {
        mwPerror("Error getting CLOCK_REALTIME");
        return 1;
    }

    t->sec = (uint64_t) ts.tv_sec;
    t->nSec = (uint64_t) ts.tv_nsec;

    return 0;
}

#elif HAVE_MACH_ABSOLUTE_TIME

int mwGetHighResTime_RealTime(MWHighResTime* timer)
{
    uint64_t t;
    static mach_timebase_info_data_t timeInfo;

    if (timeInfo.denom == 0)
    {
        (void) mach_timebase_info(&timeInfo);
    }

    t = mach_absolute_time();

    /* Convert to nanoseconds */
    t *= timeInfo.numer;
    t /= timeInfo.denom;

    timer->sec = t / nsPerSec;
    timer->nSec = t - nsPerSec * timer->sec;

    return 0;
}

#elif defined(_WIN32)

int mwGetHighResTime_RealTime(MWHighResTime* t)
{
    mw_printf("Implement me\n");
    return 1;
}

#else

int mwGetHighResTime_RealTime(MWHighResTime* t)
{
    mw_printf("Implement me\n");
    return 1;
}

#endif /* HAVE_CLOCK_GETTIME */
