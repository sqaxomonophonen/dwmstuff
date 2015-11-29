Changes from vanilla:
 - font changed to Terminus
 - bar in bottom
 - MODKEY set to OS key (mod4)
 - applied http://dwm.suckless.org/patches/focusadjacenttag (except for
   tagtoleft/right which didn't work, possibly because the patch was targeted
   at 6.0? I wasn't interested anyway)

Generate with `( diff -up config.def.h config.h ; git diff ) > /path/to/patch`
