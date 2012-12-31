%{

static PyObject* PyExc_ExwordForbidden;
static PyObject* PyExc_ExwordNotFound;
static PyObject* PyExc_ExwordOther;
static PyObject* PyExc_ExwordInternal;

int check_error() {
	int ret = 0;
	if (err_no) {
		ret = err_no;
		err_no = 0;
	}
	return ret;
}


void throw_exword_exception(int code) {
	switch(code) {
	case EXWORD_ERROR_FORBIDDEN:
		PyErr_SetString(PyExc_ExwordForbidden, "Access Forbidden");
		break;
	case EXWORD_ERROR_NOT_FOUND:
		PyErr_SetString(PyExc_ExwordNotFound, "Device or File Not Found");
		break;
	case EXWORD_ERROR_INTERNAL:
		PyErr_SetString(PyExc_ExwordInternal, "Internal Error");
		break;
	case EXWORD_ERROR_NO_MEM:
		PyErr_SetString(PyExc_MemoryError, "Insufficient Memory");
		break;
	default:
		PyErr_SetString(PyExc_ExwordOther, "Unknown Error");
	}
}

%}


%init %{

PyExc_ExwordForbidden = PyErr_NewException("_exword.PyExc_ExwordForbidden", NULL, NULL);
Py_INCREF(PyExc_ExwordForbidden);
PyModule_AddObject(m, "PyExc_ExwordForbidden", PyExc_ExwordForbidden);

PyExc_ExwordNotFound = PyErr_NewException("_exword.PyExc_ExwordNotFound", NULL, NULL);
Py_INCREF(PyExc_ExwordNotFound);
PyModule_AddObject(m, "PyExc_ExwordNotFound", PyExc_ExwordNotFound);

PyExc_ExwordInternal = PyErr_NewException("_exword.PyExc_ExwordInternal", NULL, NULL);
Py_INCREF(PyExc_ExwordInternal);
PyModule_AddObject(m, "PyExc_ExwordInternal", PyExc_ExwordInternal);

PyExc_ExwordOther = PyErr_NewException("_exword.PyExc_ExwordOther", NULL, NULL);
Py_INCREF(PyExc_ExwordOther);
PyModule_AddObject(m, "PyExc_ExwordOther", PyExc_ExwordOther);

%}

%exception Exword::model {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::capacity {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::Connect {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::DirList {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::SetPath {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::GetFile {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::SendFile {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::RemoveFile {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::AuthInfo {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::AuthChallenge {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::UserId {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::CryptKey {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::Lock {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::Unlock {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::CName {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception Exword::SdFormat {
	int err;
	$action
	if((err = check_error())) {
		throw_exword_exception(err);
		SWIG_fail;
	}
}

%exception exword_list_t::__getitem__ {
	$action
	if(check_error()) {
		SWIG_exception_fail(SWIG_IndexError, "Index out of bounds");
	}
}

%pythoncode {
PyExc_ExwordForbidden = _exword.PyExc_ExwordForbidden
PyExc_ExwordNotFound = _exword.PyExc_ExwordNotFound
PyExc_ExwordInternal = _exword.PyExc_ExwordInternal
PyExc_ExwordOther = _exword.PyExc_ExwordOther
}
