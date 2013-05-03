#!/bin/bash
for i in 0 1 2 3 4 5 6 7 8 9 a 
do 
  cat ../10_10000_800_"$i"_65536_131072_32768/*.log | grep $1 | awk '{print $11;}' | awk '{tot+=$1;}END{print tot/10;}'
done