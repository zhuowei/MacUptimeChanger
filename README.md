A macOS kext to modify the system's uptime.

```
$ uname -a
Darwin local 18.0.0 Darwin Kernel Version 18.0.0: Fri May 25 16:54:22 PDT 2018; root:xnu-4903.200.199.11.1~1/RELEASE_X86_64 x86_64
$ date
Wed Jul 25 23:48:52 EDT 2018
$ sudo sysctl kern.changeboottime=-4861708948
kern.changeboottime: 1532575384 -> -4861708948
$ uptime
23:48  up 74007 days, 22:31, 4 users, load averages: 0.89 0.97 1.27
```

## Usage

**This makes your computer unstable. Do NOT run on a computer you care about.**

Disable SIP, Build in Xcode, then

```
sudo kextunload -b com.worthdoingbadly.UptimeChanger
sudo chown -R root:wheel UptimeChanger.kext
sudo kextutil UptimeChanger.kext
sudo sudo sysctl kern.changeboottime=<unix timestamp of system boot>
```

Note: setting the timestamp to a negative value will cause ssh to fail with

```
select: invalid argument
```

Setting it back fixes it.

## Development notes

Built in Xcode 10 beta 1 on macOS 10.14 developer beta 1.

Recalculate dependencies with

`kextlibs -xml UptimeChanger.kext`

and pasting the result into Info.plist

## License

My code is licensed under the [Creative Commons CC0](https://creativecommons.org/publicdomain/zero/1.0/) license; do whatever you want with it.

kernel_resolver.c and kernel_resolver.h are from https://github.com/jzdziarski/kernelresolver, and is copyright Snare and Jonathan Zdziarski. Ask them for the license.
