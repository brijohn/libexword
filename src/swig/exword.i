%module exword

%{
/* Put headers and other declarations here */
#define SWIG_FILE_WITH_INIT
#include "exword.h"
int err_no = 0;

typedef struct {
	exword_t *device;
	uint16_t options;
} Exword;

int Exword_debug_get(Exword *e) {
	return exword_get_debug(e->device);
}

void Exword_debug_set(Exword *e, int debug) {
	exword_set_debug(e->device, debug);
}

exword_model_t * Exword_model_get(Exword *e) {
	exword_model_t *model;
	model = malloc(sizeof(exword_model_t));
	if (model) {
		memset(model, 0, sizeof(exword_model_t));
		err_no = exword_get_model(e->device, model);
	}
	return model;
}

exword_capacity_t * Exword_capacity_get(Exword *e) {
	exword_capacity_t *cap;
	cap = malloc(sizeof(exword_capacity_t));
	if (cap) {
		memset(cap, 0, sizeof(exword_capacity_t));
		err_no = exword_get_capacity(e->device, cap);
	}
	return cap;
}

int Exword_connected_get(Exword *e) {
	return exword_is_connected(e->device);
}

%}

%include "stdint.i"
%include "constants.i"
%include "exceptions.i"
%include "typemaps.i"
%include "callbacks.i"

typedef struct {
%immutable;
	char model[14];
	char sub_model[6];
	char ext_model[6];
	short capabilities;
%mutable;
} exword_model_t;

typedef struct {
%immutable;
	uint64_t total;
	uint64_t free;
%mutable;
} exword_capacity_t;


typedef struct {
%immutable;
	uint8_t  flags;
	char * name;
%mutable;
} exword_dirent_t;


%ignore exword_list_t::files;
%ignore exword_list_t::len;
%inline {
	typedef struct exword_list_t {
		exword_dirent_t *files;
		int len;
	} exword_list_t;
}


%extend exword_list_t {
	~exword_list_t() {
		if($self->files) {
			exword_free_list($self->files);
		}
		free($self);
	}
	int __len__() {
		return $self->len;
	}
	exword_dirent_t * __getitem__(size_t i) {
		if (i >= $self->len) {
			err_no = 20;
			return NULL;
		}
		return &($self->files[i]);
	}
}


typedef struct {
%immutable;
	uint16_t options;
%mutable;
} Exword;

%extend Exword {
	int debug;
	const uint8_t connected;
	const exword_model_t model;
	const exword_capacity_t capacity;
	Exword() {
		exword_t *device = exword_init();
		if (device == NULL)
			return NULL;
		Exword *e = calloc(1, sizeof(Exword));
		e->device = device;
		return e;
	}
	~Exword() {
		exword_deinit($self->device);
		free($self);
	}
	void Connect(uint16_t mode, uint8_t region) {
		$self->options = mode | region;
		err_no = exword_connect($self->device, $self->options);
	}
	void Disconnect() {
		exword_disconnect($self->device);
	}
	exword_dirent_t * DirList() {
		uint16_t count;
		exword_dirent_t * list;
		err_no = exword_list($self->device, &list, &count);
		return list;
	}
	void SetPath(char * path, uint8_t mkdir) {
		err_no = exword_setpath($self->device, path, mkdir);
	}
	void SendFile(char * filename, char * buffer, int len) {
		err_no = exword_send_file($self->device, filename, buffer, len);
	}
	void GetFile(char *filename, char **buffer, int *len) {
		err_no = exword_get_file($self->device, filename, buffer, len);
	}
	void RemoveFile(char * filename) {
		int longname = 0;
		exword_model_t model;
		if ($self->options & EXWORD_MODE_TEXT) {
			err_no = exword_get_model($self->device, &model);
			if (err_no != EXWORD_SUCCESS)
				return;
			if (model.capabilities & CAP_P)
				longname = 1;
		}
		err_no = exword_remove_file($self->device, filename, longname);
	}
	void AuthInfo(char blk1[16], char blk2[24], char *challenge) {
		exword_authinfo_t i;
		memset(&i, 0, sizeof(exword_authinfo_t));
		memcpy(i.blk1, blk1, 16);
		memcpy(i.blk2, blk2, 24);
		err_no = exword_authinfo($self->device, &i);
		memcpy(challenge, i.challenge, 20);
	}
	void AuthChallenge(char challenge[20]) {
		exword_authchallenge_t c;
		memset(&c, 0, sizeof(exword_authchallenge_t));
		memcpy(c.challenge, challenge, 20);
		err_no = exword_authchallenge($self->device, c);
	}
	void UserId(char user[16]) {
		exword_userid_t u;
		memset(&u, 0, sizeof(exword_userid_t));
		memcpy(u.name, user, 16);
		err_no = exword_userid($self->device, u);
	}
	void CryptKey(char blk1[16], char blk2[12], char *key, char *xorkey) {
		exword_cryptkey_t ck;
		memset(&ck, 0, sizeof(exword_cryptkey_t));
		memcpy(ck.blk1, blk1, 16);
		memcpy(ck.blk2, blk2, 12);
		err_no = exword_cryptkey($self->device, &ck);
		memcpy(key, ck.key, 16);
		memcpy(xorkey, ck.xorkey, 16);
	}
	void Lock() {
		err_no = exword_lock($self->device);
	}
	void Unlock() {
		err_no = exword_unlock($self->device);
	}
	void CName(char *name, char *id) {
		err_no = exword_cname($self->device, name, id);
	}
	void SdFormat() {
		err_no = exword_sd_format($self->device);
	}
}

%inline %{

char * CryptData(char *data, int len, char key[16]) {
	crypt_data(data, len, key);
	return data;
}

void GenerateXorKey(char key[16], char * xor) {
	get_xor_key(key, 16, xor);
}

%}

