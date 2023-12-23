class BoardState
  attr_accessor :x, :y, :last_rendered_block_state

  attr_reader :board, :shape, :shape_orientation, :next_shape, :score

  def initialize(x, y, screen)
    @x, @y = x, y
    @screen = screen
    @last_x, @last_y = 0, 0
    @board = new_empty_board
    update_board_bitmap
    @next_shape = BlockShapes.random_shape
    sp = BlockShapes.random_shape
    @shape_name = sp[0]
    @shape = sp[1]
    @shape_orientation, @last_shape_orientation = 0, 0
    @last_rendered_block_state = { x: x, y: y, orientation: 0 }
    @score = 0
    @lock_delay = LOCK_DELAY
    build_shape_bitmaps
  end

  LOCK_DELAY = 30

  SHAPE_COLOURS = {sq: :yellow, i: :cyan, l: :orange, j: :blue, s: :green, z: :red, t: :purple}.freeze

  LEFT_AND_RIGHT_BLOCKS = ([:grey] + [false] * 10 + [:grey]).freeze
  HORIZONTAL_WALL = ([:grey]*12).freeze
  BELOW_BOTTOM = ([false] * 12).freeze

  EMPTY_BOARD =
    (
      [ HORIZONTAL_WALL ] +
      [ LEFT_AND_RIGHT_BLOCKS ] * 20 +
      [ HORIZONTAL_WALL ] +
      [ BELOW_BOTTOM, BELOW_BOTTOM ] # 2 lines below bottom border for easier processing
    ).freeze

  FOUR_ONES = "1111".to_i(2).freeze

  def new_empty_board
    new_board = Array.new

    EMPTY_BOARD.each { |row|
      new_board.push row.compact # compact simply copies if no nil present
    }

    new_board
  end

  def update_board_bitmap
    @board_bitmap = @board.map { |row|
      ((row[0] ? 1 : 0) << 11) |
      ((row[1] ? 1 : 0) << 10) |
      ((row[2] ? 1 : 0) << 9) |
      ((row[3] ? 1 : 0) << 8) |
      ((row[4] ? 1 : 0) << 7) |
      ((row[5] ? 1 : 0) << 6) |
      ((row[6] ? 1 : 0) << 5) |
      ((row[7] ? 1 : 0) << 4) |
      ((row[8] ? 1 : 0) << 3) |
      ((row[9] ? 1 : 0) << 2) |
      ((row[10] ? 1 : 0) << 1) |
      (row[11] ? 1 : 0)
    }
  end

  def update_last_frame_block_state
    @last_rendered_block_state = { x: @x, y: @y, orientation: @shape_orientation }
  end

  def full_row_idxs
    idxs = []
    (1..20).each {|idx|
      if @board[idx].reduce {|a,b| a&&b}
        idxs << idx
      end
    }
    idxs
  end

  def clear_full_rows
    fr_idxs = full_row_idxs
    fr_idxs.each { |i|
      @board.delete_at(i)
      @board.insert(1, LEFT_AND_RIGHT_BLOCKS.compact) # compact simply copies if no nil present
    }
    scores = [0, 1, 2, 3, 8]
    @score += scores[fr_idxs.size]
    fr_idxs.size
  end

  def whiten_full_rows
    rows = full_row_idxs
    rows.each { |r|
      (1..10).to_a.each { |c|
        @screen.draw_colour_square(c, r, :white, false)
      }
    }
  end

  def save_to_board
    @shape[@shape_orientation].each_with_index { |row, rownum|
      row.each_with_index { |cell, colnum|
        @board[@y+rownum][@x+colnum] = cell if cell
      }
    }
  end

  def next_orientation(orientation)
    (orientation + 1) % 4
  end

  def previous_orientation(orientation)
    (orientation + 3) % 4
  end

  def clockwise
    if can_rotate_cw?
      @shape_orientation = next_orientation(@shape_orientation)
      @lock_delay = LOCK_DELAY
    end
  end

  def anticlockwise
    if can_rotate_anticw?
      @shape_orientation = previous_orientation(@shape_orientation)
      @lock_delay = LOCK_DELAY
    end
  end

  def zero_if_negative(x)
    if x < 0 ; 0 ; else ; x ; end
  end

  ROW_OFFSETS = [12, 8, 4, 0].freeze
  def board_section_for(x, y, horizontal_shift, vertical_shift)
    left = x + horizontal_shift
    top = y + vertical_shift
    bitmap = 0
    shift = 0
    [0, 1, 2, 3].each { |idx|
      # board is 12 x 22
      # mask (four_ones) is 1111, shifting 8 gets them to 0-3 from left
      shift = 8 - left
      bitmap |= (@board_bitmap[top+idx] & (FOUR_ONES << shift)) >> (shift - ROW_OFFSETS[idx])
    }
    bitmap
  end

  private
  def build_shape_bitmaps
    @shape_bitmaps = {}
    all_shapes = BlockShapes::all_shapes
    all_shapes.each { |curr_shape|
      @shape_bitmaps[curr_shape[0]] = curr_shape[1].map { |curr_shape_orientation|
        ((curr_shape_orientation[0][0] ? 1 : 0 )<< 15) |
        ((curr_shape_orientation[0][1] ? 1 : 0 )<< 14) |
        ((curr_shape_orientation[0][2] ? 1 : 0 )<< 13) |
        ((curr_shape_orientation[0][3] ? 1 : 0 )<< 12) |
        ((curr_shape_orientation[1][0] ? 1 : 0 )<< 11) |
        ((curr_shape_orientation[1][1] ? 1 : 0 )<< 10) |
        ((curr_shape_orientation[1][2] ? 1 : 0 )<< 9) |
        ((curr_shape_orientation[1][3] ? 1 : 0 )<< 8) |
        ((curr_shape_orientation[2][0] ? 1 : 0 )<< 7) |
        ((curr_shape_orientation[2][1] ? 1 : 0 )<< 6) |
        ((curr_shape_orientation[2][2] ? 1 : 0 )<< 5) |
        ((curr_shape_orientation[2][3] ? 1 : 0 )<< 4) |
        ((curr_shape_orientation[3][0] ? 1 : 0 )<< 3) |
        ((curr_shape_orientation[3][1] ? 1 : 0 )<< 2) |
        ((curr_shape_orientation[3][2] ? 1 : 0 )<< 1) |
        (curr_shape_orientation[3][3] ? 1 : 0 )
      }
    }
    puts "---------------------------------------------------------"
    p @shape_bitmaps
    puts "---------------------------------------------------------"
  end

  def row(row_idx, l, r, left_outside_area, right_outside_area)
    if row_idx < 0 || row_idx >= @board.size || r < 0 || l > 11
      [false, false, false, false]
    else
      left_outside_area + Array(@board[row_idx][l..r]) + right_outside_area
    end
  end

  public
  def can_rotate_cw?
    board_section = board_section_for(@x, @y, 0, 0)
    next_shape = @shape_bitmaps[@shape_name][next_orientation(@shape_orientation)]
    !collides?(board_section, next_shape)
  end

  def can_rotate_anticw?
    board_section = board_section_for(@x, @y, 0, 0)
    previous_shape = @shape_bitmaps[@shape_name][previous_orientation(@shape_orientation)]
    !collides?(board_section, previous_shape)
  end

  def decrement_lock_deloy
    @lock_delay -= 1
  end

  def move_left
    if can_go_left?
      @lock_delay = LOCK_DELAY
      @x -= 1
    end
  end

  def can_go_left?
    board_section = board_section_for(@x, @y, -1, 0)
    !collides?(board_section, @shape_bitmaps[@shape_name][@shape_orientation])
  end

  def collides?(board_section, block_bitmap)
    (board_section & block_bitmap) != 0
  end

  def move_right
    if can_go_right?
      @x += 1
      @lock_delay = LOCK_DELAY
    end
  end

  def can_go_right?
    board_section = board_section_for(@x, @y, 1, 0)
    !collides?(board_section, @shape_bitmaps[@shape_name][@shape_orientation])
  end

  def move_down
    if can_drop?
      @y += 1
      @lock_delay = LOCK_DELAY
    end
  end

  def move_down!
    @y += 1
    @lock_delay = LOCK_DELAY
  end

  def can_drop?
    board_section = board_section_for(@x, @y, 0, 1)
    result = !collides?(board_section, @shape_bitmaps[@shape_name][@shape_orientation])
    result
  end

  def lock_delay_elapsed?
    @lock_delay <= 0
  end

  def save_current_position
    @last_x = @x
    @last_y = @y
    @last_shape_orientation = @shape_orientation
  end

  def moved_horizontal?
    @last_x != @x
  end

  def moved_vertical?
    @last_y != @y
  end

  def rotated?
    @last_shape_orientation != @shape_orientation
  end

  def moved?
    @last_x != @x || @last_y != @y
  end

  def render_if_moved(dc2d)
    four_ones = "1111".to_i(2)
    if moved? || rotated?
      # building 5x4 bits for current
      block_bitmap = @shape_bitmaps[@shape_name][@shape_orientation]
      current_block = (block_bitmap & four_ones) |
        (block_bitmap & (four_ones << 4)) << 1 |
        (block_bitmap & (four_ones << 8)) << 2 |
        (block_bitmap & (four_ones << 12)) << 3

      # building 5x4 bits for previous block position / orientation
      prev_bitmap = @shape_bitmaps[@shape_name][@last_rendered_block_state[:orientation]]
      prev_block = (prev_bitmap & four_ones) |
        (prev_bitmap & (four_ones << 4)) << 1 |
        (prev_bitmap & (four_ones << 8)) << 2 |
        (prev_bitmap & (four_ones << 12)) << 3

      moved_right = @x > @last_rendered_block_state[:x] # block moved right
      moved_left = @x < @last_rendered_block_state[:x] # block moved left

      if moved_right
        prev_block <<= 1 # shift previous to left
      elsif moved_left
        current_block <<= 1 # shift current block to left
      end
      moved_down = @y > @last_rendered_block_state[:y] # block moved down
      if moved_down
        prev_block <<= 5 # move previous up 1 row
      end

      @to_erase = (prev_block ^ current_block) & prev_block
      @to_draw = (prev_block ^ current_block) & current_block

      @move_left_adjust = (moved_left ? 1 : 0)

      # erase squares without looping
      erase_cell(24, -1, -1) if (@to_erase >> 24) & 1 ==1
      erase_cell(23, 0, -1) if (@to_erase >> 23) & 1 ==1
      erase_cell(22, 1, -1) if (@to_erase >> 22) & 1 ==1
      erase_cell(21, 2, -1) if (@to_erase >> 21) & 1 ==1
      erase_cell(20, 3, -1) if (@to_erase >> 20) & 1 ==1
      erase_cell(19, -1, 0) if (@to_erase >> 19) & 1 ==1
      erase_cell(18, 0, 0) if (@to_erase >> 18) & 1 ==1
      erase_cell(17, 1, 0) if (@to_erase >> 17) & 1 ==1
      erase_cell(16, 2, 0) if (@to_erase >> 16) & 1 ==1
      erase_cell(15, 3, 0) if (@to_erase >> 15) & 1 ==1
      erase_cell(14, -1, 1) if (@to_erase >> 14) & 1 ==1
      erase_cell(13, 0, 1) if (@to_erase >> 13) & 1 ==1
      erase_cell(12, 1, 1) if (@to_erase >> 12) & 1 ==1
      erase_cell(11, 2, 1) if (@to_erase >> 11) & 1 ==1
      erase_cell(10, 3, 1) if (@to_erase >> 10) & 1 ==1
      erase_cell(9, -1, 2) if (@to_erase >> 9) & 1 ==1
      erase_cell(8, 0, 2) if (@to_erase >> 8) & 1 ==1
      erase_cell(7, 1, 2) if (@to_erase >> 7) & 1 ==1
      erase_cell(6, 2, 2) if (@to_erase >> 6) & 1 ==1
      erase_cell(5, 3, 2) if (@to_erase >> 5) & 1 ==1
      erase_cell(4, -1, 3) if (@to_erase >> 4) & 1 ==1
      erase_cell(3, 0, 3) if (@to_erase >> 3) & 1 ==1
      erase_cell(2, 1, 3) if (@to_erase >> 2) & 1 ==1
      erase_cell(1, 2, 3) if (@to_erase >> 1) & 1 ==1
      erase_cell(0, 3, 3) if @to_erase & 1 ==1

      # draw cells without looping
      draw_cell(24, -1, -1) if (@to_draw >> 24) & 1 ==1
      draw_cell(23, 0, -1) if (@to_draw >> 23) & 1 ==1
      draw_cell(22, 1, -1) if (@to_draw >> 22) & 1 ==1
      draw_cell(21, 2, -1) if (@to_draw >> 21) & 1 ==1
      draw_cell(20, 3, -1) if (@to_draw >> 20) & 1 ==1
      draw_cell(19, -1, 0) if (@to_draw >> 19) & 1 ==1
      draw_cell(18, 0, 0) if (@to_draw >> 18) & 1 ==1
      draw_cell(17, 1, 0) if (@to_draw >> 17) & 1 ==1
      draw_cell(16, 2, 0) if (@to_draw >> 16) & 1 ==1
      draw_cell(15, 3, 0) if (@to_draw >> 15) & 1 ==1
      draw_cell(14, -1, 1) if (@to_draw >> 14) & 1 ==1
      draw_cell(13, 0, 1) if (@to_draw >> 13) & 1 ==1
      draw_cell(12, 1, 1) if (@to_draw >> 12) & 1 ==1
      draw_cell(11, 2, 1) if (@to_draw >> 11) & 1 ==1
      draw_cell(10, 3, 1) if (@to_draw >> 10) & 1 ==1
      draw_cell(9, -1, 2) if (@to_draw >> 9) & 1 ==1
      draw_cell(8, 0, 2) if (@to_draw >> 8) & 1 ==1
      draw_cell(7, 1, 2) if (@to_draw >> 7) & 1 ==1
      draw_cell(6, 2, 2) if (@to_draw >> 6) & 1 ==1
      draw_cell(5, 3, 2) if (@to_draw >> 5) & 1 ==1
      draw_cell(4, -1, 3) if (@to_draw >> 4) & 1 ==1
      draw_cell(3, 0, 3) if (@to_draw >> 3) & 1 ==1
      draw_cell(2, 1, 3) if (@to_draw >> 2) & 1 ==1
      draw_cell(1, 2, 3) if (@to_draw >> 1) & 1 ==1
      draw_cell(0, 3, 3) if @to_draw & 1 ==1

      update_last_frame_block_state
    end
  end

  def erase_cell(idx, x_, y)
    x = x_ + @move_left_adjust
    @screen.draw_black_square(@x+x, @y+y, false)
  end

  def draw_cell(idx, x_, y)
    x = x_ + @move_left_adjust
    @screen.draw_colour_square(@x+x, @y+y, SHAPE_COLOURS[@shape_name], false)
  end

  def render_block
    @shape[@shape_orientation].each_with_index { |row, rownum|
      row.each_with_index { |cell, colnum|
        @screen.draw_colour_square(@x+colnum, @y+rownum, SHAPE_COLOURS[@shape_name], false) if cell
      }
    }
  end

  def whiten_curr_pos
     @shape[@shape_orientation].each_with_index { |row, rownum|
      row.each_with_index { |_cell, colnum|
        @screen.draw_colour_square(@x+colnum, @y+rownum, :white, false) if _cell
      }
    }
  end

  def next_block(x, y)
    @x, @y = x, y
    @last_x, @last_y = 0, 0
    @shape_name = @next_shape[0]
    @shape = @next_shape[1]
    @next_shape = BlockShapes.random_shape
    @shape_orientation = 0
    @last_rendered_block_state = { x: @x, y: @y, orientation: 0 }
  end
end

class Screen
  LEFT_SPACE_PX=260
  TOP_SPACE_PX=20

  attr_reader :dc2d

  def initialize(dc2d)
    @dc2d = dc2d
  end

  def render_score(board_state)
    @dc2d::render_score(board_state.score)
  end

  def render_upcoming_block_pane(board_state)
    normalised_next_shape = make_4x4(board_state.next_shape[1][0])
    normalised_next_shape.each_with_index { |row, rownum|
      row.each_with_index { |cell, colnum|
        if cell
          draw_colour_square(colnum + 14, rownum + 2, cell, true)
        else
          draw_black_square(colnum + 14, rownum + 2, true)
        end
      }
    }
  end

  def make_4x4(shape)
    shape.map { |row| row + [false] * (4 - row.size) }
  end

  def draw_square(x, y, r, g, b, ignore_boundary)
    if (x > 0 && x < 11 && y > 0 && y < 21) || ignore_boundary
      @dc2d::draw20x20_640(x*20+LEFT_SPACE_PX, y*20+TOP_SPACE_PX, r, g, b)
    end
  end

  def draw_colour_square(x, y, colour, ignore_boundary)
    draw_square(x, y, *(BlockShapes.colour_to_rgb colour), ignore_boundary)
  end

  def draw_black_square(x, y, ignore_boundary)
    draw_square(x, y, 0, 0, 0, ignore_boundary)
  end

  def draw_board(board)
    board.each_with_index { |row, y|
      row.each_with_index { |cell, x|
        if cell
          draw_colour_square(x, y, cell, true)
        else
          draw_black_square(x, y, true)
        end
      }
    }
  end
end

class GameState
  FPS = 60
  BUFSIZE = 100

  attr_accessor :frame, :tick, :curr_wait, :ticks_since_wait_change, :button_state_unchanged_for, :board_state, :button_states, :curr_button_index

  def initialize(dc2d, screen, board_state_class)
    @screen = screen
    @board_state_class = board_state_class

    @cont = {}
    @cont[:start], @cont[:d_left], @cont[:d_right], @cont[:d_up],
      @cont[:d_down], @cont[:a], @cont[:b] = dc2d::get_button_masks

    reset
  end

  def reset
    @frame = 0
    @tick = 0
    @curr_wait = 10
    @ticks_since_wait_change = 0
    @board_state = @board_state_class.new(4, 1, @screen)
    @unchanged_buttons = {left: 0, right: 0, up: 0, down: 0, a: 0, b: 0}
    @held_buttons = {start: 0, d_left: 0, d_right: 0, d_up: 0, d_down: 0, a: 0, b: 0}
    @controller_buf_last_index = 0 # we read from last index + 1 when getting input
    @button_states = [0] * BUFSIZE
    @curr_button_index = 0
    @btn_pressed = {left: nil, right: nil, up: nil, down: nil, a: nil, b: nil}
  end

  def last_rendered_block_state=(block_state)
    @board_state.last_rendered_block_state = block_state
  end

  def increment_frame
    @frame = (@frame + 1) % FPS
  end

  def update_button_states(dc2d)
    while next_state = dc2d.get_next_button_state(@curr_button_index) do
      @button_states[@curr_button_index] = next_state
      @curr_button_index = (@curr_button_index + 1) % BUFSIZE
    end
    @curr_button_index = (@curr_button_index - 1) % BUFSIZE # wind back as we've gone past.
  end

  def discard_button_buffer(dc2d)
    @curr_button_index = dc2d::get_current_button_index
  end

  def exit_input?(frame_idxs)
    frame_idxs.any? { |idx| @cont[:start] & @button_states[idx] != 0 && @cont[:a] & @button_states[idx] != 0 && @cont[:b] & @button_states[idx] != 0 }
  end

  def update_board_for_indices(frame_idxs, dc2d)
    @btn_pressed[:d_left] = nil
    @btn_pressed[:d_right] = nil
    @btn_pressed[:d_up] = nil
    @btn_pressed[:d_down] = nil
    @btn_pressed[:a] = nil
    @btn_pressed[:b] = nil

    input_summary = 65535 # 16-bits of 1s
    # this seems slightly faster than using reduce
    frame_idxs.each { |v| input_summary &= @button_states[v] }

    [:d_left, :d_right, :d_up, :d_down, :a, :b].each { |key|
      current_key_state = @cont[key]
      if input_summary & current_key_state != 0
        @held_buttons[key] += 1
      else
        @held_buttons[key] = 0
        @btn_pressed[key] = (current_key_state & @button_states[frame_idxs[-1]]) != 0
      end
    }

    left_input = @btn_pressed[:d_left] || @held_buttons[:d_left] > 15
    right_input = @btn_pressed[:d_right] || @held_buttons[:d_right] > 15
    @board_state.move_left if left_input
    @board_state.move_right if right_input

    @board_state.clockwise if @btn_pressed[:a]
    @board_state.anticlockwise if @btn_pressed[:b]

    unless @board_state.moved_vertical?
      @board_state.move_down if @btn_pressed[:d_down] || @held_buttons[:d_down] > 15
    end
  end
end

class MainGame
  SOLID_BOARD_BEFORE_START = ([].push [:grey] * 12) * 22

  def initialize(screen, dc2d)
    @dc2d = dc2d
    @screen = screen.new(@dc2d)
    @game_state = GameState.new(@dc2d, @screen, BoardState)
  end

  def wait_for_start_button
    while true do
      rand(1) # hopefully this would give us a "more random" start point
      button_state = @dc2d::get_button_state

      break if button_state && @dc2d::start_btn?(button_state)
    end
  end

  def main_loop
    @dc2d::clear_score()

    @screen.draw_board(SOLID_BOARD_BEFORE_START)

    @dc2d::init_controller_buffer()
    @dc2d::start_controller_reader()

    exiting = false

    while !exiting do # 'main loop'
      wait_for_start_button
      running = true

      @game_state.reset

      @screen.draw_board(@game_state.board_state.board)
      @screen.render_upcoming_block_pane(@game_state.board_state)
      @screen.render_score(@game_state.board_state)

      prev0 = 0
      current0 = 0

      @game_state.board_state.render_block

      @game_state.discard_button_buffer(@dc2d)

      while running do
        @dc2d::waitvbl

        prev0 = current0

        @game_state.increment_frame

        prev_button_index = @game_state.curr_button_index
        @game_state.update_button_states(@dc2d)

        frame_idxs = if prev_button_index > @game_state.curr_button_index
          (prev_button_index..(GameState::BUFSIZE-1)).to_a + (0..@game_state.curr_button_index).to_a
        else
          (prev_button_index..@game_state.curr_button_index).to_a
        end

        if @game_state.exit_input?(frame_idxs)
          exiting = true
          running = false
        end

        @game_state.update_board_for_indices(frame_idxs, @dc2d)

        @game_state.board_state.render_if_moved(@dc2d)

        @game_state.board_state.decrement_lock_deloy
        next unless (@game_state.frame % 3) == 0

        @game_state.board_state.save_current_position

        # Go to top if tick has not reached current wait
        @game_state.tick = (@game_state.tick + 1) % @game_state.curr_wait
        next unless @game_state.tick == 0

        # Make dropping faster
        @game_state.ticks_since_wait_change += 1
        if @game_state.ticks_since_wait_change >= 30
          @game_state.curr_wait -= 1 unless @game_state.curr_wait == 1
          @game_state.ticks_since_wait_change = 0
        end

        if @game_state.board_state.can_drop?
          @game_state.board_state.move_down!
        elsif @game_state.board_state.lock_delay_elapsed?
          @game_state.board_state.save_to_board
          @game_state.board_state.whiten_curr_pos
          @game_state.board_state.whiten_full_rows
          @game_state.board_state.clear_full_rows
          @screen.draw_board(@game_state.board_state.board) # re-render whole board

          @game_state.board_state.next_block(4, 0)
          @game_state.board_state.render_block
          if !@game_state.board_state.can_drop?
            # Stacked to the top...
            running = false
            @game_state.board_state.move_down!
            @game_state.board_state.save_to_board
            @screen.draw_board(@game_state.board_state.board) # re-render whole board with finished state
          end
          @game_state.board_state.move_down!
          @screen.render_upcoming_block_pane(@game_state.board_state)
          @screen.render_score(@game_state.board_state)
          @game_state.board_state.update_board_bitmap
          GC.start

          @game_state.discard_button_buffer(@dc2d)
        end

        #@game_state.board_state.render_if_moved(@dc2d)
      end
    end
    puts $profile
  end
end
