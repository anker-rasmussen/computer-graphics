#!/bin/bash
cmake -B build && cmake --build build && \
cd build && env -u WAYLAND_DISPLAY DISPLAY=:1 ./Lab1
