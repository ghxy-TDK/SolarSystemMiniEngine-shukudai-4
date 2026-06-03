#pragma once

// ============================================================
//  core/timer.h
//  帧计时工具。每帧调用 tick() 更新，其余系统通过
//  get_time() / delta_time() 查询。
// ============================================================

namespace Timer {

// 返回自程序启动以来经过的秒数（高精度，double）。
double get_time();

// 返回上一帧与当前帧之间的时间差（秒，float）。
// 首帧返回 0.0f。
float delta_time();

// 必须在每帧开始时调用（通常在 display callback 最开头）。
// 内部更新 previous_time 并重新计算 delta。
void tick();

} // namespace Timer
