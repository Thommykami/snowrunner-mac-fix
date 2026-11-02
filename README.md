# SnowRunner (Windows) startup crash fix for Wine on macOS

If the Windows/Steam version of SnowRunner crashes on your Mac at the loading spinner or right at the title screen (CrossOver, Highball, Hadron...), this might be why.

I figured this out together with Claude. I'm not a Wine developer, so take the explanation with that in mind.

## What's going on

At startup SnowRunner creates session GUIDs for its telemetry using `UuidCreateSequential`. Those GUIDs contain the MAC address of the first network adapter. Wine just takes the first adapter that `GetAdaptersInfo` returns.

On macOS, the first adapters are `gif0` and `stf0`. They're built-in tunnel interfaces that don't have a MAC address. So every GUID ends in `000000000000`, and the game crashes shortly after. Windows and Linux don't have these interfaces, which is why it works fine there (and on Steam Deck).

You can't remove `gif0`/`stf0` on recent macOS (`ifconfig destroy` fails), and setting them down doesn't help because Wine lists them anyway.

## The fix

A small proxy `iphlpapi.dll`. It forwards every function to Wine's own `iphlpapi.dll` and only changes `GetAdaptersInfo`: adapters with a real MAC address are moved to the front of the list. That's all it does. See [`src/proxy.c`](src/proxy.c).

## Install

1. Download `iphlpapi.dll` from the [Releases](../../releases) page (or build it yourself, see below).
2. Copy it into the game folder, next to `SnowRunner.exe`:
   `steamapps/common/SnowRunner/Sources/Bin/`
3. Tell Wine to use it for SnowRunner only. Run this with the `wine` of your setup, with `WINEPREFIX` pointing at the game's prefix:
   ```
   wine reg add "HKCU\Software\Wine\AppDefaults\SnowRunner.exe\DllOverrides" /v iphlpapi /d native,builtin /f
   ```

Example for **Hadron** (Steam on Mac):
```bash
export WINEPREFIX="$HOME/Library/Application Support/Steam/steamapps/compatdata/1465360/pfx"
W=/Applications/Hadron.app/Contents/SharedSupport/runtime/dist/bin/wine
cp iphlpapi.dll "$HOME/Library/Application Support/Steam/steamapps/common/SnowRunner/Sources/Bin/"
"$W" reg add 'HKCU\Software\Wine\AppDefaults\SnowRunner.exe\DllOverrides' /v iphlpapi /d native,builtin /f
```

For CrossOver you can also add the override in the bottle's Wine Configuration (Libraries tab: add `iphlpapi`, set to native then builtin).

After a game update or "Verify integrity of game files" in Steam, the DLL may be gone. Just copy it back.

## Uninstall

Delete `iphlpapi.dll` from the game folder and remove the override:
```
wine reg delete "HKCU\Software\Wine\AppDefaults\SnowRunner.exe\DllOverrides" /v iphlpapi /f
```

## Tested

- Hadron 0.3.2, MacBook Pro M3 Pro, Steam version of SnowRunner: works.
- CrossOver / Whisky: not tested yet, but it's the same Wine code. Let me know if it works for you.

## Build

Needs Python 3 and a MinGW-w64 x86_64 cross compiler (`brew install mingw-w64` on macOS).
```
./build.sh
```
Output: `build/iphlpapi.dll`.

## Proper fix

The real fix belongs in Wine: `UuidCreateSequential` (rpcrt4) should skip adapters without a MAC address. Reported upstream.
