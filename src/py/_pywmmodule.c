#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <wlr/util/log.h>
#include "wm/wm.h"
#include "wm/wm_config.h"
#include "wm/wm_server.h"
#include "py/_pywm_callbacks.h"
#include "py/_pywm_view.h"
#include "py/_pywm_widget.h"

static void sig_handler(int sig) {
    void *array[10];
    size_t size;

    size = backtrace(array, 100);
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

static void set_config(struct wm_config* conf, PyObject* dict, int reconfigure){
    if(reconfigure){
        wm_config_reset_default(conf);
    }

    PyObject* o;
    o = PyDict_GetItemString(dict, "outputs");
    if(o && PyList_Check(o)){
        PyObject* l = o;
        for(int i=0; i<PyList_Size(l); i++){
            PyObject* c = PyList_GetItem(l, i);
            char* name = "";
            double scale = 0.;
            int width = 0;
            int height = 0;
            int mHz = 0;
            int pos_x = WM_CONFIG_POS_MIN - 1;
            int pos_y = WM_CONFIG_POS_MIN - 1;
            int transform = 0;

            o = PyDict_GetItemString(c, "name"); if(o){ name = PyBytes_AsString(o); }
            o = PyDict_GetItemString(c, "scale"); if(o){ scale = PyFloat_AsDouble(o); }
            o = PyDict_GetItemString(c, "width"); if(o){ width = PyLong_AsLong(o); }
            o = PyDict_GetItemString(c, "height"); if(o){ height = PyLong_AsLong(o); }
            o = PyDict_GetItemString(c, "mHz"); if(o){ mHz = PyLong_AsLong(o); }
            o = PyDict_GetItemString(c, "pos_x"); if(o){ pos_x = PyLong_AsLong(o); }
            o = PyDict_GetItemString(c, "pos_y"); if(o){ pos_y = PyLong_AsLong(o); }
            o = PyDict_GetItemString(c, "transform"); if(o){ transform = PyLong_AsLong(o); }

            wm_config_add_output(conf, name, scale, width, height, mHz, pos_x, pos_y, transform);
        }
    }
    o = PyDict_GetItemString(dict, "xkb_model"); if(o){ strncpy(conf->xkb_model, PyBytes_AsString(o), WM_CONFIG_STRLEN-1); }
    o = PyDict_GetItemString(dict, "xkb_layout"); if(o){ strncpy(conf->xkb_layout, PyBytes_AsString(o), WM_CONFIG_STRLEN-1); }
    o = PyDict_GetItemString(dict, "xkb_options"); if(o){ strncpy(conf->xkb_options, PyBytes_AsString(o), WM_CONFIG_STRLEN-1); }

    o = PyDict_GetItemString(dict, "xcursor_theme"); if(o){ wm_config_set_xcursor_theme(conf, PyBytes_AsString(o)); }
    o = PyDict_GetItemString(dict, "xcursor_size"); if(o){ wm_config_set_xcursor_size(conf, PyLong_AsLong(o)); }

    o = PyDict_GetItemString(dict, "focus_follows_mouse"); if(o){ conf->focus_follows_mouse = o == Py_True; }
    o = PyDict_GetItemString(dict, "constrain_popups_to_toplevel"); if(o){ conf->constrain_popups_to_toplevel = o == Py_True; }
    o = PyDict_GetItemString(dict, "encourage_csd"); if(o){ conf->encourage_csd = o == Py_True; }

    o = PyDict_GetItemString(dict, "tap_to_click"); if(o){ conf->tap_to_click = o == Py_True; }
    o = PyDict_GetItemString(dict, "natural_scroll"); if(o){ conf->natural_scroll = o == Py_True; }

    o = PyDict_GetItemString(dict, "enable_xwayland"); if(o){ conf->enable_xwayland = o == Py_True; }
    o = PyDict_GetItemString(dict, "debug"); if(o){ conf->debug = o == Py_True; }

    if(reconfigure){
        wlr_log(WLR_DEBUG, "Reconfiguring PyWM...");
        wm_config_reconfigure(conf, get_wm()->server);
        wlr_log(WLR_DEBUG, "...done");
    }
}


static void handle_update_view(struct wm_view* view){
    PyGILState_STATE gil = PyGILState_Ensure();
    _pywm_views_update_single(view);
    PyGILState_Release(gil);
}

static void handle_update(){
    PyGILState_STATE gil = PyGILState_Ensure();

    PyObject* args = Py_BuildValue("()");
    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->update, args, NULL);
    Py_XDECREF(args);

    int update_cursor;
    int update_cursor_x;
    int update_cursor_y;
    double lock_perc;
    int terminate;
    const char* open_virtual_output, *close_virtual_output;
    PyObject* config;

    if(!PyArg_ParseTuple(res,
                "iiidsspO",
                &update_cursor,
                &update_cursor_x,
                &update_cursor_y,
                &lock_perc,
                &open_virtual_output,
                &close_virtual_output,
                &terminate,
                &config)){
        PyErr_SetString(PyExc_TypeError, "Cannot parse query return");
    }else{
        if(update_cursor >= 0){
            wm_update_cursor(update_cursor, update_cursor_x, update_cursor_y);
        }
        wm_set_locked(lock_perc);
        if(terminate){
            wm_terminate();
        }

        if(strlen(open_virtual_output) > 0){
            wm_open_virtual_output(open_virtual_output);
        }
        if(strlen(close_virtual_output) > 0){
            wm_close_virtual_output(close_virtual_output);
        }
        if(config && config != Py_None){
            set_config(get_wm()->server->wm_config, config, 1);
        }
    }
    Py_XDECREF(res);


    _pywm_widgets_update();

    _pywm_views_update();


    PyGILState_Release(gil);
}



static PyObject* _pywm_run(PyObject* self, PyObject* args, PyObject* kwargs){
    /* Dubug: Print stacktrace upon segfault etc. */
    signal(SIGSEGV, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    wlr_log(WLR_INFO, "Running PyWM...\n");

    int status = 0;

    struct wm_config conf;
    wm_config_init_default(&conf);

    if(kwargs){
        set_config(&conf, kwargs, 0);
    }

    /* Register callbacks immediately, might be called during init */
    get_wm()->callback_update = handle_update;
    get_wm()->callback_update_view = handle_update_view;
    _pywm_callbacks_init();

    wm_init(&conf);

    Py_BEGIN_ALLOW_THREADS;
    status = wm_run();
    Py_END_ALLOW_THREADS;

    wlr_log(WLR_INFO, "...finished\n");

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
    { "run",                       (PyCFunction)_pywm_run,           METH_VARARGS | METH_KEYWORDS,   "Start the compositor in this thread" },
    { "register",                  _pywm_register,                   METH_VARARGS,                   "Register callback"  },

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
