# KallistiOS ##version##
#
# block_shapes.rb - Block shapes definition
# Copyright (C) 2019-2024 Yuji Yokoo
# Copyright (C) 2020-2024 Mickaël "SiZiOUS" Cardoso
#
# Mrbtris
# A sample Tetris clone for Sega Dreamcast written in Ruby
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

class BlockShapes
  def self.freeze_3_levels(x)
    x.map { |shape| shape.map { |row| row.freeze }.freeze }.freeze
  end

  SQ = freeze_3_levels [
      [ [false, :yellow, :yellow, false], [false, :yellow, :yellow, false], [], [] ],
      [ [false, :yellow, :yellow, false], [false, :yellow, :yellow, false], [], [] ],
      [ [false, :yellow, :yellow, false], [false, :yellow, :yellow, false], [], [] ],
      [ [false, :yellow, :yellow, false], [false, :yellow, :yellow, false], [], [] ],
    ]

  I = freeze_3_levels [
      [ [], [:cyan, :cyan, :cyan, :cyan], [], [] ],
      [ [false, false, :cyan, false], [false, false, :cyan, false], [false, false, :cyan, false], [false, false, :cyan, false] ],
      [ [], [], [:cyan, :cyan, :cyan, :cyan], [] ],
      [ [false, :cyan, false, false], [false, :cyan, false, false], [false, :cyan, false, false], [false, :cyan, false, false] ],
    ]

  L = freeze_3_levels  [
      [ [false, false, :orange, false], [:orange, :orange, :orange, false], [], [] ],
      [ [false, :orange, false, false], [false, :orange, false, false], [false, :orange, :orange, false], [] ],
      [ [], [:orange, :orange, :orange, false], [:orange, false, false, false], [] ],
      [ [:orange, :orange, false, false], [false, :orange, false, false], [false, :orange, false, false], [] ],
    ]

  J = freeze_3_levels [
      [ [:blue, false, false, false], [:blue, :blue, :blue, false], [], [] ],
      [ [false, :blue, :blue, false], [false, :blue, false, false], [false, :blue, false, false], [] ],
      [ [], [:blue, :blue, :blue, false], [false, false, :blue, false], [] ],
      [ [false, :blue, false, false], [false, :blue, false, false], [:blue, :blue, false, false], [] ],
    ]

  S = freeze_3_levels [
      [ [false, :green, :green, false], [:green, :green, false, false], [], [] ],
      [ [false, :green, false, false], [false, :green, :green, false], [false, false, :green, false], [] ],
      [ [], [false, :green, :green, false], [:green, :green, false, false], [] ],
      [ [:green, false, false, false], [:green, :green, false, false], [false, :green, false, false], [] ],
    ]

  Z = freeze_3_levels [
      [ [:red, :red, false, false], [false, :red, :red, false], [], [] ],
      [ [false, false, :red, false], [false, :red, :red, false], [false, :red, false, false], [] ],
      [ [], [:red, :red, false, false], [false, :red, :red, false], [] ],
      [ [false, :red, false, false], [:red, :red, false, false], [:red, false, false, false], [] ],
    ]

  T = freeze_3_levels [
      [ [false, :purple, false, false], [:purple, :purple, :purple, false], [], [] ],
      [ [false, :purple, false, false], [false, :purple, :purple, false], [false, :purple, false, false], [] ],
      [ [], [:purple, :purple, :purple, false], [false, :purple, false, false], [] ],
      [ [false, :purple, false, false], [:purple, :purple, false, false], [false, :purple, false, false], [] ],
    ]

  def self.colour_to_rgb(colour)
    case colour
    when :grey
      [192, 192, 192]
    when :cyan
      [0, 192, 192]
    when :yellow
      [192, 192, 0]
    when :purple
      [128, 0, 128]
    when :green
      [0, 128, 0]
    when :red
      [255, 0, 0]
    when :blue
      [0, 0, 255]
    when :orange
      [255, 165, 0]
    when :white
      [255, 255, 255]
    else
      [0, 0, 0]
    end
  end

  def self.random_shape
    candidates = all_shapes

    candidates[rand(candidates.size)]
  end

  def self.all_shapes
    [[:sq, SQ], [:i, I], [:l, L], [:j, J], [:s, S], [:z, Z], [:t, T]]
  end
end
