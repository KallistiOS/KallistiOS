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
