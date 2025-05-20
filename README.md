# clod {#mainpage}

_A lump or mass especially of earth or clay_

##### Examples of clod in a sentence

- Her husband's such a clod.
- The ground was thin, with clods of turf washed away by recent rain, and the dark soil beneath had pushed its way to the surface once again.

### Structure

- [cli](./cli) command line interface.
- [libclod](./libclod) C library - the main point of this project.
- [mod](./mod) minecraft mod that provides an in-game command line interface. (no work started yet)
- [api](./api) java bindings for the library. (no work started yet)

### Dependencies

Make sure these are findable by meson.

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

### Development

You can use IDEA, but you won't even get syntax highlighting for the C stuff.
Or, you can try VSCode and its extensions, but then the java stuff is janky and lacks features.
Or you give up the fancy-pants features and UI by using an editor like Vim, but then you gotta learn how to use it.

Kinda tough deciding which of your friends get to die, isn't it? Good news is you got this one choom who's already dead. And he'd be honoured to join you on a wild suicide run.
You, me, the terminal and the text editor. Kinda sounds like a Eurodyne lyric, I know, but trust me - we'll go fuckin' nova.

### Building

Use gradle and/or meson as normal. Those unfamiliar with C should note that you're responsible for providing dependencies - typically at the OS level.

To control the modloader and minecraft version, reference the `<modloader>-<minecraft_version>` gradle subproject.
Some examples; `gradle fabric-1.19.1:runClient`, `gradle fabric-1.20.1:build`, `gradle forge-1.19.4:runClient`, `gradle paper-1.21.5:build` and `gradle neoforge-1.21.0:runServer` all operate as you would expect.
`build_all_targets.sh` is generated for convenience.

### Platforms

If I'm honest with myself, I can only properly support GCC linux.x86_64 -> linux.x86_64.
That being said, I would like to support as many platforms as possible.
If you use a different platform I'd love any help you can offer in supporting it.

At the time of writing;
Any combination of *BSD, Linux, x86_64 and ARM should work perfectly.
With any luck I've one-shot OSX support - I've tried to avoid anything not posix.
Windows is missing a few platform dependent functions, but ~~shouldn't require rewriting anything I hope~~.
I've accidentally coupled too closely with methods I didn't realise don't exist in windows - some code needs rewriting by someone on a windows system.
