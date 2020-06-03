#!/bin/bash
# Compiling and running the Rock Paper Scissors program


gcc rock_paper_scissors.c -o rock_paper_scissors -lpthread

echo 'Running Rock Paper Scissors..'

./rock_paper_scissors

