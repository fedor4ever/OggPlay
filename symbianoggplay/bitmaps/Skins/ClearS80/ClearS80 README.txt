ClearS80 skin for OggPlay
=========================
Version 08-01-2008, by Chris Handley

ClearS80 is a skin for OggPlay running on Series 80 devices, like the Nokia
9500.  ClearS80 is heavily based upon the IcyS80 skin, which was the only S80
skin provided with OggPlay.


Installation
------------
None needed, as it comes with OggPlay, and to my surprise it may even become
the default skin.


What is it like?
----------------
The purpose of ClearS80 is to show the text as clearly as possible, so that it
is readable under less than ideal conditions (such as reading it from a
distance, with glare on the screen).  I achieved this by:

1.  Making the background to the text areas either pure black or white.  Except
for the Repeat & Random texts, where I was able to give their white text a black
outline instead.

2.  Highlighted text is red, as are the volume/progress guages.

3.  The icon next to a playing/paused file is red, and is also solid when
playing.


Other improvements:

1.  The main lister shows 50% more music files!

2.  The folder icons for Titles, Albums, Artists & Genres are hand-drawn
depictions of what they are supposed to be.  I spent far too long trying to find
a suitable representation that could be drawn as an outline in just a few pixels
(around 8x8).  This was much harder than it might look!

3.  Subfolders now have a different icon from files.

4.  All text areas, as well as the areas used for guages, have a subtle grey
outline, so their edges are always clear, as well as maintaining a consistent
style.  And these edges match those defined in the skin file, rather than just
loosely matching it (as IcyS80 does).


How did you create it?
----------------------
With difficulty!  First I had to "reverse engineer" the IcyS80 skin, using the
BMCONV program that comes with some Symbian SDKs.  I eventually ended-up with
about 48 BMP files, and a batch file which could be used to create a new MBM
file (including the correct bitmap depths, which was a big hassle).

I then went about editing the relevant BMP files with Microsoft Paint, editing
the SKN file with Notepad, and uploading the new MBM/SKN files to my Nokia 9500
for testing.  At one point I had to add an extra BMP file (as one BMP had a dual
purpose), so ClearS80's MBM no-longer exactly matches the layout of IcyS80.


You can get all the files necessary to edit & recreate the ClearS80.mbm file
from here:
http://symbianoggplay.cvs.sourceforge.net/symbianoggplay/symbianoggplay/bitmaps/Skins/ClearS80/

As long as you have a Symbian SDK installed, then simply double-clicking on the
"ClearS80_CreateMBM" batch file will run the BMCONV program with the correct
parameters (all 50 of them!) required to recreate the ClearS80.mbm file.  If it
does not work, then you need to obtain a recent version of BMCONV, and put it in
the same folder as the batch file.  Email me if you can't find a suitable
version of BMCONV.


As long as the author of IcyS80 does not object, I put all of my files in the
public domain - so you can do anything you please with them!  You could slightly
modify ClearS80 for yourself, or perhaps use it to create an entirely new skin.
All I recommend is that you give your new skin a different name, to avoid any
confusion with my skin.  And of course it would be nice if you mentioned me...


Possible improvements
---------------------
1.  I'm not entirely happy with the Artists icon.  Perhaps it should show two
people?  But don't bother trying to make him larger, as otherwise his
arms/legs/head will just look out of proportion - unless he is made too large to
be an icon.

2.  Use a larger font?

3.  Perhaps move the progress guage & text to the right side (under the OggPlay
logo), so that we get an extra line of text?

However, none of these issues is annoying enough for me to bother dealing with
them.  Thus this first public release of ClearS80 is likely to be it's last too!


Contact the author
------------------
My name is Christopher Handley, I live in England, and you can find my current
email address at this web page:
http://email.cshandley.co.uk

Sorry if my address is hard to read, but I have learnt to be fairly paranoid
about spam!

If you have trouble contacting me, you could try getting hold of me through
FreEpoc at http://www.freepoc.org
