#!/bin/bash

git add $1
git commit -m "Add $1 files"
git push origin master 
