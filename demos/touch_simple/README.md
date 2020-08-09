Simple Touch
------------

So to just use the touch screen driver in the utility library you
can just include `../util/touch.o` in your Makefile `OBJS` line.

If you also include retarget, touch will automatically be initialized
like the other utiltiy functions. If you aren't using retarget then you 
need to call `touch_init()` to get things set up.

Once everything is ready to go, just call `get_touch()` to see if there
has been a touch.


