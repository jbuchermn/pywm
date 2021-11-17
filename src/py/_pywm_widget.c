#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <drm_fourcc.h>

#include "wm/wm.h"
#include "wm/wm_widget.h"
#include "py/_pywm_widget.h"
#include "py/_pywm_callbacks.h"
#include "wm/wm_util.h"

static struct _pywm_widgets widgets = { 0 };
static long next_handle = 1;

void _pywm_widget_init(struct _pywm_widget* _widget, struct wm_widget* widget){
    _widget->handle = (next_handle++);
    _widget->widget = widget;
    _widget->next_widget = NULL;
}

void _pywm_widget_update(struct _pywm_widget* widget){
    PyObject* args = Py_BuildValue("(l)", widget->handle);
    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->update_widget, args, NULL);
    Py_XDECREF(args);
    if(res && res != Py_None){
        double x, y, w, h;
        double mask_x, mask_y, mask_w, mask_h;
        char* output_name;
        double opacity;
        int z_index;
        int lock_enabled;
        if(!PyArg_ParseTuple(res, 
                    "p(dddd)(dddd)sdi",
                    &lock_enabled,
                    &x, &y, &w, &h,
                    &mask_x, &mask_y, &mask_w, &mask_h,
                    &output_name,
                    &opacity,
                    &z_index)){
            PyErr_SetString(PyExc_TypeError, "Cannot parse update_widget return");
            return;
        }

        wm_content_set_opacity(&widget->widget->super, opacity);
        if(w >= 0.0 && h >= 0.0)
            wm_content_set_box(&widget->widget->super, x, y, w, h);
        wm_content_set_mask(&widget->widget->super, mask_x, mask_y, mask_w, mask_h);
        wm_content_set_z_index(&widget->widget->super, z_index);
        wm_content_set_lock_enabled(&widget->widget->super, lock_enabled);

        wm_content_set_output(&widget->widget->super, output_name, NULL);
    }
    Py_XDECREF(res);

    args = Py_BuildValue("(l)", widget->handle);
    res = PyObject_Call(_pywm_callbacks_get_all()->update_widget_pixels, args, NULL);
    Py_XDECREF(args);
    if(res && res != Py_None){
        /* Handle update_pixels */
        int stride, width, height;
        PyObject* data;
        if(!PyArg_ParseTuple(res, "iiiS", &stride, &width, &height, &data)){
            PyErr_SetString(PyExc_TypeError, "Cannot parse update_widget_pixels return");
            return;
        }

        wm_widget_set_pixels(widget->widget,
                DRM_FORMAT_ARGB8888,
                stride,
                width,
                height,
                PyBytes_AsString(data));
    }

    Py_XDECREF(res);
}

long _pywm_widgets_add(struct wm_widget* widget){
    struct _pywm_widget* it;
    for(it = widgets.first_widget; it && it->next_widget; it=it->next_widget);
    struct _pywm_widget** insert;
    if(it){
        insert = &it->next_widget;
    }else{
        insert = &widgets.first_widget;
    }

    *insert = malloc(sizeof(struct _pywm_widget));
    _pywm_widget_init(*insert, widget);
    return (*insert)->handle;
}

long _pywm_widgets_remove(struct wm_widget* widget){
    struct _pywm_widget* remove;
    if(widgets.first_widget && widgets.first_widget->widget == widget){
        remove = widgets.first_widget;
        widgets.first_widget = remove->next_widget;
    }else{
        struct _pywm_widget* prev;
        for(prev = widgets.first_widget; prev && prev->next_widget && prev->next_widget->widget != widget; prev=prev->next_widget);
        assert(prev);

        remove = prev->next_widget;
        prev->next_widget = remove->next_widget;
    }

    assert(remove);
    long handle = remove->handle;
    free(remove);

    return handle;
}

long _pywm_widgets_get_handle(struct wm_widget* widget){
    for(struct _pywm_widget* it = widgets.first_widget; it; it=it->next_widget){
        if(it->widget == widget) return it->handle;
    }

    return 0;
}


void _pywm_widgets_update(){
    TIMER_START(widgets_update);

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

        struct wm_widget* widget = _pywm_widgets_from_handle(handle);
        if(!widget){
            PyErr_SetString(PyExc_TypeError, "Widget has been destroyed");
            goto err;
        }
        _pywm_widgets_remove(widget);
        wm_destroy_widget(widget);
    }
    Py_XDECREF(res);

    /* Query for a new widget */
    args = Py_BuildValue("(l)", next_handle);
    res = PyObject_Call(_pywm_callbacks_get_all()->query_new_widget, args, NULL);
    Py_XDECREF(args);
    if(res == Py_True){
        struct wm_widget* widget = wm_create_widget();
        _pywm_widgets_add(widget);
    }
    Py_XDECREF(res);

    /* Update existing widgets */
    for(struct _pywm_widget* widget = widgets.first_widget; widget; widget=widget->next_widget){
        _pywm_widget_update(widget);
    }

err:


    TIMER_STOP(widgets_update);
    TIMER_PRINT(widgets_update);
}


struct _pywm_widget* _pywm_widgets_container_from_handle(long handle){

    for(struct _pywm_widget* widget = widgets.first_widget; widget; widget=widget->next_widget){
        if(widget->handle == handle) return widget;
    }
    
    return NULL;
}

struct wm_widget* _pywm_widgets_from_handle(long handle){
    struct _pywm_widget* widget = _pywm_widgets_container_from_handle(handle);
    if(!widget){
        return NULL;
    }
 
    return widget->widget;
}

