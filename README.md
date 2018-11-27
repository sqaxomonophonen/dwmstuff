# dwmstuff
tools and patches for dwm

## Ubuntu 18.04 gnome-control-center vs dwm
Most options are missing when `gnome-control-center` by itself, so put
something like this in `/usr/bin/cfg`:

```
#!/bin/sh
env XDG_CURRENT_DESKTOP=GNOME gnome-control-center
```

and run that instead.
