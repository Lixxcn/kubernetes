/*
Copyright 2016 The Kubernetes Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// 包含必要的系统头文件
#include <signal.h>    // 信号处理相关函数
#include <stdio.h>     // 标准输入输出函数
#include <stdlib.h>    // 标准库函数
#include <string.h>    // 字符串处理函数
#include <sys/types.h> // 系统数据类型定义
#include <sys/wait.h>  // 进程等待相关函数
#include <unistd.h>    // UNIX标准函数

// 定义字符串化宏，用于将传入的参数转换为字符串
#define STRINGIFY(x) #x
#define VERSION_STRING(x) STRINGIFY(x)

// 如果没有定义VERSION，则默认设置为HEAD
#ifndef VERSION
#define VERSION HEAD
#endif

// 信号处理函数：处理终止信号（SIGINT和SIGTERM）
static void sigdown(int signo) {
  // 打印收到的信号信息
  psignal(signo, "Shutting down, got signal");
  // 正常退出程序
  exit(0);
}

// 信号处理函数：处理子进程结束信号（SIGCHLD）
static void sigreap(int signo) {
  // 循环等待所有子进程，防止出现僵尸进程
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int main(int argc, char **argv) {
  int i;
  // 处理命令行参数
  for (i = 1; i < argc; ++i) {
    // 如果有-v参数，打印版本信息并退出
    if (!strcasecmp(argv[i], "-v")) {
      printf("pause.c %s\n", VERSION_STRING(VERSION));
      return 0;
    }
  }

  // 检查当前进程是否为PID 1进程
  if (getpid() != 1)
    // 如果不是PID 1，输出警告信息（不作为错误处理，因为pause容器可能在基础设施容器之外使用）
    fprintf(stderr, "Warning: pause should be the first process\n");

  // 注册SIGINT信号处理函数
  if (sigaction(SIGINT, &(struct sigaction){.sa_handler = sigdown}, NULL) < 0)
    return 1;
  // 注册SIGTERM信号处理函数
  if (sigaction(SIGTERM, &(struct sigaction){.sa_handler = sigdown}, NULL) < 0)
    return 2;
  // 注册SIGCHLD信号处理函数，设置SA_NOCLDSTOP标志（子进程停止时不发送SIGCHLD信号）
  if (sigaction(SIGCHLD, &(struct sigaction){.sa_handler = sigreap,
                                             .sa_flags = SA_NOCLDSTOP},
                NULL) < 0)
    return 3;

  // 无限循环，调用pause()函数使进程休眠，直到收到信号
  for (;;)
    pause();
  // 理论上永远不会执行到这里，如果执行到这里就输出错误信息
  fprintf(stderr, "Error: infinite loop terminated\n");
  return 42;
}
