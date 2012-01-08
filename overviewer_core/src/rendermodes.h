/* 
 * This file is part of the Minecraft Overviewer.
 *
 * Minecraft Overviewer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * Minecraft Overviewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Overviewer.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * To make a new render primitive (the C part, at least):
 *
 *     * add a data struct and extern'd interface declaration below
 *
 *     * fill in this interface struct in primitives/(yourmode).c
 *         (see primitives/base.c for an example: the "base" primitive)
 *
 *     * add your primitive to the list in rendermodes.c
 */

#ifndef __RENDERMODES_H_INCLUDED__
#define __RENDERMODES_H_INCLUDED__

#include <Python.h>
#include "overviewer.h"

/* render primitive interface */
typedef struct {
    /* the name of this mode */
    const char *name;    
    /* the size of the local storage for this rendermode */
    unsigned int data_size;
    
    /* may return non-zero on error, last arg is the python support object */
    int (*start)(void *, RenderState *, PyObject *);
    void (*finish)(void *, RenderState *);
    /* returns non-zero to skip rendering this block because it's not visible */
    int (*occluded)(void *, RenderState *, int, int, int);
    /* returns non-zero to skip rendering this block because the user doesn't
     * want it visible */
    int (*hidden)(void *, RenderState *, int, int, int);
    /* last two arguments are img and mask, from texture lookup */
    void (*draw)(void *, RenderState *, PyObject *, PyObject *, PyObject *);
} RenderPrimitiveInterface;

/* A quick note about the difference between occluded and hidden:
 *
 * Occluded should be used to tell the renderer that a block will not be
 * visible in the final image because other blocks will be drawn on top of
 * it. This is a potentially *expensive* check that should be used rarely,
 * usually only once per block. The idea is this check is expensive, but not
 * as expensive as drawing the block itself.
 *
 * Hidden is used to tell the renderer not to draw the block, usually because
 * the current rendermode depends on those blocks being hidden to do its
 * job. For example, cave mode uses this to hide non-cave blocks. This check
 * should be *cheap*, as it's potentially called many times per block. For
 * example, in lighting mode it is called at most 4 times per block.
 */

/* convenience wrapper for a single primitive + interface */
typedef struct {
    void *primitive;
    RenderPrimitiveInterface *iface;
} RenderPrimitive;

/* wrapper for passing around rendermodes */
struct _RenderMode {
    unsigned int num_primitives;
    RenderPrimitive **primitives;
    RenderState *state;
};

/* functions for creating / using rendermodes */
RenderMode *render_mode_create(PyObject *mode, RenderState *state);
void render_mode_destroy(RenderMode *self);
int render_mode_occluded(RenderMode *self, int x, int y, int z);
int render_mode_hidden(RenderMode *self, int x, int y, int z);
void render_mode_draw(RenderMode *self, PyObject *img, PyObject *mask, PyObject *mask_light);

/* helper function for reading in rendermode options
   works like PyArg_ParseTuple on a support object */
int render_mode_parse_option(PyObject *support, const char *name, const char *format, ...);

/* XXX individual rendermode interface declarations follow */
#ifdef OLD_MODES

/* OVERLAY */
typedef struct {
    /* top facemask and white color image, for drawing overlays */
    PyObject *facemask_top, *white_color;
    /* can be overridden in derived classes to control
       overlay alpha and color
       last four vars are r, g, b, a out */
    void (*get_color)(void *, RenderState *,
                      unsigned char *, unsigned char *, unsigned char *, unsigned char *);
} RenderModeOverlay;
extern RenderModeInterface rendermode_overlay;

/* LIGHTING */
typedef struct {
    /* inherits from normal render mode */
    RenderModeNormal parent;
    
    PyObject *facemasks_py;
    PyObject *facemasks[3];
    
    /* extra data, loaded off the chunk class */
    PyObject *skylight, *blocklight;
    PyObject *left_skylight, *left_blocklight;
    PyObject *right_skylight, *right_blocklight;
    PyObject *up_left_skylight, *up_left_blocklight;
    PyObject *up_right_skylight, *up_right_blocklight;
    
    /* light color image, loaded if color_light is True */
    PyObject *lightcolor;
    
    /* can be overridden in derived rendermodes to control lighting
       arguments are data, skylight, blocklight, return RGB */
    void (*calculate_light_color)(void *, unsigned char, unsigned char, unsigned char *, unsigned char *, unsigned char *);
    
    /* can be set to 0 in derived modes to indicate that lighting the chunk
     * sides is actually important. Right now, this is used in cave mode
     */
    int skip_sides;
    
    float shade_strength;
    int color_light;
    int night;
} RenderModeLighting;
extern RenderModeInterface rendermode_lighting;

/* exposed so it can be used in other per-face occlusion checks */
int rendermode_lighting_is_face_occluded(RenderState *state, int skip_sides, int x, int y, int z);

/* exposed so sub-modes can look at colors directly */
void get_lighting_color(RenderModeLighting *self, RenderState *state,
                        int x, int y, int z,
                        unsigned char *r, unsigned char *g, unsigned char *b);

/* SMOOTH LIGHTING */
typedef struct {
    /* inherits from lighting */
    RenderModeLighting parent;
} RenderModeSmoothLighting;
extern RenderModeInterface rendermode_smooth_lighting;

/* SPAWN */
typedef struct {
    /* inherits from overlay */
    RenderModeOverlay parent;
    
    PyObject *skylight, *blocklight;
} RenderModeSpawn;
extern RenderModeInterface rendermode_spawn;

/* CAVE */
typedef struct {
    /* render blocks with lighting mode */
    RenderModeLighting parent;

    /* data used to know where the surface is */
    PyObject *skylight;
    PyObject *left_skylight;
    PyObject *right_skylight;
    PyObject *up_left_skylight;
    PyObject *up_right_skylight;

    /* data used to know where the surface is */
    PyObject *blocklight;
    PyObject *left_blocklight;
    PyObject *right_blocklight;
    PyObject *up_left_blocklight;
    PyObject *up_right_blocklight;

    /* colors used for tinting */
    PyObject *depth_colors;
    
    int depth_tinting;
    int only_lit;
    int lighting;
} RenderModeCave;
extern RenderModeInterface rendermode_cave;

/* MINERAL */
typedef struct {
    /* inherits from overlay */
    RenderModeOverlay parent;
    
    void *minerals;
} RenderModeMineral;
extern RenderModeInterface rendermode_mineral;
#endif /* OLD_MODES */

#endif /* __RENDERMODES_H_INCLUDED__ */
