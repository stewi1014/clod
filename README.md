# clod {#mainpage}

_A lump or mass especially of earth or clay_

##### Examples of clod in a sentence

- Her husband's such a clod.
- The ground was thin, with clods of turf washed away by recent rain, and the dark soil beneath had pushed its way to the surface once again.

### Structure

- [cli](./cli) command line interface.
- [libclod](./libclod) library.
- [mod](./mod) minecraft mod that provides an in-game command line interface. (no work started yet)

### Dependencies

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

### Development

You can use CLion or IDEA, but you won't even get syntax highlighting for the Java or C stuff.
Or, you can try VSCode and its extensions, but then the java stuff is janky and lacks features.
Or you give up the fancy-pants features and UI by using an editor like Vim, but then you gotta learn how to use it.

Kinda tough deciding which of your friends get to die, isn't it? Good news is you got this one choom who's already dead. And he'd be honoured to join you on a wild suicide run.
You, me, the terminal and the text editor. Kinda sounds like a Eurodyne lyric, I know, but trust me - we'll go fuckin' nova.

### Building

Use CMake as normal. Those unfamiliar with C should note that you're responsible for providing dependencies.

### Platforms

If I'm honest with myself, I can only properly support GCC linux.x86_64 -> linux.x86_64.
That being said, I would like to support as many platforms as possible.
If you use a different platform I'd love any help you can offer in supporting it.

At the time of writing;
Any combination of *BSD, Linux, x86_64 and ARM should work perfectly if the system is up to date.
With any luck I've one-shot OSX support - I've tried to have pure posix fallbacks for everything.
Windows is missing some platform-dependent functions, and support will need to be added.
