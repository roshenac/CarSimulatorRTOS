# Simulation of car operation using a Real-Time Operating System (RTOS)

## Description
  A real-time operating system is created in order so simulate a number of car processes. These include: brake, acceleration, odometry, speed, average speed, side lights, engine state and indicators.

## Method
A thread scheduallar is used in order to carry out multiple processes. The maximum number of additional threads we were able to use was 10. As a result, several processes with the same repetition rate had to go under a single thread. 4 semaphores are used which allows controlled access between these processes.

## Authors
  Roshenac Mitchell - March 2016
