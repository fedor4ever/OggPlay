/*
 *  Copyright (c) 2003 L. H. Wilden.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// j_code() function return type
// 0x0 means set code by hand
#define GB_CODE      0x0001
#define BIG5_CODE    0x0002
#define HZ_CODE      0x0004
#define UNI_CODE     0x0010
#define UTF7_CODE    0x0020
#define UTF8_CODE    0x0040
#define OTHER_CODE   0x8000

extern int j_code(const char * buff,int count);
