Terminal Emulation
------------------

##Interesting Side Trips

###Fonts and Characters

Terminals used very simple character generator ROMS. These were
preprogrammed with a series of bits that would turn a CRT beam
on or off. They were chunky and pretty hard to read as you
can see with this [simulated VT320][vt320] font.

The constraint on terminal fonts were two fold, first every
character had to take the same space (the character generator
roms reserved a small binary bitmap for each glyph of exactly
the same size). Second, the circuitry driving the CRT in those
terminals was primitive. Inexpensive terminals had exactly two
states, on or off. More expensive terminals could modulate the
beam to perhaps four distinct intensities. In all non "color"
terminals, the screen color was a function of the single 
phosphor on the screen.

So for this project I really wanted to use antialiased fonts
(they look better and use the screen better). And that meant
figuring out how to get antialiased fonts on the screen.

What I came up with is something of a compromise, I use my
Linux host to render anti-aliased fonts into a bitmap of
fixed size, then I generate source code which declares an
array with that data in it already. At that point my
code just copies chunks of memory to render characters,
very much like the terminals of old, but with more 256 grey
levels.

[vt320]: https://fonts.google.com/specimen/VT323

###Rendering the screen

Old school terminals would simply store attributes and character
codes in some static RAM. The character code would index into a
font ROM and the attributes would usually configure inverse video,
underline, or bold. And the way those were implemented was fairly
simple; "Inverse video" inverted the bits coming out of the character
generator ROM. "Bold" was simply another address bit on that ROM
and selected different characters patterns. The "Underline" attribute
usually just forced the bit output to be 'true' for the characters
baseline. We can do all of those except "underline" is not quite
simple in software.

Displaying the screen is another interesting trip. One way to do
that is to just render into a frame buffer and then you have an
image to show of the screen. This is simple and straight forward but
you lose the ability to go back and figure out what character was
on row 3, column 10 (or some other arbitrary location).

But you can also create an array that is the "contents" of the
screen from a logical level, and then just translate that into an
image when ever you need to update the screen. That is what I've
done in this code, but there are two ways to translate on to the
screen.

In one you first clear the entire image to what it would be if
it were filled with spaces using the current background color.
Then you iterate through the state array and write out each
character that isn't a space or, if it is a space has a different
background color. In situations where the screen is mostly space
that is faster.

The alternative, is to simply always write every single character
space. That happens in finite time every time, and is slightly
faster than the first case when the first case has every character
filled with a non-background character, but this version is slower
for versions that aren't full.

I implement both.
