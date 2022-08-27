
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

#ifndef DCLIB_NUMERIC_H
#define DCLIB_NUMERIC_H 1

#include "dclib-types.h"
#include "dclib-debug.h"
#include <math.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			float vector			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[float3]]

typedef struct float3
{
    union
    {
	u32   u[3];
	float v[3];
	struct
	{
	    float x;
	    float y;
	    float z;
	}
	__attribute__ ((packed));
    }
    __attribute__ ((packed));
}
__attribute__ ((packed)) float3;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[float3List_t]]

typedef struct float3List_t
{
    float3	*list;		// list of data
    uint	*sort;		// NULL or 'size' alloced elements
    uint	used;		// used elements of 'list'
    uint	size;		// alloced elements of 'list'

} float3List_t;

//-----------------------------------------------------------------------------

void InitializeF3L
(
    float3List_t *f3l,		// valid data
    uint	 bin_search	// >0: enable binary search
				// and allocate 'bin_search' elements
);

void ResetF3L ( float3List_t * f3l );
float3 * AppendF3L ( float3List_t * f3l );
uint FindInsertFloatF3L ( float3List_t * f3l, float3 *val, bool fast );
float3 * GrowF3L ( float3List_t * f3l, uint n );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			double vector			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[double3]]

typedef struct double3
{
    union
    {
	u64    u[3];
	double v[3];
	struct
	{
	    double x;
	    double y;
	    double z;
	};
    };

} double3;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[double3List_t]]

typedef struct double3List_t
{
    double3	*list;		// list of data
    uint	used;		// used elements of 'list'
    uint	size;		// alloced elements of 'list'

} double3List_t;

//-----------------------------------------------------------------------------

void InitializeD3L ( double3List_t * d3l );
void ResetD3L ( double3List_t * d3l );
double3 * AppendD3L ( double3List_t * d3l );
uint FindInsertFloatD3L ( double3List_t * d3l, double3 *val, bool fast );
double3 * GrowD3L ( double3List_t * d3l, uint n );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			2D vector macros		///////////////
///////////////////////////////////////////////////////////////////////////////

#define SideOfLine(ax,ay,bx,by,px,py) \
	( ((bx)-(ax)) * ((py)-(ay)) - ((by)-(ay)) * ((px)-(ax)) )
#define SideOfLineXY(a,b,p) \
	( ((b).x-(a).x) * ((p).y-(a).y) - ((b).y-(a).y) * ((p).x-(a).x) )
#define SideOfLineXZ(a,b,p) \
	( ((b).x-(a).x) * ((p).z-(a).z) - ((b).z-(a).z) * ((p).x-(a).x) )
#define SideOfLineYZ(a,b,p) \
	( ((b).y-(a).y) * ((p).z-(a).z) - ((b).z-(a).z) * ((p).y-(a).y) )

//
///////////////////////////////////////////////////////////////////////////////
///////////////			3D vector macros		///////////////
///////////////////////////////////////////////////////////////////////////////

#define NULL_EPSILON 1e-9 // 'fabs(value) < NULL_EPSILON' is 0.0
#define MIN_DEGREE   1e-4 // 'fabs(value) < MIN_DEGREE' is 0.0

#define Assign3(d,s) do { (d).x = (s).x; (d).y = (s).y; (d).z = (s).z; } while (0)

#define IsNull3(d) ( (d).x == 0.0 && (d).y == 0.0 && (d).z == 0.0 )
#define IsNull3e(d,e) ( fabs((d).x) < (e) \
		     && fabs((d).y) < (e) \
		     && fabs((d).z) < (e) )

#define IsOne3(d) ( (d).x == 1.0 && (d).y == 1.0 && (d).z == 1.0 )
#define IsOne3e(d,e) (  fabs((d).x-1.0) < (e) \
		     && fabs((d).y-1.0) < (e) \
		     && fabs((d).z-1.0) < (e) )

#define IsVal3(d,v) ( (d).x == (v) && (d).y == (v) && (d).z == (v) )
#define IsVal3e(d,v,e) ( fabs( (d).x - (v) ) < (e) \
		      && fabs( (d).y - (v) ) < (e) \
		      && fabs( (d).z - (v) ) < (e) )

#define IsEQ3(a,b) ( (a).x == (b).x && (a).y == (b).y && (a).z == (b).z )
#define IsEQ3e(a,b,e) ( fabs( (a).x - (b).x ) < (e) \
		     && fabs( (a).y - (b).y ) < (e) \
		     && fabs( (a).z - (b).z ) < (e) )

#define Add3(d,a,b) do { (d).x = (a).x + (b).x; \
			 (d).y = (a).y + (b).y; \
			 (d).z = (a).z + (b).z; \
		    } while (0)

#define Sub3(d,a,b) do { (d).x = (a).x - (b).x; \
			 (d).y = (a).y - (b).y; \
			 (d).z = (a).z - (b).z; \
		    } while (0)

#define Neg3(v) do { (v).x = -(v).x; (v).y = -(v).y; (v).z = -(v).z; } while (0)

#define Div3f(d,a,f) do { (d).x = (a).x / (f); \
			  (d).y = (a).y / (f); \
			  (d).z = (a).z / (f); \
		    } while (0)

#define Mult3f(d,a,f) do { (d).x = (a).x * (f); \
			   (d).y = (a).y * (f); \
			   (d).z = (a).z * (f); \
		    } while (0)

#define LengthSqare3(d) ( (d).x*(d).x + (d).y*(d).y + (d).z*(d).z )
#define Length3(d)  sqrt( (d).x*(d).x + (d).y*(d).y + (d).z*(d).z )
#define DotProd3(a,b)   ( (a).x*(b).x + (a).y*(b).y + (a).z*(b).z )

#define CrossProd3(d,a,b) do {  (d).x = (a).y*(b).z - (a).z*(b).y; \
				(d).y = (a).z*(b).x - (a).x*(b).z; \
				(d).z = (a).x*(b).y - (a).y*(b).x; \
			  } while (0)

#define CrossProd3x(d,a,b) do { typeof((d).x) xx = (a).y*(b).z - (a).z*(b).y; \
				typeof((d).y) yy = (a).z*(b).x - (a).x*(b).z; \
					   (d).z = (a).x*(b).y - (a).y*(b).x; \
				(d).x = xx; (d).y = yy; \
			  } while (0)

#define Unit3(v) do { double l = Length3(v); \
			if (l) { (v).x /= l; (v).y /= l; (v).z /= l; }} while (0)
#define Unit3f(v,f) do { double l = Length3(v); \
			if (l) { l = (f)/l; \
				(v).x *= l; (v).y *= l; (v).z *= l; }} while (0)

#define MinMax3p(min,max,a,b,c) do { \
	if (a<b) { min = a < c ? a : c; max = b > c ? b : c; } \
	    else { min = b < c ? b : c; max = a > c ? a : c; } } while (0)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			check numbers			///////////////
///////////////////////////////////////////////////////////////////////////////

static inline bool IsNormalF ( float f )
{
    return isfinite(f);
//    const int fpclass = fpclassify(f);
//    return fpclass == FP_NORMAL || fpclass == FP_ZERO;
}

static inline bool IsNormalD ( double d )
{
    return isfinite(d);
//    const int fpclass = fpclassify(d);
//    return fpclass == FP_NORMAL || fpclass == FP_ZERO;
}

bool IsNormalF3	  ( float  *f3 );
bool IsNormalF3be ( float  *f3 ); // big endian!
bool IsNormalD3   ( double *d3 );

bool IsEqualD3 ( const double3 *a, const double3 *b,
			double null_epsilon, double diff_epsilon );

//--- new functions
bool IsSameF ( float  a, float  b, int bit_diff );
bool IsSameD ( double a, double b, int bit_diff );
bool IsSameF3 ( const float  *a, const float  *b, int bit_diff );
bool IsSameD3 ( const double *a, const double *b, int bit_diff );

//--- radiant and gegree
#define rad2deg(n) ( (n)*(180.0/M_PI) )
#define deg2rad(n) ( (n)*(M_PI/180.0) )

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
);

//-----------------------------------------------------------------------------

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
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    float34			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[float34]]

typedef struct float34 // transformation matrix
{
    union
    {
	float v[12];
	float m[3][4];
	struct
	{
	    float x[4];
	    float y[4];
	    float z[4];
	}
	__attribute__ ((packed));
    }
    __attribute__ ((packed));
}
__attribute__ ((packed)) float34;

//-----------------------------------------------------------------------------

void ClearFloat34 ( float34 *f34 );
void SetupFloat34 ( float34 *f34 );

void SetupMatrixF34
(
    float34	*mat,		// data to setup
    float3	*scale,		// NULL or scaling vector
    float3	*rotate,	// NULL or rotation vector in *radiant*
    float3	*translate	// NULL or translation vector
);

void CalcInverseMatrixF34
(
    float34		*i,	// store inverse matrix here
    const float34	*t	// transformation matrix
);

void MultiplyF34
(
    float34		*dest,	// valid, store result here, may be 'src1' or 'src2'
    const float34	*src1,	// first valid source, if NULL use dest
    const float34	*src2	// second valid source, if NULL use dest
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    double34			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[double34]]

typedef struct double34 // transformation matrix
{
    union
    {
	double v[12];
	double m[3][4];
	struct
	{
	    double x[4];
	    double y[4];
	    double z[4];
	}
	__attribute__ ((packed));
    }
    __attribute__ ((packed));
}
__attribute__ ((packed)) double34;

//-----------------------------------------------------------------------------

void ClearDouble34 ( double34 *d34 );
void SetupDouble34 ( double34 *d34 );

void SetupMatrixD34
(
    double34	*mat,		// data to setup
    double3	*scale,		// NULL or scaling vector
    double3	*rotate,	// NULL or rotation vector in *radiant*
    double3	*translate	// NULL or translation vector
);

void CalcInverseMatrixD34
(
    double34		*i,	// store inverse matrix here
    const double34	*t	// transformation matrix
);

void MultiplyD34
(
    double34		*dest,	// valid, store result here, may be 'src1' or 'src2'
    const double34	*src1,	// first valid source, if NULL use dest
    const double34	*src2	// second valid source, if NULL use dest
);

//-----------------------------------------------------------------------------

void CopyF34toD34
(
    double34		*dest,	// destination matrix
    const float34	*src	// source matrix
);

void CopyD34toF34
(
    float34		*dest,	// destination matrix
    const double34	*src	// source matrix
);

//-----------------------------------------------------------------------------

static inline float3 TransformF3D34 ( const double34 *d34, const float3 *val )
{
    DASSERT(d34);
    DASSERT(val);
    const double *m = d34->v;
    float3 res;
    res.x = m[  0] * val->x + m[  1] * val->y + m[  2] * val->z + m[  3];
    res.y = m[4+0] * val->x + m[4+1] * val->y + m[4+2] * val->z + m[4+3];
    res.z = m[8+0] * val->x + m[8+1] * val->y + m[8+2] * val->z + m[8+3];
    return res;
}

static inline double3 TransformD3D34 ( const double34 *d34, const double3 *val )
{
    DASSERT(d34);
    DASSERT(val);
    const double *m = d34->v;
    double3 res;
    res.x = m[  0] * val->x + m[  1] * val->y + m[  2] * val->z + m[  3];
    res.y = m[4+0] * val->x + m[4+1] * val->y + m[4+2] * val->z + m[4+3];
    res.z = m[8+0] * val->x + m[8+1] * val->y + m[8+2] * val->z + m[8+3];
    return res;
}

static inline float3 TransformF3F34 ( const float34 *d34, const float3 *val )
{
    DASSERT(d34);
    DASSERT(val);
    const float *m = d34->v;
    float3 res;
    res.x = m[  0] * val->x + m[  1] * val->y + m[  2] * val->z + m[  3];
    res.y = m[4+0] * val->x + m[4+1] * val->y + m[4+2] * val->z + m[4+3];
    res.z = m[8+0] * val->x + m[8+1] * val->y + m[8+2] * val->z + m[8+3];
    return res;
}

static inline double3 TransformD3F34 ( const float34 *d34, const double3 *val )
{
    DASSERT(d34);
    DASSERT(val);
    const float *m = d34->v;
    double3 res;
    res.x = m[  0] * val->x + m[  1] * val->y + m[  2] * val->z + m[  3];
    res.y = m[4+0] * val->x + m[4+1] * val->y + m[4+2] * val->z + m[4+3];
    res.z = m[8+0] * val->x + m[8+1] * val->y + m[8+2] * val->z + m[8+3];
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    MatrixD_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MatrixD_t]]

typedef struct MatrixD_t
{
    //--- status

    bool	valid;			// false: data structure needs initialization
    bool	norm_valid;		// false: normed values are invalid
    bool	tmatrix_valid;		// false: transformation matrix is invalid
    bool	imatrix_valid;		// false: inverse matrix is invalid
    uint	sequence_number;	// Used for tracking. It is incremented
					// on every new calculation by CalcNormMatrixD()

    //--- matrix usage status

    u8		use_matrix;		// 0: matrix not needed for transformation
					// 1: matrix needed for transformation
					// 2: matrix is the only valid reference

    //--- status, only valid, if 'norm_valid'

    u8		scale_enabled;		// scaling is enabled:     1=x, 2=y, 4=z
    u8		rotate_enabled;		// rotation is enabled:    1=x, 2=y, 4=z
    u8		translate_enabled;	// translation is enabled: 1=x, 2=y, 4=z
    u8		transform_enabled;	// 'OR' result of the 3 enabled values above
					// OR 'use_matrix<<3'

    //--- base values: clear 'norm_valid' and '*matrix_valid' on change

    double3	scale;			// scale vector
    double3	scale_origin;		// origin for scaling
    double3	shift;			// shift vector
    double3	rotate_deg;		// rotation in degree and radiant.
    double3	rotate_rad;		//   both values are used as sum.
    double3	rotate_origin[3];	// origin for rotation
    double3	translate;		// translation vector

    //--- normed values, calculated by CalcNormMatrixD()

    double3	norm_scale;		// scale vector
    double3	norm_rotate_deg;	// rotation vector in degree
    double3	norm_rotate_rad;	// rotation vector in radiant
    double3	norm_translate;		// translation vector
    double3	norm_pos2d;		// recommended positions for 2d transformations

    //--- matrices, calculated by CalcMatrixD()

    double34	trans_matrix;		// transformation matrix
    double34	inv_matrix;		// inverse transformation matrix
}
MatrixD_t;

//-----------------------------------------------------------------------------

// statistics
extern u64 N_MatrixD_forward;  // total number of forward transformations
extern u64 N_MatrixD_inverse;  // total number of inverse transformations

//-----------------------------------------------------------------------------

void InitializeMatrixD ( MatrixD_t * mat );

void CopyMatrixD
(
    MatrixD_t	*dest,		// valid destination
    const
      MatrixD_t	*src		// NULL or source
);

void TermAssignMatrixD
(
    // called after dirext matrix manipulations
    MatrixD_t	* mat		// valid data structure
);

//-----------------------------------------------------------------------------

MatrixD_t * SetScaleMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* scale,	// NULL: clear scale, not NULL: set scale
    double3	* origin	// not NULL: Origin for scaling
);

MatrixD_t * SetShiftMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* shift		// NULL: clear Shift, not NULL: set Shift
);

MatrixD_t * SetScaleShiftMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    uint	xyz,		// 0..2: x/y/z coordinate to set
    double	old1,		// old value of first point
    double	new1,		// new value of first point
    double	old2,		// old value of second point
    double	new2		// new value of second point
);

MatrixD_t * SetRotateMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    uint	xyz,		// 0..2: x/y/z coordinate to set
    double	degree,		// angle in degree
    double	radiant,	// angle in radiant
    double3	* origin	// not NULL: Origin for the rotation
);

MatrixD_t * SetTranslateMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* translate	// NULL: clear translate, not NULL: set translate
);

MatrixD_t * SetMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
     double34	* src		// NULL or matrix to set
);

//-----------------------------------------------------------------------------

MatrixD_t * SetAScaleMatrixD
(
    // calculate the matrix as rotation around a specified axis

    MatrixD_t	* mat,		// valid data structure
    double	scale,		// scale factor
    double3	* dir		// NULL or scaling direction as vector
);

MatrixD_t * SetARotateMatrixD
(
    // calculate the matrix as rotation around a specified axis

    MatrixD_t	* mat,		// valid data structure
    double	degree,		// angle in degree
    double	radiant,	// angle in radiant, add to degree with factor
    double3	* pt1,		// point #1 to define the axis; if NULL use (0,0,0)
    double3	* pt2		// point #2 to define the axis; if NULL use (0,0,0)
);

//-----------------------------------------------------------------------------

MatrixD_t * CheckStatusMatrixD
(
    MatrixD_t	* mat		// valid data structure
);

MatrixD_t * CalcNormMatrixD
(
    MatrixD_t	* mat		// valid data structure
);

MatrixD_t * CalcTransMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc transformation matrix only
				//        if needed for transformation
				// true:  ignore 'rotate_enabled'
);

MatrixD_t * CalcMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc both matrices only
				//        if needed for transformation
				// true:  calc matrices always
);

MatrixD_t * MultiplyMatrixD
(
    MatrixD_t	*dest,		// valid, store result here, may be 'src1' or 'src2'
    MatrixD_t	*src1,		// first valid source, if NULL use dest
    MatrixD_t	*src2,		// second valid source, if NULL use dest
    uint	inverse_mode	// bit field:
				//  1: use inverse matrix of src1
				//  2: use inverse matrix of src2
);

MatrixD_t * CalcVectorsMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    bool	always		// false: calc vectors only if matrix is valid
				// true:  calc matrix if invalid & vectors always
);

static inline bool TransformationEnabledMatrixD ( MatrixD_t * mat )
{
    return CalcNormMatrixD(mat)->transform_enabled != 0;
}

//-----------------------------------------------------------------------------

double3 TransformD3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
);

double3 InvTransformD3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
);

void TransformD3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
);

void InvTransformD3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    double3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
);

//-----------------------------------------------------------------------------

float3 TransformF3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      float3	* val		// value to transform
);

float3 InvTransformF3MatrixD
(
    MatrixD_t	* mat,		// valid data structure
    const
      float3	* val		// value to transform
);

void TransformF3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    float3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
);

void InvTransformF3NMatrixD
(
    MatrixD_t	* mat,		// valid data structure
    float3	* val,		// value array to transform
    int		n,		// number of 3D vectors in 'val'
    uint	off		// if n>1: offset from one to next 'val' vector
);

//-----------------------------------------------------------------------------

double3 BaseTransformD3MatrixD
(
    // transform using the base parameters

    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
);


double3 NormTransformD3MatrixD
(
    // transform using the norm parameters

    MatrixD_t	* mat,		// valid data structure
    const
      double3	* val		// value to transform
);

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
);

//-----------------------------------------------------------------------------

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
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////				CRC16			///////////////
///////////////////////////////////////////////////////////////////////////////

#define CRC16_CCITT_POLYNOM 0x1021

void CreateCRC16Table ( u16 table[0x100], u16 polynom );
const u16 * GetCRC16Table ( u16 polynom );

u16 CalcCRC16 ( cvp data, uint data_size, u16 polynom, u16 preset );
static inline u16 CalcCRC16CCITT ( cvp data, uint data_size )
	{ return CalcCRC16(data,data_size,CRC16_CCITT_POLYNOM,0); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    misc			///////////////
///////////////////////////////////////////////////////////////////////////////

static inline int float2int    ( float  f ) { return (int) truncf(f+0.5); }
static inline int double2int   ( double d ) { return (int) trunc (d+0.5); }
static inline s64 float2int64  ( float  f ) { return (s64) truncf(f+0.5); }
static inline s64 double2int64 ( double d ) { return (s64) trunc (d+0.5); }

///////////////////////////////////////////////////////////////////////////////

u64 MulDivU64 ( u64 factor1, u64 factor2, u64 divisor );
s64 MulDivS64 ( s64 factor1, s64 factor2, s64 divisor );

///////////////////////////////////////////////////////////////////////////////

// all values are multiple of 26*26*100 = 67600 = 0x00010810
#define GOOD_MAX_NICE4_5      67600  //     1 * 67600
#define GOOD_MAX_NICE4_6     946400  //    14 * 67600
#define GOOD_MAX_NICE4_7    9937200  //   147 * 67600
#define GOOD_MAX_NICE4_8   99980400  //  1479 * 67600
#define GOOD_MAX_NICE4_9  999939200  // 14792 * 67600
#define GOOD_MAX_NICE4   4294966000  // 63535 * 67600 = 0xfffffaf0 = -1296

ccp PrintNiceID4 ( uint nice_gid );
char * ScanNiceID4 ( int * res, ccp arg );

///////////////////////////////////////////////////////////////////////////////

// all values are multiple of 26*26*1000 = 676000 = 0x000a50a0
#define GOOD_MAX_NICE5_6     676000  //    1 * 676000
#define GOOD_MAX_NICE5_7    9464000  //   14 * 676000
#define GOOD_MAX_NICE5_8   99372000  //  147 * 676000
#define GOOD_MAX_NICE5_9  999804000  // 1479 * 676000
#define GOOD_MAX_NICE5   4294628000  // 6353 * 676000 = 0xfffad2a0 = -339296

ccp PrintNiceID5 ( uint nice_gid );
char * ScanNiceID5 ( int * res, ccp arg );

///////////////////////////////////////////////////////////////////////////////

// all values are multiple of 26*26*26*1000 = 17576000 = 0x010c3040
#define GOOD_MAX_NICE6_8   87880000  //   5 * 17576000
#define GOOD_MAX_NICE6_9  984256000  //  56 * 17576000
#define GOOD_MAX_NICE6   4288544000  // 244 * 17576000 = 0xff9dfd00 = -6423296

ccp PrintNiceID6 ( uint nice_gid );
char * ScanNiceID6 ( int * res, ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_NUMERIC_H

