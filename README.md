# Reverse shell for macOS
A reverse shell written in C and python. This is not intended for malicious purposes and only a proof of concept.

To compile run (in `c/`):
```
    make
```

Then run
```
    python3 server.py
```
on the server and
```
    c/bin/client
```
on the target

