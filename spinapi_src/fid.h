/* fid.h
 * $Date: 2008/04/28 18:29:51 $
 *
 * To get the latest version of this code, or to contact us for support, please
 * visit http://www.spincore.com
 */

/* Copyright (c) 2008 SpinCore Technologies, Inc.
 *
 * This software is provided 'as-is', without any express or implied warranty. 
 * In no event will the authors be held liable for any damages arising from the 
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, 
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be appreciated
 * but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

//felix file defines
#define FID_TYPE 0
#define DC -1
#define FOURIER_NUMBER 0
#define LINE_BROAD 0
#define GAUSS_BROAD 0

//felix display params
#define DISP_MODE 0
#define PH0 0
#define PH1 0
#define REF_LINE_FREQ 0
#define REF_POINT 0
// Units to use for time domain display.
// 0 = point
// 1 = sec
#define FID_UNITS 1
// Units to use for frequency display.
// 0 = ppm
// 1 = hz
#define SPEC_UNITS 1
// Controls whether the units axis is shown or not
#define AXIS 1
#define SCALE_OFFSET 0
#define START_PLOT 0
#define WIDTH_PLOT 0
#define VERT_OFFSET 0
#define VERT_SCALE 1.0
#define FID_START 0
//#define FID_WIDTH 0  ** not used, NP used in make_felix instead
#define FID_VERT_OFFSET 0
#define FID_VERT_SCALE 1.0

// Note: bool has to be defined as a 16 bit integer for the file to be written
// properly
#ifndef bool
#define bool short
#endif
