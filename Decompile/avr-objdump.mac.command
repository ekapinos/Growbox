#!/bin/bash
cd "$(dirname "$0")"
echo Coping elf files
cp /var/folders/sj/0kxybf1d0cq1qrmfxxtqs0gc0000gn/T/build7879803778579705725.tmp/*.elf . 
echo Decompiling starts
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-objdump -S *.elf > dump-S.txt
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-objdump -D *.elf > dump-D.txt
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-objdump -h *.elf > dump_h.txt
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-objdump -i *.elf > dump_i.txt
/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-objdump -s *.elf > dump_s.txt
echo Decompiling finished

osascript -e 'tell application "Terminal" to quit' &