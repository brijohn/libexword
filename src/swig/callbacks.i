
%{


static void PythonXferCallBack(char *filename, uint32_t transferred, uint32_t length, void *user_data)
{
   PyObject *func, *arglist;
   PyObject *result;
   func = (PyObject *)user_data;
   arglist = Py_BuildValue("(skk)", filename, transferred, length);
   result = PyEval_CallObject(func, arglist);
   Py_DECREF(arglist);
   Py_XDECREF(result);
}

static void PythonDisconnectCallBack(int reason, void *user_data)
{
   PyObject *func, *arglist;
   PyObject *result;
   func = (PyObject *)user_data;
   arglist = Py_BuildValue("(k)", reason);
   result = PyEval_CallObject(func, arglist);
   Py_DECREF(arglist);
   Py_XDECREF(result);
}

%}

%typemap(in) PyObject *PyFunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

%extend Exword {
	void SetGetCallback(PyObject *PyFunc) {
		exword_register_xfer_get_callback($self->device, PythonXferCallBack, (void *) PyFunc);
		Py_INCREF(PyFunc);
	}

	void SetPutCallback(PyObject *PyFunc) {
		exword_register_xfer_put_callback($self->device, PythonXferCallBack, (void *) PyFunc);
		Py_INCREF(PyFunc);
	}
	void SetDisconnectCallback(PyObject *PyFunc) {
		exword_register_disconnect_callback($self->device, PythonDisconnectCallBack, (void *) PyFunc);
		Py_INCREF(PyFunc);
	}
}

