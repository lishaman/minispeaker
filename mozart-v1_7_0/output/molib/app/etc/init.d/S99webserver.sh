#!/bin/bash

# start webserver(config ap info)
httpd -p 80 -h /var/www -c /etc/httpd.conf
