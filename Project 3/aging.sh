#!/usr/bin/env bash
java vmsim -n 8 -a aging -r 1 gcc.trace
java vmsim -n 8 -a aging -r 1 bzip.trace
java vmsim -n 16 -a aging -r 1 gcc.trace
java vmsim -n 16 -a aging -r 1 bzip.trace
java vmsim -n 32 -a aging -r 1 gcc.trace
java vmsim -n 32 -a aging -r 1 bzip.trace
java vmsim -n 64 -a aging -r 1 gcc.trace
java vmsim -n 64 -a aging -r 1 bzip.trace
