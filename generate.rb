#!/usr/bin/env ruby
require "open-uri"

emojis = []
bytes = []
open("https://unicode.org/Public/emoji/12.0/emoji-test.txt") do |io|
  text = io.read
  text.scan(/ # (\S) /u) do |m|
    emojis.push(m[0])
  end
end

bytes = emojis.map { |str| [str.bytesize, *str.bytes] }.flatten

File.open("emojis.c", "w") do |c|
  c.write "#include <stdint.h>\n"
  c.write "uint8_t emojis[#{bytes.size}] = {\n  "
  c.write bytes.each_slice(8).map { |x|
    x.map { |v| "0x%02x" % v }.join(", ")
  }.join(",\n  ")
  c.write "\n};\n"
end
File.open("emojis.h", "w") do |c|
  c.write "#include <stdint.h>\n"
  c.write("extern uint8_t emojis[#{bytes.size}];")
end
