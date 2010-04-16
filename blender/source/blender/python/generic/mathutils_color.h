/*
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Joseph Gilbert
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#ifndef EXPP_color_h
#define EXPP_color_h

#include <Python.h>

extern PyTypeObject color_Type;
#define ColorObject_Check(_v) PyObject_TypeCheck((_v), &color_Type)

typedef struct {
	PyObject_VAR_HEAD
	float *col;					/*1D array of data */
	PyObject *cb_user;			/* if this vector references another object, otherwise NULL, *Note* this owns its reference */
	unsigned char cb_type;		/* which user funcs do we adhere to, RNA, GameObject, etc */
	unsigned char cb_subtype;	/* subtype: location, rotation... to avoid defining many new functions for every attribute of the same type */
	unsigned char wrapped;		/* wrapped data type? */
	/* end BaseMathObject */

} ColorObject;

/*struct data contains a pointer to the actual data that the
object uses. It can use either PyMem allocated data (which will
be stored in py_data) or be a wrapper for data allocated through
blender (stored in blend_data). This is an either/or struct not both*/

//prototypes
PyObject *newColorObject( float *col, int type, PyTypeObject *base_type);
PyObject *newColorObject_cb(PyObject *cb_user, int cb_type, int cb_subtype);

#endif				/* EXPP_color_h */