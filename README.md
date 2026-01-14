# hBootPatcher

An iBoot Patcher.

## Building

Building is supported on macOS and Linux.

You must use a sufficiently recent version of GNU Make, and not a
2006 GNU Make.

## License

hBootPatcher is licensed under the MIT license, as included in the [LICENSE](./LICENSE) file.

Plooshfinder is used, which is licensed under GNU Lesser General Public License v3.0.

Certain minor parts are from m1n1:

- Copyright The Asahi Linux Contributors

Certain patch maskmatch in iorvbar.c, recfg.c, aes.c are taken from [pongoOS patches.S](https://github.com/palera1n/PongoOS/blob/b1038fd75a2608aa8dbc0dd3c94a0f1f97118c28/src/boot/patches.S#L299). However
it is believed this do not constitute being a derived work of pongoOS as
they are directly derived from Apple's logic, and no pongoOS code is copied
verbatim. Both pongoOS and this project operate under the assumption that
it is OK to derive patches based on Apple's logic as such.

This project is an iBoot patcher and iBoot is (c) Apple, Inc.. The acceptability of this
practice depends on the jurisdiction. Please check your local laws.
