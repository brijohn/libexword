
%include <cstring.i>

%typemap(out) exword_dirent_t * Exword::DirList {
  exword_list_t *list;
  size_t i;
  list = (exword_list_t *)calloc(1, sizeof(exword_list_t));
  list->files = $1;
  if($1 != NULL) {
    for ( i = 0; $1[i].name != NULL; ++i) {
      list->len += 1;
    }
  }
  $result = SWIG_NewPointerObj(SWIG_as_voidptr(list), $descriptor(exword_list_t *), SWIG_POINTER_OWN | 0);
}

%typemap(out) uint8_t connected %{
  $result = PyBool_FromLong((long)$1);
%}

%typemap(out) char * name %{
  if (arg1->flags & 0x2) {
#if PY_VERSION_HEX >= 0x03000000
    $result = PyUnicode_Decode($1, arg1->size - 5, "utf-16be", "strict");
#else
    $result = PyString_Decode($1, arg1->size - 5, "utf-16be", "strict");
#endif
  } else {
    $result = SWIG_FromCharPtrAndSize($1, ($1 ? strlen($1) : 0));
  }
%}

%typemap(out) SWIGTYPE * SUBOBJECT %{
  $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), $1_descriptor, SWIG_POINTER_OWN | 0);
%}

%apply SWIGTYPE * SUBOBJECT { exword_model_t * model };
%apply SWIGTYPE * SUBOBJECT { exword_capacity_t * capacity };


%cstring_output_allocate_size(char **buffer, int *len, free(*$1));
%cstring_chunk_output(char *xor, 16)
%cstring_chunk_output(char *challenge, 20)
%cstring_chunk_output(char *key, 16)
%cstring_chunk_output(char *xorkey, 16)
%cstring_input_binary(char *buffer, int len);
%cstring_input_binary(char *data, int len);

