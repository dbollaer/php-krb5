/**
* Copyright (c) 2008 Moritz Bechler
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
**/

/* CHANGELOG (more recent on top)
 *
 * 2010-05-27 Mark Seecof	add getPrincipal(), getRealm(), getLifetime(),
 * 				renew(), getTktAttrs() methods; support more
 * 				options to initKeytab/Password(); check only
 * 				primary TGT in isValid(); fix some bugs.
 * 				
 * 2010-04-11 Moritz Bechler	RC2 release
 */

#include "config.h"
#include "php_krb5.h"

#include "ext/standard/info.h"
#include "ext/standard/base64.h"

#include <sys/time.h>
#include <arpa/inet.h>

#ifdef HAVE_KADM5
#include "kdb.h"
#endif


/* Class definition */

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_none, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_initPassword, 0, 0, 2)
	ZEND_ARG_INFO(0, principal)
	ZEND_ARG_INFO(0, pass)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_initKeytab, 0, 0, 2)
	ZEND_ARG_INFO(0, principal)
	ZEND_ARG_INFO(0, keytab)
	ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_open, 0, 0, 1)
	ZEND_ARG_INFO(0, src)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_save, 0, 0, 1)
	ZEND_ARG_INFO(0, dest)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_isValid, 0, 0, 0)
	ZEND_ARG_INFO(0, timeRemain)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_KRB5CCache_getTktAttrs, 0, 0, 0)
	ZEND_ARG_INFO(0, prefix)
ZEND_END_ARG_INFO()

PHP_METHOD(KRB5CCache, initPassword);
PHP_METHOD(KRB5CCache, initKeytab);
PHP_METHOD(KRB5CCache, getName);
PHP_METHOD(KRB5CCache, getPrincipal);
PHP_METHOD(KRB5CCache, getRealm);
PHP_METHOD(KRB5CCache, getLifetime);
PHP_METHOD(KRB5CCache, getEntries);
PHP_METHOD(KRB5CCache, open);
PHP_METHOD(KRB5CCache, save);
PHP_METHOD(KRB5CCache, isValid);
PHP_METHOD(KRB5CCache, getTktAttrs);
PHP_METHOD(KRB5CCache, renew);

static zend_function_entry krb5_ccache_functions[] = {
		PHP_ME(KRB5CCache, initPassword, arginfo_KRB5CCache_initPassword, ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, initKeytab,   arginfo_KRB5CCache_initKeytab,   ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getName,      arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getPrincipal, arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getRealm,     arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getLifetime,  arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getEntries,   arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, open,         arginfo_KRB5CCache_open,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, save,         arginfo_KRB5CCache_save,         ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, isValid,      arginfo_KRB5CCache_isValid,      ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, getTktAttrs,  arginfo_KRB5CCache_getTktAttrs,  ZEND_ACC_PUBLIC)
		PHP_ME(KRB5CCache, renew,        arginfo_KRB5CCache_none,         ZEND_ACC_PUBLIC)
		PHP_FE_END
};


zend_module_entry krb5_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_KRB5_EXT_NAME,
    NULL,
    PHP_MINIT(krb5),
    PHP_MSHUTDOWN(krb5),
    NULL,
    NULL,
    PHP_MINFO(krb5),
    PHP_KRB5_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_KRB5
	ZEND_GET_MODULE(krb5)
#endif


krb5_error_code php_krb5_display_error(krb5_context ctx, krb5_error_code code, char* str TSRMLS_DC);
static void php_krb5_ccache_object_dtor(zend_object *obj);

/*  Initialization functions */
zend_object_handlers krb5_ccache_handlers;
zend_object * php_krb5_ticket_object_new( zend_class_entry *ce);

PHP_MINIT_FUNCTION(krb5)
{
	zend_class_entry krb5_ccache;

	INIT_CLASS_ENTRY(krb5_ccache, "KRB5CCache", krb5_ccache_functions);
	krb5_ce_ccache = zend_register_internal_class(&krb5_ccache);
	krb5_ce_ccache->create_object = php_krb5_ticket_object_new;

	memcpy(&krb5_ccache_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	krb5_ccache_handlers.free_obj = php_krb5_ccache_object_dtor;
#ifdef HAVE_KADM5
	if(php_krb5_kadm5_register_classes(TSRMLS_C) != SUCCESS) {
		return FAILURE;
	}
#endif

	/* register constants */
	REGISTER_LONG_CONSTANT("GSS_C_DELEG_FLAG", GSS_C_DELEG_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_MUTUAL_FLAG", GSS_C_MUTUAL_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_REPLAY_FLAG", GSS_C_REPLAY_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_SEQUENCE_FLAG", GSS_C_SEQUENCE_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_CONF_FLAG", GSS_C_CONF_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_INTEG_FLAG", GSS_C_INTEG_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_ANON_FLAG", GSS_C_ANON_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_PROT_READY_FLAG", GSS_C_PROT_READY_FLAG, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_TRANS_FLAG", GSS_C_TRANS_FLAG, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT("GSS_C_BOTH", GSS_C_BOTH, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_INITIATE", GSS_C_INITIATE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT("GSS_C_ACCEPT", GSS_C_ACCEPT, CONST_CS | CONST_PERSISTENT );
	
#ifdef KRB5_TL_DB_ARGS
	REGISTER_LONG_CONSTANT("KRB5_TL_DB_ARGS", KRB5_TL_DB_ARGS, CONST_CS | CONST_PERSISTENT );
#endif

	if(php_krb5_gssapi_register_classes(TSRMLS_C) != SUCCESS) {
		return FAILURE;
	}

	if(php_krb5_negotiate_auth_register_classes(TSRMLS_C) != SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(krb5)
{
	if(php_krb5_gssapi_shutdown(TSRMLS_C) != SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}


PHP_MINFO_FUNCTION(krb5)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Kerberos 5 support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_KRB5_VERSION);
#ifdef HAVE_KRB5_MIT
	php_info_print_table_row(2, "Kerberos library", "MIT");
#endif

#ifdef HAVE_KRB5_HEIMDAL
	php_info_print_table_row(2, "Kerberos library", "Heimdal");
#endif

#ifdef KRB5_VERSION
	php_info_print_table_row(2, "Library version", KRB5_VERSION);
#endif

#ifdef HAVE_KADM5
	php_info_print_table_row(2, "KADM5 support", "yes");
#else
	php_info_print_table_row(2, "KADM5 support", "no");
#endif

	php_info_print_table_row(2, "GSSAPI/SPNEGO auth support", "yes");
	php_info_print_table_end();
}


/*  Constructors/Destructors */
/* {{{ */
static void php_krb5_ccache_object_dtor(zend_object *obj)
{
	krb5_ccache_object* ticket = php_krb5_ccache_fetch_object(obj);

	if(ticket) {
		zend_object_std_dtor(&ticket->std);
		krb5_cc_destroy(ticket->ctx, ticket->cc);
		krb5_free_context(ticket->ctx);

		if(ticket->keytab) {
			efree(ticket->keytab);
		}

		efree(ticket);
	}
}
/* }}} */

/* {{{ */
zend_object * php_krb5_ticket_object_new(zend_class_entry *ce)
{
	krb5_ccache_object *object;
	krb5_error_code ret = 0;

	object = ecalloc(1, sizeof(krb5_ccache_object) + zend_object_properties_size(ce));

	/* intialize context */
	if((ret = krb5_init_context(&object->ctx))) {
		zend_throw_exception(NULL, "Cannot initialize Kerberos5 context",0 TSRMLS_CC);
	}

	// initialize random ccache
	if((ret = krb5_cc_new_unique(object->ctx, "MEMORY", "", &object->cc))) {
		zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Cannot open credential cache (%s)", krb5_get_error_message(object->ctx,ret), ret);
	}

	zend_object_std_init(&object->std, ce TSRMLS_CC);

	krb5_ccache_handlers.offset = XtOffsetOf(krb5_ccache_object, std);
	object->std.handlers = &krb5_ccache_handlers;
	return &object->std;
}
/* }}} */

/* Helper functions */
/* {{{ Parse options array for initKeytab()/initPassword() */
static int php_krb5_parse_init_creds_opts(zval *opts, krb5_get_init_creds_opt *cred_opts, char **in_tkt_svc, char **vfy_keytab TSRMLS_DC)
{
	int retval = 0;
	zval *tmp = NULL;

	if (Z_TYPE_P(opts) != IS_ARRAY) {
		return KRB5KRB_ERR_GENERIC;
	}

	/* forwardable */
	tmp = zend_hash_str_find(HASH_OF(opts), "forwardable", sizeof("forwardable") - 1);
	if (tmp != NULL) {
		const int flag =  zval_get_long(tmp) ? 1 : 0;
		krb5_get_init_creds_opt_set_forwardable(cred_opts, flag);
	}


	/* proxiable */
	tmp = zend_hash_str_find(HASH_OF(opts), "proxiable", sizeof("proxiable"));
	if (tmp != NULL) {
		const int flag =  zval_get_long(tmp) ? 1 : 0;
		krb5_get_init_creds_opt_set_proxiable(cred_opts, flag);
	}

#ifdef HAVE_KRB5_INIT_CREDS_CANONICALIZE
	/* canonicalize */
	tmp = zend_hash_str_find(HASH_OF(opts), "canonicalize", sizeof("canonicalize") - 1);
	if (tmp != NULL) {
		const int flag =  zval_get_long(tmp) ? 1 : 0;
		krb5_get_init_creds_opt_set_canonicalize(cred_opts, flag);
	}
#endif /* HAVE_KRB5_INIT_CREDS_CANONICALIZE */

	/* tkt_life */
	tmp = zend_hash_str_find(HASH_OF(opts), "tkt_life", sizeof("tkt_life") - 1);
	if (tmp != NULL) {
		const zend_long lval =  zval_get_long(tmp);
		krb5_get_init_creds_opt_set_tkt_life(cred_opts, lval);
	}

	/* renew_life */
	tmp = zend_hash_str_find(HASH_OF(opts), "renew_life", sizeof("renew_life") - 1);
	if (tmp !=NULL) {
		const zend_long lval =  zval_get_long(tmp);
		krb5_get_init_creds_opt_set_renew_life(cred_opts, lval);
	}
	
	/* service_name (krb5 arg "in_tkt_service") */
	tmp = zend_hash_str_find(HASH_OF(opts), "service_name", sizeof("service_name") - 1);
	if (tmp != NULL) {
		zend_string *sval =  zval_get_string(tmp);
		if ((*in_tkt_svc = emalloc(1+ZSTR_LEN(sval)))) {
			strncpy(*in_tkt_svc, ZSTR_VAL(sval), ZSTR_LEN(sval));
			(*in_tkt_svc)[ZSTR_LEN(sval)] = '\0';
		}
		zend_string_release(sval);
	}

	/* verification keytab name */
	tmp = zend_hash_str_find(HASH_OF(opts), "verify_keytab", sizeof("verify_keytab") - 1);
	if (tmp != NULL) {
		zend_string *sval =  zval_get_string(tmp);
		if ((*vfy_keytab = emalloc(1+ZSTR_LEN(sval)))) {
			strncpy(*vfy_keytab, ZSTR_VAL(sval), ZSTR_LEN(sval));
			(*vfy_keytab)[ZSTR_LEN(sval)] = '\0';
		}
		zend_string_release(sval);
	}

	return retval;
} /* }}} */

/* {{{ */
krb5_error_code php_krb5_display_error(krb5_context ctx, krb5_error_code code, char* str TSRMLS_DC) {
	const char *errstr = krb5_get_error_message(ctx,code);

	zend_throw_exception_ex(NULL, 0 TSRMLS_CC, str, errstr);

	return code;
}
/* }}} */

/* {{{  Copies one ccache to another*/
static krb5_error_code php_krb5_copy_ccache(krb5_context ctx, const krb5_ccache src, krb5_ccache dest TSRMLS_DC)
{
	krb5_error_code retval = 0;
	krb5_principal princ;

	if((retval = krb5_cc_get_principal(ctx,src,&princ))) {
		return php_krb5_display_error(ctx, retval,  "Failed to retrieve principal from source ccache (%s)" TSRMLS_CC);
	}

	if((retval = krb5_cc_initialize(ctx,dest,princ))) {
		krb5_free_principal(ctx, princ);
		return php_krb5_display_error(ctx, retval,  "Failed to initialize destination ccache (%s)" TSRMLS_CC);
	}

	krb5_free_principal(ctx, princ);


#ifdef HAVE_KRB5_HEIMDAL
	if((retval = krb5_cc_copy_cache(ctx, src, dest))) {
		return php_krb5_display_error(ctx, retval,  "Cannot copy given credential cache (%s)" TSRMLS_CC);
	}
#else
	krb5_cc_cursor cursor;
	if((retval = krb5_cc_start_seq_get(ctx,src,&cursor))) {
		return retval;
	}

	krb5_creds creds;
	while(krb5_cc_next_cred(ctx,src,&cursor,&creds) == 0) {
		if((retval = krb5_cc_store_cred(ctx, dest,&creds))) {
			krb5_cc_end_seq_get(ctx,src,&cursor);
			return retval;
		}
		krb5_free_cred_contents(ctx, &creds);
	}

	krb5_cc_end_seq_get(ctx,src,&cursor);
#endif

	return retval;
}
/* }}} */


/* {{{ extract realm string */
static char *php_krb5_get_realm(krb5_context ctx, krb5_principal princ TSRMLS_DC)
{
#ifdef HAVE_KRB5_PRINCIPAL_GET_REALM
	return krb5_principal_get_realm(ctx, princ);
#else
	krb5_data *data;

	data = krb5_princ_realm(ctx, princ);
	if ((data != NULL) && (data->data != NULL)) return data->data;
	/* else */
	return NULL;
#endif
}
/* }}} */

/* {{{ Get expiration times for primary TGT in cache */
static krb5_error_code php_krb5_get_tgt_expire(krb5_ccache_object *ccache, long *endtime, long *renew_until TSRMLS_DC)
{
	krb5_error_code retval = 0;
	char *errstr = NULL;
	krb5_principal princ;
	int have_princ = 0;
	krb5_creds in_cred;
	int have_in_cred = 0;
	krb5_creds *credptr = NULL;
	int have_credptr = 0;
	char *realm;

	do {
		memset(&princ, 0, sizeof(princ));
		if ((retval = krb5_cc_get_principal(ccache->ctx,ccache->cc, &princ))) {
			errstr = "Failed to retrieve principal from source ccache (%s)";
			break;
		}
		have_princ = 1;

		if (!(realm = php_krb5_get_realm(ccache->ctx, princ TSRMLS_CC))) {
			retval = KRB5KRB_ERR_GENERIC;
			errstr = "Failed to extract realm from principal (%s)";
			break;
		}

		memset(&in_cred, 0, sizeof(in_cred));
		in_cred.client = princ;

		if ((retval = krb5_build_principal(ccache->ctx, &in_cred.server, strlen(realm), realm, "krbtgt", realm, NULL))) {
			errstr = "Failed to build krbtgt principal (%s)";
			break;
		}
		have_in_cred = 1;

		if ((retval = krb5_get_credentials(ccache->ctx, KRB5_GC_CACHED, ccache->cc, &in_cred, &credptr))) {
			errstr = "Failed to retrieve krbtgt ticket from cache (%s)";
			break;
		}
		have_credptr = 1;

	} while (0);

	if (have_princ) krb5_free_principal(ccache->ctx, princ);
	if (have_in_cred) krb5_free_principal(ccache->ctx, in_cred.server);



	if (have_credptr) {
		krb5_free_cred_contents(ccache->ctx, credptr);
		*endtime = credptr->times.endtime;
		*renew_until = credptr->times.renew_till;
	}

	if (errstr != NULL) {
		php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
	}

	return retval;
}
/* }}} */

/* {{{ verify a (client's) new TGT using keytab */
static krb5_error_code php_krb5_verify_tgt(krb5_ccache_object *ccache, krb5_creds *creds, char *vfy_keytab TSRMLS_DC)
{
	krb5_error_code retval = 0;
	krb5_error_code r2val;
	krb5_keytab ktab;
	int have_ktab = 0;
	krb5_kt_cursor cursor;
	int have_cursor = 0;
	krb5_keytab_entry entry;
	int have_entry = 0;
	krb5_principal princ;
	int have_princ = 0;
	krb5_verify_init_creds_opt opts;


	if (!vfy_keytab || !*vfy_keytab) {
		return KRB5_KT_NOTFOUND;
	}

    do {
	memset(&ktab, 0, sizeof(ktab));
	if ((retval = krb5_kt_resolve(ccache->ctx, vfy_keytab, &ktab))) {
		break;
	}
	have_ktab = 1;

	memset(&cursor, 0, sizeof(cursor));
	if ((retval = krb5_kt_start_seq_get(ccache->ctx, ktab, &cursor))) {
		break;
	}
	have_cursor = 1;

	memset(&entry, 0, sizeof(entry));
	if ((retval = krb5_kt_next_entry(ccache->ctx, ktab, &entry, &cursor))) {
		break;
	}
	have_entry = 1;

	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_copy_principal(ccache->ctx, entry.principal, &princ))) {
		break;
	}
	have_princ = 1;

	krb5_verify_init_creds_opt_init(&opts);
	krb5_verify_init_creds_opt_set_ap_req_nofail(&opts, 1);

	if ((retval = krb5_verify_init_creds(ccache->ctx, creds, princ, ktab, NULL, &opts))) {
		break;
	}
    } while (0);

        if (have_ktab && (r2val = krb5_kt_close(ccache->ctx, ktab))) {
		php_krb5_display_error(ccache->ctx, r2val, "Failed to close keytab (%s)" TSRMLS_CC);
	}

	if (have_cursor && (r2val = krb5_kt_end_seq_get(ccache->ctx, ktab, &cursor))) {
		php_krb5_display_error(ccache->ctx, r2val, "Failed to free keytab cursor (%s)" TSRMLS_CC);
	}

	if (have_entry &&
#ifdef HAVE_KRB5_HEIMDAL
		(r2val = krb5_kt_free_entry(ccache->ctx, &entry))
#else
		(r2val = krb5_free_keytab_entry_contents(ccache->ctx, &entry))
#endif
			) {
		php_krb5_display_error(ccache->ctx, r2val, "Failed to free keytab entry (%s)" TSRMLS_CC);
	}

	if (have_princ) krb5_free_principal(ccache->ctx, princ);

	return retval;
}
/* }}} */


/* KRB5CCache Methods */

/* {{{ proto string KRB5CCache::getName(  )
   Gets the name/identifier of this credential cache */
PHP_METHOD(KRB5CCache, getName)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());

	const char *tmpname = krb5_cc_get_name(ccache->ctx, ccache->cc);
	const char *tmptype = krb5_cc_get_type(ccache->ctx, ccache->cc);
	char *name = NULL;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	name = emalloc(strlen(tmpname) + strlen(tmptype) + 2);
	*name = 0;
	strcat(name, tmptype);
	strcat(name, ":");
	strcat(name, tmpname);

	RETVAL_STRING(name);
	efree(name);
}
/* }}} */

/* {{{ proto bool KRB5CCache::open( string $src )
   Copies the contents of the credential cache given by $dest to this credential cache */
PHP_METHOD(KRB5CCache, open)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	char *sccname = NULL;
	size_t sccname_len = 0;
	krb5_error_code retval = 0;
	krb5_ccache src;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, ARG_PATH, &sccname, &sccname_len) == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	if((retval = krb5_cc_resolve(ccache->ctx, sccname, &src))) {
		php_krb5_display_error(ccache->ctx, retval,  "Cannot open given credential cache (%s)" TSRMLS_CC);
		RETURN_FALSE;
	}

	if((retval = php_krb5_copy_ccache(ccache->ctx, src, ccache->cc TSRMLS_CC))) {
		krb5_cc_close(ccache->ctx, src);
		php_krb5_display_error(ccache->ctx, retval,  "Failed to copy credential cache (%s)" TSRMLS_CC);
		RETURN_FALSE;
	}

	krb5_cc_close(ccache->ctx, src);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool KRB5CCache::save( string $dest )
   Copies the contents of this credential cache to the credential cache given by $dest */
PHP_METHOD(KRB5CCache, save)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	char *sccname = NULL;
	size_t sccname_len = 0;
	krb5_error_code retval = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, ARG_PATH, &sccname, &sccname_len) == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	krb5_ccache dest = NULL;
	if((retval = krb5_cc_resolve(ccache->ctx, sccname, &dest))) {
		php_krb5_display_error(ccache->ctx, retval,  "Cannot open given credential cache (%s)" TSRMLS_CC);
		RETURN_FALSE;
	}

	if((retval = php_krb5_copy_ccache(ccache->ctx, ccache->cc, dest TSRMLS_CC))) {
		krb5_cc_close(ccache->ctx, dest);
		php_krb5_display_error(ccache->ctx, retval,  "Failed to copy credential cache (%s)" TSRMLS_CC);
		RETURN_FALSE;
	}

	krb5_cc_close(ccache->ctx, dest);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool KRB5CCache::initPassword( string $principal, string $pass [, array $options ])
   Gets a TGT for the given principal using the given credentials */
PHP_METHOD(KRB5CCache, initPassword)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	char *errstr = "";

	char *sprinc = NULL;
	int sprinc_len;
	char *spass = NULL;
	int spass_len;
	zval *opts = NULL;

	krb5_principal princ;
	int have_princ = 0;
	krb5_get_init_creds_opt *cred_opts;
	int have_cred_opts = 0;
	char *in_tkt_svc = NULL;
	char *vfy_keytab = NULL;
	krb5_creds creds;
	int have_creds = 0;

#ifndef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	krb5_get_init_creds_opt cred_opts_struct;
	cred_opts = &cred_opts_struct;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|a", &sprinc, &sprinc_len, &spass, &spass_len, &opts) == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

    do {
	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_parse_name(ccache->ctx, sprinc, &princ))) {
		errstr = "Cannot parse Kerberos principal (%s)";
		break;
		}
	have_princ = 1;

#ifdef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	if ((retval = krb5_get_init_creds_opt_alloc(ccache->ctx, &cred_opts))) {
		errstr = "Cannot allocate cred_opts (%s)";
		break;
	}
#else
	krb5_get_init_creds_opt_init(cred_opts);
#endif
	have_cred_opts = 1;

	if (opts != NULL) {
		if ((retval = php_krb5_parse_init_creds_opts(opts, cred_opts, &in_tkt_svc, &vfy_keytab TSRMLS_CC))) {
			errstr = "Cannot parse credential options (%s)";
			break;
		}
	}

	memset(&creds, 0, sizeof(creds));
	if ((retval = krb5_get_init_creds_password(ccache->ctx, &creds, princ, spass, NULL, 0, 0, in_tkt_svc, cred_opts))) {
		errstr = "Cannot get ticket (%s)";
		break;
	}
	have_creds = 1;

	if((retval = krb5_cc_initialize(ccache->ctx, ccache->cc, princ))) {
		errstr = "Failed to initialize credential cache (%s)";
		break;
	}

	if((retval = krb5_cc_store_cred(ccache->ctx, ccache->cc, &creds))) {
		errstr = "Failed to store ticket in credential cache (%s)";
		break;
	}

	if (vfy_keytab && *vfy_keytab && (retval = php_krb5_verify_tgt(ccache, &creds, vfy_keytab TSRMLS_CC))) {
		errstr = "Failed to verify ticket (%s)";
		break;
	}

    } while (0);

	if (have_princ) krb5_free_principal(ccache->ctx, princ);

#ifdef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	if (have_cred_opts) krb5_get_init_creds_opt_free(ccache->ctx, cred_opts);
#endif

	if (in_tkt_svc) efree(in_tkt_svc);
	if (vfy_keytab) efree(vfy_keytab);
	if (have_creds) krb5_free_cred_contents(ccache->ctx, &creds);

	if (retval) {
		php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool KRB5CCache::initKeytab( string $principal, string $keytab_file [, array $options ])
   Gets a TGT for the given principal using the credentials in the given keytab */
PHP_METHOD(KRB5CCache, initKeytab)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	char *errstr = "";

	char *sprinc = NULL;
	int sprinc_len;
	char *skeytab = NULL;
	int skeytab_len;
	zval *opts = NULL;

	krb5_principal princ;
	int have_princ = 0;
	krb5_keytab keytab;
	int have_keytab = 0;
	krb5_get_init_creds_opt *cred_opts;
	int have_cred_opts = 0;
	char *in_tkt_svc = NULL;
	char *vfy_keytab = NULL;
	krb5_creds creds;
	int have_creds = 0;

#ifndef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	krb5_get_init_creds_opt cred_opts_struct;
	cred_opts = &cred_opts_struct;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s" ARG_PATH "|a", &sprinc, &sprinc_len, &skeytab, &skeytab_len, &opts) == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	if ( php_check_open_basedir(skeytab TSRMLS_CC)) {
		RETURN_FALSE;
	}

    do {
	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_parse_name(ccache->ctx, sprinc, &princ))) {
		errstr = "Cannot parse Kerberos principal (%s)";
		break;
	}
	have_princ = 1;

	memset(&keytab, 0, sizeof(keytab));
	if ((retval = krb5_kt_resolve(ccache->ctx, skeytab, &keytab))) {
		errstr = "Cannot load keytab (%s)";
		break;
	}
	have_keytab = 1;

#ifdef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	if ((retval = krb5_get_init_creds_opt_alloc(ccache->ctx, &cred_opts))) {
		errstr = "Cannot allocate cred_opts (%s)";
		break;
	}
#else
	krb5_get_init_creds_opt_init(cred_opts);
#endif
	have_cred_opts = 1;

	if(opts) {
		if ((retval = php_krb5_parse_init_creds_opts(opts, cred_opts, &in_tkt_svc, &vfy_keytab TSRMLS_CC))) {
			errstr = "Cannot parse credential options";
			break;
		}
	}

	memset(&creds, 0, sizeof(creds));
	if ((retval = krb5_get_init_creds_keytab(ccache->ctx, &creds, princ, keytab, 0, in_tkt_svc, cred_opts))) {
		errstr = "Cannot get ticket (%s)";
		break;
	}
	have_creds = 1;

	if ((retval = krb5_cc_initialize(ccache->ctx, ccache->cc, princ))) {
		errstr = "Failed to initialize credential cache (%s)";
		break;
	}

	if((retval = krb5_cc_store_cred(ccache->ctx, ccache->cc, &creds))) {
		errstr = "Failed to store ticket in credential cache (%s)";
		break;
	}

	if (vfy_keytab && *vfy_keytab && (retval = php_krb5_verify_tgt(ccache, &creds, vfy_keytab TSRMLS_CC))) {
		errstr = "Failed to verify ticket (%s)";
		break;
	}

    } while (0);

	if (have_princ) krb5_free_principal(ccache->ctx, princ);
	if (have_keytab) krb5_kt_close(ccache->ctx, keytab);

#ifdef KRB5_GET_INIT_CREDS_OPT_CANONICALIZE
	if (have_cred_opts) krb5_get_init_creds_opt_free(ccache->ctx, cred_opts);
#endif

	if (in_tkt_svc) efree(in_tkt_svc);
	if (vfy_keytab) efree(vfy_keytab);
	if (have_creds) krb5_free_cred_contents(ccache->ctx, &creds);

	if (retval) {
		php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
		RETURN_FALSE;
	}

	ccache->keytab = estrdup(skeytab);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string KRB5CCache::getPrincipal( )
   Returns name of primary principal (client) associated with cache */
PHP_METHOD(KRB5CCache, getPrincipal)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	krb5_principal princ;
	char *princname = NULL;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_cc_get_principal(ccache->ctx, ccache->cc, &princ))) {
		php_krb5_display_error(ccache->ctx, retval, "Failed to retrieve principal from source ccache (%s)" TSRMLS_CC);
		RETURN_EMPTY_STRING();
	}

	if ((retval = krb5_unparse_name(ccache->ctx, princ, &princname))) {
		krb5_free_principal(ccache->ctx,princ);
		php_krb5_display_error(ccache->ctx, retval, "Failed to unparse principal name (%s)" TSRMLS_CC);
		RETURN_EMPTY_STRING();
	}

	RETVAL_STRING(princname);

		krb5_free_principal(ccache->ctx,princ);
	free(princname);
	}
/* }}} */

/* {{{ proto string KRB5CCache::getRealm( )
   Returns name of realm for primary principal */
PHP_METHOD(KRB5CCache, getRealm)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	krb5_principal princ;
	char *realm;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_cc_get_principal(ccache->ctx,ccache->cc,&princ))) {
		php_krb5_display_error(ccache->ctx, retval, "Failed to retrieve principal from source ccache (%s)" TSRMLS_CC);
		RETURN_EMPTY_STRING();
	}

	if (!(realm = php_krb5_get_realm(ccache->ctx, princ TSRMLS_CC))) {
		krb5_free_principal(ccache->ctx,princ);
		php_krb5_display_error(ccache->ctx, KRB5KRB_ERR_GENERIC, "Failed to extract realm from principal (%s)" TSRMLS_CC);
		RETURN_EMPTY_STRING();
	}

	RETVAL_STRING(realm);

	krb5_free_principal(ccache->ctx,princ);
}
/* }}} */

/* {{{ proto array KRB5CCache::getLifetime( )
   Return array with primary TGT's endtime and renew_until times in it */
PHP_METHOD(KRB5CCache, getLifetime)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	long endtime, renew_until;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	array_init(return_value);

	if ((retval = php_krb5_get_tgt_expire(ccache,&endtime,&renew_until TSRMLS_CC))) {
		php_krb5_display_error(ccache->ctx, retval, "Failed to get TGT times (%s)" TSRMLS_CC);
		return;
	}

	add_assoc_long(return_value, "endtime", endtime);
	add_assoc_long(return_value, "renew_until", renew_until);
}
/* }}} */

/* {{{ proto array KRB5CCache::getEntries( )
   Fetches all principal names for which tickets are available in this cache */
PHP_METHOD(KRB5CCache, getEntries)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	char *errstr = "";
	krb5_cc_cursor cursor;
	int have_cursor = 0;
	krb5_creds creds;
	int have_creds = 0;
	char *princname;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	array_init(return_value);

    do {
	memset(&cursor, 0, sizeof(cursor));
	if((retval = krb5_cc_start_seq_get(ccache->ctx,ccache->cc,&cursor))) {
		errstr = "Failed to initialize ccache iterator (%s)";
		break;
	}
	have_cursor = 1;

	memset(&creds, 0, sizeof(creds));
	while(krb5_cc_next_cred(ccache->ctx,ccache->cc,&cursor,&creds) == 0) {
		have_creds = 1;
		if(creds.server) {
			princname = NULL;
			if((retval = krb5_unparse_name(ccache->ctx, creds.server, &princname))) {
				errstr = "Failed to unparse principal name (%s)";
				break;
			}
			add_next_index_string(return_value, princname);
			free(princname);
		}
		krb5_free_cred_contents(ccache->ctx, &creds);
		have_creds = 0;
	}

    } while (0);

	if (have_creds) krb5_free_cred_contents(ccache->ctx, &creds);

	if (have_cursor) krb5_cc_end_seq_get(ccache->ctx, ccache->cc, &cursor);

	if (*errstr) {
		php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
		array_init(return_value);
	}
}
/* }}} */

/* {{{ proto bool KRB5CCache::isValid( [ int $timeRemain = 0 ] )
   Checks whether the primary TGT in the cache is still valid and will remain valid for the given number of seconds */
PHP_METHOD(KRB5CCache, isValid)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	long endtime, renew_until, then;
	krb5_timestamp now;
	long need = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &need) == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	if ((retval = php_krb5_get_tgt_expire(ccache,&endtime,&renew_until TSRMLS_CC))) {
		RETURN_FALSE;
	}

	if((retval = krb5_timeofday(ccache->ctx, &now))) {
		php_krb5_display_error(ccache->ctx, retval, "Failed to obtain time (%s)" TSRMLS_CC);
	}

	then = now + need + 60; /* modest allowance for clock drift */

	if (then > endtime) {
			RETURN_FALSE;
		}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array KRB5CCache::getTktAttrs( [string prefix])
   Fetches principals and (readable) attributes of ticket(s) in cache */
PHP_METHOD(KRB5CCache, getTktAttrs)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	char *errstr = "";
	krb5_cc_cursor cursor;
	int have_cursor = 0;
	krb5_creds creds;
	int have_creds = 0;
	zval tktinfo;
	char *princname;
	long princ_len;
	long tktflags;
	char strflags[65];
	char *q = strflags + sizeof(strflags) - 1;
	char *p;
	krb5_ticket *tkt;
	char *encstr;
#define ENCSTRMAX 256
	krb5_address *tktaddr, **tkt_addrs;
	zval addrlist;
	struct in_addr ipaddr;
#ifdef INET6_ADDRSTRLEN
	struct in6_addr ip6addr;
	char straddr[INET6_ADDRSTRLEN];
#endif
	char *prefix = NULL;
	long pfx_len = 0;

	array_init(return_value);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &prefix, &pfx_len) == FAILURE) {
		return;
	}
	if (pfx_len == 0) prefix = NULL;

    do {
	memset(&cursor, 0, sizeof(cursor));
	if ((retval = krb5_cc_start_seq_get(ccache->ctx,ccache->cc,&cursor))) {
		errstr = "Failed to initialize ccache iterator (%s)";
		break;
	}
	have_cursor = 1;

	memset(&creds, 0, sizeof(creds));
	while (krb5_cc_next_cred(ccache->ctx,ccache->cc,&cursor,&creds) == 0) {
		have_creds = 1;
		if (creds.server) {
			array_init(&tktinfo);

			princname = NULL;
			if ((retval = krb5_unparse_name(ccache->ctx, creds.server, &princname))) {
				errstr = "Failed to unparse server principal name (%s)";
				break;
			}

			princ_len = princname ? strlen(princname) : 0;
			if (prefix && ((princ_len < pfx_len) || strncmp(princname,prefix,pfx_len))) {
				free(princname);
				krb5_free_cred_contents(ccache->ctx, &creds);
				have_creds = 0;
				continue;
			}

			add_assoc_string(&tktinfo, "server", (princname?princname:""));
			free(princname);

			princname = NULL;
			if((retval = krb5_unparse_name(ccache->ctx, creds.client, &princname))) {
				errstr = "Failed to unparse client principal name (%s)";
				break;
			}
			add_assoc_string(&tktinfo, "client", (princname?princname:""));
			free(princname);

			add_assoc_long(&tktinfo, "authtime", creds.times.authtime);
			add_assoc_long(&tktinfo, "starttime", creds.times.starttime);
			add_assoc_long(&tktinfo, "endtime", creds.times.endtime);

			/* Darn it, "till" is NOT an abbreviation of "until" */
			add_assoc_long(&tktinfo, "renew_until", creds.times.renew_till);

			tktflags = creds.ticket_flags;
			p = strflags;
			*p = '\0';
			if ((tktflags & TKT_FLG_FORWARDABLE) && (p < q)) *(p++) = 'F';
			if ((tktflags & TKT_FLG_FORWARDED) && (p < q)) *(p++) = 'f';
			if ((tktflags & TKT_FLG_PROXIABLE) && (p < q)) *(p++) = 'P';
			if ((tktflags & TKT_FLG_PROXY) && (p < q)) *(p++) = 'p';
			if ((tktflags & TKT_FLG_MAY_POSTDATE) && (p < q)) *(p++) = 'D';
			if ((tktflags & TKT_FLG_POSTDATED) && (p < q)) *(p++) = 'd';
			if ((tktflags & TKT_FLG_INVALID) && (p < q)) *(p++) = 'i';
			if ((tktflags & TKT_FLG_RENEWABLE) && (p < q)) *(p++) = 'R';
			if ((tktflags & TKT_FLG_INITIAL) && (p < q)) *(p++) = 'I';
			if ((tktflags & TKT_FLG_PRE_AUTH) && (p < q)) *(p++) = 'A';
			if ((tktflags & TKT_FLG_HW_AUTH) && (p < q)) *(p++) = 'H';
		if ((tktflags & TKT_FLG_TRANSIT_POLICY_CHECKED) && (p < q)) *(p++) = 'T';
			if ((tktflags & TKT_FLG_OK_AS_DELEGATE) && (p < q)) *(p++) = 'O';
#ifdef TKT_FLG_ENC_PA_REP
			if ((tktflags & TKT_FLG_ENC_PA_REP) && (p < q)) *(p++) = 'e';
#endif
			if ((tktflags & TKT_FLG_ANONYMOUS) && (p < q)) *(p++) = 'a';
			*p = '\0';

			add_assoc_string(&tktinfo, "flags", strflags);

#ifdef HAVE_KRB5_HEIMDAL
			encstr = NULL;
			if ((retval = krb5_enctype_to_string(ccache->ctx,creds.keyblock.enctype, &encstr)))
#else
			encstr = malloc(ENCSTRMAX);
			if ((retval = krb5_enctype_to_string(creds.keyblock.enctype, encstr, ENCSTRMAX)))
#endif
			{
				if (!encstr) encstr = malloc(ENCSTRMAX);
				snprintf(encstr, ENCSTRMAX, "enctype %d", creds.keyblock.enctype);
			}
			add_assoc_string(&tktinfo, "skey_enc", encstr);
			free(encstr);

			if ((retval = krb5_decode_ticket(&creds.ticket,&tkt))) {
				errstr = "Failed to decode ticket data (%s)";
				break;
			} else {
#ifdef HAVE_KRB5_HEIMDAL
				encstr = NULL;
				if((retval = krb5_enctype_to_string(ccache->ctx,creds.keyblock.enctype, &encstr)))
#else
				encstr = malloc(ENCSTRMAX);
				if((retval = krb5_enctype_to_string(tkt->enc_part.enctype, encstr, ENCSTRMAX)))
#endif
				{
					if (!encstr) encstr = malloc(ENCSTRMAX);
					snprintf(encstr, ENCSTRMAX, "enctype %d", tkt->enc_part.enctype);
				}
				add_assoc_string(&tktinfo, "tkt_enc", encstr);
				free(encstr);
				krb5_free_ticket(ccache->ctx, tkt);
			}

			array_init(&addrlist);
			tkt_addrs = creds.addresses;
			if (tkt_addrs) while((tktaddr = *(tkt_addrs++))) {
				if ((tktaddr->addrtype == ADDRTYPE_INET) && (tktaddr->length == 4)) {
					memcpy(&(ipaddr.s_addr), tktaddr->contents, tktaddr->length);

#ifndef INET6_ADDRSTRLEN
					add_next_index_string(&addrlist, inet_ntoa(ipaddr));
				}
#if 0
 { match curlies
#endif
#else /* ! INET6_ADDRSTRLEN */
					if (inet_ntop(AF_INET, &ipaddr, straddr, sizeof(straddr))) {
						add_next_index_string(&addrlist, straddr);
					}
				}
				if ((tktaddr->addrtype == ADDRTYPE_INET6) && (tktaddr->length >= 4)) {
					memcpy(ip6addr.s6_addr, tktaddr->contents, tktaddr->length);
					if (inet_ntop(AF_INET6, &ipaddr, straddr, sizeof(straddr))) {
						add_next_index_string(&addrlist, straddr);
					}
				}
#endif /* INET6_ADDRSTRLEN */
			}
			add_assoc_zval(&tktinfo, "addresses", &addrlist);
			add_next_index_zval(return_value,&tktinfo);
		}

		krb5_free_cred_contents(ccache->ctx, &creds);
		have_creds = 0;
	} /* while creds */

	if (have_creds) krb5_free_cred_contents(ccache->ctx, &creds);

    } while (0);

	if (have_cursor) krb5_cc_end_seq_get(ccache->ctx,ccache->cc,&cursor);

	if (*errstr) {
		php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
		array_init(return_value);
	}
}
#undef ENCSTRMAX
/* }}} */

/* {{{ proto bool KRB5CCache::renew( )
   Renew default TGT and purge other tickets from cache, return FALSE on failure  */
PHP_METHOD(KRB5CCache, renew)
{
	krb5_ccache_object* ccache = Z_KRB5_CCACHE_OBJ_P(getThis());
	krb5_error_code retval = 0;
	char *errstr = "";
	long endtime, renew_until;
	krb5_timestamp now;
	krb5_principal princ;
	int have_princ = 0;
	krb5_creds creds;
	int have_creds = 0;

	if (zend_parse_parameters_none() == FAILURE) {
		zend_throw_exception(NULL, "Failed to parse arglist", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

    do {
	if ((retval = php_krb5_get_tgt_expire(ccache, &endtime, &renew_until TSRMLS_CC))) {
		errstr = "Failed to get renew_until () (%s)";
		break;
	}

	if ((retval = krb5_timeofday(ccache->ctx, &now))) {
		errstr = "Failed to read clock in renew() (%s)";
		break;
	}

	if (now > renew_until) {
		/* ticket is not renewable, but... */
		if (now >= endtime) retval = -1; /* ...is it still useful? */
		break;
	}

	memset(&princ, 0, sizeof(princ));
	if ((retval = krb5_cc_get_principal(ccache->ctx, ccache->cc, &princ))) {
		errstr = "Failed to get principal from cache (%s)";
		break;
	}
	have_princ = 1;

	memset(&creds, 0, sizeof(creds));
	if ((retval = krb5_get_renewed_creds(ccache->ctx, &creds, princ, ccache->cc, NULL))) {
		errstr = "Failed to renew TGT in cache (%s)";
		break;
	}
	have_creds = 1;

	if ((retval = krb5_cc_initialize(ccache->ctx, ccache->cc, princ))) {
		errstr = "Failed to reinitialize ccache after TGT renewal (%s)";
		break;
	}

	if((retval = krb5_cc_store_cred(ccache->ctx, ccache->cc, &creds))) {
		errstr = "Failed to store renewed TGT in ccache (%s)";
		break;
	}
    } while (0);

	if (have_princ) krb5_free_principal(ccache->ctx, princ);
	if (have_creds) krb5_free_cred_contents(ccache->ctx, &creds);

	if (retval) {
		if (*errstr) {
			php_krb5_display_error(ccache->ctx, retval, errstr TSRMLS_CC);
		}
		RETURN_FALSE;
	}

	/* otherwise */
	RETURN_TRUE;
}
/* }}} */

/* bottom of file */
