/*
 * Copyright 2008-2014 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <citrusleaf/cf_b64.h>
#include <citrusleaf/cf_proto.h>

#include <aerospike/as_bytes.h>
#include <aerospike/as_nil.h>
#include <aerospike/as_msgpack.h>

#include <citrusleaf/cl_udf.h>
#include <citrusleaf/cl_write.h>
#include <citrusleaf/cl_parsers.h>

#include "internal.h"

#ifdef __APPLE__
#include <libgen.h>
#endif

/******************************************************************************
 * MACROS
 ******************************************************************************/

#define LOG(msg, ...) \
	// { printf("%s@%s:%d - ", __func__, __FILE__, __LINE__); printf(msg, ##__VA_ARGS__ ); printf("\n"); }

/*
 * Mapping between string udf type and integer type
 */
#define MAX_UDF_TYPE 1
#define UDF_TYPE_LUA 0 
char * cl_udf_type_str[] = {"LUA", 0};

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct cl_udf_filelist_s {
	int         	capacity;
	int         	size;
	cl_udf_file *   files;
} cl_udf_filelist;

/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static void * cl_udf_info_parse(char * key, char * value, void * context)
{
	LOG("key = %s, value=%s", key, value);

	cl_udf_info * info = (cl_udf_info *) context;

	if ( strcmp(key,"error") == 0 ) {
		info->error = strdup(value);
	}
	if ( strcmp(key,"filename") == 0 ) {
		strncpy(info->filename, value, strlen(value));
	}
	else if ( strcmp(key,"gen") == 0 ) {
		info->gen = strdup(value);
	}
	else if ( strcmp(key,"content") == 0 ) {
		as_bytes_destroy(&info->content);
		int c_len = (int)strlen(value);
		uint8_t * c = (uint8_t *) malloc(c_len + 1);
		memcpy(c, value, c_len);
		c[c_len] = 0;
		as_bytes_init_wrap(&info->content, c, c_len + 1, true /*memcpy*/);
	}
	// else if ( strcmp(key,"files") == 0 ) {
	// 	info->files = strdup(value);
	// }
	// else if ( strcmp(key,"count") == 0 ) {
	// 	info->count = atoi(value);
	// }
	else if (strcmp(key, "hash") == 0 ) {
		memcpy(info->hash, value, strlen(value));
	}
	return info;
}


static void * cl_udf_file_parse(char * key, char * value, void * context)
{
	LOG("key = %s, value=%s", key, value);

	cl_udf_file * file = (cl_udf_file *) context;

	if ( strcmp(key,"filename") == 0 ) {
		strncpy(file->name, value, strlen(value));
	}
	else if ( strcmp(key,"content") == 0 ) {
		as_bytes_destroy(file->content);
		int c_len = (int)strlen(value);
		uint8_t * c = (uint8_t *) malloc(c_len + 1);
		memcpy(c, value, c_len);
		c[c_len] = 0;
		as_bytes_init_wrap(file->content, c, c_len + 1, true /*memcpy*/);
	}
	else if (strcmp(key, "hash") == 0 ) {
		memcpy(file->hash, value, strlen(value));
	}
	else if (strcmp(key, "type") == 0 ) {
		file->type = AS_UDF_LUA;
	}

	return file;
}

static void *  cl_udf_filelist_parse(char * filedata, void * context) {

	cl_udf_filelist * filelist = (cl_udf_filelist *) context;
	
	LOG("filedata = %s, cap = %d, size = %d", filedata, filelist->capacity, filelist->size);

	if ( filelist->size < filelist->capacity ) {
		cl_parameters_parser parser = {
			.delim = ',',
			.context = &filelist->files[filelist->size],
			.callback = cl_udf_file_parse
		};
		cl_parameters_parse(filedata, &parser);
		filelist->size++;
	}

	return filelist;
}

/*
static void * cl_udf_list_parse(char * key, char * value, void * context)
{
	LOG("key = %s, value=%s", key, value);

	cl_udf_filelist * filelist = (cl_udf_filelist *) context;
	
	if ( strcmp(key,"files") == 0 ) {
		cl_seq_parser parser = {
			.delim = ';',
			.context = filelist,
			.callback = cl_udf_filelist_parse
		};
		cl_seq_parse(value, &parser);
	}

	return context;
}
*/


static as_val * cl_udf_bin_to_val(as_serializer * ser, cl_bin * bin) {

	as_val * val = NULL;

	switch( bin->object.type ) {
		case CL_INT : {
			val = (as_val *) as_integer_new(bin->object.u.i64);
			break;
		}
		case CL_STR : {
			// steal the pointer from the object into the val
			val = (as_val *) as_string_new(strdup(bin->object.u.str), true /*ismalloc*/);
			break;
		}
		case CL_BLOB:
		case CL_JAVA_BLOB:
		case CL_CSHARP_BLOB:
		case CL_PYTHON_BLOB:
		case CL_RUBY_BLOB:
		case CL_ERLANG_BLOB:
		{
			uint8_t *b = malloc(sizeof(bin->object.sz));
			memcpy(b, bin->object.u.blob, bin->object.sz);
			val = (as_val *)as_bytes_new_wrap(b, (uint32_t)bin->object.sz, true /*ismalloc*/);
		}
		case CL_LIST :
		case CL_MAP : {
			// use a temporary buffer, which doesn't need to be destroyed
			as_buffer buf = {
				.capacity = (uint32_t) bin->object.sz,
				.size = (uint32_t) bin->object.sz,
				.data = (uint8_t *) bin->object.u.blob
			};
			as_serializer_deserialize(ser, &buf, &val);
			break;
		}
		case CL_NULL : {
			val = (as_val*) &as_nil;
			break;
		}
		default : {
			val = NULL;
			break;
		}
	}
	return val;
}

as_val * citrusleaf_udf_bin_to_val(as_serializer * ser, cl_bin * bin) {
	return cl_udf_bin_to_val(ser, bin);
}

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

cl_rv citrusleaf_udf_list(as_cluster *asc, cl_udf_file ** files, int * count, char ** resp) {
	
	*files = NULL;
	*count = 0;

	char *  query   = "udf-list";
	char *  result  = 0;
	
	int rc = citrusleaf_info_cluster(asc, query, &result, true, /* check bounds */ true, 100);
	
	if (rc) {
		*resp = result;
		return rc;
	}
				
	// The code below needs to be kept, it populates the udf file-list and count
	// It has only 1 error check.

	/**
	 * result   := {request}\t{response}
	 * response := filename=<name>,hash=<hash>,type=<type>[:filename=<name>...]
	 */
	
	char * response = strchr(result, '\t') + 1;

	// Calculate number of files mentioned in response
	// Entry for each file is seperated by delim ';'
	char * haystack = response;
	while ( (haystack = strstr (haystack, "filename")) != 0 ) 
	{
		*count = *count + 1;
		haystack = haystack + 8;
	}
	
	if (*count == 0)
	{
		// No files at server
		*files = NULL;
		free(result);
		return 0;
	}
	
	// Allocate memory for filelist
	// caller has to free memory for .files
	cl_udf_filelist filelist = {
		.capacity   = *count,              // Allocated size
		.size       = 0,                   // Actual entries
		.files      = (cl_udf_file *) calloc((*count), sizeof(cl_udf_file)) 
	};

	cl_seq_parser parser = {
		.delim = ';',
		.context = &filelist,
		.callback = cl_udf_filelist_parse
	};
	cl_seq_parse(response, &parser);

	*files = filelist.files;
	*count = filelist.size;

	free(result);
	result = NULL;

	return 0;
}

cl_rv citrusleaf_udf_get(as_cluster *asc, const char * filename, cl_udf_file * file, cl_udf_type udf_type, char ** result) {
	return citrusleaf_udf_get_with_gen(asc, filename, file, 0, NULL, result);
}

cl_rv citrusleaf_udf_get_with_gen(as_cluster *asc, const char * filename, cl_udf_file * file, cl_udf_type udf_type, char **gen, char ** resp) {

	if ( file->content ) return -1;

	char    query[512]  = {0};
	char *  result      = NULL;

	snprintf(query, sizeof(query), "udf-get:filename=%s;", filename);

	int rc = citrusleaf_info_cluster(asc, query, &result, true, /* check bounds */ true, 100);

	if (rc) {
		*resp = result;
		return rc;
	}
	
	// Keeping some useful but hack-based checks,
	// The code below needs to be removed once server-side bug gets fixed.

	/**
	 * result   := {request}\t{response}
	 * response := gen=<string>;content=<string>
	 */

	char * response = strchr(result, '\t') + 1;
	
	cl_udf_info info = { NULL };

	cl_parameters_parser parser = {
		.delim = ';',
		.context = &info,
		.callback = cl_udf_info_parse
	};
	cl_parameters_parse(response, &parser);

	free(result);
	result = NULL;

	if ( info.error ) {
		if ( resp ) {
			*resp = info.error;
			info.error = NULL;
		}
		cl_udf_info_destroy(&info);
		return 1;
	}

	if ( info.content.size == 0 ) {
		*resp = strdup("file_not_found");
		cl_udf_info_destroy(&info);
		return 2;
	}

	uint8_t *   content = info.content.value;
	uint32_t    size    = 0;

	// info.content.size includes a null-terminator.
	cf_b64_validate_and_decode_in_place(content, info.content.size - 1, &size);
	// TODO - do we want to check the validation result?

	file->content = as_bytes_new_wrap(content, size, true);

	info.content.value = NULL;
	info.content.free = false;
	info.content.size = 0;
	info.content.capacity = 0;

	as_bytes_destroy(&info.content);
   
	strcpy(file->name, filename);

	// Update file hash
	unsigned char hash[SHA_DIGEST_LENGTH];
#ifdef __APPLE__
	// Openssl is deprecated on mac, but the library is still included.
	// Save old settings and disable deprecated warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
	SHA1(info.content.value, info.content.size, hash);
#ifdef __APPLE__
	// Restore old settings.
#pragma GCC diagnostic pop
#endif
	cf_convert_sha1_to_hex(hash, file->hash);
	
	if ( gen ) {
		*gen = info.gen;
		info.gen = NULL;
	}

	cl_udf_info_destroy(&info);

	return 0;
}

cl_rv citrusleaf_udf_put(as_cluster *asc, const char * filename, as_bytes *content, cl_udf_type udf_type, char ** result) {

	if ( !filename || !(content)) {
		fprintf(stderr, "filename and content required\n");
		return AEROSPIKE_ERR_CLIENT;
	}

	char * query = NULL;
	
	as_string filename_string;
	const char * filebase = as_basename(&filename_string, filename);

	if (udf_type != UDF_TYPE_LUA)
	{
		fprintf(stderr, "Invalid UDF type");
		as_string_destroy(&filename_string);
		return AEROSPIKE_ERR_REQUEST_INVALID;
	}

	uint32_t encoded_len = cf_b64_encoded_len(content->size);
	char * content_base64 = malloc(encoded_len + 1);

	cf_b64_encode(content->value, content->size, content_base64);
	content_base64[encoded_len] = 0;

	if (! asprintf(&query, "udf-put:filename=%s;content=%s;content-len=%d;udf-type=%s;",
			filebase, content_base64, encoded_len, cl_udf_type_str[udf_type])) {
		fprintf(stderr, "Query allocation failed");
		as_string_destroy(&filename_string);
		return AEROSPIKE_ERR_CLIENT;
	}
	
	as_string_destroy(&filename_string);
	// fprintf(stderr, "QUERY: |%s|\n",query);
	
	int rc = citrusleaf_info_cluster(asc, query, result, true, false, 1000);
	free(query);
	free(content_base64);

	if (rc) {
		return rc;
	}

	free(*result);
	return 0;
}

cl_rv citrusleaf_udf_remove(as_cluster *asc, const char * filename, char ** response) {

	char    query[512]  = {0};

	snprintf(query, sizeof(query), "udf-remove:filename=%s;", filename);

	int rc = citrusleaf_info_cluster(asc, query, response, true, /* check bounds */ true, 100);

	if (rc) {
		return rc;
	}

	free(*response);
	return 0;
}

void cl_udf_info_destroy(cl_udf_info * info)
{
	if ( info->error ) {
		free(info->error);
	}
	
	as_val_destroy(&info->content);
	
	if ( info->gen ) {
		free(info->gen);
	}
	
	// if ( info->files ) {
	// 	free(info->files);
	// }

	info->error = NULL;
	info->gen = NULL;
	// info->files = NULL;
	// info->count = 0;
}
