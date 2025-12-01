# Jot
Minimal Terminal Text Editor for Windows

## Build
```powershell
make
```

## Run
Defaults: Line numbers and guide are ON at column 90.
```powershell
.\jot.exe [-u] [-n] [-g <col>] [-t] [-h] [filename]
```

## Flags
- `-g <col>` or `-g=<col>` : Enable vertical guide at column `<col>` (default `90`).
- `-h` : Hide the help/keybindings line (help is shown by default).
- `-n` : Enable line numbers (right-aligned, followed by a period, e.g. ` 10.`).
- `-t` : Hide the title line (`Jot - <filename>`).
- `-u` : Unix Mode — Ctrl+C acts like SIGINT; copy key becomes `Ctrl+K`.

Short options can be combined (e.g. `-thu` is equivalent to `-t -h -u`).

## Keys
- `Ctrl+C`: Copy current line (unless started with `-u`).
- `Ctrl+D`: Duplicate current line (insert below).
- `Ctrl+K`: Copy current line when started with `-u`.
- `Ctrl+S`: Save (if no filename given, saves to `untitled.txt`).
- `Ctrl+V`: Paste clipboard at cursor (insert, does not overwrite).
- `Ctrl+Z`: Undo.
- `ESC`: Quit.

## Examples
- Open with defaults (numbers & guide on):
	```powershell
	.\jot.exe notes.txt
	```
- Open in Unix Mode and hide help/title:
	```powershell
	.\jot.exe -thu notes.txt
	```
- Open with guide at column 80 and no line numbers:
	```powershell
	.\jot.exe -g=80 notes.txt
	```

## Notes
- Line numbers and the guide are visual only and are not written to the file.
- The vertical guide is drawn by changing console cell attributes (visual overlay), not by inserting characters into the buffer.