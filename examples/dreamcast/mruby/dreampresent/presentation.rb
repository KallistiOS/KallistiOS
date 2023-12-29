# KallistiOS ##version##
#
# presentation.rb - Presentation handler
# Copyright (C) 2019-2024 Yuji Yokoo
# Copyright (C) 2020-2024 Mickaël "SiZiOUS" Cardoso
#
# Dreampresent
# A simple presentation tool for Sega Dreamcast written in Ruby
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

class Presentation
  include Commands

  def initialize(dc_kos, pages)
    @dc_kos, @pages = dc_kos, pages
    @start_time = Time.now
  end

  def run
    idx = 0
    time_adjustment = 0
    while(idx < @pages.length)
      input = @pages[idx].show(@dc_kos,
        PresentationState.new(idx, time_adjustment),
        @start_time
      )

      case
      when input == NEXT_PAGE
        idx += 1
      when input == PREVIOUS_PAGE
        idx -= 1
      when input == QUIT
        idx += @pages.length
      when input == SWITCH_VIDEO_MODE
        @dc_kos.next_video_mode
      when input == RESET_TIMER
        @start_time = Time.now
        idx += 1
      when input == FWD
        time_adjustment += 300
        time_adjustment = (40 * 60) if time_adjustment > (40 * 60)
      when input == REW
        time_adjustment -= 300
        time_adjustment = 0 if time_adjustment < 0
      end
      puts "- - - - current idx = #{idx}"
      idx = 0 if idx < 0 # do not wrap around backwards
    end
  end
end
