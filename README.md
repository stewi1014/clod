# CLOD
_A lump or mass especially of earth or clay_

##### Examples of clod in a sentence
 - Her husband's such a clod.
 - The ground was thin, with clods of turf washed away by recent rain, and the dark soil beneath had pushed its way to the surface once again.

### Structure
 - [cli](./cli) command line interface.
 - [liblod](./liblod) library for dealing with LOD data.
 - [libmc](./libmc) library for dealing with minecraft data.

### Development
You can use IDEA, but you won't even get syntax highlighting for the C stuff.
Or, you can try VSCode and its extensions, but then the java stuff is janky and lacks features.
Or you give up the fancy-pants features and UI by using an editor like Vim, but then you gotta learn how to use it.

Kinda tough deciding which of your friends get to die, isn't it? Good news is you got this one choom who's already dead. And he'd be honoured to join you on a wild suicide run.
You, me, the terminal and the text editor. Kinda sounds like a Eurodyne lyric, I know, but trust me - we'll go fuckin' nova.

### Build system
Use gradle and/or meson.

To control the modloader and minecraft version, reference the '<modloader>-<minecraft_version>' gradle subproject.
Some examples; `gradle fabric-1.19.1:runClient`, `gradle fabric-1.20.1:build`, `gradle forge-1.19.4:runClient`, `gradle paper-1.21.5:build` and `gradle neoforge-1.21.0:runServer` all operate as you would expect.
`build_all_targets.sh` is generated for convenience.

I can only really support GCC linux.x86_64 -> linux.x86_64, but I strive to respect other build and target environments.
