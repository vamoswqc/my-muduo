aux_source_directory(. TEST_SRC)

add_executable(muduo_server ${TEST_SRC})

# 链接自己的库和系统库
target_link_libraries(muduo_server my-muduo)
target_link_libraries(muduo_server muduo_net muduo_base pthread)
