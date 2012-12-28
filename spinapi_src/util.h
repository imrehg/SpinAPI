/* util.h
 * Miscellaneous support functions.
 *
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


#ifndef _UTIL_H
#define _UTIL_H

extern char *spinerr;

char do_amcc_inp (int card_num, unsigned int address);
int do_amcc_outp (int card_num, unsigned int address, char data);
int do_amcc_outp_old (int card_num, unsigned int address, int data);

char *my_strcat (char *a, char *b);
char *my_sprintf (char *format, ...);

void _debug (const char* function, char *format, ...);
extern int do_debug;

#define debug(FORMAT, ...) if(do_debug) _debug(__FUNCTION__, FORMAT, ##__VA_ARGS__)

#endif /* #ifndef _UTIL_H */
