
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

#ifndef DC_LIB_PUNYCODE_H
#define DC_LIB_PUNYCODE_H 1

#include "dclib-basics.h"
#include <limits.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Domain2*()			///////////////
///////////////////////////////////////////////////////////////////////////////

uint Domain2UTF8
(
    // returns the number of scanned bytes of 'source' or 0 on error

    char	*buf,			// valid destination buffer
    int		buf_size,		// size of 'buf' >= 4
    const void	*source,		// NULL or UTF-8 domain to translate
    int		source_len		// length of 'source'; if <0: use strlen(source)
);

///////////////////////////////////////////////////////////////////////////////

uint Domain2ASCII
(
    // returns the number of scanned bytes of 'source' or 0 on error

    char	*buf,			// valid destination buffer
    int		buf_size,		// size of 'buf' >= 5
    const void	*source,		// NULL or ASCII domain to translate
    int		source_len		// length of 'source'; if <0: use strlen(source)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			punycode lib			///////////////
///////////////////////////////////////////////////////////////////////////////


enum punycode_status
{
  punycode_success,
  punycode_bad_input,   /* Input is invalid.                       */
  punycode_big_output,  /* Output would exceed the space provided. */
  punycode_overflow     /* Input needs wider integers to process.  */
};

#if UINT_MAX >= (1 << 26) - 1
  typedef unsigned int punycode_uint;
#else
  typedef unsigned long punycode_uint;
#endif

///////////////////////////////////////////////////////////////////////////////

enum punycode_status punycode_encode
(
    punycode_uint		input_length,
    const punycode_uint		input[],
    const unsigned char		case_flags[],
    punycode_uint		*output_length,
    char			output[]
);

    /* punycode_encode() converts Unicode to Punycode.  The input     */
    /* is represented as an array of Unicode code points (not code    */
    /* units; surrogate pairs are not allowed), and the output        */
    /* will be represented as an array of ASCII code points.  The     */
    /* output string is *not* null-terminated; it will contain        */
    /* zeros if and only if the input contains zeros.  (Of course     */
    /* the caller can leave room for a terminator and add one if      */
    /* needed.)  The input_length is the number of code points in     */
    /* the input.  The output_length is an in/out argument: the       */
    /* caller passes in the maximum number of code points that it     */
    /* can receive, and on successful return it will contain the      */
    /* number of code points actually output.  The case_flags array   */
    /* holds input_length boolean values, where nonzero suggests that */
    /* the corresponding Unicode character be forced to uppercase     */
    /* after being decoded (if possible), and zero suggests that      */
    /* it be forced to lowercase (if possible).  ASCII code points    */
    /* are encoded literally, except that ASCII letters are forced    */
    /* to uppercase or lowercase according to the corresponding       */
    /* uppercase flags.  If case_flags is a null pointer then ASCII   */
    /* letters are left as they are, and other code points are        */
    /* treated as if their uppercase flags were zero.  The return     */
    /* value can be any of the punycode_status values defined above   */
    /* except punycode_bad_input; if not punycode_success, then       */
    /* output_size and output might contain garbage.                  */

///////////////////////////////////////////////////////////////////////////////

enum punycode_status punycode_decode
(
    punycode_uint	input_length,
    const		char input[],
    punycode_uint	*output_length,
    punycode_uint	output[],
    unsigned char	case_flags[]
);

    /* punycode_decode() converts Punycode to Unicode.  The input is  */
    /* represented as an array of ASCII code points, and the output   */
    /* will be represented as an array of Unicode code points.  The   */
    /* input_length is the number of code points in the input.  The   */
    /* output_length is an in/out argument: the caller passes in      */
    /* the maximum number of code points that it can receive, and     */
    /* on successful return it will contain the actual number of      */
    /* code points output.  The case_flags array needs room for at    */
    /* least output_length values, or it can be a null pointer if the */
    /* case information is not needed.  A nonzero flag suggests that  */
    /* the corresponding Unicode character be forced to uppercase     */
    /* by the caller (if possible), while zero suggests that it be    */
    /* forced to lowercase (if possible).  ASCII code points are      */
    /* output already in the proper case, but their flags will be set */
    /* appropriately so that applying the flags would be harmless.    */
    /* The return value can be any of the punycode_status values      */
    /* defined above; if not punycode_success, then output_length,    */
    /* output, and case_flags might contain garbage.  On success, the */
    /* decoder will never need to write an output_length greater than */
    /* input_length, because of how the encoding is defined.          */

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DC_LIB_PUNYCODE_H
