#!/usr/bin/env python
import sys
def main(argv):
    if len(argv) < 3:
        print >> sys.stderr, __doc__
        sys.exit(1)
    in_f = open(argv[1])
    out_f = open(argv[2],"wt")
    out_f.write("#include <gpio.h>\n")
    out_f.write("__initdata int gpio_ss_table[][2] = {\n")
    for line in in_f:
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("\xef\xbb\xbf"):
            continue
        words = line.split()
        if len(words) >= 3:
            if(len(words[2])) < 16:
               out_f.write("\t{%s,\t%s\t\t},\t/* %s */\n" % (words[1],words[2],words[0]))
            else:
               out_f.write("\t{%s,\t%s\t},\t/* %s */\n" % (words[1],words[2],words[0]))
    in_f.close()
    out_f.write("\t{GSS_TABLET_END\t,GSS_TABLET_END\t}\n");
    out_f.write("};\n");
    out_f.close()
if __name__ == "__main__":
  main(sys.argv)
