#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <wlr/util/log.h>
#include "wm/wm.h"
#include "wm/wm_config.h"
#include "py/_pywm_callbacks.h"
#include "py/_pywm_view.h"
#include "py/_pywm_widget.h"

static void sigsegv_handler(int sig) {
    void *array[10];
    size_t size;

    size = backtrace(array, 10);
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}


static bool cursor_update_pending = false;
static bool terminate_pending = false;

static void handle_update(){
    if(cursor_update_pending){
        wm_update_cursor();
        cursor_update_pending = false;
    }

    if(terminate_pending){
        wm_terminate();
    }

    _pywm_widgets_update();
    _pywm_views_update();
}

static PyObject* _pywm_update_cursor(PyObject* self, PyObject* args){
    cursor_update_pending = true;

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject* _pywm_terminate(PyObject* self, PyObject* args){
    terminate_pending = true;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* _pywm_run(PyObject* self, PyObject* args, PyObject* kwargs){
    /* Dubug: Print stacktrace upon segfault */
    signal(SIGSEGV, sigsegv_handler);

    int status;

    double output_scale = 1.;
    const char* xcursor_theme = NULL;
    const char* xcursor_name = "left_ptr";
    int xcursor_size = 24;
    char* kwlist[] = {
        "output_scale",
        "xcursor_theme",
        "xcursor_name",
        "xcursor_size",
        NULL
    };
    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "|dssi", kwlist, &output_scale, &xcursor_theme, &xcursor_name, &xcursor_size)){
        PyErr_SetString(PyExc_TypeError, "Arguments");
        return NULL;
    }

    /* Register callbacks immediately, might be called during init */
    get_wm()->callback_update = handle_update;
    _pywm_callbacks_init();

    struct wm_config config = {
        .output_scale = output_scale,
        .xcursor_theme = xcursor_theme,
        .xcursor_name = xcursor_name,
        .xcursor_size = xcursor_size
    };
    wm_init(&config);

    Py_BEGIN_ALLOW_THREADS;
    status = wm_run();
    Py_END_ALLOW_THREADS;

    return Py_BuildValue("i", status);
}

static PyObject* _pywm_register(PyObject* self, PyObject* args){
    const char* name;
    PyObject* callback;

    if(!PyArg_ParseTuple(args, "sO", &name, &callback)){
        PyErr_SetString(PyExc_TypeError, "Invalid parameters");
        return NULL;
    }

    if(!PyCallable_Check(callback)){
        PyErr_SetString(PyExc_TypeError, "Object is not callable");
        return NULL;
    }
    
    PyObject** target = _pywm_callbacks_get(name);
    if(!target){
        PyErr_SetString(PyExc_TypeError, "Unknown callback");
        return NULL;
    }

    Py_XDECREF(*target);
    *target = callback;
    Py_INCREF(*target);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef _pywm_methods[] = {
    { "run",                    (PyCFunction)_pywm_run,       METH_VARARGS | METH_KEYWORDS,   "Start the compoitor in this thread" },
    { "terminate",              _pywm_terminate,              METH_VARARGS,                   "Terminate compositor"  },
    { "register",               _pywm_register,               METH_VARARGS,                   "Register callback"  },
    { "update_cursor",          _pywm_update_cursor,          METH_VARARGS,                   "Update cursor position within clients after moving a client"  },
    { "view_get_box",           _pywm_view_get_box,           METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "view_get_dimensions",    _pywm_view_get_dimensions,    METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "view_get_info",          _pywm_view_get_info,          METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "view_set_box",           _pywm_view_set_box,           METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "view_set_dimensions",    _pywm_view_set_dimensions,    METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "view_focus",             _pywm_view_focus,             METH_VARARGS,                   "" },
    { "widget_create",          _pywm_widget_create,          METH_VARARGS,                   "" },
    { "widget_destroy",         _pywm_widget_destroy,         METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "widget_set_box",         _pywm_widget_set_box,         METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "widget_set_layer",       _pywm_widget_set_layer,       METH_VARARGS,                   "" },  /* Asynchronous. segfaults? */
    { "widget_set_pixels",      _pywm_widget_set_pixels,      METH_VARARGS,                   "" },

    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef _pywm = {
    PyModuleDef_HEAD_INIT,
    "_pywm",
    "",
    -1,
    _pywm_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__pywm(void){
    return PyModule_Create(&_pywm);
}
