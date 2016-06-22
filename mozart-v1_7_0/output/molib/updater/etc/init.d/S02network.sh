#!/bin/bash

# networking
ifconfig lo 127.0.0.1 up

# start network_manager
network_manager -B
