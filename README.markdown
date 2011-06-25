# php throttle module

This module allows to throttle file upload speed, for debugging / testing purposes.

## Install

``` sh
phpize
./configure
make
make install
```

## Ini settings

``` ini
; load throttle module
extension=throttle.so
; speed limit, in bytes
throttle.speed=10240
; log debug infos
throttle.debug=0
```

Both settings are per-dir and can be overriden on a per-directory basis; for example in an Apache's .htaccess:

```
php_value throttle.speed 2048
```

