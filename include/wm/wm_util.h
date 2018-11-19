#ifndef WM_UTIL_H
#define WM_UTIL_H

#define wm_offset_of(_struct_, _member_)  (size_t)&(((struct _struct_ *)0)->_member_)
 
#define wm_cast(_subclass_, _superclass_pt_)                           \
                                                                           \
   ((struct _subclass_ *)(                                                 \
      (unsigned char *)(_superclass_pt_)                                   \
      - wm_offset_of(_subclass_, super)                                    \
      )                                                                    \
    )

#endif
