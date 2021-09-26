# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test create proper pool
execute(${TEST_EXECUTABLE} c ${DIR}/testfile "test" 20 0600)

check_file_exists(${DIR}/testfile)

finish()
