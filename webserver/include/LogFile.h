#pragma once

#include "FileUtil.h"
#include <mutex>
#include <memory>
#include <ctime>

/**
 * @brief 日志文件管理类
 * 负责日志文件的创建、写入、滚动和刷新等操作
 * 支持按大小和时间自动滚动日志文件
 */