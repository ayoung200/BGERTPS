/**
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): David Millan Escriva, Juho Vepsäläinen, Bob Holcomb, Thomas Dinges
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "BLI_blenlib.h"
#include "BLI_math.h"

#include "DNA_ID.h"
#include "DNA_node_types.h"
#include "DNA_image_types.h"
#include "DNA_material_types.h"
#include "DNA_mesh_types.h"
#include "DNA_action_types.h"
#include "DNA_color_types.h"
#include "DNA_customdata_types.h"
#include "DNA_gpencil_types.h"
#include "DNA_ipo_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_space_types.h"
#include "DNA_screen_types.h"
#include "DNA_texture_types.h"
#include "DNA_text_types.h"
#include "DNA_userdef_types.h"

#include "BKE_context.h"
#include "BKE_curve.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_node.h"
#include "BKE_object.h"
#include "BKE_texture.h"
#include "BKE_text.h"
#include "BKE_utildefines.h"

#include "CMP_node.h"
#include "SHD_node.h"

#include "BIF_gl.h"
#include "BIF_glutil.h"

#include "MEM_guardedalloc.h"

#include "ED_node.h"
#include "ED_space_api.h"
#include "ED_screen.h"
#include "ED_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_view2d.h"
#include "UI_interface.h"
#include "UI_resources.h"

#include "RE_pipeline.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "node_intern.h"


/* autocomplete callback for buttons */
static void autocomplete_vcol(bContext *C, char *str, void *arg_v)
{
	Mesh *me;
	CustomDataLayer *layer;
	AutoComplete *autocpl;
	int a;

	if(str[0]==0)
		return;

	autocpl= autocomplete_begin(str, 32);
		
	/* search if str matches the beginning of name */
	for(me= G.main->mesh.first; me; me=me->id.next)
		for(a=0, layer= me->fdata.layers; a<me->fdata.totlayer; a++, layer++)
			if(layer->type == CD_MCOL)
				autocomplete_do_name(autocpl, layer->name);
	
	autocomplete_end(autocpl, str);
}

static int verify_valid_vcol_name(char *str)
{
	Mesh *me;
	CustomDataLayer *layer;
	int a;
	
	if(str[0]==0)
		return 1;

	/* search if str matches the name */
	for(me= G.main->mesh.first; me; me=me->id.next)
		for(a=0, layer= me->fdata.layers; a<me->fdata.totlayer; a++, layer++)
			if(layer->type == CD_MCOL)
				if(strcmp(layer->name, str)==0)
					return 1;
	
	return 0;
}

/* ****************** GENERAL CALLBACKS FOR NODES ***************** */

static void node_ID_title_cb(bContext *C, void *node_v, void *unused_v)
{
	bNode *node= node_v;
	
	if(node->id) {
		test_idbutton(node->id->name+2);	/* library.c, verifies unique name */
		BLI_strncpy(node->name, node->id->name+2, 21);
	}
}

#if 0
/* XXX not used yet, make compiler happy :) */
static void node_group_alone_cb(bContext *C, void *node_v, void *unused_v)
{
	bNode *node= node_v;
	
	nodeCopyGroup(node);

	// allqueue(REDRAWNODE, 0);
}

/* ****************** BUTTON CALLBACKS FOR ALL TREES ***************** */

static void node_buts_group(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiBlock *block= uiLayoutAbsoluteBlock(layout);
	bNode *node= ptr->data;
	rctf *butr= &node->butr;

	if(node->id) {
		uiBut *bt;
		short width;
		
		uiBlockBeginAlign(block);
		
		/* name button */
		width= (short)(butr->xmax-butr->xmin - (node->id->us>1?19.0f:0.0f));
		bt= uiDefBut(block, TEX, B_NOP, "NT:",
					 (short)butr->xmin, (short)butr->ymin, width, 19, 
					 node->id->name+2, 0.0, 19.0, 0, 0, "NodeTree name");
		uiButSetFunc(bt, node_ID_title_cb, node, NULL);
		
		/* user amount */
		if(node->id->us>1) {
			char str1[32];
			sprintf(str1, "%d", node->id->us);
			bt= uiDefBut(block, BUT, B_NOP, str1, 
						 (short)butr->xmax-19, (short)butr->ymin, 19, 19, 
						 NULL, 0, 0, 0, 0, "Displays number of users.");
			uiButSetFunc(bt, node_group_alone_cb, node, NULL);
		}
		
		uiBlockEndAlign(block);
	}	
}
#endif

static void node_buts_value(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	bNode *node= ptr->data;
	PointerRNA sockptr;
	PropertyRNA *prop;
	
	/* first socket stores value */
	prop = RNA_struct_find_property(ptr, "outputs");
	RNA_property_collection_lookup_int(ptr, prop, 0, &sockptr);
	
	uiItemR(layout, "", 0, &sockptr, "default_value", 0);
}

static void node_buts_rgb(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	bNode *node= ptr->data;
	PointerRNA sockptr;
	PropertyRNA *prop;
	
	/* first socket stores value */
	prop = RNA_struct_find_property(ptr, "outputs");
	RNA_property_collection_lookup_int(ptr, prop, 0, &sockptr);
	
	col = uiLayoutColumn(layout, 0);
	uiItemR(col, "", 0, &sockptr, "default_value", 0);
}

static void node_buts_mix_rgb(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiLayout *row;

	bNodeTree *ntree= (bNodeTree*)ptr->id.data;

	row= uiLayoutRow(layout, 1);
	uiItemR(row, "", 0, ptr, "blend_type", 0);
	if(ntree->type == NTREE_COMPOSIT)
		uiItemR(row, "", ICON_IMAGE_RGB_ALPHA, ptr, "alpha", 0);
}

static void node_buts_time(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *row;
#if 0
	/* XXX no context access here .. */
	bNode *node= ptr->data;
	CurveMapping *cumap= node->storage;
	
	if(cumap) {
		cumap->flag |= CUMA_DRAW_CFRA;
		if(node->custom1<node->custom2)
			cumap->sample[0]= (float)(CFRA - node->custom1)/(float)(node->custom2-node->custom1);
	}
#endif

	uiTemplateCurveMapping(layout, ptr, "curve", 's', 0);

	row= uiLayoutRow(layout, 1);
	uiItemR(row, "Sta", 0, ptr, "start", 0);
	uiItemR(row, "End", 0, ptr, "end", 0);
}

static void node_buts_colorramp(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiTemplateColorRamp(layout, ptr, "color_ramp", 0);
}

static void node_buts_curvevec(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiTemplateCurveMapping(layout, ptr, "mapping", 'v', 0);
}

static float *_sample_col= NULL;	// bad bad, 2.5 will do better?
void node_curvemap_sample(float *col)
{
	_sample_col= col;
}

static void node_buts_curvecol(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	bNode *node= ptr->data;
	CurveMapping *cumap= node->storage;

	if(_sample_col) {
		cumap->flag |= CUMA_DRAW_SAMPLE;
		VECCOPY(cumap->sample, _sample_col);
	}
	else 
		cumap->flag &= ~CUMA_DRAW_SAMPLE;

	uiTemplateCurveMapping(layout, ptr, "mapping", 'c', 0);
}

static void node_buts_normal(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiBlock *block= uiLayoutAbsoluteBlock(layout);
	bNode *node= ptr->data;
	rctf *butr= &node->butr;
	bNodeSocket *sock= node->outputs.first;		/* first socket stores normal */
	
	uiDefButF(block, BUT_NORMAL, B_NODE_EXEC, "", 
			  (short)butr->xmin, (short)butr->xmin, butr->xmax-butr->xmin, butr->xmax-butr->xmin, 
			  sock->ns.vec, 0.0f, 1.0f, 0, 0, "");
}
#if 0 // not used in 2.5x yet
static void node_browse_tex_cb(bContext *C, void *ntree_v, void *node_v)
{
	bNodeTree *ntree= ntree_v;
	bNode *node= node_v;
	Tex *tex;
	
	if(node->menunr<1) return;
	
	if(node->id) {
		node->id->us--;
		node->id= NULL;
	}
	tex= BLI_findlink(&G.main->tex, node->menunr-1);

	node->id= &tex->id;
	id_us_plus(node->id);
	BLI_strncpy(node->name, node->id->name+2, 21);
	
	nodeSetActive(ntree, node);
	
	if( ntree->type == NTREE_TEXTURE )
		ntreeTexCheckCyclics( ntree );
	
	// allqueue(REDRAWBUTSSHADING, 0);
	// allqueue(REDRAWNODE, 0);
	NodeTagChanged(ntree, node); 
	
	node->menunr= 0;
}
#endif
static void node_dynamic_update_cb(bContext *C, void *ntree_v, void *node_v)
{
	Material *ma;
	bNode *node= (bNode *)node_v;
	ID *id= node->id;
	int error= 0;

	if (BTST(node->custom1, NODE_DYNAMIC_ERROR)) error= 1;

	/* Users only have to press the "update" button in one pynode
	 * and we also update all others sharing the same script */
	for (ma= G.main->mat.first; ma; ma= ma->id.next) {
		if (ma->nodetree) {
			bNode *nd;
			for (nd= ma->nodetree->nodes.first; nd; nd= nd->next) {
				if ((nd->type == NODE_DYNAMIC) && (nd->id == id)) {
					nd->custom1= 0;
					nd->custom1= BSET(nd->custom1, NODE_DYNAMIC_REPARSE);
					nd->menunr= 0;
					if (error)
						nd->custom1= BSET(nd->custom1, NODE_DYNAMIC_ERROR);
				}
			}
		}
	}

	// allqueue(REDRAWBUTSSHADING, 0);
	// allqueue(REDRAWNODE, 0);
	// XXX BIF_preview_changed(ID_MA);
}

static void node_buts_texture(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	bNode *node= ptr->data;

	short multi = (
		node->id &&
		((Tex*)node->id)->use_nodes &&
		(node->type != CMP_NODE_TEXTURE) &&
		(node->type != TEX_NODE_TEXTURE)
	);
	
	uiItemR(layout, "", 0, ptr, "texture", 0);
	
	if(multi) {
		/* Number Drawing not optimal here, better have a list*/
		uiItemR(layout, "", 0, ptr, "node_output", 0);
	}
}

static void node_buts_math(uiLayout *layout, bContext *C, PointerRNA *ptr)
{ 
	uiItemR(layout, "", 0, ptr, "operation", 0);
}

/* ****************** BUTTON CALLBACKS FOR SHADER NODES ***************** */

static void node_browse_text_cb(bContext *C, void *ntree_v, void *node_v)
{
	bNodeTree *ntree= ntree_v;
	bNode *node= node_v;
	ID *oldid;
	
	if(node->menunr<1) return;
	
	if(node->id) {
		node->id->us--;
	}
	oldid= node->id;
	node->id= BLI_findlink(&G.main->text, node->menunr-1);
	id_us_plus(node->id);
	BLI_strncpy(node->name, node->id->name+2, 21); /* huh? why 21? */

	node->custom1= BSET(node->custom1, NODE_DYNAMIC_NEW);
	
	nodeSetActive(ntree, node);

	// allqueue(REDRAWBUTSSHADING, 0);
	// allqueue(REDRAWNODE, 0);

	node->menunr= 0;
}

static void node_texmap_cb(bContext *C, void *texmap_v, void *unused_v)
{
	init_mapping(texmap_v);
}

static void node_shader_buts_material(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	bNode *node= ptr->data;
	uiLayout *col;
	
	uiTemplateID(layout, C, ptr, "material", "MATERIAL_OT_new", NULL, NULL);
	
	if(!node->id) return;
	
	col= uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "diffuse", 0);
	uiItemR(col, NULL, 0, ptr, "specular", 0);
	uiItemR(col, NULL, 0, ptr, "invert_normal", 0);
}

static void node_shader_buts_mapping(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *row;
	
	uiItemL(layout, "Location:", 0);
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "", 0, ptr, "location", 0);
	
	uiItemL(layout, "Rotation:", 0);
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "", 0, ptr, "rotation", 0);
	
	uiItemL(layout, "Scale:", 0);
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "", 0, ptr, "scale", 0);
	
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "Min", 0, ptr, "clamp_minimum", 0);
	uiItemR(row, "", 0, ptr, "minimum", 0);
	
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "Max", 0, ptr, "clamp_maximum", 0);
	uiItemR(row, "", 0, ptr, "maximum", 0);
	
}

static void node_shader_buts_vect_math(uiLayout *layout, bContext *C, PointerRNA *ptr)
{ 
	uiItemR(layout, "", 0, ptr, "operation", 0);
}

static void node_shader_buts_geometry(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 0);
	uiItemR(col, "UV", 0, ptr, "uv_layer", 0);
	uiItemR(col, "VCol", 0, ptr, "color_layer", 0);
}

static void node_shader_buts_dynamic(uiLayout *layout, bContext *C, PointerRNA *ptr)
{ 
	uiBlock *block= uiLayoutAbsoluteBlock(layout);
	bNode *node= ptr->data;
	bNodeTree *ntree= ptr->id.data;
	rctf *butr= &node->butr;
	uiBut *bt;
	// XXX SpaceNode *snode= curarea->spacedata.first;
	short dy= (short)butr->ymin;
	int xoff=0;

	/* B_NODE_EXEC is handled in butspace.c do_node_buts */
	if(!node->id) {
			char *strp;
			IDnames_to_pupstring(&strp, NULL, "", &(G.main->text), NULL, NULL);
			node->menunr= 0;
			bt= uiDefButS(block, MENU, B_NODE_EXEC/*+node->nr*/, strp, 
							butr->xmin, dy, 19, 19, 
							&node->menunr, 0, 0, 0, 0, "Browses existing choices");
			uiButSetFunc(bt, node_browse_text_cb, ntree, node);
			xoff=19;
			if(strp) MEM_freeN(strp);	
	}
	else {
		bt = uiDefBut(block, BUT, B_NOP, "Update",
				butr->xmin+xoff, butr->ymin+20, 50, 19,
				&node->menunr, 0.0, 19.0, 0, 0, "Refresh this node (and all others that use the same script)");
		uiButSetFunc(bt, node_dynamic_update_cb, ntree, node);

		if (BTST(node->custom1, NODE_DYNAMIC_ERROR)) {
			// UI_ThemeColor(TH_REDALERT);
			// XXX ui_rasterpos_safe(butr->xmin + xoff, butr->ymin + 5, snode->aspect);
			// XXX snode_drawstring(snode, "Error! Check console...", butr->xmax - butr->xmin);
			;
		}
	}
}

/* only once called */
static void node_shader_set_butfunc(bNodeType *ntype)
{
	switch(ntype->type) {
		/* case NODE_GROUP:	 note, typeinfo for group is generated... see "XXX ugly hack" */

		case SH_NODE_MATERIAL:
		case SH_NODE_MATERIAL_EXT:
			ntype->uifunc= node_shader_buts_material;
			break;
		case SH_NODE_TEXTURE:
			ntype->uifunc= node_buts_texture;
			break;
		case SH_NODE_NORMAL:
			ntype->uifunc= node_buts_normal;
			break;
		case SH_NODE_CURVE_VEC:
			ntype->uifunc= node_buts_curvevec;
			break;
		case SH_NODE_CURVE_RGB:
			ntype->uifunc= node_buts_curvecol;
			break;
		case SH_NODE_MAPPING:
			ntype->uifunc= node_shader_buts_mapping;
			break;
		case SH_NODE_VALUE:
			ntype->uifunc= node_buts_value;
			break;
		case SH_NODE_RGB:
			ntype->uifunc= node_buts_rgb;
			break;
		case SH_NODE_MIX_RGB:
			ntype->uifunc= node_buts_mix_rgb;
			break;
		case SH_NODE_VALTORGB:
			ntype->uifunc= node_buts_colorramp;
			break;
		case SH_NODE_MATH: 
			ntype->uifunc= node_buts_math;
			break; 
		case SH_NODE_VECT_MATH: 
			ntype->uifunc= node_shader_buts_vect_math;
			break; 
		case SH_NODE_GEOMETRY:
			ntype->uifunc= node_shader_buts_geometry;
			break;
		case NODE_DYNAMIC:
			ntype->uifunc= node_shader_buts_dynamic;
			break;
		default:
			ntype->uifunc= NULL;
	}
}

/* ****************** BUTTON CALLBACKS FOR COMPOSITE NODES ***************** */

static void node_browse_image_cb(bContext *C, void *ntree_v, void *node_v)
{
	bNodeTree *ntree= ntree_v;
	bNode *node= node_v;
	
	nodeSetActive(ntree, node);
	
	if(node->menunr<1) return;
	if(node->menunr==32767) {	/* code for Load New */
		/// addqueue(curarea->win, UI_BUT_EVENT, B_NODE_LOADIMAGE); XXX
	}
	else {
		if(node->id) node->id->us--;
		node->id= BLI_findlink(&G.main->image, node->menunr-1);
		id_us_plus(node->id);

		BLI_strncpy(node->name, node->id->name+2, 21);

		NodeTagChanged(ntree, node); 
		BKE_image_signal((Image *)node->id, node->storage, IMA_SIGNAL_USER_NEW_IMAGE);
		// addqueue(curarea->win, UI_BUT_EVENT, B_NODE_EXEC); XXX
	}
	node->menunr= 0;
}

static void node_active_cb(bContext *C, void *ntree_v, void *node_v)
{
	nodeSetActive(ntree_v, node_v);
}

static void node_composit_buts_image(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	bNode *node= ptr->data;
	PointerRNA imaptr;
	PropertyRNA *prop;
	
	uiTemplateID(layout, C, ptr, "image", NULL, "IMAGE_OT_open", NULL);
	
	if(!node->id) return;
	
	prop = RNA_struct_find_property(ptr, "image");
	if (!prop || RNA_property_type(prop) != PROP_POINTER) return;
	imaptr= RNA_property_pointer_get(ptr, prop);
	
	col= uiLayoutColumn(layout, 0);
	
	uiItemR(col, NULL, 0, &imaptr, "source", 0);
	
	if (ELEM(RNA_enum_get(&imaptr, "source"), IMA_SRC_SEQUENCE, IMA_SRC_MOVIE)) {
		col= uiLayoutColumn(layout, 1);
		uiItemR(col, NULL, 0, ptr, "frames", 0);
		uiItemR(col, NULL, 0, ptr, "start", 0);
		uiItemR(col, NULL, 0, ptr, "offset", 0);
		uiItemR(col, NULL, 0, ptr, "cyclic", 0);
		uiItemR(col, NULL, 0, ptr, "auto_refresh", UI_ITEM_R_ICON_ONLY);
	}

	col= uiLayoutColumn(layout, 0);
	
	if (RNA_enum_get(&imaptr, "type")== IMA_TYPE_MULTILAYER)
		uiItemR(col, NULL, 0, ptr, "layer", 0);
}

static void node_composit_buts_renderlayers(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	bNode *node= ptr->data;
	uiLayout *col;

	uiTemplateID(layout, C, ptr, "scene", NULL, NULL, NULL);
	
	if(!node->id) return;

	col= uiLayoutColumn(layout, 0);
	uiItemR(col, "", 0, ptr, "layer", 0);
	
	/* XXX Missing 're-render this layer' button - needs completely new implementation */
}


static void node_composit_buts_blur(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 0);
	
	uiItemR(col, "", 0, ptr, "filter_type", 0);
	if (RNA_enum_get(ptr, "filter_type")!= R_FILTER_FAST_GAUSS) {
		uiItemR(col, NULL, 0, ptr, "bokeh", 0);
		uiItemR(col, NULL, 0, ptr, "gamma", 0);
	}
	
	uiItemR(col, NULL, 0, ptr, "relative", 0);
	col= uiLayoutColumn(layout, 1);
	if (RNA_boolean_get(ptr, "relative")) {
		uiItemR(col, "X", 0, ptr, "factor_x", 0);
		uiItemR(col, "Y", 0, ptr, "factor_y", 0);
	}
	else {
		uiItemR(col, "X", 0, ptr, "sizex", 0);
		uiItemR(col, "Y", 0, ptr, "sizey", 0);
	}
}

static void node_composit_buts_dblur(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	uiItemR(layout, NULL, 0, ptr, "iterations", 0);
	uiItemR(layout, NULL, 0, ptr, "wrap", 0);
	
	col= uiLayoutColumn(layout, 1);
	uiItemL(col, "Center:", 0);
	uiItemR(col, "X", 0, ptr, "center_x", 0);
	uiItemR(col, "Y", 0, ptr, "center_y", 0);
	
	uiItemS(layout);
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "distance", 0);
	uiItemR(col, NULL, 0, ptr, "angle", 0);
	
	uiItemS(layout);
	
	uiItemR(layout, NULL, 0, ptr, "spin", 0);
	uiItemR(layout, NULL, 0, ptr, "zoom", 0);
}

static void node_composit_buts_bilateralblur(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "iterations", 0);
	uiItemR(col, NULL, 0, ptr, "sigma_color", 0);
	uiItemR(col, NULL, 0, ptr, "sigma_space", 0);
}

static void node_composit_buts_defocus(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *sub, *col;
	
	col= uiLayoutColumn(layout, 0);
	uiItemL(col, "Bokeh Type:", 0);
	uiItemR(col, "", 0, ptr, "bokeh", 0);
	uiItemR(col, NULL, 0, ptr, "angle", 0);

	uiItemR(layout, NULL, 0, ptr, "gamma_correction", 0);

	col = uiLayoutColumn(layout, 0);
	uiLayoutSetActive(col, RNA_boolean_get(ptr, "use_zbuffer")==0);
	uiItemR(col, NULL, 0, ptr, "f_stop", 0);

	uiItemR(layout, NULL, 0, ptr, "max_blur", 0);
	uiItemR(layout, NULL, 0, ptr, "threshold", 0);

	col = uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "preview", 0);
	sub = uiLayoutColumn(col, 0);
	uiLayoutSetActive(sub, RNA_boolean_get(ptr, "preview"));
	uiItemR(sub, NULL, 0, ptr, "samples", 0);
	
	col = uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "use_zbuffer", 0);
	sub = uiLayoutColumn(col, 0);
	uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_zbuffer"));
	uiItemR(sub, NULL, 0, ptr, "z_scale", 0);
}

/* qdn: glare node */
static void node_composit_buts_glare(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiItemR(layout, "", 0, ptr, "glare_type", 0);
	uiItemR(layout, "", 0, ptr, "quality", 0);

	if (RNA_enum_get(ptr, "glare_type")!= 1) {
		uiItemR(layout, NULL, 0, ptr, "iterations", 0);
	
		if (RNA_enum_get(ptr, "glare_type")!= 0) 
			uiItemR(layout, NULL, 0, ptr, "color_modulation", UI_ITEM_R_SLIDER);
	}
	
	uiItemR(layout, NULL, 0, ptr, "mix", 0);		
	uiItemR(layout, NULL, 0, ptr, "threshold", 0);

	if (RNA_enum_get(ptr, "glare_type")== 2) {
		uiItemR(layout, NULL, 0, ptr, "streaks", 0);		
		uiItemR(layout, NULL, 0, ptr, "angle_offset", 0);
	}
	if (RNA_enum_get(ptr, "glare_type")== 0 || RNA_enum_get(ptr, "glare_type")== 2) {
		uiItemR(layout, NULL, 0, ptr, "fade", UI_ITEM_R_SLIDER);
		
		if (RNA_enum_get(ptr, "glare_type")== 0) 
			uiItemR(layout, NULL, 0, ptr, "rotate_45", 0);
	}
	if (RNA_enum_get(ptr, "glare_type")== 1) {
		uiItemR(layout, NULL, 0, ptr, "size", 0);
	}
}

static void node_composit_buts_tonemap(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiLayout *col;

	col = uiLayoutColumn(layout, 0);
	uiItemR(col, "", 0, ptr, "tonemap_type", 0);
	if (RNA_enum_get(ptr, "tonemap_type")== 0) {
		uiItemR(col, NULL, 0, ptr, "key", UI_ITEM_R_SLIDER);
		uiItemR(col, NULL, 0, ptr, "offset", 0);
		uiItemR(col, NULL, 0, ptr, "gamma", 0);
	}
	else {
		uiItemR(col, NULL, 0, ptr, "intensity", 0);
		uiItemR(col, NULL, 0, ptr, "contrast", UI_ITEM_R_SLIDER);
		uiItemR(col, NULL, 0, ptr, "adaptation", UI_ITEM_R_SLIDER);
		uiItemR(col, NULL, 0, ptr, "correction", UI_ITEM_R_SLIDER);
	}
}

static void node_composit_buts_lensdist(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;

	col= uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "projector", 0);

	col = uiLayoutColumn(col, 0);
	uiLayoutSetActive(col, RNA_boolean_get(ptr, "projector")==0);
	uiItemR(col, NULL, 0, ptr, "jitter", 0);
	uiItemR(col, NULL, 0, ptr, "fit", 0);
}

static void node_composit_buts_vecblur(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "samples", 0);
	uiItemR(col, "Blur", 0, ptr, "factor", 0);
	
	col= uiLayoutColumn(layout, 1);
	uiItemL(col, "Speed:", 0);
	uiItemR(col, "Min", 0, ptr, "min_speed", 0);
	uiItemR(col, "Max", 0, ptr, "max_speed", 0);

	uiItemR(layout, NULL, 0, ptr, "curved", 0);
}

static void node_composit_buts_filter(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, "", 0, ptr, "filter_type", 0);
}

static void node_composit_buts_flip(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, "", 0, ptr, "axis", 0);
}

static void node_composit_buts_crop(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	uiItemR(layout, NULL, 0, ptr, "crop_size", 0);
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, "Left", 0, ptr, "x1", 0);
	uiItemR(col, "Right", 0, ptr, "x2", 0);
	uiItemR(col, "Up", 0, ptr, "y1", 0);
	uiItemR(col, "Down", 0, ptr, "y2", 0);
}

static void node_composit_buts_splitviewer(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *row, *col;
	
	col= uiLayoutColumn(layout, 0);
	row= uiLayoutRow(col, 0);
	uiItemR(row, NULL, 0, ptr, "axis", UI_ITEM_R_EXPAND);
	uiItemR(col, NULL, 0, ptr, "factor", 0);
}

static void node_composit_buts_map_value(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *sub, *col;
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "offset", 0);
	uiItemR(col, NULL, 0, ptr, "size", 0);
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "use_min", 0);
	sub =uiLayoutColumn(col, 0);
	uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_min"));
	uiItemR(sub, "", 0, ptr, "min", 0);
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "use_max", 0);
	sub =uiLayoutColumn(col, 0);
	uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_max"));
	uiItemR(sub, "", 0, ptr, "max", 0);
}

static void node_composit_buts_alphaover(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiLayout *col;
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "convert_premul", 0);
	uiItemR(col, NULL, 0, ptr, "premul", 0);
}

static void node_composit_buts_hue_sat(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col =uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "hue", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "sat", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "val", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_dilateerode(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, NULL, 0, ptr, "distance", 0);
}

static void node_composit_buts_diff_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "tolerance", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "falloff", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_distance_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "tolerance", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "falloff", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_color_spill(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *row, *col;
	
	col =uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "factor", 0);
	row= uiLayoutRow(col, 0);
	uiItemR(row, NULL, 0, ptr, "channel", UI_ITEM_R_EXPAND);
}

static void node_composit_buts_chroma_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "acceptance", 0);
	uiItemR(col, NULL, 0, ptr, "cutoff", 0);
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "lift", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "gain", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "shadow_adjust", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_color_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "h", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "s", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "v", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_channel_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{	
	uiLayout *col, *row;

	row= uiLayoutRow(layout, 0);
	uiItemR(row, NULL, 0, ptr, "color_space", UI_ITEM_R_EXPAND);

	row= uiLayoutRow(layout, 0);
	uiItemR(row, NULL, 0, ptr, "channel", UI_ITEM_R_EXPAND);

	col =uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "high", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "low", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_luma_matte(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, NULL, 0, ptr, "high", UI_ITEM_R_SLIDER);
	uiItemR(col, NULL, 0, ptr, "low", UI_ITEM_R_SLIDER);
}

static void node_composit_buts_map_uv(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, NULL, 0, ptr, "alpha", 0);
}

static void node_composit_buts_id_mask(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, NULL, 0, ptr, "index", 0);
}

static void node_composit_buts_file_output(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col, *row;

	col= uiLayoutColumn(layout, 0);
	uiItemR(col, "", 0, ptr, "filename", 0);
	uiItemR(col, "", 0, ptr, "image_type", 0);
	
	row= uiLayoutRow(layout, 0);
	if (RNA_enum_get(ptr, "image_type")== R_OPENEXR) {
		uiItemR(row, NULL, 0, ptr, "exr_half", 0);
		uiItemR(row, "", 0, ptr, "exr_codec", 0);
	}
	else if (RNA_enum_get(ptr, "image_type")== R_JPEG90) {
		uiItemR(row, NULL, 0, ptr, "quality", UI_ITEM_R_SLIDER);
	}
	
	row= uiLayoutRow(layout, 1);
	uiItemR(row, "Start", 0, ptr, "start_frame", 0);
	uiItemR(row, "End", 0, ptr, "end_frame", 0);
}

static void node_composit_buts_scale(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, "", 0, ptr, "space", 0);
}

static void node_composit_buts_invert(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 0);
	uiItemR(col, NULL, 0, ptr, "rgb", 0);
	uiItemR(col, NULL, 0, ptr, "alpha", 0);
}

static void node_composit_buts_premulkey(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, "", 0, ptr, "mapping", 0);
}

static void node_composit_buts_view_levels(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, NULL, 0, ptr, "channel", UI_ITEM_R_EXPAND);
}

/* only once called */
static void node_composit_set_butfunc(bNodeType *ntype)
{
	switch(ntype->type) {
		/* case NODE_GROUP:	 note, typeinfo for group is generated... see "XXX ugly hack" */

		case CMP_NODE_IMAGE:
			ntype->uifunc= node_composit_buts_image;
			break;
		case CMP_NODE_R_LAYERS:
			ntype->uifunc= node_composit_buts_renderlayers;
			break;
		case CMP_NODE_NORMAL:
			ntype->uifunc= node_buts_normal;
			break;
		case CMP_NODE_CURVE_VEC:
			ntype->uifunc= node_buts_curvevec;
			break;
		case CMP_NODE_CURVE_RGB:
			ntype->uifunc= node_buts_curvecol;
			break;
		case CMP_NODE_VALUE:
			ntype->uifunc= node_buts_value;
			break;
		case CMP_NODE_RGB:
			ntype->uifunc= node_buts_rgb;
			break;
		case CMP_NODE_FLIP:
			ntype->uifunc= node_composit_buts_flip;
			break;
		case CMP_NODE_SPLITVIEWER:
			ntype->uifunc= node_composit_buts_splitviewer;
			break;
		case CMP_NODE_MIX_RGB:
			ntype->uifunc= node_buts_mix_rgb;
			break;
		case CMP_NODE_VALTORGB:
			ntype->uifunc= node_buts_colorramp;
			break;
		case CMP_NODE_CROP:
			ntype->uifunc= node_composit_buts_crop;
			break;
		case CMP_NODE_BLUR:
			ntype->uifunc= node_composit_buts_blur;
			break;
		case CMP_NODE_DBLUR:
			ntype->uifunc= node_composit_buts_dblur;
			break;
		case CMP_NODE_BILATERALBLUR:
			ntype->uifunc= node_composit_buts_bilateralblur;
			break;
		case CMP_NODE_DEFOCUS:
			ntype->uifunc = node_composit_buts_defocus;
			break;
		case CMP_NODE_GLARE:
			ntype->uifunc = node_composit_buts_glare;
			break;
		case CMP_NODE_TONEMAP:
			ntype->uifunc = node_composit_buts_tonemap;
			break;
		case CMP_NODE_LENSDIST:
			ntype->uifunc = node_composit_buts_lensdist;
			break;
		case CMP_NODE_VECBLUR:
			ntype->uifunc= node_composit_buts_vecblur;
			break;
		case CMP_NODE_FILTER:
			ntype->uifunc= node_composit_buts_filter;
			break;
		case CMP_NODE_MAP_VALUE:
			ntype->uifunc= node_composit_buts_map_value;
			break;
		case CMP_NODE_TIME:
			ntype->uifunc= node_buts_time;
			break;
		case CMP_NODE_ALPHAOVER:
			ntype->uifunc= node_composit_buts_alphaover;
			break;
		case CMP_NODE_HUE_SAT:
			ntype->uifunc= node_composit_buts_hue_sat;
			break;
		case CMP_NODE_TEXTURE:
			ntype->uifunc= node_buts_texture;
			break;
		case CMP_NODE_DILATEERODE:
			ntype->uifunc= node_composit_buts_dilateerode;
			break;
		case CMP_NODE_OUTPUT_FILE:
			ntype->uifunc= node_composit_buts_file_output;
			break;	
		case CMP_NODE_DIFF_MATTE:
			ntype->uifunc=node_composit_buts_diff_matte;
			break;
		case CMP_NODE_DIST_MATTE:
			ntype->uifunc=node_composit_buts_distance_matte;
			break;
		case CMP_NODE_COLOR_SPILL:
			ntype->uifunc=node_composit_buts_color_spill;
			break;
		case CMP_NODE_CHROMA_MATTE:
			ntype->uifunc=node_composit_buts_chroma_matte;
			break;
		case CMP_NODE_COLOR_MATTE:
			ntype->uifunc=node_composit_buts_color_matte;
			break;
		case CMP_NODE_SCALE:
			ntype->uifunc= node_composit_buts_scale;
			break;
		case CMP_NODE_CHANNEL_MATTE:
			ntype->uifunc= node_composit_buts_channel_matte;
			break;
		case CMP_NODE_LUMA_MATTE:
			ntype->uifunc= node_composit_buts_luma_matte;
			break;
		case CMP_NODE_MAP_UV:
			ntype->uifunc= node_composit_buts_map_uv;
			break;
		case CMP_NODE_ID_MASK:
			ntype->uifunc= node_composit_buts_id_mask;
			break;
		case CMP_NODE_MATH:
			ntype->uifunc= node_buts_math;
			break;
		case CMP_NODE_INVERT:
			ntype->uifunc= node_composit_buts_invert;
			break;
		case CMP_NODE_PREMULKEY:
			ntype->uifunc= node_composit_buts_premulkey;
			break;
      case CMP_NODE_VIEW_LEVELS:
			ntype->uifunc=node_composit_buts_view_levels;
 			break;
		default:
			ntype->uifunc= NULL;
	}
}

/* ****************** BUTTON CALLBACKS FOR TEXTURE NODES ***************** */

static void node_texture_buts_bricks(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiLayout *col;
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, "Offset", 0, ptr, "offset", 0);
	uiItemR(col, "Frequency", 0, ptr, "offset_frequency", 0);
	
	col= uiLayoutColumn(layout, 1);
	uiItemR(col, "Squash", 0, ptr, "squash", 0);
	uiItemR(col, "Frequency", 0, ptr, "squash_frequency", 0);
}

/* Copied from buttons_shading.c -- needs unifying */
static char* noisebasis_menu()
{
	static char nbmenu[256];
	sprintf(nbmenu, "Noise Basis %%t|Blender Original %%x%d|Original Perlin %%x%d|Improved Perlin %%x%d|Voronoi F1 %%x%d|Voronoi F2 %%x%d|Voronoi F3 %%x%d|Voronoi F4 %%x%d|Voronoi F2-F1 %%x%d|Voronoi Crackle %%x%d|CellNoise %%x%d", TEX_BLENDER, TEX_STDPERLIN, TEX_NEWPERLIN, TEX_VORONOI_F1, TEX_VORONOI_F2, TEX_VORONOI_F3, TEX_VORONOI_F4, TEX_VORONOI_F2F1, TEX_VORONOI_CRACKLE, TEX_CELLNOISE);
	return nbmenu;
}

static void node_texture_buts_proc(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiBlock *block= uiLayoutAbsoluteBlock(layout);
	bNode *node= ptr->data;
	rctf *butr= &node->butr;
	Tex *tex = (Tex *)node->storage;
	short x,y,w,h;
	
	x = butr->xmin;
	y = butr->ymin;
	w = butr->xmax - x;
	h = butr->ymax - y;
	
	switch( tex->type ) {
		case TEX_BLEND:
			uiBlockBeginAlign( block );
			uiDefButS( block, MENU, B_NODE_EXEC,
				"Linear %x0|Quad %x1|Ease %x2|Diag %x3|Sphere %x4|Halo %x5|Radial %x6",
				x, y+20, w, 20, &tex->stype, 0, 1, 0, 0, "Blend Type" );
			uiDefButBitS(block, TOG, TEX_FLIPBLEND, B_NODE_EXEC, "Flip XY", x, y, w, 20,
				&tex->flag, 0, 0, 0, 0, "Flips the direction of the progression 90 degrees");
			uiBlockEndAlign( block );
			break;
			
		case TEX_MARBLE:
			uiBlockBeginAlign(block);
		
			uiDefButS(block, ROW, B_NODE_EXEC, "Soft",       0*w/3+x, 40+y, w/3, 18, &tex->stype, 2.0, (float)TEX_SOFT, 0, 0, "Uses soft marble"); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Sharp",      1*w/3+x, 40+y, w/3, 18, &tex->stype, 2.0, (float)TEX_SHARP, 0, 0, "Uses more clearly defined marble"); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Sharper",    2*w/3+x, 40+y, w/3, 18, &tex->stype, 2.0, (float)TEX_SHARPER, 0, 0, "Uses very clearly defined marble"); 
			
			uiDefButS(block, ROW, B_NODE_EXEC, "Soft noise", 0*w/2+x, 20+y, w/2, 19, &tex->noisetype, 12.0, (float)TEX_NOISESOFT, 0, 0, "Generates soft noise");
			uiDefButS(block, ROW, B_NODE_EXEC, "Hard noise", 1*w/2+x, 20+y, w/2, 19, &tex->noisetype, 12.0, (float)TEX_NOISEPERL, 0, 0, "Generates hard noise");
			
			uiDefButS(block, ROW, B_NODE_EXEC, "Sin",        0*w/3+x,  0+y, w/3, 18, &tex->noisebasis2, 8.0, 0.0, 0, 0, "Uses a sine wave to produce bands."); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Saw",        1*w/3+x,  0+y, w/3, 18, &tex->noisebasis2, 8.0, 1.0, 0, 0, "Uses a saw wave to produce bands"); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Tri",        2*w/3+x,  0+y, w/3, 18, &tex->noisebasis2, 8.0, 2.0, 0, 0, "Uses a triangle wave to produce bands"); 
		
			uiBlockEndAlign(block);
			break;
			
		case TEX_WOOD:
			uiDefButS(block, MENU, B_TEXPRV, noisebasis_menu(), x, y+64, w, 18, &tex->noisebasis, 0,0,0,0, "Sets the noise basis used for turbulence");
			
			uiBlockBeginAlign(block);
			uiDefButS(block, ROW, B_TEXPRV,             "Bands",     x,  40+y, w/2, 18, &tex->stype, 2.0, (float)TEX_BANDNOISE, 0, 0, "Uses standard noise"); 
			uiDefButS(block, ROW, B_TEXPRV,             "Rings", w/2+x,  40+y, w/2, 18, &tex->stype, 2.0, (float)TEX_RINGNOISE, 0, 0, "Lets Noise return RGB value"); 
			
			uiDefButS(block, ROW, B_NODE_EXEC, "Sin", 0*w/3+x,  20+y, w/3, 18, &tex->noisebasis2, 8.0, (float)TEX_SIN, 0, 0, "Uses a sine wave to produce bands."); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Saw", 1*w/3+x,  20+y, w/3, 18, &tex->noisebasis2, 8.0, (float)TEX_SAW, 0, 0, "Uses a saw wave to produce bands"); 
			uiDefButS(block, ROW, B_NODE_EXEC, "Tri", 2*w/3+x,  20+y, w/3, 18, &tex->noisebasis2, 8.0, (float)TEX_TRI, 0, 0, "Uses a triangle wave to produce bands");
			
			uiDefButS(block, ROW, B_NODE_EXEC, "Soft noise", 0*w/2+x, 0+y, w/2, 19, &tex->noisetype, 12.0, (float)TEX_NOISESOFT, 0, 0, "Generates soft noise");
			uiDefButS(block, ROW, B_NODE_EXEC, "Hard noise", 1*w/2+x, 0+y, w/2, 19, &tex->noisetype, 12.0, (float)TEX_NOISEPERL, 0, 0, "Generates hard noise");
			uiBlockEndAlign(block);
			break;
			
		case TEX_CLOUDS:
			uiDefButS(block, MENU, B_TEXPRV, noisebasis_menu(), x, y+60, w, 18, &tex->noisebasis, 0,0,0,0, "Sets the noise basis used for turbulence");
			
			uiBlockBeginAlign(block);
			uiDefButS(block, ROW, B_TEXPRV, "B/W",       x, y+38, w/2, 18, &tex->stype, 2.0, (float)TEX_DEFAULT, 0, 0, "Uses standard noise"); 
			uiDefButS(block, ROW, B_TEXPRV, "Color", w/2+x, y+38, w/2, 18, &tex->stype, 2.0, (float)TEX_COLOR, 0, 0, "Lets Noise return RGB value"); 
			uiDefButS(block, ROW, B_TEXPRV, "Soft",      x, y+20, w/2, 18, &tex->noisetype, 12.0, (float)TEX_NOISESOFT, 0, 0, "Generates soft noise");
			uiDefButS(block, ROW, B_TEXPRV, "Hard",  w/2+x, y+20, w/2, 18, &tex->noisetype, 12.0, (float)TEX_NOISEPERL, 0, 0, "Generates hard noise");
			uiBlockEndAlign(block);
			
			uiDefButS(block, NUM, B_TEXPRV, "Depth:", x, y, w, 18, &tex->noisedepth, 0.0, 6.0, 0, 0, "Sets the depth of the cloud calculation");
			break;
			
		case TEX_DISTNOISE:
			uiBlockBeginAlign(block);
			uiDefButS(block, MENU, B_TEXPRV, noisebasis_menu(), x, y+18, w, 18, &tex->noisebasis2, 0,0,0,0, "Sets the noise basis to distort");
			uiDefButS(block, MENU, B_TEXPRV, noisebasis_menu(), x, y,    w, 18, &tex->noisebasis,  0,0,0,0, "Sets the noise basis which does the distortion");
			uiBlockEndAlign(block);
			break;
	}
}

static void node_texture_buts_image(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiBlock *block= uiLayoutAbsoluteBlock(layout);
	bNode *node= ptr->data;
	bNodeTree *ntree= ptr->id.data;
	rctf *butr= &node->butr;
	char *strp;
	uiBut *bt;

	uiBlockBeginAlign(block);
	
	/* browse button */
	IMAnames_to_pupstring(&strp, NULL, "LOAD NEW %x32767", &(G.main->image), NULL, NULL);
	node->menunr= 0;
	bt= uiDefButS(block, MENU, B_NOP, strp, 
				  butr->xmin, butr->ymin, 19, 19, 
				  &node->menunr, 0, 0, 0, 0, "Browses existing choices");
	uiButSetFunc(bt, node_browse_image_cb, ntree, node);
	if(strp) MEM_freeN(strp);
	
	/* Add New button */
	if(node->id==NULL) {
		bt= uiDefBut(block, BUT, B_NODE_LOADIMAGE, "Load New",
					 butr->xmin+19, butr->ymin, (short)(butr->xmax-butr->xmin-19.0f), 19, 
					 NULL, 0.0, 0.0, 0, 0, "Add new Image");
		uiButSetFunc(bt, node_active_cb, ntree, node);
	}
	else {
		/* name button */
		short xmin= (short)butr->xmin, xmax= (short)butr->xmax;
		short width= xmax - xmin - 19;
		
		bt= uiDefBut(block, TEX, B_NOP, "IM:",
					 xmin+19, butr->ymin, width, 19, 
					 node->id->name+2, 0.0, 19.0, 0, 0, "Image name");
		uiButSetFunc(bt, node_ID_title_cb, node, NULL);
	}
}

static void node_texture_buts_output(uiLayout *layout, bContext *C, PointerRNA *ptr)
{
	uiItemR(layout, "", 0, ptr, "output_name", 0);
}

/* only once called */
static void node_texture_set_butfunc(bNodeType *ntype)
{
	if( ntype->type >= TEX_NODE_PROC && ntype->type < TEX_NODE_PROC_MAX ) {
		ntype->uifunc = node_texture_buts_proc;
	}
	else switch(ntype->type) {
		
		case TEX_NODE_MATH:
			ntype->uifunc = node_buts_math;
			break;
		
		case TEX_NODE_MIX_RGB:
			ntype->uifunc = node_buts_mix_rgb;
			break;
			
		case TEX_NODE_VALTORGB:
			ntype->uifunc = node_buts_colorramp;
			break;
			
		case TEX_NODE_CURVE_RGB:
			ntype->uifunc= node_buts_curvecol;
			break;
			
		case TEX_NODE_CURVE_TIME:
			ntype->uifunc = node_buts_time;
			break;
			
		case TEX_NODE_TEXTURE:
			ntype->uifunc = node_buts_texture;
			break;
			
		case TEX_NODE_BRICKS:
			ntype->uifunc = node_texture_buts_bricks;
			break;
			
		case TEX_NODE_IMAGE:
			ntype->uifunc = node_texture_buts_image;
			break;
			
		case TEX_NODE_OUTPUT:
			ntype->uifunc = node_texture_buts_output;
			break;
			
		default:
			ntype->uifunc= NULL;
	}
}

/* ******* init draw callbacks for all tree types, only called in usiblender.c, once ************* */

void ED_init_node_butfuncs(void)
{
	bNodeType *ntype;
	
	/* shader nodes */
	ntype= node_all_shaders.first;
	while(ntype) {
		node_shader_set_butfunc(ntype);
		ntype= ntype->next;
	}
	/* composit nodes */
	ntype= node_all_composit.first;
	while(ntype) {
		node_composit_set_butfunc(ntype);
		ntype= ntype->next;
	}
	ntype = node_all_textures.first;
	while(ntype) {
		node_texture_set_butfunc(ntype);
		ntype= ntype->next;
	}
}

/* ************** Generic drawing ************** */

#if 0
void node_rename_but(char *s)
{
	uiBlock *block;
	ListBase listb={0, 0};
	int dy, x1, y1, sizex=80, sizey=30;
	short pivot[2], mval[2], ret=0;
	
	getmouseco_sc(mval);

	pivot[0]= CLAMPIS(mval[0], (sizex+10), G.curscreen->sizex-30);
	pivot[1]= CLAMPIS(mval[1], (sizey/2)+10, G.curscreen->sizey-(sizey/2)-10);
	
	if (pivot[0]!=mval[0] || pivot[1]!=mval[1])
		warp_pointer(pivot[0], pivot[1]);

	mywinset(G.curscreen->mainwin);
	
	x1= pivot[0]-sizex+10;
	y1= pivot[1]-sizey/2;
	dy= sizey/2;
	
	block= uiNewBlock(&listb, "button", UI_EMBOSS, UI_HELV, G.curscreen->mainwin);
	uiBlockSetFlag(block, UI_BLOCK_LOOP|UI_BLOCK_REDRAW|UI_BLOCK_NUMSELECT|UI_BLOCK_ENTER_OK);
	
	/* buttons have 0 as return event, to prevent menu to close on hotkeys */
	uiBlockBeginAlign(block);
	
	uiDefBut(block, TEX, B_NOP, "Name: ", (short)(x1),(short)(y1+dy), 150, 19, s, 0.0, 19.0, 0, 0, "Node user name");
	
	uiBlockEndAlign(block);

	uiDefBut(block, BUT, 32767, "OK", (short)(x1+150), (short)(y1+dy), 29, 19, NULL, 0, 0, 0, 0, "");

	uiBoundsBlock(block, 2);

	ret= uiDoBlocks(&listb, 0, 0);
}

#endif

void draw_nodespace_back_pix(ARegion *ar, SpaceNode *snode, int color_manage)
{
	
	if((snode->flag & SNODE_BACKDRAW) && snode->treetype==NTREE_COMPOSIT) {
		Image *ima= BKE_image_verify_viewer(IMA_TYPE_COMPOSITE, "Viewer Node");
		void *lock;
		ImBuf *ibuf= BKE_image_acquire_ibuf(ima, NULL, &lock);
		if(ibuf) {
			float x, y; 
			
			wmPushMatrix();
			
			/* somehow the offset has to be calculated inverse */
			
			glaDefine2DArea(&ar->winrct);
			/* ortho at pixel level curarea */
			wmOrtho2(-0.375, ar->winx-0.375, -0.375, ar->winy-0.375);
			
			x = (ar->winx-ibuf->x)/2 + snode->xof;
			y = (ar->winy-ibuf->y)/2 + snode->yof;
			
			if(!ibuf->rect) {
				if(color_manage)
					ibuf->profile= IB_PROFILE_SRGB;
				else
					ibuf->profile = IB_PROFILE_NONE;
				IMB_rect_from_float(ibuf);
			}

			if(ibuf->rect)
				glaDrawPixelsSafe(x, y, ibuf->x, ibuf->y, ibuf->x, GL_RGBA, GL_UNSIGNED_BYTE, ibuf->rect);
			
			wmPopMatrix();
		}

		BKE_image_release_ibuf(ima, lock);
	}
}

#if 0
/* note: needs to be userpref or opengl profile option */
static void draw_nodespace_back_tex(ScrArea *sa, SpaceNode *snode)
{

	draw_nodespace_grid(snode);
	
	if(snode->flag & SNODE_BACKDRAW) {
		Image *ima= BKE_image_verify_viewer(IMA_TYPE_COMPOSITE, "Viewer Node");
		ImBuf *ibuf= BKE_image_get_ibuf(ima, NULL);
		if(ibuf) {
			int x, y;
			float zoom = 1.0;

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			
			glaDefine2DArea(&sa->winrct);

			if(ibuf->x > sa->winx || ibuf->y > sa->winy) {
				float zoomx, zoomy;
				zoomx= (float)sa->winx/ibuf->x;
				zoomy= (float)sa->winy/ibuf->y;
				zoom = MIN2(zoomx, zoomy);
			}
			
			x = (sa->winx-zoom*ibuf->x)/2 + snode->xof;
			y = (sa->winy-zoom*ibuf->y)/2 + snode->yof;

			glPixelZoom(zoom, zoom);

			glColor4f(1.0, 1.0, 1.0, 1.0);
			if(ibuf->rect)
				glaDrawPixelsTex(x, y, ibuf->x, ibuf->y, GL_UNSIGNED_BYTE, ibuf->rect);
			else if(ibuf->channels==4)
				glaDrawPixelsTex(x, y, ibuf->x, ibuf->y, GL_FLOAT, ibuf->rect_float);

			glPixelZoom(1.0, 1.0);

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}
}
#endif

/* if v2d not NULL, it clips and returns 0 if not visible */
int node_link_bezier_points(View2D *v2d, SpaceNode *snode, bNodeLink *link, float coord_array[][2], int resol)
{
	float dist, vec[4][2];
	
	/* in v0 and v3 we put begin/end points */
	if(link->fromsock) {
		vec[0][0]= link->fromsock->locx;
		vec[0][1]= link->fromsock->locy;
	}
	else {
		if(snode==NULL) return 0;
		vec[0][0]= snode->mx;
		vec[0][1]= snode->my;
	}
	if(link->tosock) {
		vec[3][0]= link->tosock->locx;
		vec[3][1]= link->tosock->locy;
	}
	else {
		if(snode==NULL) return 0;
		vec[3][0]= snode->mx;
		vec[3][1]= snode->my;
	}
	
	dist= 0.5f*ABS(vec[0][0] - vec[3][0]);
	
	/* check direction later, for top sockets */
	vec[1][0]= vec[0][0]+dist;
	vec[1][1]= vec[0][1];
	
	vec[2][0]= vec[3][0]-dist;
	vec[2][1]= vec[3][1];
	
	if(v2d && MIN4(vec[0][0], vec[1][0], vec[2][0], vec[3][0]) > v2d->cur.xmax); /* clipped */	
	else if (v2d && MAX4(vec[0][0], vec[1][0], vec[2][0], vec[3][0]) < v2d->cur.xmin); /* clipped */
	else {
		
		/* always do all three, to prevent data hanging around */
		forward_diff_bezier(vec[0][0], vec[1][0], vec[2][0], vec[3][0], coord_array[0], resol, sizeof(float)*2);
		forward_diff_bezier(vec[0][1], vec[1][1], vec[2][1], vec[3][1], coord_array[0]+1, resol, sizeof(float)*2);
		
		return 1;
	}
	return 0;
}

#define LINK_RESOL	24
void node_draw_link_bezier(View2D *v2d, SpaceNode *snode, bNodeLink *link, int th_col1, int th_col2, int do_shaded)
{
	float coord_array[LINK_RESOL+1][2];
	
	if(node_link_bezier_points(v2d, snode, link, coord_array, LINK_RESOL)) {
		float dist, spline_step = 0.0f;
		int i;
		
		/* we can reuse the dist variable here to increment the GL curve eval amount*/
		dist = 1.0f/(float)LINK_RESOL;
		
		glBegin(GL_LINE_STRIP);
		for(i=0; i<=LINK_RESOL; i++) {
			if(do_shaded) {
				UI_ThemeColorBlend(th_col1, th_col2, spline_step);
				spline_step += dist;
			}				
			glVertex2fv(coord_array[i]);
		}
		glEnd();
	}
}

/* note; this is used for fake links in groups too */
void node_draw_link(View2D *v2d, SpaceNode *snode, bNodeLink *link)
{
	int do_shaded= 1, th_col1= TH_WIRE, th_col2= TH_WIRE;
	
	if(link->fromnode==NULL && link->tonode==NULL)
		return;
	
	if(link->fromnode==NULL || link->tonode==NULL) {
		UI_ThemeColor(TH_WIRE);
		do_shaded= 0;
	}
	else {
		/* going to give issues once... */
		if(link->tosock->flag & SOCK_UNAVAIL)
			return;
		if(link->fromsock->flag & SOCK_UNAVAIL)
			return;
		
		/* a bit ugly... but thats how we detect the internal group links */
		if(link->fromnode==link->tonode) {
			UI_ThemeColorBlend(TH_BACK, TH_WIRE, 0.25f);
			do_shaded= 0;
		}
		else {
			/* check cyclic */
			if(link->fromnode->level >= link->tonode->level && link->tonode->level!=0xFFF) {
				if(link->fromnode->flag & SELECT)
					th_col1= TH_EDGE_SELECT;
				if(link->tonode->flag & SELECT)
					th_col2= TH_EDGE_SELECT;
			}				
			else {
				UI_ThemeColor(TH_REDALERT);
				do_shaded= 0;
			}
		}
	}
	
	node_draw_link_bezier(v2d, snode, link, th_col1, th_col2, do_shaded);
}


