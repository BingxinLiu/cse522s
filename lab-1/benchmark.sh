#!/bin/bash
echo "benchmark start" >> result;
echo "size = 64" >> result;

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./dense_mm 64 >> result;
done

echo "size = 238" >> result

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./dense_mm 128 >> result;
done

echo "size = 256" >> result;

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./dense_mm 256 >> result;
done

echo "size = 512" >> result;

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./dense_mm 512 >> result;
done

echo "benchmark end" >> result;

