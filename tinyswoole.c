#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_tinyswoole.h"

zend_class_entry *tinyswoole_ce;


ZEND_BEGIN_ARG_INFO_EX(arginfo_tinyswoole__construct, 0, 0, 2)
	ZEND_ARG_INFO(0, ip)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

static inline zval* tsw_zend_read_property(zend_class_entry *class_ptr, zval *obj, const char *s, int len, int silent)
{
    zval rv;
    return zend_read_property(class_ptr, obj, s, len, silent, &rv);
}

PHP_METHOD(tinyswoole, __construct)
{
	char *ip;
	size_t ip_len;
	long port;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &ip, &ip_len, &port) == FAILURE) {
		RETURN_NULL();
	}

	zend_update_property_string(tinyswoole_ce, getThis(), "ip", sizeof("ip") - 1, ip);
	zend_update_property_long(tinyswoole_ce, getThis(), "port", sizeof("port") - 1, port);
}

PHP_METHOD(tinyswoole, start)
{
	zval *ip;
	zval *port;

	printf("running server...\n");

	ip = tsw_zend_read_property(tinyswoole_ce, getThis(), "ip", sizeof("ip") - 1, 0);
	port = tsw_zend_read_property(tinyswoole_ce, getThis(), "port", sizeof("port") - 1, 0);

	server(Z_STRVAL(*ip), Z_LVAL(*port));
}

zend_function_entry tinyswoole_method[]=
{
	ZEND_ME(tinyswoole, __construct, arginfo_tinyswoole__construct, ZEND_ACC_PUBLIC)
	ZEND_ME(tinyswoole, start, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(tinyswoole)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "tinyswoole_server", tinyswoole_method);
	tinyswoole_ce = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_null(tinyswoole_ce, "ip", sizeof("ip") - 1, ZEND_ACC_PRIVATE);
	zend_declare_property_null(tinyswoole_ce, "port", sizeof("port") - 1, ZEND_ACC_PRIVATE);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(tinyswoole)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(tinyswoole)
{
#if defined(COMPILE_DL_TINYSWOOLE) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(tinyswoole)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(tinyswoole)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "tinyswoole support", "enabled");
	php_info_print_table_end();
}

const zend_function_entry tinyswoole_functions[] = {
	PHP_FE_END
};

zend_module_entry tinyswoole_module_entry = {
	STANDARD_MODULE_HEADER,
	"tinyswoole",
	tinyswoole_functions,
	PHP_MINIT(tinyswoole),
	PHP_MSHUTDOWN(tinyswoole),
	PHP_RINIT(tinyswoole),
	PHP_RSHUTDOWN(tinyswoole),
	PHP_MINFO(tinyswoole),
	PHP_TINYSWOOLE_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_TINYSWOOLE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(tinyswoole)
#endif