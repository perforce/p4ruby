/*******************************************************************************
 * Hack to get garbage collection working reliably and portably.
 ******************************************************************************/

#ifndef _GC_HACK_INCLUDED
#  define _GC_HACK_INCLUDED

#  define rb_gc_mark(value) ((void (*)(VALUE))(rb_gc_mark))(value)

#endif
