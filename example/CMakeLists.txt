add_executable(main main.cpp ${SRC_LIST} ${SRC_LIST1} ${SRC_LIST2} ${SRC_LIST4})
add_executable(client ${SRC_LIST3})
target_link_libraries(main  muduo_net muduo_base pthread mysqlclient hiredis)
target_link_libraries(client pthread)