/*
 *   libperfmap: a JVM agent to create perf-<pid>.map files for consumption
 *               with linux perf-tools
 *   Copyright (C) 2013-2015 Johannes Rudolph<johannes.rudolph@gmail.com>
 *   Copyright (C) 2021 Ankush Rayabhari<ankush.rayabhari@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "util/profile_map_generator.h"

#include <string_view>
#include <unistd.h>

namespace kush::util {

ProfileMapGenerator::ProfileMapGenerator() {
  auto pid = getpid();
  auto file_name = "/tmp/perf-" + std::to_string(pid) + ".map";
  fout_.open(file_name, std::ofstream::out | std::ofstream::trunc);
}

ProfileMapGenerator::~ProfileMapGenerator() { fout_.close(); }

void ProfileMapGenerator::AddEntry(void* code_addr, uint64_t code_size,
                                   std::string_view func_name) {
  uint64_t addr = reinterpret_cast<uint64_t>(code_addr);
  fout_ << std::hex << addr << ' ' << std::hex << code_size << ' ' << func_name
        << '\n';
}

}  // namespace kush::util
