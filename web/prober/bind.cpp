#include <Python.h>

#include "bind.hpp"
#include "methodobject.h"
#include "object.h"

static PyMethodDef Prober_methods[] = {
    {"print_all", (PyCFunction)Prober_print_all, METH_VARARGS, ""},
    {NULL} /* Sentinel */
};

static PyTypeObject ProberType{
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0) //
                   .tp_name = "proberffi.Prober",
    .tp_basicsize = sizeof(ProberObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("prober"),
    .tp_new = Prober_new,
    .tp_init = (initproc)Prober_init,
    .tp_dealloc = (destructor)Prober_dealloc,
    .tp_methods = Prober_methods,
};

static PyModuleDef probermodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "proberffi",
    .m_doc = "",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_proberffi(void) {
  PyObject *m;
  if (PyType_Ready(&ProberType) < 0)
    return NULL;

  m = PyModule_Create(&probermodule);
  if (m == NULL)
    return NULL;

  if (PyModule_AddObjectRef(m, "Prober", (PyObject *)&ProberType) < 0) {
    Py_DECREF(m);
    return NULL;
  }

  return m;
}
PyObject *Prober_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  ProberObject *self;
  self = (ProberObject *)type->tp_alloc(type, 0);
  self->p = new Prober();
  return (PyObject *)self;
}
void Prober_dealloc(ProberObject *t) { free(t->p); }

PyObject *Prober_print_all(ProberObject *self, PyObject *args) {
  char *buf;
  char *f_name = NULL, *f_args = NULL;
  int ret, i;

  if (!self->filename) {
    av_log(NULL, AV_LOG_ERROR, "You have to specify one input file.\n");
    ret = AVERROR(EINVAL);
  } else if (self->filename) {
    ret = self->p->probe_file(self->filename, self->p->print_input_filename);
    if (ret < 0 && 0) {
      self->p->show_error(ret);
    }
  }

  return PyBool_FromLong(ret < 0);
}

int Prober_init(ProberObject *self, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple(args, "s", &self->filename)) {
    return -1;
  }

  return 0;
}
