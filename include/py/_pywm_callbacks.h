#ifndef _PYWM_CALLBACKS_H
#define _PYWM_CALLBACKS_H

#include <Python.h>

struct _pywm_callbacks {
    PyObject* ready;
    PyObject* layout_change;
    PyObject* motion;
    PyObject* button;
    PyObject* axis;
    PyObject* key;
    PyObject* modifiers;

    PyObject* update_view;
    PyObject* destroy_view;
    PyObject* view_event;
    PyObject* view_resized;

    PyObject* query_new_widget;
    PyObject* update_widget;
    PyObject* update_widget_pixels;
    PyObject* query_destroy_widget;

    PyObject* update;
};

void _pywm_callbacks_init();
PyObject** _pywm_callbacks_get(const char* name);

struct _pywm_callbacks* _pywm_callbacks_get_all();

#endif
