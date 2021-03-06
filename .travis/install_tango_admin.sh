#!/usr/bin/env bash

set -e

admin_dir="/home/tango/tango_admin"

echo "Build tango_admin"
docker exec cpp_tango mkdir -p "${admin_dir}/build"
docker exec cpp_tango cmake -H"${admin_dir}" -B"${admin_dir}/build"
docker exec cpp_tango make -C "${admin_dir}/build"

echo "Install tango_admin"
docker exec --user root cpp_tango cp "${admin_dir}/build/tango_admin" /usr/local/bin
