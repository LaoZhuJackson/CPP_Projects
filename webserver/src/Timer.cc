#include "Timer.h"

void Timer::restart(Timestamp now){
    // 如果是重复定时事件，则继续添加定时事件，得到新事件到期事件
    if(repeat_) expiration_ = addTime(now, interval_);
    // 否则则延长时间
    else expiration_ = Timestamp();
}