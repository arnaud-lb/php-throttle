/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2011 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Arnaud Le Blanc <arnaud.lb@gmail.com>                        |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_throttle.h"
#include "rfc1867.h"

ZEND_DECLARE_MODULE_GLOBALS(throttle);

static int php_throttle_rfc1867_callback(unsigned int event, void *event_data, void **extra TSRMLS_DC);
static int (*php_throttle_rfc1867_orig_callback)(unsigned int event, void *event_data, void **extra TSRMLS_DC);

/* True global resources - no need for thread safety here */
static int le_throttle;

/* {{{ throttle_functions[]
 *
 * Every user visible function must have an entry in throttle_functions[].
 */
const zend_function_entry throttle_functions[] = {
	{NULL, NULL, NULL}	/* Must be the last line in throttle_functions[] */
};
/* }}} */

/* {{{ throttle_module_entry
 */
zend_module_entry throttle_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"throttle",
	throttle_functions,
	PHP_MINIT(throttle),
	PHP_MSHUTDOWN(throttle),
	NULL,
	NULL,
	PHP_MINFO(throttle),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1",
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_THROTTLE
ZEND_GET_MODULE(throttle)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("throttle.speed",		"0", PHP_INI_PERDIR, OnUpdateLong, speed, zend_throttle_globals, throttle_globals)
    STD_PHP_INI_ENTRY("throttle.debug",		"0", PHP_INI_PERDIR, OnUpdateBool, debug, zend_throttle_globals, throttle_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_throttle_init_globals
 */
static void php_throttle_init_globals(zend_throttle_globals *throttle_globals)
{
	memset(throttle_globals, 0, sizeof(*throttle_globals));
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(throttle)
{
	REGISTER_INI_ENTRIES();

	php_throttle_rfc1867_orig_callback = php_rfc1867_callback;
	php_rfc1867_callback = php_throttle_rfc1867_callback;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(throttle)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(throttle)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "throttle support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

static void throttle_debug(const char *format, ...)
{
	char *buf = NULL;
	va_list ap;
	TSRMLS_FETCH();

	if (!THROTTLE_G(debug)) {
		return;
	}

	va_start(ap, format);
	vspprintf(&buf, 0, format, ap);
	va_end(ap);

	sapi_module.log_message(buf TSRMLS_CC);
	efree(buf);
}

static void throttle_wait(size_t total_bytes_processed TSRMLS_DC)
{
	struct timeval tv;
	double time;
	double min_time;
	size_t bytes_processed;
	
	bytes_processed = total_bytes_processed - THROTTLE_G(post_bytes_processed);

	throttle_debug("throttle: bytes processed: %zd (total: %zd)\n", bytes_processed, total_bytes_processed);

	min_time = (double) bytes_processed / (double) THROTTLE_G(speed);

	gettimeofday(&tv, NULL);
	time = (double) tv.tv_sec + tv.tv_usec / 1000000.0;

	if (time < THROTTLE_G(lasttime) + min_time) {

		useconds_t usec;

		throttle_debug("throttle: waiting for %f seconds (until %f)\n", THROTTLE_G(lasttime) + min_time - time, THROTTLE_G(lasttime) + min_time);

		usec = (THROTTLE_G(lasttime) + min_time - time) * 1000000.0;
		usleep(usec);

		gettimeofday(&tv, NULL);
		time = (double) tv.tv_sec + tv.tv_usec / 1000000.0;
	}

	THROTTLE_G(post_bytes_processed) = total_bytes_processed;
	THROTTLE_G(lasttime) = time;
}

static int php_throttle_rfc1867_callback(unsigned int event, void *event_data, void **extra TSRMLS_DC)
{
	int retval = SUCCESS;

	if (php_throttle_rfc1867_orig_callback) {
		retval = php_throttle_rfc1867_orig_callback(event, event_data, extra TSRMLS_CC);
	}
	if (THROTTLE_G(speed) < 1) {
		return retval;
	}

	switch(event) {
	case MULTIPART_EVENT_START: {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		THROTTLE_G(lasttime) = (double) tv.tv_sec + tv.tv_usec / 1000000.0;
		THROTTLE_G(post_bytes_processed) = 0;
		break;
	}
	case MULTIPART_EVENT_FORMDATA: {
		multipart_event_formdata *data = (multipart_event_formdata *) event_data;
		throttle_wait(data->post_bytes_processed TSRMLS_CC);
		break;
	}
	case MULTIPART_EVENT_FILE_START: {
		multipart_event_file_start *data = (multipart_event_file_start *) event_data;
		throttle_wait(data->post_bytes_processed TSRMLS_CC);
		break;
	}
	case MULTIPART_EVENT_FILE_DATA: {
		multipart_event_file_data *data = (multipart_event_file_data *) event_data;
		throttle_wait(data->post_bytes_processed TSRMLS_CC);
		break;
	}
	case MULTIPART_EVENT_FILE_END: {
		multipart_event_file_end *data = (multipart_event_file_end *) event_data;
		throttle_wait(data->post_bytes_processed TSRMLS_CC);
		break;
	}
	case MULTIPART_EVENT_END: {
		multipart_event_end *data = (multipart_event_end *) event_data;
		throttle_wait(data->post_bytes_processed TSRMLS_CC);
		break;
	}
	}

	return retval;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
