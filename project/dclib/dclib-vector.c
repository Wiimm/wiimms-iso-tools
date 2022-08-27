
/***************************************************************************
 *                                                                         *
 *                     _____     ____                                      *
 *                    |  __ \   / __ \   _     _ _____                     *
 *                    | |  \ \ / /  \_\ | |   | |  _  \                    *
 *                    | |   \ \| |      | |   | | |_| |                    *
 *                    | |   | || |      | |   | |  ___/                    *
 *                    | |   / /| |   __ | |   | |  _  \                    *
 *                    | |__/ / \ \__/ / | |___| | |_| |                    *
 *                    |_____/   \____/  |_____|_|_____/                    *
 *                                                                         *
 *                       Wiimms source code library                        *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *        Copyright (c) 2012-2022 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include <string.h>

#include "dclib-basics.h"
#include "dclib-numeric.h"
#include "dclib-debug.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			float3 list support		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeF3L
(
    float3List_t *f3l,		// valid data
    uint	 bin_search	// >0: enable binary search
				// and allocate 'bin_search' elements
)
{
    DASSERT(f3l);
    memset(f3l,0,sizeof(*f3l));
    if (bin_search)
    {
	f3l->size = bin_search;
	f3l->list = MALLOC( f3l->size * sizeof(*f3l->list) );
	f3l->sort = MALLOC( f3l->size * sizeof(*f3l->sort) );
    }
}

///////////////////////////////////////////////////////////////////////////////

void ResetF3L ( float3List_t * f3l )
{
    DASSERT(f3l);
    FREE(f3l->list);
    FREE(f3l->sort);
    memset(f3l,0,sizeof(*f3l));
}

///////////////////////////////////////////////////////////////////////////////

float3 * AppendF3L ( float3List_t * f3l )
{
    DASSERT( f3l );
    DASSERT( f3l->used <= f3l->size );
    DASSERT( !f3l->list == !f3l->size );

    if ( f3l->used == f3l->size )
    {
	f3l->size = 3*f3l->size/2 + 100;
	f3l->list = REALLOC( f3l->list, f3l->size * sizeof(*f3l->list) );
	if (f3l->sort)
	    f3l->sort = REALLOC( f3l->sort, f3l->size * sizeof(*f3l->sort) );
    }

    DASSERT( f3l->list );
    DASSERT( f3l->used < f3l->size );

    if (f3l->sort)
	f3l->sort[f3l->used] = 0;

    float3 * item = f3l->list + f3l->used++;
    item->x = item->y = item->z = 0.0;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

uint FindInsertFloatF3L ( float3List_t * f3l, float3 *val, bool fast )
{
    DASSERT( f3l );
    DASSERT( f3l->used <= f3l->size );
    DASSERT( !f3l->list == !f3l->size );
    DASSERT( val );

    int beg = 0;
    if (!fast)
    {
	if (f3l->sort)
	{
	    int end = f3l->used - 1;
	    while ( beg <= end )
	    {
		uint idx = (beg+end)/2;
		DASSERT( idx >= 0 && idx < f3l->used );
		DASSERT_MSG( f3l->sort[idx] < f3l->used,
			"idx=%d, sort-idx=%d, used=%u, size=%u\n",
			idx, f3l->sort[idx], f3l->used, f3l->size );
		int stat = memcmp( val,f3l->list + f3l->sort[idx], sizeof(*val) );
		if ( stat < 0 )
		    end = idx - 1 ;
		else if ( stat > 0 )
		    beg = idx + 1;
		else
		    return f3l->sort[idx];
	    }
	}
	else
	{
	    float3 *ptr = f3l->list;
	    float3 *end = ptr + f3l->used;
	    for ( ; ptr < end; ptr++ )
		if (!memcmp(val,ptr,sizeof(*val)))
		    return ptr - f3l->list;
	}
    }
    else if (f3l->sort)
    {
	// 'sort' not needed
	BINGO; // don't remove this line!
	FREE(f3l->sort);
	f3l->sort = 0;
    }

    float3 *res = AppendF3L(f3l);
    DASSERT(res);
    memcpy(res,val,sizeof(*res));

    const uint idx = f3l->used - 1;
    DASSERT( idx < f3l->used );
    if (f3l->sort)
    {
	DASSERT( beg >= 0 && beg <= idx );
	uint *dest = f3l->sort + beg;
	memmove( dest+1, dest, (idx-beg)*sizeof(*f3l->sort) );
	*dest = idx;

     #if HAVE_PRINT0
	if ( f3l->used < 10 )
	{
	    uint i;
	    for ( i = 0; i < f3l->used; i++ )
	    {
		float3 *f = f3l->list + f3l->sort[i];
		PRINT("INS %2u: %2u -> %08x %08x %08x %11.5f %11.5f %11.5f\n",
			i, f3l->sort[i],
			be32(&f->x), be32(&f->y), be32(&f->z),
			f->x, f->y, f->z );
	    }
	    PRINT("---\n");
	}
     #endif
    }

    return idx;
}

///////////////////////////////////////////////////////////////////////////////

float3 * GrowF3L ( float3List_t * f3l, uint n )
{
    DASSERT( f3l );
    DASSERT( f3l->used <= f3l->size );
    DASSERT( !f3l->list == !f3l->size );

    const uint new_used = f3l->used + n;
    if ( new_used > f3l->size || !f3l->size )
    {
	f3l->size = new_used ? new_used : 10;
	f3l->list = REALLOC( f3l->list, f3l->size * sizeof(*f3l->list)) ;
    }

    DASSERT( f3l->list );
    DASSERT( f3l->used + n <= f3l->size );

    float3 * result = f3l->list + f3l->used;
    f3l->used = new_used;
    return result;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			double3 list support		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeD3L ( double3List_t * d3l )
{
    DASSERT(d3l);
    memset(d3l,0,sizeof(*d3l));
}

///////////////////////////////////////////////////////////////////////////////

void ResetD3L ( double3List_t * d3l )
{
    DASSERT(d3l);
    FREE(d3l->list);
    memset(d3l,0,sizeof(*d3l));
}

///////////////////////////////////////////////////////////////////////////////

double3 * AppendD3L ( double3List_t * d3l )
{
    DASSERT( d3l );
    DASSERT( d3l->used <= d3l->size );
    DASSERT( !d3l->list == !d3l->size );

    if ( d3l->used == d3l->size )
    {
	d3l->size = 3*d3l->size/2 + 100;
	d3l->list = REALLOC( d3l->list, d3l->size * sizeof(*d3l->list)) ;
    }

    DASSERT( d3l->list );
    DASSERT( d3l->used < d3l->size );

    double3 * item = d3l->list + d3l->used++;
    item->x = item->y = item->z = 0.0;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

uint FindInsertFloatD3L ( double3List_t * d3l, double3 *val, bool fast )
{
    DASSERT( d3l );
    DASSERT( d3l->used <= d3l->size );
    DASSERT( !d3l->list == !d3l->size );
    DASSERT( val );

    // first: reduce precision
    double3 temp;
    temp.x = double2float(val->x);
    temp.y = double2float(val->y);
    temp.z = double2float(val->z);

    if (!fast)
    {
	double3 *ptr = d3l->list;
	double3 *end = ptr + d3l->used;
	for ( ; ptr < end; ptr++ )
	    if (!memcmp(&temp,ptr,sizeof(temp)))
		return ptr - d3l->list;
    }

    double3 *res = AppendD3L(d3l);
    DASSERT(res);
    memcpy(res,&temp,sizeof(*res));
    return d3l->used - 1;
}

///////////////////////////////////////////////////////////////////////////////

double3 * GrowD3L ( double3List_t * d3l, uint n )
{
    DASSERT( d3l );
    DASSERT( d3l->used <= d3l->size );
    DASSERT( !d3l->list == !d3l->size );

    const uint new_used = d3l->used + n;
    if ( new_used > d3l->size || !d3l->size )
    {
	d3l->size = new_used ? new_used : 10;
	d3l->list = REALLOC( d3l->list, d3l->size * sizeof(*d3l->list)) ;
    }

    DASSERT( d3l->list );
    DASSERT( d3l->used + n <= d3l->size );

    double3 * result = d3l->list + d3l->used;
    d3l->used = new_used;
    return result;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			check numbers			///////////////
///////////////////////////////////////////////////////////////////////////////

bool IsNormalF3 ( float *f3 )
{
    DASSERT(f3);
    const int fpclass = fpclassify(*f3++);
    if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
    {
	const int fpclass = fpclassify(*f3++);
	if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
	{
	    const int fpclass = fpclassify(*f3);
	    return fpclass == FP_NORMAL || fpclass == FP_ZERO;
	}
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool IsNormalF3be ( float *f3 )
{
    DASSERT(f3);
    const int fpclass = fpclassify(bef4(f3));
    if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
    {
	const int fpclass = fpclassify(bef4(++f3));
	if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
	{
	    const int fpclass = fpclassify(bef4(++f3));
	    return fpclass == FP_NORMAL || fpclass == FP_ZERO;
	}
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool IsNormalD3 ( double *d3 )
{
    DASSERT(d3);
    const int fpclass = fpclassify(*d3++);
    if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
    {
	const int fpclass = fpclassify(*d3++);
	if ( fpclass == FP_NORMAL || fpclass == FP_ZERO )
	{
	    const int fpclass = fpclassify(*d3);
	    return fpclass == FP_NORMAL || fpclass == FP_ZERO;
	}
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool IsEqualD3 ( const double3 *a, const double3 *b,
			double null_epsilon, double diff_epsilon )
{
    DASSERT(a);
    DASSERT(b);
    DASSERT( null_epsilon > 0.0 );
    DASSERT( diff_epsilon > 0.0 );

    double diff;

    diff = fabs( a->x - b->x );
    if ( diff > null_epsilon )
    {
	const double abs_a = fabs(a->x);
	const double abs_b = fabs(b->x);
	if ( diff > ( abs_a > abs_b ? abs_a : abs_b ) * diff_epsilon )
	    return false;
    }

    diff = fabs( a->y - b->y );
    if ( diff > null_epsilon )
    {
	const double abs_a = fabs(a->y);
	const double abs_b = fabs(b->y);
	if ( diff > ( abs_a > abs_b ? abs_a : abs_b ) * diff_epsilon )
	    return false;
    }

    diff = fabs( a->z - b->z );
    if ( diff > null_epsilon )
    {
	const double abs_a = fabs(a->z);
	const double abs_b = fabs(b->z);
	if ( diff > ( abs_a > abs_b ? abs_a : abs_b ) * diff_epsilon )
	    return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool IsSameF ( float a, float b, int bit_diff )
{
    float delta = ldexpf( fabsf(a-b), 23-bit_diff );
    return IsNormalF(delta) && delta <= fabsf(a) && delta <= fabsf(b);
}

bool IsSameD ( double a, double b, int bit_diff )
{
    double delta = ldexp( fabs(a-b), 52-bit_diff );
    return IsNormalD(delta) && delta <= fabs(a) && delta <= fabs(b);
}

bool IsSameF3 ( const float  *a, const float  *b, int bit_diff )
{
    return IsSameF(a[0],b[0],bit_diff)
	&& IsSameF(a[1],b[1],bit_diff)
	&& IsSameF(a[2],b[2],bit_diff);
}

bool IsSameD3 ( const double *a, const double *b, int bit_diff )
{
    return IsSameD(a[0],b[0],bit_diff)
	&& IsSameD(a[1],b[1],bit_diff)
	&& IsSameD(a[2],b[2],bit_diff);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			2D vector functions		///////////////
///////////////////////////////////////////////////////////////////////////////

uint PointsInConvexPolygonF
(
    // return, how many points are inside the polygon

    float	*pts,	// points: pointer to a array with '2*n_pts' elements
    uint	n_pts,	// Number of (x,y) pairs in 'pts'

    float	*pol,	// polygon: pointer to a array with '2*n_pol' elements
    uint	n_pol,	// number of (x,y) pairs in 'pol'

    bool	all,	// false: count the numbers of points inside
			// true:  return '1' for all points inside and otherwise '0'

    int		*r_dir	// If not NULL then return:
			//	-1: polygon is defined counterclockwise
			//	 0: unknown
			//	+1: polygon is defined clockwise
			// If at least one point is inside, the result is known.
)
{
    if ( !n_pts || n_pol < 3 )
	return 0;
    DASSERT(pts);
    DASSERT(pol);

    int pol_dir = 0;
    uint count = 0;
    for(;;)
    {
	float *a = pol + 2*(n_pol-1);
	float *b = pol;
	int dir = pol_dir;
	uint i = n_pol;
	for(;;)
	{
	    const double dstat = SideOfLine(a[0],a[1],b[0],b[1],pts[0],pts[1]);
	    const int new_dir = dstat > 0.0 ? -1 : dstat < 0;
	    noPRINT("STAT: %2d %2d %2d %4g\n", pol_dir, dir, new_dir, dstat );
	    if (!dir)
		dir = new_dir;
	    else if ( new_dir && dir != new_dir )
	    {
		if (!all)
		    break;
		if (r_dir)
		    *r_dir = pol_dir;
		return 0;
	    }

	    if ( --i > 0 )
	    {
		a  = b;
		b += 2;
		continue;
	    }

	    if (!pol_dir)
		pol_dir = dir;
	    count++;
	    break;
	}

	if ( --n_pts > 0 )
	{
	    pts += 2;
	    continue;
	}

	if (r_dir)
	    *r_dir = pol_dir;
	noPRINT("DIR:  %2d\n",pol_dir);
	return all ? 1 : count;
    }
}

///////////////////////////////////////////////////////////////////////////////

uint PointsInConvexPolygonD
(
    // return, how many points are inside the polygon

    double	*pts,	// points: pointer to a array with '2*n_pts' elements
    uint	n_pts,	// Number of (x,y) pairs in 'pts'

    double	*pol,	// polygon: pointer to a array with '2*n_pol' elements
    uint	n_pol,	// number of (x,y) pairs in 'pol'

    bool	all,	// false: count the numbers of points inside
			// true:  return '1' for all points inside and otherwise '0'

    int		*r_dir	// If not NULL then return:
			//	-1: polygon is defined counterclockwise
			//	 0: unknown
			//	+1: polygon is defined clockwise
			// If at least one point is inside, the result is known.
)
{
    if ( !n_pts || n_pol < 3 )
	return 0;
    DASSERT(pts);
    DASSERT(pol);

    int pol_dir = 0;
    uint count = 0;
    for(;;)
    {
	double *a = pol + 2*(n_pol-1);
	double *b = pol;
	int dir = pol_dir;
	uint i = n_pol;
	for(;;)
	{
	    const double dstat = SideOfLine(a[0],a[1],b[0],b[1],pts[0],pts[1]);
	    const int new_dir = dstat > 0.0 ? -1 : dstat < 0;
	    noPRINT("STAT: %2d %2d %2d %4g\n", pol_dir, dir, new_dir, dstat );
	    if (!dir)
		dir = new_dir;
	    else if ( new_dir && dir != new_dir )
	    {
		if (!all)
		    break;
		if (r_dir)
		    *r_dir = pol_dir;
		return 0;
	    }

	    if ( --i > 0 )
	    {
		a  = b;
		b += 2;
		continue;
	    }

	    if (!pol_dir)
		pol_dir = dir;
	    count++;
	    break;
	}

	if ( --n_pts > 0 )
	{
	    pts += 2;
	    continue;
	}

	if (r_dir)
	    *r_dir = pol_dir;
	noPRINT("DIR:  %2d\n",pol_dir);
	return all ? 1 : count;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    float34			///////////////
///////////////////////////////////////////////////////////////////////////////

void ClearFloat34 ( float34 *f34 )
{
    DASSERT(f34);
    float *d = f34->v;
    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = 0.0;
}

///////////////////////////////////////////////////////////////////////////////

void SetupFloat34 ( float34 *f34 )
{
    DASSERT(f34);
    float *d = f34->v;
    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = 0.0;
    f34->m[0][0] = f34->m[1][1] = f34->m[2][2] = 1.0;
}

///////////////////////////////////////////////////////////////////////////////

void SetupMatrixF34
(
    float34	*mat,		// data to setup
    float3	*scale,		// NULL or scaling vector
    float3	*rotate,	// NULL or rotation vector in *radiant*
    float3	*translate	// NULL or translation vector
)
{
    // This function use 'scale', 'rotate' and 'translate' to calculate the
    // transformation matrix. The calculations have been verified against
    // original MDL files of Nintendo. The rotation is done in right-handed
    // rotation with Euler angles (in degree) and x-y-z convention:
    //   => http://en.wikipedia.org/wiki/Matrix_rotation#General_rotations

    DASSERT(mat);
    float *t = mat->v;


    //--- setup scale

    float3 myscale;
    if (scale)
    {
	myscale.x = fabs(scale->x) < NULL_EPSILON ? 1.0 : scale->x;
	myscale.y = fabs(scale->y) < NULL_EPSILON ? 1.0 : scale->y;
	myscale.z = fabs(scale->z) < NULL_EPSILON ? 1.0 : scale->z;
    }
    else
	myscale.x = myscale.y = myscale.z = 1.0;


    //--- calc translation matrix by scale and rotation

    if (rotate)
    {
	// be sure, that null values are exact
	const double sin_x = fabs(rotate->x) < NULL_EPSILON ? 0.0 : sin( rotate->x );
	const double cos_x = fabs(rotate->x) < NULL_EPSILON ? 1.0 : cos( rotate->x );
	const double sin_y = fabs(rotate->y) < NULL_EPSILON ? 0.0 : sin( rotate->y );
	const double cos_y = fabs(rotate->y) < NULL_EPSILON ? 1.0 : cos( rotate->y );
	const double sin_z = fabs(rotate->z) < NULL_EPSILON ? 0.0 : sin( rotate->z );
	const double cos_z = fabs(rotate->z) < NULL_EPSILON ? 1.0 : cos( rotate->z );

	t[  0] = myscale.x * (   cos_y * cos_z );
	t[  1] = myscale.y * ( - cos_x * sin_z  +  sin_x * sin_y * cos_z );
	t[  2] = myscale.z * (   sin_x * sin_z  +  cos_x * sin_y * cos_z );

	t[4+0] = myscale.x * (   cos_y * sin_z );
	t[4+1] = myscale.y * (   cos_x * cos_z  +  sin_x * sin_y * sin_z );
	t[4+2] = myscale.z * ( - sin_x * cos_z  +  cos_x * sin_y * sin_z );

	t[8+0] = myscale.x * ( - sin_y );
	t[8+1] = myscale.y * (   sin_x * cos_y );
	t[8+2] = myscale.z * (   cos_x * cos_y );
    }
    else
    {
	t[  0] = myscale.x;
	t[4+1] = myscale.y;
	t[8+2] = myscale.z;

	t[  1] = 0.0;
	t[  2] = 0.0;
	t[4+0] = 0.0;
	t[4+2] = 0.0;
	t[8+0] = 0.0;
	t[8+1] = 0.0;
    }


    //--- assign translation

    if (translate)
    {
	t[  3] = translate->x;
	t[4+3] = translate->y;
	t[8+3] = translate->z;
    }
    else
    {
	t[  3] = 0.0;
	t[4+3] = 0.0;
	t[8+3] = 0.0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void CalcInverseMatrixF34
(
    float34		*i,	// store inverse matrix here
    const float34	*t	// transformation matrix
)
{
    DASSERT(i);
    DASSERT(t);

    //--- calc inverse matrix

    const double det	= t->v[  0] * t->v[4+1] * t->v[8+2]
			+ t->v[  1] * t->v[4+2] * t->v[8+0]
			+ t->v[  2] * t->v[4+0] * t->v[8+1]
			- t->v[  2] * t->v[4+1] * t->v[8+0]
			- t->v[  1] * t->v[4+0] * t->v[8+2]
			- t->v[  0] * t->v[4+2] * t->v[8+1];

    if ( fabs(det) < NULL_EPSILON )
    {
	// this should never happen => initialize inv_matrix with 0.0
	ClearFloat34(i);
    }
    else
    {
	const double det1 = 1.0 / det; // multplication is faster than division

	i->v[  0] = ( t->v[4+1] * t->v[8+2] - t->v[4+2] * t->v[8+1] ) * det1;
	i->v[  1] = ( t->v[  2] * t->v[8+1] - t->v[  1] * t->v[8+2] ) * det1;
	i->v[  2] = ( t->v[  1] * t->v[4+2] - t->v[  2] * t->v[4+1] ) * det1;

	i->v[4+0] = ( t->v[4+2] * t->v[8+0] - t->v[4+0] * t->v[8+2] ) * det1;
	i->v[4+1] = ( t->v[  0] * t->v[8+2] - t->v[  2] * t->v[8+0] ) * det1;
	i->v[4+2] = ( t->v[  2] * t->v[4+0] - t->v[  0] * t->v[4+2] ) * det1;

	i->v[8+0] = ( t->v[4+0] * t->v[8+1] - t->v[4+1] * t->v[8+0] ) * det1;
	i->v[8+1] = ( t->v[  1] * t->v[8+0] - t->v[  0] * t->v[8+1] ) * det1;
	i->v[8+2] = ( t->v[  0] * t->v[4+1] - t->v[  1] * t->v[4+0] ) * det1;
    }

    //--- calc inverse translation

    i->v[  3] =	- t->v[  3] * i->v[  0]
		- t->v[4+3] * i->v[  1]
		- t->v[8+3] * i->v[  2];

    i->v[4+3] =	- t->v[  3] * i->v[4+0]
		- t->v[4+3] * i->v[4+1]
		- t->v[8+3] * i->v[4+2];

    i->v[8+3] =	- t->v[  3] * i->v[8+0]
		- t->v[4+3] * i->v[8+1]
		- t->v[8+3] * i->v[8+2];
}

///////////////////////////////////////////////////////////////////////////////

void MultiplyF34
(
    float34		*dest,	// valid, store result here, may be 'src1' or 'src2'
    const float34	*src1,	// first valid source, if NULL use dest
    const float34	*src2	// second valid source, if NULL use dest
)
{
    DASSERT(dest);

    if ( dest == src1 || dest == src2 || !src1 || !src2 )
    {
	float34 temp;
	MultiplyF34( &temp, src1 ? src1 : dest, src2 ? src2 : dest );
	*dest = temp;
	return;
    }
    DASSERT( src1 && src1 != dest );
    DASSERT( src2 && src2 != dest );

    float *res = dest->v;
    const float *a = src1->v;
    const float *b = src2->v;

    res[0+0] = a[0+0]*b[0+0] + a[4+0]*b[0+1] + a[8+0]*b[0+2];
    res[0+1] = a[0+1]*b[0+0] + a[4+1]*b[0+1] + a[8+1]*b[0+2];
    res[0+2] = a[0+2]*b[0+0] + a[4+2]*b[0+1] + a[8+2]*b[0+2];
    res[0+3] = a[0+3]*b[0+0] + a[4+3]*b[0+1] + a[8+3]*b[0+2] + b[0+3];

    res[4+0] = a[0+0]*b[4+0] + a[4+0]*b[4+1] + a[8+0]*b[4+2];
    res[4+1] = a[0+1]*b[4+0] + a[4+1]*b[4+1] + a[8+1]*b[4+2];
    res[4+2] = a[0+2]*b[4+0] + a[4+2]*b[4+1] + a[8+2]*b[4+2];
    res[4+3] = a[0+3]*b[4+0] + a[4+3]*b[4+1] + a[8+3]*b[4+2] + b[4+3];

    res[8+0] = a[0+0]*b[8+0] + a[4+0]*b[8+1] + a[8+0]*b[8+2];
    res[8+1] = a[0+1]*b[8+0] + a[4+1]*b[8+1] + a[8+1]*b[8+2];
    res[8+2] = a[0+2]*b[8+0] + a[4+2]*b[8+1] + a[8+2]*b[8+2];
    res[8+3] = a[0+3]*b[8+0] + a[4+3]*b[8+1] + a[8+3]*b[8+2] + b[8+3];
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    double34			///////////////
///////////////////////////////////////////////////////////////////////////////

void ClearDouble34 ( double34 *d34 )
{
    DASSERT(d34);
    double *d = d34->v;
    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = 0.0;
}

///////////////////////////////////////////////////////////////////////////////

void SetupDouble34 ( double34 *d34 )
{
    DASSERT(d34);
    double *d = d34->v;
    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = 0.0;
    d34->m[0][0] = d34->m[1][1] = d34->m[2][2] = 1.0;
}

///////////////////////////////////////////////////////////////////////////////

void SetupMatrixD34
(
    double34	*mat,		// data to setup
    double3	*scale,		// NULL or scaling vector
    double3	*rotate,	// NULL or rotation vector in *radiant*
    double3	*translate	// NULL or translation vector
)
{
    // This function use 'scale', 'rotate' and 'translate' to calculate the
    // transformation matrix. The calculations have been verified against
    // original MDL files of Nintendo. The rotation is done in right-handed
    // rotation with Euler angles (in degree) and x-y-z convention:
    //   => http://en.wikipedia.org/wiki/Matrix_rotation#General_rotations

    DASSERT(mat);
    double *t = mat->v;


    //--- setup scale

    double3 myscale;
    if (scale)
    {
	myscale.x = fabs(scale->x) < NULL_EPSILON ? 1.0 : scale->x;
	myscale.y = fabs(scale->y) < NULL_EPSILON ? 1.0 : scale->y;
	myscale.z = fabs(scale->z) < NULL_EPSILON ? 1.0 : scale->z;
    }
    else
	myscale.x = myscale.y = myscale.z = 1.0;


    //--- calc translation matrix by scale and rotation

    if (rotate)
    {
	// be sure, that null values are exact
	const double sin_x = fabs(rotate->x) < NULL_EPSILON ? 0.0 : sin( rotate->x );
	const double cos_x = fabs(rotate->x) < NULL_EPSILON ? 1.0 : cos( rotate->x );
	const double sin_y = fabs(rotate->y) < NULL_EPSILON ? 0.0 : sin( rotate->y );
	const double cos_y = fabs(rotate->y) < NULL_EPSILON ? 1.0 : cos( rotate->y );
	const double sin_z = fabs(rotate->z) < NULL_EPSILON ? 0.0 : sin( rotate->z );
	const double cos_z = fabs(rotate->z) < NULL_EPSILON ? 1.0 : cos( rotate->z );

	t[  0] = myscale.x * (   cos_y * cos_z );
	t[  1] = myscale.y * ( - cos_x * sin_z  +  sin_x * sin_y * cos_z );
	t[  2] = myscale.z * (   sin_x * sin_z  +  cos_x * sin_y * cos_z );

	t[4+0] = myscale.x * (   cos_y * sin_z );
	t[4+1] = myscale.y * (   cos_x * cos_z  +  sin_x * sin_y * sin_z );
	t[4+2] = myscale.z * ( - sin_x * cos_z  +  cos_x * sin_y * sin_z );

	t[8+0] = myscale.x * ( - sin_y );
	t[8+1] = myscale.y * (   sin_x * cos_y );
	t[8+2] = myscale.z * (   cos_x * cos_y );
    }
    else
    {
	t[  0] = myscale.x;
	t[4+1] = myscale.y;
	t[8+2] = myscale.z;

	t[  1] = 0.0;
	t[  2] = 0.0;
	t[4+0] = 0.0;
	t[4+2] = 0.0;
	t[8+0] = 0.0;
	t[8+1] = 0.0;
    }


    //--- assign translation

    if (translate)
    {
	t[  3] = translate->x;
	t[4+3] = translate->y;
	t[8+3] = translate->z;
    }
    else
    {
	t[  3] = 0.0;
	t[4+3] = 0.0;
	t[8+3] = 0.0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void CalcInverseMatrixD34
(
    double34		*i,	// store inverse matrix here
    const double34	*t	// transformation matrix
)
{
    DASSERT(i);
    DASSERT(t);

    //--- calc inverse matrix

    const double det	= t->v[  0] * t->v[4+1] * t->v[8+2]
			+ t->v[  1] * t->v[4+2] * t->v[8+0]
			+ t->v[  2] * t->v[4+0] * t->v[8+1]
			- t->v[  2] * t->v[4+1] * t->v[8+0]
			- t->v[  1] * t->v[4+0] * t->v[8+2]
			- t->v[  0] * t->v[4+2] * t->v[8+1];

    if ( fabs(det) < NULL_EPSILON )
    {
	// this should never happen => initialize inv_matrix with 0.0
	ClearDouble34(i);
    }
    else
    {
	const double det1 = 1.0 / det; // multplication is faster than division

	i->v[  0] = ( t->v[4+1] * t->v[8+2] - t->v[4+2] * t->v[8+1] ) * det1;
	i->v[  1] = ( t->v[  2] * t->v[8+1] - t->v[  1] * t->v[8+2] ) * det1;
	i->v[  2] = ( t->v[  1] * t->v[4+2] - t->v[  2] * t->v[4+1] ) * det1;

	i->v[4+0] = ( t->v[4+2] * t->v[8+0] - t->v[4+0] * t->v[8+2] ) * det1;
	i->v[4+1] = ( t->v[  0] * t->v[8+2] - t->v[  2] * t->v[8+0] ) * det1;
	i->v[4+2] = ( t->v[  2] * t->v[4+0] - t->v[  0] * t->v[4+2] ) * det1;

	i->v[8+0] = ( t->v[4+0] * t->v[8+1] - t->v[4+1] * t->v[8+0] ) * det1;
	i->v[8+1] = ( t->v[  1] * t->v[8+0] - t->v[  0] * t->v[8+1] ) * det1;
	i->v[8+2] = ( t->v[  0] * t->v[4+1] - t->v[  1] * t->v[4+0] ) * det1;
    }

    //--- calc inverse translation

    i->v[  3] =	- t->v[  3] * i->v[  0]
		- t->v[4+3] * i->v[  1]
		- t->v[8+3] * i->v[  2];

    i->v[4+3] =	- t->v[  3] * i->v[4+0]
		- t->v[4+3] * i->v[4+1]
		- t->v[8+3] * i->v[4+2];

    i->v[8+3] =	- t->v[  3] * i->v[8+0]
		- t->v[4+3] * i->v[8+1]
		- t->v[8+3] * i->v[8+2];
}

///////////////////////////////////////////////////////////////////////////////

void MultiplyD34
(
    double34		*dest,	// valid, store result here, may be 'src1' or 'src2'
    const double34	*src1,	// first valid source, if NULL use dest
    const double34	*src2	// second valid source, if NULL use dest
)
{
    DASSERT(dest);

    if ( dest == src1 || dest == src2 || !src1 || !src2 )
    {
	double34 temp;
	MultiplyD34( &temp, src1 ? src1 : dest, src2 ? src2 : dest );
	*dest = temp;
	return;
    }
    DASSERT( src1 && src1 != dest );
    DASSERT( src2 && src2 != dest );

    double *res = dest->v;
    const double *a = src1->v;
    const double *b = src2->v;

    res[0+0] = a[0+0]*b[0+0] + a[4+0]*b[0+1] + a[8+0]*b[0+2];
    res[0+1] = a[0+1]*b[0+0] + a[4+1]*b[0+1] + a[8+1]*b[0+2];
    res[0+2] = a[0+2]*b[0+0] + a[4+2]*b[0+1] + a[8+2]*b[0+2];
    res[0+3] = a[0+3]*b[0+0] + a[4+3]*b[0+1] + a[8+3]*b[0+2] + b[0+3];

    res[4+0] = a[0+0]*b[4+0] + a[4+0]*b[4+1] + a[8+0]*b[4+2];
    res[4+1] = a[0+1]*b[4+0] + a[4+1]*b[4+1] + a[8+1]*b[4+2];
    res[4+2] = a[0+2]*b[4+0] + a[4+2]*b[4+1] + a[8+2]*b[4+2];
    res[4+3] = a[0+3]*b[4+0] + a[4+3]*b[4+1] + a[8+3]*b[4+2] + b[4+3];

    res[8+0] = a[0+0]*b[8+0] + a[4+0]*b[8+1] + a[8+0]*b[8+2];
    res[8+1] = a[0+1]*b[8+0] + a[4+1]*b[8+1] + a[8+1]*b[8+2];
    res[8+2] = a[0+2]*b[8+0] + a[4+2]*b[8+1] + a[8+2]*b[8+2];
    res[8+3] = a[0+3]*b[8+0] + a[4+3]*b[8+1] + a[8+3]*b[8+2] + b[8+3];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CopyF34toD34
(
    double34		*dest,	// destination matrix
    const float34	*src	// source matrix
)
{
    DASSERT(dest);
    DASSERT(src);

    double *d = dest->v;
    const float *s = src->v;

    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = *s++;
}

///////////////////////////////////////////////////////////////////////////////

void CopyD34toF34
(
    float34		*dest,	// destination matrix
    const double34	*src	// source matrix
)
{
    DASSERT(dest);
    DASSERT(src);

    float *d = dest->v;
    const double *s = src->v;

    uint n;
    for ( n = 0; n < 12; n++ )
	*d++ = *s++;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MatrixD_t: setup		///////////////
///////////////////////////////////////////////////////////////////////////////

// statistics
u64 N_MatrixD_forward = 0;  // total number of forward transformations
u64 N_MatrixD_inverse = 0;  // total number of inverse transformations

///////////////////////////////////////////////////////////////////////////////

void InitializeMatrixD ( MatrixD_t * mat )
{
    DASSERT(mat);
//    const uint seqnum = mat->valid ? mat->sequence_number : 0;
    memset(mat,0,sizeof(*mat));
//    mat->sequence_number = seqnum;

    mat->scale.x	= mat->scale.y		= mat->scale.z		= 1.0;
    mat->scale_origin.x	= mat->scale_origin.y	= mat->scale_origin.z	= 0.0;

    mat->shift		= mat->scale_origin;
    mat->rotate_deg	= mat->scale_origin;
    mat->rotate_rad	= mat->scale_origin;
    mat->translate	= mat->scale_origin;

    double *d = (double*)mat->rotate_origin;
    uint n;
    for ( n = 0; n < 9; n++ )
	*d++ = 0.0;

    mat->valid = true;
}

///////////////////////////////////////////////////////////////////////////////

void CopyMatrixD
(
    MatrixD_t	*dest,		// valid destination
    const
      MatrixD_t	*src		// NULL or source
)
{
    DASSERT(dest);

    if ( src && src->valid )
    {
	const uint seqnum = dest->sequence_number;
	*dest = *src;
	dest->sequence_number = seqnum+1;
    }
    else
	InitializeMatrixD(dest);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetScaleMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* scale,	// NULL: clear scale, not NULL: set scale
    double3	* origin	// not NULL: Origin for scaling
)
{
    DASSERT(mat);
    if (!mat->valid)
	InitializeMatrixD(mat);
    mat->norm_valid = mat->tmatrix_valid = mat->imatrix_valid = false;

    if (scale)
	mat->scale = *scale;
    else
    {
	mat->scale.x = mat->scale.y = mat->scale.z = 1.0;
	origin = 0;
    }

    if (origin)
	mat->scale_origin = *origin;
    else
	mat->scale_origin.x = mat->scale_origin.y = mat->scale_origin.z = 0.0;

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetShiftMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* shift		// NULL: clear Shift, not NULL: set Shift
)
{
    DASSERT(mat);
    if (!mat->valid)
	InitializeMatrixD(mat);
    mat->norm_valid = mat->tmatrix_valid = mat->imatrix_valid = false;

    if (shift)
	mat->shift = *shift;
    else
	mat->shift.x = mat->shift.y = mat->shift.z = 0.0;

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetScaleShiftMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    uint	xyz,		// 0..2: x/y/z coordinate to set
    double	old1,		// old value of first point
    double	new1,		// new value of first point
    double	old2,		// old value of second point
    double	new2		// new value of second point
)
{
    DASSERT(mat);
    if (!mat->valid)
	InitializeMatrixD(mat);

    if ( fabs(old1-old2) >= NULL_EPSILON ) // ignore othwerwise
    {
	mat->norm_valid = mat->tmatrix_valid = mat->imatrix_valid = false;

	const double scale = ( new2 - new1 ) / ( old2 - old1 );
	mat->scale.v[xyz] = scale;
	mat->scale_origin.v[xyz] = 0.0;

	// new = old * scale + shift
	mat->shift.v[xyz] = new1 - old1 * scale;
    }

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetRotateMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    uint	xyz,		// 0..2: x/y/z coordinate to set
    double	degree,		// angle in degree
    double	radiant,	// angle in radiant
    double3	* origin	// not NULL: Origin for the rotation
)
{
    DASSERT(mat);
    DASSERT(xyz<3);

    if (!mat->valid)
	InitializeMatrixD(mat);
    mat->norm_valid = mat->tmatrix_valid = mat->imatrix_valid = false;

    mat->rotate_deg.v[xyz] = degree;
    mat->rotate_rad.v[xyz] = radiant;

    if (origin)
	mat->rotate_origin[xyz] = *origin;
    else
    {
	origin = mat->rotate_origin + xyz;
	origin->x = origin->y = origin->z = 0.0;
    }

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetTranslateMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* translate	// NULL: clear translate, not NULL: set translate
)
{
    DASSERT(mat);
    if (!mat->valid)
	InitializeMatrixD(mat);
    mat->norm_valid = mat->tmatrix_valid = mat->imatrix_valid = false;

    if (translate)
	mat->translate = *translate;
    else
	mat->translate.x = mat->translate.y = mat->translate.z = 0.0;

    return mat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

MatrixD_t * CheckStatusMatrixD
(
    MatrixD_t	* mat		// valid data structure
)
{
    DASSERT(mat);

    if (mat->tmatrix_valid)
    {
	double *t = mat->trans_matrix.v;

	//--- check rotation (only disabled for all axis)

	if ( mat->rotate_enabled & 1
	   && fabs( t[4+2] ) < NULL_EPSILON
	   && fabs( t[8+1] ) < NULL_EPSILON
	   )
	{
	   mat->rotate_enabled &= ~1;
	   t[4+2] = 0.0;
	   t[8+1] = 0.0;
	}

	if ( mat->rotate_enabled & 2
	   && fabs( t[  2] ) < NULL_EPSILON
	   && fabs( t[8+0] ) < NULL_EPSILON
	   )
	{
	   mat->rotate_enabled &= ~2;
	   t[  2] = 0.0;
	   t[8+0] = 0.0;
	}

	if ( mat->rotate_enabled & 4
	   && fabs( t[  1] ) < NULL_EPSILON
	   && fabs( t[4+0] ) < NULL_EPSILON
	   )
	{
	   mat->rotate_enabled &= ~4;
	   t[  1] = 0.0;
	   t[4+0] = 0.0;
	}


	//--- check scaling (don't touch scale if roating is enabled)

	if ( mat->scale_enabled & 1
	    && !( mat->rotate_enabled & ~1 )
	    && fabs( t[  0] - 1.0 ) < NULL_EPSILON )
	{
	    t[  0] = 1.0;
	    mat->scale_enabled &= ~1;
	}

	if ( mat->scale_enabled & 2
	    && !( mat->rotate_enabled & ~2 )
	    && fabs( t[4+1] - 1.0 ) < NULL_EPSILON )
	{
	    t[4+1] = 1.0;
	    mat->scale_enabled &= ~2;
	}

	if ( mat->scale_enabled & 4
	    && !( mat->rotate_enabled & ~4 )
	    && fabs( t[8+2] - 1.0 ) < NULL_EPSILON )
	{
	    t[8+2] = 1.0;
	    mat->scale_enabled &= ~4;
	}


	//--- check transformation

	if ( mat->translate_enabled & 1 && fabs( t[  3] ) < NULL_EPSILON )
	{
	    t[  3] = 0.0;
	    mat->translate_enabled &= ~1;
	}

	if ( mat->translate_enabled & 2 && fabs( t[4+3] ) < NULL_EPSILON )
	{
	    t[4+3] = 0.0;
	    mat->translate_enabled &= ~2;
	}

	if ( mat->translate_enabled & 4 && fabs( t[8+3] ) < NULL_EPSILON )
	{
	    t[8+3] = 0.0;
	    mat->translate_enabled &= ~4;
	}

	mat->transform_enabled	= mat->scale_enabled
				| mat->rotate_enabled
				| mat->translate_enabled
				| mat->use_matrix << 3;
    }

    return mat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MatrixD_t: specials		///////////////
///////////////////////////////////////////////////////////////////////////////

void TermAssignMatrixD
(
    // called after dirext matrix manipulations
    MatrixD_t	* mat		// valid data structure
)
{
    DASSERT(mat);

    mat->valid			= true;
    mat->norm_valid		= false;
    mat->tmatrix_valid		= true;
    mat->imatrix_valid		= false;
    mat->use_matrix		= 2;
    mat->scale_enabled		= 7;
    mat->rotate_enabled		= 7;
    mat->translate_enabled	= 7;
    mat->sequence_number++;

    double34 *m			= &mat->trans_matrix;
    mat->norm_scale.x		= m->x[0];
    mat->norm_scale.y		= m->y[1];
    mat->norm_scale.z		= m->z[2];
    mat->norm_translate.x	= m->x[3];
    mat->norm_translate.y	= m->y[3];
    mat->norm_translate.z	= m->z[3];

    CheckStatusMatrixD(mat);

    if (!mat->rotate_enabled)
    {
	mat->use_matrix		= 0;
	mat->norm_valid		= true;
	mat->norm_rotate_deg.x	= 0;
	mat->norm_rotate_deg.y	= 0;
	mat->norm_rotate_deg.z	= 0;
	mat->norm_rotate_rad	= mat->norm_rotate_deg;

	mat->scale		= mat->norm_scale;
	mat->translate		= mat->norm_translate;
    }
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
     double34	* src		// NULL or matrix to set
)
{
    DASSERT(mat);
    InitializeMatrixD(mat);
    if (src)
    {
	mat->trans_matrix = *src;
	TermAssignMatrixD(mat);
    }
    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetAScaleMatrixD
(
    // calculate the matrix as rotation around a specified axis

    MatrixD_t	* mat,		// valid data structure
    double	scale,		// scale factor
    double3	* dir		// NULL or scaling direction as vector
)
{
    DASSERT(mat);
    InitializeMatrixD(mat);

    if (dir)
    {
	const double hrot = atan2(dir->x,dir->z);
	const double hlen = sqrt( dir->x*dir->x + dir->z*dir->z );
	const double vrot = -atan2(dir->y,hlen);

	PRINT("HROT: %6.4f %7.2f / %11.3f\n",hrot,hrot*180.0/M_PI,hlen);
	PRINT("VROT: %6.4f %7.2f\n",vrot,vrot*180.0/M_PI);

	MatrixD_t mrot;
	InitializeMatrixD(&mrot);
	mrot.rotate_rad.x = vrot;
	mrot.rotate_rad.y = hrot;
	CalcMatrixD(&mrot,true);
	DASSERT(mrot.imatrix_valid);
	SetMatrixD(mat,&mrot.inv_matrix);

	double3 scale3;
	scale3.x = scale3.y = 0;
	scale3.z = scale;
	SetScaleMatrixD(&mrot,&scale3,0);
	MultiplyMatrixD(mat,0,&mrot,0);
	TermAssignMatrixD(mat);
    }

    //--- terminate

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * SetARotateMatrixD
(
    // calculate the matrix as rotation around a specified axis

    MatrixD_t	* mat,		// valid data structure
    double	degree,		// angle in degree
    double	radiant,	// angle in radiant, add to degree with factor
    double3	* pt1,		// point #1 to define the axis; if NULL use (0,0,0)
    double3	* pt2		// point #2 to define the axis; if NULL use (0,0,0)
)
{
    DASSERT(mat);
    InitializeMatrixD(mat);

    double3 dir, base;
    if (pt1)
	base = *pt1;
    else
	base.x = base.y = base.z = 0.0;

    if (pt2)
	Sub3(dir,*pt2,base);
    else
    {
	dir.x = -base.x;
	dir.y = -base.y;
	dir.z = -base.z;
    }

 #if 1
    radiant += degree * (M_PI/180.0);
    const double len = Length3(dir);
    if ( len >= NULL_EPSILON )
 #else
    radiant = fmod( degree * (M_PI/180.0) + radiant + M_PI, 2*M_PI) - M_PI;
    const double len = Length3(dir);
    if ( len >= NULL_EPSILON && fabs(radiant) >= NULL_EPSILON )
 #endif
    {
	dir.x /= len;
	dir.y /= len;
	dir.z /= len;

	const double sin0 = sin(radiant);
	const double cos0 = cos(radiant);
	const double cos1 = 1.0 - cos0;

	double *t = mat->trans_matrix.v;

	t[  0] = dir.x * dir.x * cos1 +         cos0;
	t[  1] = dir.x * dir.y * cos1 - dir.z * sin0;
	t[  2] = dir.x * dir.z * cos1 + dir.y * sin0;

	t[4+0] = dir.y * dir.x * cos1 + dir.z * sin0;
	t[4+1] = dir.y * dir.y * cos1 +         cos0;
	t[4+2] = dir.y * dir.z * cos1 - dir.x * sin0;

	t[8+0] = dir.z * dir.x * cos1 - dir.y * sin0;
	t[8+1] = dir.z * dir.y * cos1 + dir.x * sin0;
	t[8+2] = dir.z * dir.z * cos1 +         cos0;

	double3 delta = TransformD3D34(&mat->trans_matrix,&base);
	t[  3] = base.x - delta.x;
	t[4+3] = base.y - delta.y;
	t[8+3] = base.z - delta.z;

	TermAssignMatrixD(mat);
    }

    //--- terminate

    return mat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MatrixD_t: calc			///////////////
///////////////////////////////////////////////////////////////////////////////

MatrixD_t * CalcNormMatrixD
(
    MatrixD_t	* mat		// valid data structure
)
{
    DASSERT(mat);
    if (!mat->valid)
	InitializeMatrixD(mat);

    if ( mat->norm_valid || mat->tmatrix_valid )
	return mat;

    mat->sequence_number++;
    mat->norm_valid = true;
    mat->scale_enabled = mat->rotate_enabled = mat->translate_enabled = 0;


    //--- scale & shift

    double3 trans;

    uint xyz;
    for ( xyz = 0; xyz < 3; xyz++ )
    {
	double scale = mat->scale.v[xyz];
	if ( fabs(scale) < NULL_EPSILON || fabs(scale-1.0) < NULL_EPSILON )
	    mat->scale.v[xyz] = scale = 1.0;
	else
	    mat->scale_enabled |= 1 << xyz;

	mat->norm_scale.v[xyz] = scale;

	trans.v[xyz] = mat->shift.v[xyz]
			+ ( 1.0 - scale ) * mat->scale_origin.v[xyz];
    }


    //--- rotate

    mat->norm_pos2d.x = mat->norm_pos2d.y = mat->norm_pos2d.z = 0;

    for ( xyz = 0; xyz < 3; xyz++ )
    {
	double deg = fmod( mat->rotate_deg.v[xyz]
			   + mat->rotate_rad.v[xyz] * (180.0/M_PI)
			   + 180.0, 360.0 ) - 180.0;
	if ( fabs(deg) < MIN_DEGREE )
	{
	    mat->norm_rotate_deg.v[xyz] = 0.0;
	    mat->norm_rotate_rad.v[xyz] = 0.0;
	}
	else
	{
	    mat->rotate_enabled |= 1 << xyz;
	    mat->norm_rotate_deg.v[xyz] = deg;
	    double rad = deg * (M_PI/180.0);
	    mat->norm_rotate_rad.v[xyz] = rad;

	    // translate = rot_origin + rotated(translate-rot_origin)

	    Sub3(trans,trans,mat->rotate_origin[xyz]);

	    const uint a = (xyz+2) % 3;
	    const uint b = (xyz+1) % 3;
	    rad += atan2(trans.v[a],trans.v[b]);
	    const double len = sqrt(trans.v[a]*trans.v[a]+trans.v[b]*trans.v[b]);

	    trans.v[a] = sin(rad) * len;
	    trans.v[b] = cos(rad) * len;

	    Add3(trans,trans,mat->rotate_origin[xyz]);

	    mat->norm_pos2d.v[a] += mat->rotate_origin[xyz].v[a];
	    mat->norm_pos2d.v[b] += mat->rotate_origin[xyz].v[b];
	}
    }

    if ( (mat->rotate_enabled|1) == 7 ) mat->norm_pos2d.x /= 2;
    if ( (mat->rotate_enabled|2) == 7 ) mat->norm_pos2d.y /= 2;
    if ( (mat->rotate_enabled|4) == 7 ) mat->norm_pos2d.z /= 2;


    //--- translate

    mat->translate_enabled = false;

    for ( xyz = 0; xyz < 3; xyz++ )
    {
	const double temp = trans.v[xyz] + mat->translate.v[xyz];
	if ( fabs(temp) < NULL_EPSILON )
	    mat->norm_translate.v[xyz] = 0.0;
	else
	{
	     mat->norm_translate.v[xyz] = temp;
	     mat->translate_enabled |= 1 << xyz;
	}
    }


    //--- status

    if ( mat->use_matrix < 2 )
    {
	mat->use_matrix = mat->rotate_enabled ? 1 : 0;
	mat->tmatrix_valid = mat->imatrix_valid = false;
    }

    mat->transform_enabled = mat->scale_enabled
			   | mat->rotate_enabled
			   | mat->translate_enabled
			   | mat->use_matrix << 3;

    TRACE("%%%%%%%%%% CalcNormMatrixD(%p)    => %u|%u|%u = %02x %%%%%%%%%%\n",
	mat, mat->scale_enabled, mat->rotate_enabled,
	mat->translate_enabled, mat->transform_enabled );

    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * CalcTransMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc transformation matrix only
				//        if needed for transformation
				// true:  calc matrix always
				// if 'tmatrix_valid', the matrix is never calculated
)
{
    DASSERT(mat);
    CalcNormMatrixD(mat);

    if ( mat->tmatrix_valid || !always && !mat->use_matrix )
	return mat;

    TRACE("%%%%%%%%%% CalcTransMatrixD(%p,%d) => %u|%u|%u = %02x %%%%%%%%%%\n",
	mat, always, mat->scale_enabled, mat->rotate_enabled,
	mat->translate_enabled, mat->transform_enabled );
    DASSERT(mat->use_matrix<2);

    SetupMatrixD34( &mat->trans_matrix,
		    &mat->norm_scale,
		    &mat->norm_rotate_rad,
		    &mat->norm_translate );

    mat->tmatrix_valid = true;
    mat->imatrix_valid = false;
    CheckStatusMatrixD(mat);
    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * CalcMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc both matrices only
				//        if needed for transformation
				// true:  calc matrices always
)
{
    DASSERT(mat);

    if ( mat->imatrix_valid || !always && !mat->use_matrix )
	return mat;

    CalcTransMatrixD(mat,true);

    TRACE("%%%%%%%%%% CalcMatrixD     (%p,%d) => %u|%u|%u = %02x %%%%%%%%%%\n",
	mat, always, mat->scale_enabled, mat->rotate_enabled,
	mat->translate_enabled, mat->transform_enabled );

    CalcInverseMatrixD34(&mat->inv_matrix,&mat->trans_matrix);
    mat->imatrix_valid = true;
    return mat;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * MultiplyMatrixD
(
    MatrixD_t	*dest,		// valid, store result here, may be 'src1' or 'src2'
    MatrixD_t	*src1,		// first valid source, if NULL use dest
    MatrixD_t	*src2,		// second valid source, if NULL use dest
    uint	inverse_mode	// bit field:
				//  1: use inverse matrix of src1
				//  2: use inverse matrix of src2
)
{
    DASSERT(dest);

    if ( dest == src1 || dest == src2 || !src1 || !src2 )
    {
	MatrixD_t temp;
	MultiplyMatrixD( &temp, src1 ? src1 : dest, src2 ? src2 : dest, inverse_mode );
	*dest = temp;
	return dest;
    }
    DASSERT( src1 && src1 != dest );
    DASSERT( src2 && src2 != dest );

    CalcMatrixD(src1,true);
    CalcMatrixD(src2,true);
    InitializeMatrixD(dest);
    MultiplyD34( &dest->trans_matrix,
		inverse_mode & 1 ? &src1->inv_matrix : &src1->trans_matrix,
		inverse_mode & 2 ? &src2->inv_matrix : &src2->trans_matrix );


    //--- setup normals, simply assumption

    dest->norm_scale.x = dest->trans_matrix.m[0][0];
    dest->norm_scale.y = dest->trans_matrix.m[1][1];
    dest->norm_scale.z = dest->trans_matrix.m[2][2];

    dest->norm_rotate_deg.x = src1->norm_rotate_deg.x + src2->norm_rotate_deg.x;
    dest->norm_rotate_deg.y = src1->norm_rotate_deg.y + src2->norm_rotate_deg.y;
    dest->norm_rotate_deg.z = src1->norm_rotate_deg.z + src2->norm_rotate_deg.z;
    Mult3f(dest->norm_rotate_rad,dest->norm_rotate_deg,M_PI/180.0);

    dest->norm_translate.x = dest->trans_matrix.m[0][3];
    dest->norm_translate.y = dest->trans_matrix.m[1][3];
    dest->norm_translate.z = dest->trans_matrix.m[2][3];


    //--- setup base values

    dest->scale	     = dest->norm_scale;
    dest->rotate_deg = dest->norm_rotate_deg;
    dest->translate  = dest->norm_translate;


    //--- terminate

    TermAssignMatrixD(dest);
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

MatrixD_t * CalcVectorsMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc vectors only if matrix is valid
				// true:  calc matrix if invalid & vectors always
)
{
    DASSERT(mat);
    // search: Decomposing a rotation matrix

    if ( !always && !mat->tmatrix_valid )
	return mat;

    CalcMatrixD(mat,false);

    // [[2do]] ??? calculate scale
    //mat->norm_scale.x = 1.0;
    //mat->norm_scale.y = 1.0;
    //mat->norm_scale.z = 1.0;

    double (*m)[4] = mat->trans_matrix.m;

    // [[2do]] ??? works only for scale == 1 ==>
    mat->norm_rotate_rad.x = atan2( m[2][1], m[2][2] );
    mat->norm_rotate_rad.y = atan2( -m[2][0], sqrt( m[2][1]*m[2][1] + m[2][2]*m[2][2] ) );
    mat->norm_rotate_rad.z = atan2( m[1][0], m[0][0] );

    mat->norm_rotate_deg.x = rad2deg(mat->norm_rotate_rad.x);
    mat->norm_rotate_deg.y = rad2deg(mat->norm_rotate_rad.y);
    mat->norm_rotate_deg.z = rad2deg(mat->norm_rotate_rad.z);

    mat->norm_translate.x = m[0][3];
    mat->norm_translate.y = m[1][3];
    mat->norm_translate.z = m[2][3];

    return mat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		MatrixD_t: double transform		///////////////
///////////////////////////////////////////////////////////////////////////////

double3 TransformD3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    N_MatrixD_forward++;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->tmatrix_valid)
	    CalcTransMatrixD(mat,true);
	return TransformD3D34(&mat->trans_matrix,val);
    }

    double3 res;
    res.x = val->x * mat->norm_scale.x + mat->norm_translate.x;
    res.y = val->y * mat->norm_scale.y + mat->norm_translate.y;
    res.z = val->z * mat->norm_scale.z + mat->norm_translate.z;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

double3 InvTransformD3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    N_MatrixD_inverse++;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->imatrix_valid)
	    CalcMatrixD(mat,true);
	return TransformD3D34(&mat->inv_matrix,val);
    }

    double3 res;
    res.x = ( val->x - mat->norm_translate.x ) / mat->norm_scale.x;
    res.y = ( val->y - mat->norm_translate.y ) / mat->norm_scale.y;
    res.z = ( val->z - mat->norm_translate.z ) / mat->norm_scale.z;
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TransformD3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
)
{
    DASSERT(mat);
    DASSERT(val);
    DASSERT( n <= 1 || off >= sizeof(*val) );

    N_MatrixD_forward += n;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->tmatrix_valid)
	    CalcTransMatrixD(mat,true);

	while ( n-- > 0 )
	{
	    *val = TransformD3D34(&mat->trans_matrix,val);
	    val = (double3*)( (u8*)val + off );
	}
    }
    else if (mat->transform_enabled)
    {
	while ( n-- > 0 )
	{
	    val->x = val->x * mat->norm_scale.x + mat->norm_translate.x;
	    val->y = val->y * mat->norm_scale.y + mat->norm_translate.y;
	    val->z = val->z * mat->norm_scale.z + mat->norm_translate.z;
	    val = (double3*)( (u8*)val + off );
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void InvTransformD3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
)
{
    DASSERT(mat);
    DASSERT(val);
    DASSERT( n <= 1 || off >= sizeof(*val) );

    N_MatrixD_inverse += n;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->imatrix_valid)
	    CalcMatrixD(mat,true);

	while ( n-- > 0 )
	{
	    *val = TransformD3D34(&mat->inv_matrix,val);
	    val = (double3*)( (u8*)val + off );
	}
    }
    else if (mat->transform_enabled)
    {
	double3 f; // multiplication is faster than division
	f.x = 1.0 / mat->norm_scale.x;
	f.y = 1.0 / mat->norm_scale.y;
	f.z = 1.0 / mat->norm_scale.z;

	while ( n-- > 0 )
	{
	    val->x = ( val->x - mat->norm_translate.x ) * f.x;
	    val->y = ( val->y - mat->norm_translate.y ) * f.y;
	    val->z = ( val->z - mat->norm_translate.z ) * f.z;
	    val = (double3*)( (u8*)val + off );
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		MatrixD_t: float transform		///////////////
///////////////////////////////////////////////////////////////////////////////

float3 TransformF3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      float3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    N_MatrixD_forward++;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->tmatrix_valid)
	    CalcTransMatrixD(mat,true);
	return TransformF3D34(&mat->trans_matrix,val);
    }

    float3 res;
    res.x = val->x * mat->norm_scale.x + mat->norm_translate.x;
    res.y = val->y * mat->norm_scale.y + mat->norm_translate.y;
    res.z = val->z * mat->norm_scale.z + mat->norm_translate.z;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

float3 InvTransformF3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      float3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    N_MatrixD_inverse++;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->imatrix_valid)
	    CalcMatrixD(mat,true);
	return TransformF3D34(&mat->inv_matrix,val);
    }

    float3 res;
    res.x = ( val->x - mat->norm_translate.x ) / mat->norm_scale.x;
    res.y = ( val->y - mat->norm_translate.y ) / mat->norm_scale.y;
    res.z = ( val->z - mat->norm_translate.z ) / mat->norm_scale.z;
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TransformF3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    float3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
)
{
    DASSERT(mat);
    DASSERT(val);
    DASSERT_MSG( n <= 1 || off >= sizeof(*val), "n=%u,off=%u\n",n,off );

    N_MatrixD_forward += n;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->tmatrix_valid)
	    CalcTransMatrixD(mat,true);

	while ( n-- > 0 )
	{
	    *val = TransformF3D34(&mat->trans_matrix,val);
	    val = (float3*)( (u8*)val + off );
	}
    }
    else if (mat->transform_enabled)
    {
	while ( n-- > 0 )
	{
	    val->x = val->x * mat->norm_scale.x + mat->norm_translate.x;
	    val->y = val->y * mat->norm_scale.y + mat->norm_translate.y;
	    val->z = val->z * mat->norm_scale.z + mat->norm_translate.z;
	    val = (float3*)( (u8*)val + off );
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void InvTransformF3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    float3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
)
{
    DASSERT(mat);
    DASSERT(val);
    DASSERT( n <= 1 || off >= sizeof(*val) );

    N_MatrixD_inverse += n;

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if (mat->use_matrix)
    {
	if (!mat->imatrix_valid)
	    CalcMatrixD(mat,true);

	while ( n-- > 0 )
	{
	    *val = TransformF3D34(&mat->inv_matrix,val);
	    val = (float3*)( (u8*)val + off );
	}
    }
    else if (mat->transform_enabled)
    {
	double3 f; // multiplication is faster than division
	f.x = 1.0 / mat->norm_scale.x;
	f.y = 1.0 / mat->norm_scale.y;
	f.z = 1.0 / mat->norm_scale.z;

	while ( n-- > 0 )
	{
	    val->x = ( val->x - mat->norm_translate.x ) * f.x;
	    val->y = ( val->y - mat->norm_translate.y ) * f.y;
	    val->z = ( val->z - mat->norm_translate.z ) * f.z;
	    val = (float3*)( (u8*)val + off );
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		MatrixD_t: direct transform		///////////////
///////////////////////////////////////////////////////////////////////////////

double3 BaseTransformD3MatrixD
(
    // transform using the base parameters

    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    if (!mat->valid)
	InitializeMatrixD(mat);

    if ( mat->use_matrix > 1 )
	return TransformD3MatrixD(mat,val);

    double3 rot = mat->rotate_rad;
    Mult3f(rot,rot,180.0/M_PI);
    Add3(rot,rot,mat->rotate_deg);

    return TransformHelperD3MatrixD
	   (	&mat->scale,
		&mat->scale_origin,
		&mat->shift,
		&rot,
		mat->rotate_origin+0,
		mat->rotate_origin+1,
		mat->rotate_origin+2,
		&mat->translate,
		val
	   );
}

///////////////////////////////////////////////////////////////////////////////

double3 NormTransformD3MatrixD
(
    // transform using the norm parameters

    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
)
{
    DASSERT(mat);
    DASSERT(val);

    if ( !mat->norm_valid && !mat->tmatrix_valid )
	CalcNormMatrixD(mat);

    if ( mat->use_matrix > 1 )
	return TransformD3MatrixD(mat,val);

    return TransformHelperD3MatrixD
	   (	&mat->norm_scale,
		0,
		0,
		&mat->norm_rotate_deg,
		0,
		0,
		0,
		&mat->norm_translate,
		val
	   );
}

///////////////////////////////////////////////////////////////////////////////

double3 TransformHelperD3MatrixD
(
    // transform using vectors

    double3	*scale,		// NULL or scale vector
    double3	*scale_origin,	// NULL or origin for scaling
    double3	*shift,		// NULL or shift vector
    double3	*rotate_deg,	// NULL or rotation in degree
    double3	*xrot_origin,	// NULL or origin for x-rotation
    double3	*yrot_origin,	// NULL or origin for y-rotation
    double3	*zrot_origin,	// NULL or origin for z-rotation
    double3	*translate,	// NULL or translation vector

    const
      double3	* val		// value to transform
)
{
    DASSERT(val);

    double3 res; //, null3 = { 0.0, 0.0, 0.0 };


    //--- scale

    if (!scale)
	res = *val;
    else if (scale_origin)
    {
	res.x = ( val->x - scale_origin->x ) * scale->x + scale_origin->x;
	res.y = ( val->y - scale_origin->y ) * scale->y + scale_origin->y;
	res.z = ( val->z - scale_origin->z ) * scale->z + scale_origin->z;
    }
    else
    {
	res.x = val->x * scale->x;
	res.y = val->y * scale->y;
	res.z = val->z * scale->z;
    }


    //--- shift

    if (shift)
    {
	res.x += shift->x;
	res.y += shift->y;
	res.z += shift->z;
    }


    //--- rotate

    if (rotate_deg)
    {
	uint xyz;
	for ( xyz = 0; xyz < 3; xyz++ )
	{
	    const double deg = fmod(rotate_deg->v[xyz]+180.0,360.0) - 180.0;
	    if ( fabs(deg) >= MIN_DEGREE )
	    {
		uint a = xyz + 2; if ( a >= 3 ) a -= 3;
		uint b = xyz + 1; if ( b >= 3 ) b -= 3;
		double rad = deg * (M_PI/180.0);

		if (xrot_origin)
		{
		    const double da = res.v[a] - xrot_origin->v[a];
		    const double db = res.v[b] - xrot_origin->v[b];
		    rad += atan2(da,db);
		    const double len = sqrt(da*da+db*db);

		    res.v[a] = sin(rad) * len + xrot_origin->v[a];
		    res.v[b] = cos(rad) * len + xrot_origin->v[b];
		}
		else
		{
		    rad += atan2(res.v[a],res.v[b]);
		    const double len = sqrt(res.v[a]*res.v[a]+res.v[b]*res.v[b]);

		    res.v[a] = sin(rad) * len;
		    res.v[b] = cos(rad) * len;
		}
	    }
	    xrot_origin = yrot_origin;
	    yrot_origin = zrot_origin;
	}
    }


    //--- translate

    if (translate)
    {
	res.x += translate->x;
	res.y += translate->y;
	res.z += translate->z;
    }

    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    MatrixD_t: printing			///////////////
///////////////////////////////////////////////////////////////////////////////

static ccp print_matrix_stat
	( MatrixD_t * mat, char * buf, uint bufsize, bool for_inverse )
{
    DASSERT(buf);
    DASSERT(bufsize>10);

    char *dest = buf, *bufend = buf + bufsize;
    if (mat->scale_enabled)
	dest = StringCopyE(dest,bufend,"+scale");
    if (mat->rotate_enabled)
	dest = StringCopyE(dest,bufend,"+rotate");
    if (mat->translate_enabled)
	dest = StringCopyE(dest,bufend,"+translate");

    if ( mat->use_matrix > 1 && !for_inverse )
	dest = StringCopyE(dest,bufend,",source");
    else if (mat->use_matrix)
	dest = StringCopyE(dest,bufend,",required");

    if ( dest == buf )
	StringCopyE(dest,bufend,"+no transformation");

    return buf+1;
}

///////////////////////////////////////////////////////////////////////////////

void PrintMatrixD
(
    FILE	* f,		// file to print to
    uint	indent,		// indention
    ccp		eol,		// not NULL: print as 'end of line'

    MatrixD_t	* mat,		// valid data structure
    uint	create,		// bitfield for setup:
				//     1: create normals if invalid
				//     2: create transform matrix if invalid
				//     4: create inverse matrix if invalid
    uint	print		// bitfield print selection:
				//     1: print base vectors
				//     2: print normalized values, if valid
				//     4: print transformation matrix, if valid
				//     8: print inverse matrix, if valid
				//  0x10: print also header, if only 1 section is print
				//  0x20: print all, don't suppress NULL vectors
				//  0x40: print an additional EOL behind each section
)
{
    DASSERT(f);
    DASSERT(mat);

    //--- setup

    indent = NormalizeIndent(indent);
    if (!eol)
	eol = "\n";
    ccp sep = print & 0x40 ? eol : "";

    if ( create & 4 )
	CalcMatrixD(mat,true);
    else if ( create & 2 )
	CalcTransMatrixD(mat,true);
    else if ( create & 1 )
	CalcNormMatrixD(mat);
    else if (!mat->valid)
	InitializeMatrixD(mat);

    if (!mat->norm_valid)
	print &= ~2;
    if (!mat->tmatrix_valid)
	print &= ~4;
    else if (mat->use_matrix>1)
	print &= ~1;
    if (!mat->imatrix_valid)
	print &= ~8;

    const uint print_all = print & 0x20;
    uint print_head = print & 0x10;
    print &= 0x0f;
    if ( print != 1 && print != 2 && print != 4 && print != 8 )
	print_head = 1;


    //--- print base vectors

    if ( print & 1 )
    {
	if (print_head)
	    fprintf(f,"%*sBase Vectors:%s", indent,"", eol );

	if ( print_all || !IsOne3(mat->scale) )
	    fprintf(f,"%*s  Scale:      %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->scale.x, mat->scale.y, mat->scale.z,
		eol );

	if ( print_all || !IsNull3(mat->scale_origin) )
	    fprintf(f,"%*s    Origin:   %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->scale_origin.x, mat->scale_origin.y, mat->scale_origin.z,
		eol );

	if ( print_all || !IsNull3(mat->shift) )
	    fprintf(f,"%*s  Shift:      %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->shift.x, mat->shift.y, mat->shift.z,
		eol );

	bool print_rot = false;
	if ( print_all || !IsNull3(mat->rotate_deg) )
	{
	    print_rot = true;
	    fprintf(f,"%*s  Rotate/deg: %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->rotate_deg.x, mat->rotate_deg.y, mat->rotate_deg.z,
		eol );
	}

	if ( print_all || !IsNull3(mat->rotate_rad) )
	{
	    print_rot = true;
	    fprintf(f,"%*s  Rotate/rad: %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->rotate_rad.x, mat->rotate_rad.y, mat->rotate_rad.z,
		eol );
	}

	if (print_rot)
	{
	    double3 *d = mat->rotate_origin;
	    uint xyz;
	    for ( xyz = 0; xyz < 3; xyz++, d++ )
		if ( print_all || !IsNull3(*d) )
		    fprintf(f,"%*s    %c-origin: %11.3f %11.3f %11.3f%s",
			indent,"", 'x'+xyz,
			d->x, d->y, d->z,
			eol );
	}

	if ( print_all || !IsNull3(mat->translate) )
	    fprintf(f,"%*s  Translate:  %11.3f %11.3f %11.3f%s",
		indent,"",
		mat->translate.x, mat->translate.y, mat->translate.z,
		eol );

	fputs(sep,f);
    }


    //--- print normalized vectors

    if ( print & 2 )
    {
	if (print_head)
	    fprintf(f,"%*sNormalized Vectors (seq=%u):%s",
		indent,"", mat->sequence_number, eol );

	fprintf(f,
	    "%*s  Scale:      %11.3f %11.3f %11.3f  [%c%c%c]%s"
	    "%*s  Rotate/deg: %11.3f %11.3f %11.3f  [%c%c%c]%s"
 #if TEST0
	    "%*s  Rotate/rad: %11.3f %11.3f %11.3f%s"
 #endif
	    "%*s  Translate:  %11.3f %11.3f %11.3f  [%c%c%c]%s",

	    indent,"",
	    mat->norm_scale.x, mat->norm_scale.y, mat->norm_scale.z,
	    mat->scale_enabled & 1 ? 'x' : '-',
	    mat->scale_enabled & 2 ? 'y' : '-',
	    mat->scale_enabled & 4 ? 'z' : '-',
	    eol,

	    indent,"",
	    mat->norm_rotate_deg.x, mat->norm_rotate_deg.y, mat->norm_rotate_deg.z,
	    mat->rotate_enabled & 1 ? 'x' : '-',
	    mat->rotate_enabled & 2 ? 'y' : '-',
	    mat->rotate_enabled & 4 ? 'z' : '-',
	    eol,
 #if TEST0
	    indent,"",
	    mat->norm_rotate_rad.x, mat->norm_rotate_rad.y, mat->norm_rotate_rad.z,
	    eol,
 #endif
	    indent,"",
	    mat->norm_translate.x, mat->norm_translate.y, mat->norm_translate.z,
	    mat->translate_enabled & 1 ? 'x' : '-',
	    mat->translate_enabled & 2 ? 'y' : '-',
	    mat->translate_enabled & 4 ? 'z' : '-',
	    eol );

	if ( print_all || !IsNull3(mat->norm_pos2d) )
	    fprintf(f,"%*s  2D tf-base: %11.3f %11.3f %11.3f       (for 2D transform)%s",
		indent,"",
		mat->norm_pos2d.x, mat->norm_pos2d.y, mat->norm_pos2d.z,
		eol );

	fputs(sep,f);
    }


    //--- print transformation matrix

    char buf[50];
    if ( print & 4 )
    {
	if (print_head)
	    fprintf(f,"%*sTransformation Matrix (%s):%s",
		indent,"", print_matrix_stat(mat,buf,sizeof(buf),false), eol );

	fprintf(f,
	    "%*s  x' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%*s  y' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%*s  z' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%s",

	    indent,"",
		mat->trans_matrix.x[0],
		mat->trans_matrix.x[1],
		mat->trans_matrix.x[2],
		mat->trans_matrix.x[3],
	    eol,

	    indent,"",
		mat->trans_matrix.y[0],
		mat->trans_matrix.y[1],
		mat->trans_matrix.y[2],
		mat->trans_matrix.y[3],
	    eol,

	    indent,"",
		mat->trans_matrix.z[0],
		mat->trans_matrix.z[1],
		mat->trans_matrix.z[2],
		mat->trans_matrix.z[3],
	    eol, sep );
    }


    //--- print inverse matrix

    if ( print & 8 )
    {
	if (print_head)
	    fprintf(f,"%*sInverse Matrix (%s):%s",
			indent,"", print_matrix_stat(mat,buf,sizeof(buf),true), eol );

	fprintf(f,
	    "%*s  x' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%*s  y' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%*s  z' = %11.3f * x + %11.3f * y + %11.3f * z + %11.3f%s"
	    "%s",

	    indent,"",
		mat->inv_matrix.x[0],
		mat->inv_matrix.x[1],
		mat->inv_matrix.x[2],
		mat->inv_matrix.x[3],
	    eol,

	    indent,"",
		mat->inv_matrix.y[0],
		mat->inv_matrix.y[1],
		mat->inv_matrix.y[2],
		mat->inv_matrix.y[3],
	    eol,

	    indent,"",
		mat->inv_matrix.z[0],
		mat->inv_matrix.z[1],
		mat->inv_matrix.z[2],
		mat->inv_matrix.z[3],
	    eol, sep );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

