#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "wm/wm.h"
#include "wm/wm_view.h"
#include "wm/wm_view_xwayland.h"
#include "wm/wm_util.h"

#include "py/_pywm_view.h"
#include "py/_pywm_callbacks.h"

static struct _pywm_views views = { 0 };

void _pywm_view_init(struct _pywm_view* _view, struct wm_view* view){
    static long handle = 0;
    handle++;

    _view->handle = handle;
    _view->view = view;
    _view->next_view = NULL;
}

void _pywm_view_update(struct _pywm_view* view){

    /* General info - TODO Should not be updated this often */
    long parent_handle = 0;
    struct wm_view* parent = wm_view_get_parent(view->view);
    if(parent){
        parent_handle = _pywm_views_get_handle(parent);
    }
    int is_floating = wm_view_is_floating(view->view);

    const char* title;
    const char* app_id; 
    const char* role;
    wm_view_get_info(view->view, &title, &app_id, &role);

    bool xwayland = wm_view_is_xwayland(view->view);

    int min_w, max_w, min_h, max_h;
    wm_view_get_size_constraints(view->view, &min_w, &max_w, &min_h, &max_h);


    int width, height;
    wm_view_get_size(view->view, &width, &height);

    PyObject* args = Py_BuildValue(
            "(llOsssOiiiiii)",
            view->handle,
            parent_handle,
            is_floating ? Py_True : Py_False,
            title, app_id, role, xwayland ? Py_True : Py_False,
            min_w, max_w, min_h, max_h,
            width, height);


    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->update_view, args, NULL);
    Py_XDECREF(args);

    if(res && res != Py_None){
        double x, y, w, h;
        int focus_pending, width_pending, height_pending;
        int accepts_input, z_index;
        
        if(!PyArg_ParseTuple(res, 
                    "(dddd)p(ii)ii",
                    &x, &y, &w, &h,
                    &focus_pending,
                    &width_pending, &height_pending,
                    &accepts_input, &z_index)){
            PyErr_SetString(PyExc_TypeError, "Cannot parse update_view return");
            return;
        }

        if(w >= 0.0 && h >= 0.0)
            wm_view_set_box(view->view, x, y, w, h);
        if(width_pending > 0 && height_pending > 0)
            wm_view_request_size(view->view, width_pending, height_pending);
        if(focus_pending)
            wm_focus_view(view->view);

        view->view->accepts_input = accepts_input;
        view->view->z_index = z_index;

    }

    Py_XDECREF(res);
}

long _pywm_views_add(struct wm_view* view){
    struct _pywm_view* it;
    for(it = views.first_view; it && it->next_view; it=it->next_view);
    struct _pywm_view** insert;
    if(it){
        insert = &it->next_view;
    }else{
        insert = &views.first_view;
    }

    *insert = malloc(sizeof(struct _pywm_view));
    _pywm_view_init(*insert, view);
    return (*insert)->handle;
}

long _pywm_views_remove(struct wm_view* view){
    struct _pywm_view* remove = NULL;

    if(views.first_view && views.first_view->view == view){
        remove = views.first_view;
        views.first_view = remove->next_view;
    }else{
        struct _pywm_view* prev;
        for(prev = views.first_view; prev && prev->next_view && prev->next_view->view != view; prev=prev->next_view);

        if(prev && prev->next_view){
            remove = prev->next_view;
            prev->next_view = remove->next_view;
        }
    }

    if(remove){
        long handle = remove->handle;
        free(remove);

        return handle;
    }

    return 0;

}

long _pywm_views_get_handle(struct wm_view* view){
    for(struct _pywm_view* it = views.first_view; it; it=it->next_view){
        if(it->view == view) return it->handle;
    }

    return 0;
}

void _pywm_views_update(){
    TIMER_START(views_update);
    for(struct _pywm_view* view=views.first_view; view; view=view->next_view){
        _pywm_view_update(view);
    }
    TIMER_STOP(views_update);
    TIMER_PRINT(views_update);
}
