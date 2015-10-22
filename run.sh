#!/usr/bin/env bash

scp bin/memload.hex ubuntu@192.168.2.102:

ssh ubuntu@192.168.2.102 avrdude -c usbasp -p t2313 -U flash:w:memload.hex