#!/bin/bash

echo "Starting container: vfs-devel"
docker run --name vfs-devel -it -v ~/Projects/vfs:/vfs vfs bash
echo "Removing containter: vfs-devel"
docker rm vfs-devel
