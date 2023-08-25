# rgxdir
Groups files in a subdirectories based on a regex pattern.
I decided to write it in C instead of Rust, because the required crates required several MiB, while the C application is only 12,4KiB in total with UPX.

## Usage
Imagine the following directory:
```
/test
  /a1a
  /a1b
  /a1c
  /a2a
  /a2b
  /a2c
  /b1a
  /b1b
  /b1c
  /b2a
  /b2b
  /b2c
```

Using `rgxdir '^([0-9]{4})-([0-9]{2})-([0-9]{2}).*$' /path/to/dir/test` will result in the following directory structure:
```
/test
  /a
    /1
      /a1a
      /a1b
      /a1c
    /2
      /a2a
      /a2b
      /a2c
  /b
    /1
      /b1a
      /b1b
      /b1c
    /2
      /b2a
      /b2b
      /b2c
```
      
