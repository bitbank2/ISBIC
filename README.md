## ISBIC ##
<b>Incredibly simple bitonal image compression</b><br>
<br>
<b>What is it?</b><br>
A simple algorithm for losslessly compressing bitonal (1-bit) images.<br>
<br>
<b>Why did you write it?</b><br>
Lately I've been working on a lot of projects which target the CH32V003 RISC-V MCU. Unfortunately it only has 16K of FLASH memory and 2K of SRAM. I want to include some pre-made graphics in my projects which use graphic displays. The purpose of this code is to make a compression algorithm that can both reduce the storage size of images and use as little code as possible decompress them on the device.<br>
<br>
<b>What isn't it?</b><br>
A fast or comprehensive compression scheme. The primary goal is to reduce the graphics size and code size to fit on tiny MCUs. It doesn't need to be fast, although I always make things as fast as I can :) .<br>

