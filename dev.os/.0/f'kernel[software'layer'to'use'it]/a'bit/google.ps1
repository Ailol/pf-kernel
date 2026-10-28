#!/ bin / sh
# 1. Parse raw text stream from target source argument
src = $1;
bin = ${src %.c}.bin
# 2. Ingest stream, optimize for bare hardware, and bypass OS bloat
          gcc -
      O3 - fno - stack - protector - ffreestanding - c "$src" - o "$bin.o"
# 3. Direct architectural translation straight into execution instructions
      ld -
      nostdlib - static "$bin.o" - o "$bin"
# 4. Strip out semantics, names, symbols, and text headers completely
      strip -
      s - R.comment - R.note.gnu.property "$bin"
# 5. Volatile execution strike : Feed binary directly to the processor line
                              ./
                          "$bin"