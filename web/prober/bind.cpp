#include <Python.h>

#include "bind.hpp"
#include "prober.hpp"

char hellofunc_docs[] = "";

PyMethodDef proberffi_funcs[] = {
    {"print_all", (PyCFunction)print_all, METH_VARARGS, hellofunc_docs},
    {NULL}};

char proberffimod_docs[] = "";

PyModuleDef proberffi_mod = {PyModuleDef_HEAD_INIT,
                             "proberffi",
                             proberffimod_docs,
                             -1,
                             proberffi_funcs,
                             NULL,
                             NULL,
                             NULL,
                             NULL};

PyMODINIT_FUNC PyInit_proberffi(void) {
  return PyModule_Create(&proberffi_mod);
}

PyObject *print_all(PyObject *self, PyObject *args) {
  const char *input_filename;

  if (!PyArg_ParseTuple(args, "s", &input_filename)) {
    return NULL;
  }

  Prober *p = new Prober();

  char *buf;
  char *f_name = NULL, *f_args = NULL;
  int ret, i;

  if (!input_filename) {
    av_log(NULL, AV_LOG_ERROR, "You have to specify one input file.\n");
    ret = AVERROR(EINVAL);
  } else if (input_filename) {
    ret = p->probe_file(input_filename, p->print_input_filename);
    if (ret < 0 && 0) {
      p->show_error(ret);
    }
  }

  return PyBool_FromLong(ret < 0);
}
