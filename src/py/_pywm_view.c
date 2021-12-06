#include <Python.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "wm/wm.h"
#include "wm/wm_view.h"
#include "wm/wm_output.h"
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

    _view->update_cnt = 0;
}


void _pywm_view_update(struct _pywm_view* view){

    /* General info */
    PyObject* args_general = Py_None;
    if(++view->update_cnt % 20 == 1){
        long parent_handle = 0;
        struct wm_view* parent = wm_view_get_parent(view->view);
        if(parent){
            parent_handle = _pywm_views_get_handle(parent);
        }

        pid_t pid;
        uid_t uid;
        gid_t gid;
        wm_view_get_credentials(view->view, &pid, &uid, &gid);

        const char* title;
        const char* app_id;
        const char* role;
        wm_view_get_info(view->view, &title, &app_id, &role);

        /* If title is not yet there - resend */
        if(!title || !strcmp(title, "")){
            view->update_cnt--;
        }

        bool xwayland = wm_view_is_xwayland(view->view);

        args_general = Py_BuildValue(
                "(lOisss)",
                parent_handle,
                xwayland ? Py_True : Py_False,
                pid,
                app_id,
                role,
                title);
    }

    /* Current info */
    int width, height;
    wm_view_get_size(view->view, &width, &height);

    int is_mapped = view->view->mapped;

    int is_floating = wm_view_is_floating(view->view);
    int is_focused = wm_view_is_focused(view->view);
    int is_fullscreen = wm_view_is_fullscreen(view->view);
    int is_maximized = wm_view_is_maximized(view->view);
    int is_resizing = wm_view_is_resizing(view->view);

    int is_inhibiting_idle = wm_view_is_inhibiting_idle(view->view);

    struct wm_output* fixed_output = wm_content_get_output(&view->view->super);
    int fixed_output_key = fixed_output ? fixed_output->key : -1;

    int* size_constraints;
    int n_constraints;
    wm_view_get_size_constraints(view->view, &size_constraints, &n_constraints);
    PyObject* args_size_constraints = PyList_New(n_constraints);
    for (int i=0; i<n_constraints; i++){
        PyObject* cur = Py_BuildValue("i", size_constraints[i]);
        PyList_SetItem(args_size_constraints, i, cur);
    }

    int offset_x, offset_y;
    wm_view_get_offset(view->view, &offset_x, &offset_y);


    PyObject* args = Py_BuildValue(
            "(lOiiOOOOOOOOiii)",

            view->handle,
            args_general,

            width,
            height,

            is_mapped ? Py_True : Py_False,
            is_floating ? Py_True : Py_False,
            is_focused ? Py_True : Py_False,
            is_fullscreen ? Py_True : Py_False,
            is_maximized ? Py_True : Py_False,
            is_resizing ? Py_True : Py_False,
            is_inhibiting_idle ? Py_True : Py_False,

            args_size_constraints,

            offset_x,
            offset_y,
            fixed_output_key);



    PyObject* res = PyObject_Call(_pywm_callbacks_get_all()->update_view, args, NULL);
    if(args_general != Py_None)
        Py_XDECREF(args_general);
    Py_XDECREF(args_size_constraints);
    Py_XDECREF(args);

    if(res && res != Py_None){
        double x, y, w, h;
        double opacity;
        double mask_x, mask_y, mask_w, mask_h;
        double corner_radius;
        int floating, focus_pending, resizing_pending, fullscreen_pending, maximized_pending, close_pending;
        int width_pending, height_pending;
        int accepts_input, z_index;
        int lock_enabled;
        int new_fixed_output_key;
        double workspace_x, workspace_y, workspace_w, workspace_h;
        
        if(!PyArg_ParseTuple(res, 
                    "(dddd)(dddd)ddippi(ii)iiiiii(dddd)",
                    &x, &y, &w, &h,
                    &mask_x, &mask_y, &mask_w, &mask_h,
                    &opacity,
                    &corner_radius,

                    &z_index,
                    &accepts_input,
                    &lock_enabled,
                    &floating,

                    &width_pending, &height_pending,
                    &focus_pending,
                    &fullscreen_pending,
                    &maximized_pending,
                    &resizing_pending,
                    &close_pending,
                    &new_fixed_output_key,
                    &workspace_x, &workspace_y, &workspace_w, &workspace_h
        )){
            fprintf(stderr, "Error parsing update view return...\n");
            PyErr_SetString(PyExc_TypeError, "Cannot parse update_view return");
        }else{

            wm_content_set_opacity(&view->view->super, opacity);
            wm_content_set_mask(&view->view->super, mask_x, mask_y, mask_w, mask_h);
            wm_content_set_corner_radius(&view->view->super, corner_radius);
            if(floating >= 0)
                wm_view_set_floating(view->view, floating);
            wm_content_set_box(&view->view->super, x, y, w, h);

            if(width_pending > 0 && height_pending > 0)
                wm_view_request_size(view->view, width_pending, height_pending);

            if(focus_pending != -1 && focus_pending)
                wm_focus_view(view->view);
            if(resizing_pending != -1)
                wm_view_set_resizing(view->view, resizing_pending);
            if(fullscreen_pending != -1)
                wm_view_set_fullscreen(view->view, fullscreen_pending);
            if(maximized_pending != -1)
                wm_view_set_maximized(view->view, maximized_pending);
            if(close_pending != -1 && close_pending)
                wm_view_request_close(view->view);
            wm_content_set_z_index(&view->view->super, z_index);
            wm_content_set_lock_enabled(&view->view->super, lock_enabled);

            view->view->accepts_input = accepts_input;
            if(new_fixed_output_key != fixed_output_key)
                wm_content_set_output(&view->view->super, new_fixed_output_key, NULL);
            wm_content_set_workspace(&view->view->super, workspace_x, workspace_y, workspace_w, workspace_h);
        }

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
    for(struct _pywm_view* view=views.first_view; view; view=view->next_view){
        _pywm_view_update(view);
    }
}

void _pywm_views_update_single(struct wm_view* view){
    for(struct _pywm_view* v=views.first_view; v; v=v->next_view){
        if(v->view == view){
            _pywm_view_update(v);
            break;
        }
    }
}
