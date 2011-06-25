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

#ifndef PHP_THROTTLE_H
#define PHP_THROTTLE_H

extern zend_module_entry throttle_module_entry;
#define phpext_throttle_ptr &throttle_module_entry

#ifdef PHP_WIN32
#	define PHP_THROTTLE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_THROTTLE_API __attribute__ ((visibility("default")))
#else
#	define PHP_THROTTLE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(throttle);
PHP_MSHUTDOWN_FUNCTION(throttle);
PHP_MINFO_FUNCTION(throttle);

ZEND_BEGIN_MODULE_GLOBALS(throttle)
	long  		speed;
	zend_bool 	debug;

	double 		lasttime;
	size_t 		post_bytes_processed;
ZEND_END_MODULE_GLOBALS(throttle)

#ifdef ZTS
#define THROTTLE_G(v) TSRMG(throttle_globals_id, zend_throttle_globals *, v)
#else
#define THROTTLE_G(v) (throttle_globals.v)
#endif

#endif	/* PHP_THROTTLE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
