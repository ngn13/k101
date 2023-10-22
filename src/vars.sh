#!/bin/bash -e 
KERNEL_VERSION=5.15.135
KERNEL_MAJOR=$(echo $KERNEL_VERSION | cut -d "." -f1)
KERNEL_SUM="c31afad7842cbb8afa0bbaf66d6ae573"
PARAMS="nokaslr nosmep nosmap nopti"
