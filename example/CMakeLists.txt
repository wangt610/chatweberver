add_executable(main main.cpp ${SRC_LIST} ${SRC_LIST1} ${SRC_LIST2})
target_link_libraries(main  muduo_net muduo_base pthread mysqlclient)