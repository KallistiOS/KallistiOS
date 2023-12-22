class PresentationState
  attr_reader :page_index, :time_adjustment

  def initialize(page_index, time_adjustment)
    @page_index, @time_adjustment = page_index, time_adjustment
  end
end

class ResultConstants
  OK = 0
end

class PageTitleContent
  def initialize(content)
    @content = content
  end

  def render(dc_kos, _presentation_state, start_time)
    dc_kos.draw_str(@content, 10, 2, 'white', 'yes')
    ResultConstants::OK
  end
end

class CodeContent
  def initialize(x, y, content, show_bg)
    @x, @y, @content, @show_bg =
      x, y, content, show_bg
  end

  def render(dc_kos, _presentation_state, time_now)
    content_array = []
    split_contents = @content.split('"')
    split_contents.each_with_index do |fragment, idx|
      quoted = if idx % 2 == 0
        fragment
      else
        '"' + fragment + '"'
      end
      content_array << quoted
    end

    line_count = 0
    line_start = 0
    # colour_idx even -> not string literal. colour_idx odd -> string literal.
    content_array.each_with_index do |content, colour_idx|
      #puts "content:"
      #p content
      lines = content.split("\n")
      #puts "lines: "
      #p lines
      #puts "line_count: #{line_count}"
      #puts "line_start: #{line_start}"
      lines.each_with_index do |line, idx|
        if colour_idx % 2 == 0
          #puts "--- render code, line_start: #{line_start}"
          render_code(dc_kos, line, @x + line_start * 12, @y + (line_count + idx) * 30, @show_bg)
        else
          #puts "--- render string, line_start: #{line_start}"
          render_string(dc_kos, line, @x + line_start * 12, @y + (line_count + idx) * 30, @show_bg)
        end
        # start rendering from column 0 unless it is the last element of array
        # since lines have been split by "\n"
        line_start = 0 unless idx == lines.size - 1
      end
      line_start += lines[-1].to_s.size
      line_count += content.count("\n")
    end
    ResultConstants::OK
  end

  RBRESERVED = %w[class def end do if elsif else while for case when begin rescue raise fail]
  CRESERVED = %w[void int char return struct uint8_t extern const]
  RESERVED = RBRESERVED + CRESERVED

  # highlight while for do end def if elsif else class
  # highlight Constants
  def render_code(dc_kos, line, x, y, show_bg)
    #print "------ breaking line: "
    #p line
    #p break_line(line)
    #puts "^^^^^^ breaking line ^^^^^^"
    var_x, var_y = x, y
    break_line(line).each { |term|
      #puts "current term is: '#{term}'"
      case
      when "\n" == term
        var_x = x
        var_y += 30
      when RESERVED.include?(term.strip)
        #puts "rendering reserved: '#{term}'"
        dc_kos.draw_str(term, var_x, var_y, "yellow", show_bg)
        var_x += term.size * 12
      when uppercases.include?(term.strip[0])
        #puts "rendering upcase: '#{term}'"
        dc_kos.draw_str(term, var_x, var_y, "cyan", show_bg)
        var_x += term.size * 12
      when '@' == term.strip[0]
        #puts "rendering instance var: '#{term}'"
        dc_kos.draw_str(term, var_x, var_y, "ltgreen", show_bg)
        var_x += term.size * 12
      when '#' == term.strip[0]
        #puts "rendering comment: '#{term}'"
        dc_kos.draw_str(term, var_x, var_y, "ltblue", show_bg)
        var_x += term.size * 12
      else
        #puts "rendering other: '#{term}'"
        dc_kos.draw_str(term, var_x, var_y, "white", show_bg)
        var_x += term.size * 12
      end
    }
  end

  def uppercases
    %w[ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z ]
  end

  def alphas
    uppercases + %w[ a b c d e f g h i j k l m n o p q r s t u v w x y z ]
  end

  def nums
    %w[ 0 1 2 3 4 5 6 7 8 9 ]
  end

  def break_line(line)
    terms = []
    term = ''
    mode = :normal
    line.split('').each_with_index { |c, idx|
      is_eol = line.size == idx + 1
      #puts "current_mode: #{mode}, current_char: #{c}, is_eol: #{is_eol}"
      case
      when c == "\n"
        #puts "---------------------- adding #{term} to terms" if mode == :comment
        terms << term if term != ''
        terms << "\n"
        term = ''
        mode = :normal
      when mode == :normal
        if alphas.include?(c) || ["_", "@"].include?(c)
          terms << term if term != ''
          term = c
          mode = :word
        elsif nums.include?(c)
          terms << term if term != ''
          term = c
          terms << term if is_eol # needs this for lines like 'a += 1'
          mode = :num
        elsif '#' == c
          terms << term if term != ''
          term = c
          mode = :comment
        else
          term << c
          terms << term if is_eol
        end
      when mode == :word
        if alphas.include?(c) || c == "_" || nums.include?(c)
          term << c
          terms << term if is_eol
        else
          terms << term if term != ''
          term = c
          terms << term if is_eol # needs this for lines like 'foo(bar)'
          mode = :normal
        end
      when mode == :num
        if c == "_" || nums.include?(c)
          term << c
          terms << term if is_eol
        else
        #puts "adding term #{term}" if term != ''
          terms << term if term != ''
          term = c
          terms << term if is_eol
          mode = :normal
        end
      when mode == :comment
        #puts "****************** adding #{c} to comment"
        term << c
        terms << term if is_eol
        #puts "****************** adding #{term} to terms" if is_eol
      else
        raise "Unexpected char: #{c}"
      end
    }
    terms
  end

  def render_string(dc_kos, line, x, y, show_bg)
    dc_kos.draw_str(line, x, y, "magenta", show_bg)
  end
end

class TextContent
  def initialize(x, y, content, colour, show_bg)
    @x, @y, @content, @colour, @show_bg =
      x, y, content, colour, show_bg
  end

  def render(dc_kos, _presentation_state, time_now)
    dc_kos.draw_str(@content, @x, @y, @colour, @show_bg)
    ResultConstants::OK
  end
end

class ResizableImageContent
  def initialize(x, y, w, h, path)
    @x, @y, @w, @h, @path = x, y, w, h, path.strip
  end

  def render(dc_kos, _presentation_state, time_now)
    dc_kos.show_512x512_png(@path, @x, @y, @w, @h)
    ResultConstants::OK
  end
end

class ImageContent
  def initialize(x, y, path)
    @x, @y, @path = x, y, path.strip
  end

  def render(dc_kos, _presentation_state, time_now)
    dc_kos.render_png(@path, @x, @y)
    ResultConstants::OK
  end
end

# This renders a 'background' with 640x480 image
# It also renders timer and page progress
class PageBaseContent
  def initialize(path, page_count)
    @page_count = page_count
    @path = String(path).strip
    puts "-------- Background. path is: "
    p @path
  end

  def render(dc_kos, presentation_state, start_time)
    if @path && !@path.empty?
      dc_kos.render_png(@path, 0, 0)
    else
      puts "Rendering background image with no path. Skipping."
    end

    render_timer_progress(dc_kos, start_time, presentation_state.time_adjustment)
    render_page_progress(dc_kos, @page_count, presentation_state.page_index)
    ResultConstants::OK
  end

  def render_page_progress(dc_kos, page_count, page_index)
    PAGES_BAR_LEN = 640 - 32
    PAGES_Y_POS = 410

    pos_x =
      if page_count <= 1
        0
      else
        (page_index / (page_count - 1) * PAGES_BAR_LEN).to_i
      end

    dc_kos.render_png("/rd/swirl_blue_32x28.png", pos_x, PAGES_Y_POS)
  end

  def render_timer_progress(dc_kos, start_time, time_adjustment)
    DURATION = 20 * 60 # 20 mins
    PROGRESS_LEN = 640 - 32
    PROGRESS_Y_POS = 440

    pos_x = ((Time.now.to_i - start_time.to_i + time_adjustment) / DURATION * PROGRESS_LEN).to_i
    puts "#################### start_time: #{start_time}, adj: #{time_adjustment}, now: #{Time.now}, pos_x: #{pos_x}"
    pos_x = PROGRESS_LEN if pos_x > PROGRESS_LEN
    pos_x = 0 if pos_x < 0

    dc_kos.render_png("/rd/mruby_logo_32x35.png", pos_x, PROGRESS_Y_POS)
  end
end

# Wait for A or Start in page to go to next section
class WaitButtonContent
  def render(dc_kos, _presentation_state, time_now)
    key_input = dc_kos.next_or_back

    if key_input == Commands::NEXT_PAGE
      ResultConstants::OK
    else
      key_input
    end
  end
end

class LineContent
  def initialize(direction, x, y, len, width, colour)
    @direction, @x, @y, @len, @width, @colour =
      direction, x, y, len, width, colour.strip.downcase
  end

  def render(dc_kos, _presentation_state, time_now)
    # currently supports 'red', 'black'
    # everything else will be white
    # TODO: let's make a colour lookup class... See DcKos
    r, g, b =
      case @colour
      when 'red'
        [255, 0, 0]
      when 'yellow'
        [255, 255, 0]
      when 'black'
        [0, 0, 0]
      else
        [255, 255, 255]
      end

    if @direction == :horizontal
      (0...@width).each do |line_num|
        dc_kos.draw_horizontal_line(@x, @y + line_num, @len, r, g, b)
      end
    elsif @direction == :vertical
      (0...@width).each do |line_num|
        dc_kos.draw_vertical_line(@x + line_num, @y, @len, r, g, b)
      end
    end
    ResultConstants::OK
  end
end

class TimerReset
  def render(_dc_kos, _presentation_state, time_now)
    Commands::RESET_TIMER
  end
end

class BlockContent
  def initialize(x0, y0, x1, y1)
    @x0, @y0, @x1, @y1 = x0, y0, x1, y1
  end

  def render(dc_kos, ps, time_now)
    LineContent.new(:horizontal, @x0, @y0, (@x1 - @x0), (@y1 - @y0), 'black').render(dc_kos, ps, time_now)
  end
end

=begin
Parser parses input string (which is prepared by you, or read from a file).

= Equal sign at the beginning of line is start of new page with a title.

-txt,120,200: This means text is shown from (120, 200).
Newlines should be respected.
-img,120,300: /rd/Dreamcast.png
-txt,120,400: The above img line says show the image ("/rd/Dreamcast.png")
from (120, 300)
=

-txt,80,40: This is a title-less page.

=end

class Page
  def initialize(sections)
    @sections = sections
  end

  def show(dc_kos, presentation_state, start_time)
    puts "------ about to call each on @sections"
    p @sections
    @sections.each { |s|
      render_result = s.render(dc_kos, presentation_state, start_time)
      puts "-------- section render result: #{ render_result }"

      # return if user pressed PREV, QUIT, etc.
      return render_result unless [Commands::NEXT_PAGE, ResultConstants::OK].include?(render_result)
    }

    # if waiting for input between sections above did not result in returning
    # input result, get input result here and return it
    return dc_kos.next_or_back
  end
end

class Parser
  def parse_line_xy_wh_path(line)
    split_line = line.split(':')
    tag = split_line[0]
    _unused, x, y, w, h = tag.split(',')
    rest = split_line[1..-1].join(':')
    return x.to_i, y.to_i, w.to_i, h.to_i, rest
  end

  def parse_line_xy_col_bg(line)
    split_line = line.split(':')
    tag = split_line[0]
    _unused, x, y, colour, show_bg = tag.split(',')
    rest = split_line[1..-1].join(':')
    return x.to_i, y.to_i, colour, show_bg, rest
  end

  def parse_line_specification(line)
    split_line = line.split(':')
    tag = split_line[0]
    _unused, x, y, len, width = tag.split(',')

    return x.to_i, y.to_i, len.to_i, width.to_i, split_line[1]
  end

  def parse_line_no_xy(line)
    split_line = line.split(':')
    path = split_line[1..-1].join(':')
    return path
  end

  def parse_line_rectangle(line)
    split_line = line.split(':')
    tag = split_line[0]
    _unused, x0, y0, x1, y1 = tag.split(',')
    return x0.to_i, y0.to_i, x1.to_i, y1.to_i
  end

  def parse_code_line(line)
    split_line = line.split(':')
    tag = split_line[0]
    _unused, x, y, lang = tag.split(',')
    rest = split_line[1..-1].join(':')
    return x.to_i, y.to_i, lang, rest
  end

  def parse(input_str)
    pages = input_str
      .split("\n")
      .reject { |l| l.slice(0, 1) == '#' } # remove comment lines
      .join("\n")
      .split("\n=")

    page_count = pages.size
    last_background = [PageBaseContent.new(nil, page_count)]

    page_objs = pages.map { |page|
      parsed_page = page.split("\n-").each_with_index.map { |section, idx|
        case
        when idx == 0
          title = section.sub('=', '').strip # remove the first '='
          PageTitleContent.new(title)
        when section.slice(0,10) == 'resettimer'
          TimerReset.new
        when section.slice(0,8) == 'txtblock'
          x0, y0, x1, y1 = parse_line_rectangle(section)
          BlockContent.new(x0, y0, x1, y1)
        when section.slice(0,6) == 'img512'
          x, y, w, h, image_path = parse_line_xy_wh_path(section)
          ResizableImageContent.new(x, y, w, h, image_path)
        when section.slice(0,5) == 'hline'
          x, y, len, width, colour = parse_line_specification(section)
          LineContent.new(:horizontal, x, y, len, width, colour)
        when section.slice(0,5) == 'vline'
          x, y, len, width, colour = parse_line_specification(section)
          LineContent.new(:vertical, x, y, len, width, colour)
        when section.slice(0,4) == 'wait'
          WaitButtonContent.new
        when section.slice(0,4) == 'code'
          x, y, lang, text_content = parse_code_line(section)
          CodeContent.new(x, y, text_content, nil)
        when section.slice(0,3) == 'txt'
          x, y, colour, show_bg, text_content = parse_line_xy_col_bg(section)
          TextContent.new(x, y, text_content, colour, show_bg)
        when section.slice(0,3) == 'img'
          x, y, _colour, _bg_colour, image_path = parse_line_xy_col_bg(section)
          ImageContent.new(x, y, image_path)
        when section.slice(0,3) == 'bkg'
          bg_path = parse_line_no_xy(section)
          PageBaseContent.new(bg_path, page_count)
        else
          # not sure. keep it as nil
        end
      }.compact

      background_sections = parsed_page.select { |elem| elem.is_a?(PageBaseContent) }
      other_sections = parsed_page.select { |elem| !elem.is_a?(PageBaseContent) }

      sorted_page =
        if background_sections.empty?
          last_background + other_sections
        else
          last_background = background_sections
          background_sections + other_sections
        end

      puts "==== sorted page:"
      p sorted_page
      Page.new(sorted_page)
    }
  end
end

class PageData
  def initialize(dc_kos)
    @dc_kos = dc_kos
  end

  def all
    content_str = @dc_kos.read_whole_txt_file("/rd/content.dreampresent")
    Parser.new.parse(content_str)
  end
end
