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
