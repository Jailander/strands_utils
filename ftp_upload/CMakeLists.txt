cmake_minimum_required(VERSION 2.8.3)
project(ftp_upload)

find_package(catkin REQUIRED COMPONENTS rospy genmsg actionlib_msgs actionlib)

add_action_files(DIRECTORY action 
  FILES FtpUpload.action)
generate_messages(DEPENDENCIES actionlib_msgs)


catkin_package(
)

