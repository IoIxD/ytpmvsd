#ifndef __LIBMYPY_H__
#define __LIBMYPY_H__

#include "prober.hpp"
#include <Python.h>

typedef struct {
  PyObject_HEAD;
  Prober *p;
  const char *filename;
} ProberObject;

PyObject *Prober_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Prober_init(ProberObject *self, PyObject *args, PyObject *kwds);

void Prober_dealloc(ProberObject *self);
PyObject *Prober_print_all(ProberObject *self, PyObject *args);

#endif
