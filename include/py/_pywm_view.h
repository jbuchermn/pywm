#ifndef _PYWM_VIEW_H
#define _PYWM_VIEW_H

#include <Python.h>

struct wm_view;

struct _pywm_view {
    long handle;
    struct wm_view* view;

    struct _pywm_view* next_view;

    bool focus_pending;
    bool size_pending;
    struct {
        int width;
        int height;
    } size;
};

void _pywm_view_init(struct _pywm_view* _view, struct wm_view* view);

struct _pywm_views {
    struct _pywm_view* first_view;
};

void _pywm_views_init();
long _pywm_views_add(struct wm_view* view);
long _pywm_views_get_handle(struct wm_view* view);
long _pywm_views_remove(struct wm_view* view);
struct _pywm_view* _pywm_views_container_from_handle(long handle);
struct wm_view* _pywm_views_from_handle(long handle);

void _pywm_views_update();

/*
 * Python functions
 */
PyObject* _pywm_view_get_size_constraints(PyObject* self, PyObject* args);
PyObject* _pywm_view_get_box(PyObject* self, PyObject* args);
PyObject* _pywm_view_get_size(PyObject* self, PyObject* args);
PyObject* _pywm_view_get_info(PyObject* self, PyObject* args);
PyObject* _pywm_view_set_box(PyObject* self, PyObject* args);
PyObject* _pywm_view_set_size(PyObject* self, PyObject* args);
PyObject* _pywm_view_focus(PyObject* self, PyObject* args);
PyObject* _pywm_view_is_floating(PyObject* self, PyObject* args);
PyObject* _pywm_view_get_parent(PyObject* self, PyObject* args);

extern PyMethodDef _pywm_view_methods[];

#endif
