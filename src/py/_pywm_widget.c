#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <drm_fourcc.h>

#include "wm/wm.h"
#include "wm/wm_widget.h"
#include "wm/wm_composite.h"
#include "py/_pywm_widget.h"
#include "py/_pywm_callbacks.h"
#include "wm/wm_util.h"

static struct _pywm_widgets widgets = { 0 };
static long next_handle = 1;

void _pywm_widget_init(struct _pywm_widget* _widget, struct wm_widget* widget, struct wm_composite* composite){
    _widget->handle = (next_handle++);
    _widget->widget = widget;
    _widget->composite = composite;

    assert((_widget->widget || _widget->composite) && !(_widget->widget && _widget->composite));
    _widget->super = _widget->widget ? &_widget->widget->super : &_widget->composite->super;

    _widget->next_widget = NULL;
}

void _pywm_widget_update(struct _pywm_widget* widget){
    PyObject* args = Py_BuildValue("(l)", widget->handle);
    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->update_widget, args, NULL);
    Py_XDECREF(args);
    if(res && res != Py_None){
        double x, y, w, h;
        double mask_x, mask_y, mask_w, mask_h;
        int output_key;
        double opacity;
        double corner_radius;
        double z_index;
        int lock_enabled;
        double workspace_x, workspace_y, workspace_w, workspace_h;
        PyObject* pixels;
        PyObject* primitive;
        if(!PyArg_ParseTuple(res, 
                    "p(dddd)(dddd)iddd(dddd)OO",
                    &lock_enabled,
                    &x, &y, &w, &h,
                    &mask_x, &mask_y, &mask_w, &mask_h,
                    &output_key,
                    &opacity,
                    &corner_radius,
                    &z_index,
                    &workspace_x, &workspace_y, &workspace_w, &workspace_h, &pixels, &primitive
           )){
            PyErr_SetString(PyExc_TypeError, "Cannot parse update_widget return");
            return;
        }

        wm_content_set_opacity(widget->super, opacity);
        wm_content_set_corner_radius(widget->super, corner_radius);
        if(w >= 0.0 && h >= 0.0)
            wm_content_set_box(widget->super, x, y, w, h);
        wm_content_set_mask(widget->super, mask_x, mask_y, mask_w, mask_h);
        wm_content_set_z_index(widget->super, z_index);
        wm_content_set_lock_enabled(widget->super, lock_enabled);

        wm_content_set_output(widget->super, output_key, NULL);
        wm_content_set_workspace(widget->super, workspace_x, workspace_y, workspace_w, workspace_h);

        if(pixels && pixels != Py_None && widget->widget){
            int stride, width, height;
            PyObject* data;
            if(!PyArg_ParseTuple(pixels, "iiiS", &stride, &width, &height, &data)){
                PyErr_SetString(PyExc_TypeError, "Cannot parse pixels");
                return;
            }

            wm_widget_set_pixels(widget->widget,
                    DRM_FORMAT_ARGB8888,
                    stride,
                    width,
                    height,
                    PyBytes_AsString(data));
        }

        if(primitive && primitive != Py_None){
            char* name;
            PyObject* params_int;
            PyObject* params_float;
            if(!PyArg_ParseTuple(primitive, "sOO", &name, &params_int, &params_float)){
                PyErr_SetString(PyExc_TypeError, "Cannot parse primitive");
                return;
            }

            if(!params_int || !params_float || !PyList_Check(params_int) || !PyList_Check(params_float)){
                PyErr_SetString(PyExc_TypeError, "Cannot parse primitive lists");
                return;
            }

            int* p_int = malloc(PyList_Size(params_int) * sizeof(int));
            float* p_float = malloc(PyList_Size(params_float) * sizeof(int));

            for(int i=0; i<PyList_Size(params_int); i++){
                p_int[i] = PyLong_AsLong(PyList_GetItem(params_int, i));
            }
            for(int i=0; i<PyList_Size(params_float); i++){
                p_float[i] = PyFloat_AsDouble(PyList_GetItem(params_float, i));
            }

            if(widget->widget){
                wm_widget_set_primitive(widget->widget, strdup(name), PyList_Size(params_int), p_int, PyList_Size(params_float), p_float);
            }else{
                wm_composite_set_type(widget->composite, name, PyList_Size(params_int), p_int, PyList_Size(params_float), p_float);
            }

        }

    }

    Py_XDECREF(res);
}

long _pywm_widgets_add(struct wm_widget* widget, struct wm_composite* composite){
    struct _pywm_widget* it;
    for(it = widgets.first_widget; it && it->next_widget; it=it->next_widget);
    struct _pywm_widget** insert;
    if(it){
        insert = &it->next_widget;
    }else{
        insert = &widgets.first_widget;
    }

    *insert = malloc(sizeof(struct _pywm_widget));
    _pywm_widget_init(*insert, widget, composite);
    return (*insert)->handle;
}

long _pywm_widgets_remove(struct wm_content* content){
    struct _pywm_widget* remove;
    if(widgets.first_widget && widgets.first_widget->super == content){
        remove = widgets.first_widget;
        widgets.first_widget = remove->next_widget;
    }else{
        struct _pywm_widget* prev;
        for(prev = widgets.first_widget; prev && prev->next_widget && prev->next_widget->super != content; prev=prev->next_widget);
        assert(prev);

        remove = prev->next_widget;
        prev->next_widget = remove->next_widget;
    }

    assert(remove);
    long handle = remove->handle;
    free(remove);

    return handle;
}

long _pywm_widgets_get_handle(struct wm_content* content){
    for(struct _pywm_widget* it = widgets.first_widget; it; it=it->next_widget){
        if(it->super == content) return it->handle;
    }

    return 0;
}


void _pywm_widgets_update(){
    /* Query for a widget to destroy */
    PyObject* args = Py_BuildValue("()");
    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->query_destroy_widget, args, NULL);
    Py_XDECREF(args);
    if(res && res != Py_None){
        long handle = PyLong_AsLong(res);
        if(handle < 0){
            PyErr_SetString(PyExc_TypeError, "Expected long");
            goto err;
        }

        struct wm_content* content = _pywm_widgets_from_handle(handle);
        if(!content){
            PyErr_SetString(PyExc_TypeError, "Widget has been destroyed");
            goto err;
        }
        _pywm_widgets_remove(content);
        wm_content_destroy(content);
    }
    Py_XDECREF(res);

    /* Query for a new widget */
    args = Py_BuildValue("(l)", next_handle);
    res = PyObject_Call(_pywm_callbacks_get_all()->query_new_widget, args, NULL);
    Py_XDECREF(args);
    if(res && res != Py_None){
        long r = PyLong_AsLong(res);
        if(r == 1){
            struct wm_widget* widget = calloc(1, sizeof(struct wm_widget));
            wm_widget_init(widget, get_wm()->server);
            _pywm_widgets_add(widget, NULL);
        }else if(r == 2){
            struct wm_composite* composite = calloc(1, sizeof(struct wm_composite));
            wm_composite_init(composite, get_wm()->server);
            _pywm_widgets_add(NULL, composite);
        }
    }
    Py_XDECREF(res);

    /* Update existing widgets */
    for(struct _pywm_widget* widget = widgets.first_widget; widget; widget=widget->next_widget){
        _pywm_widget_update(widget);
    }

err:
    ;
}


struct _pywm_widget* _pywm_widgets_container_from_handle(long handle){

    for(struct _pywm_widget* widget = widgets.first_widget; widget; widget=widget->next_widget){
        if(widget->handle == handle) return widget;
    }

    return NULL;
}

struct wm_content* _pywm_widgets_from_handle(long handle){
    struct _pywm_widget* widget = _pywm_widgets_container_from_handle(handle);
    if(!widget){
        return NULL;
    }

    return widget->super;
}

