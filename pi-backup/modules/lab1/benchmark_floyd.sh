#!/bin/bash
echo "# of vertices = 64" >> res_floyd;
for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./floyd_warshall 64 >> res_floyd;
done

echo "# of vertices = 128" >> res_floyd

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./floyd_warshall 128 >> res_floyd;
done

echo "# of vertices = 256" >> res_floyd;

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./floyd_warshall 256 >> res_floyd;
done

echo "# of vertices = 512" >> res_floyd;

for loop in 1 2 3 4 5 6 7 8 9 10
do
    sudo ./floyd_warshall 512 >> res_floyd;
done


