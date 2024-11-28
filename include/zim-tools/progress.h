/*
 * Copyright (C) Kiran Mathew Koshy
 * Copyright (C) Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef ZIM_TOOL_PROGRESS_H_
#define ZIM_TOOL_PROGRESS_H_

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>

using TimePoint = std::chrono::system_clock::time_point;

class ProgressBar  // Class for implementing a progress bar(used in redundancy,
                   // url and MIME checks).
{
 private:
  double time_interval{1};       // The time interval a report will be printed.
  TimePoint last_report_time;  // Last time a report has been printed.
  int max_no{0};                 // Maximum no of times report() will be called.
  std::atomic<int> counter{0};  // Number of times report() has been called(at a
                                // particular time).
  bool report_progress{false};  // Boolean value to store wether report should
                                // display any characters.
  std::mutex mutex;

 public:
  ProgressBar(double time_interval) : time_interval(time_interval) {}

  void reset(int max_n)
  {
    max_no = max_n;
    counter = 0;
    last_report_time = TimePoint();
    time_interval = 1;
  }

  void report()
  {
    if (counter >= max_no) {
      return;
    }

    counter++;

    if (!report_progress) {
      return;
    }

    std::unique_lock<std::mutex> const lock(mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> const duration = now - last_report_time;

    if (duration.count() > time_interval) {
      std::cout << "\r" << counter << "/" << max_no << std::flush;
      last_report_time = now;
    }
    if (counter == max_no) {
      std::cout << "\r" << counter << "/" << max_no << '\n';
    }
  }

  void set_progress_report(bool report = true) { report_progress = report; }
};

#endif  // ZIM_TOOL_PROGRESS_H_
