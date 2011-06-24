dnl config.m4 for extension throttle

PHP_ARG_ENABLE(throttle, whether to enable throttle support,
Make sure that the comment is aligned:
[  --enable-throttle           Enable throttle support])

if test "$PHP_THROTTLE" != "no"; then
  PHP_NEW_EXTENSION(throttle, throttle.c, $ext_shared)
fi
